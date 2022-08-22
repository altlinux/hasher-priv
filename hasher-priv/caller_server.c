/*
 * The per-calling-user server part of the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "caller_data.h"
#include "caller_job.h"
#include "caller_server.h"
#include "communication.h"
#include "epoll.h"
#include "error_prints.h"
#include "fds.h"
#include "io_loop.h"
#include "macros.h"
#include "process.h"
#include "server_config.h"
#include "signals.h"
#include "sockets.h"
#include "xmalloc.h"
#include "xstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>

#include <sys/socket.h> /* SOCK_CLOEXEC */
#include <sys/signalfd.h>
#include <sys/stat.h> /* umask */
#include <sys/wait.h>

#include <linux/un.h>

static int
check_peer_creds(int conn)
{
	uid_t uid;
	gid_t gid;

	if (get_peercred(conn, &caller_pid, &uid, &gid) < 0)
		return -1;

	if (caller_uid != uid || caller_gid != gid) {
		error_msg("connection from [%u:%u] to %s:%u rejected",
			  uid, gid, caller_user, caller_num);
		return -1;
	}

	return 0;
}

int
caller_server_listener_init(struct hadaemon *sdae)
{
	sdae->fd_ep = -1;
	sdae->fd_signal = -1;
	sdae->fd_conn = -1;

	/* Load config according to the caller information. */
	configure_caller();

	char socketpath[UNIX_PATH_MAX];
	xsprintf(socketpath, "%s/%d:%u", SOCKETDIR, caller_uid, caller_num);

	umask(077);

	if ((sdae->fd_conn = srv_listen(socketpath)) < 0)
		return -1;

	if (chown(socketpath, caller_uid, caller_gid)) {
		perror_msg("chown: %s", socketpath);
		goto fail;
	}

	if ((sdae->fd_signal = daemon_create_signal_fd()) < 0) {
		perror_msg("signalfd");
		goto fail;
	}

	if ((sdae->fd_ep = epoll_create1(EPOLL_CLOEXEC)) < 0) {
		perror_msg("epoll_create1");
		goto fail;
	}

	if (epoll_add_in(sdae->fd_ep, sdae->fd_pipe[0]) < 0 ||
	    epoll_add_in(sdae->fd_ep, sdae->fd_conn) < 0 ||
	    epoll_add_in(sdae->fd_ep, sdae->fd_signal) < 0) {
		goto fail;
	}

	return 0;

fail:
	xclose(&sdae->fd_ep);
	xclose(&sdae->fd_signal);
	xclose(&sdae->fd_conn);

	unlink(socketpath);

	return -1;
}

static void
wait_jobs(void)
{
	for (;;) {
		int status;
		pid_t pid = waitpid_retry(-1, &status, WNOHANG);
		if (pid <= 0) {
			if (pid < 0 && errno != ECHILD)
				perror_msg("waitpid");
			break;
		}

		if (WIFEXITED(status)) {
			int rc = WEXITSTATUS(status);
			if (rc) {
				notice_msg("process %d exited, status=%d",
					   pid, rc);
			} else {
				info_msg("process %d exited", pid);
			}
		} else if (WIFSIGNALED(status)) {
			notice_msg("process %d terminated by signal %d",
				   pid, WTERMSIG(status));
		}
	}
}

void
caller_server(struct hadaemon *sdae)
{
	unsigned long n_seconds = 0;
	int finish_server = 0;

	while (!finish_server) {
		errno = 0;
		struct epoll_event ev[16];
		int fdcount = epoll_wait(sdae->fd_ep, ev, ARRAY_SIZE(ev), 1000);
		if (fdcount < 0) {
			if (errno == EINTR)
				continue;
			perror_msg_and_die("epoll_wait");
			break;
		}

		if (fdcount == 0) {
			if (++n_seconds >= server_session_timeout)
				break;
			continue;
		}

		for (int i = 0; i < fdcount; ++i) {
			if (ev[i].data.fd == sdae->fd_pipe[0]) {
				finish_server = 1;
				break;
			}

			if (!(ev[i].events & EPOLLIN))
				continue;

			if (ev[i].data.fd == sdae->fd_signal) {
				struct signalfd_siginfo fdsi;
				ssize_t size;

				size = read_retry(sdae->fd_signal, &fdsi, sizeof(fdsi));
				if (size != sizeof(fdsi)) {
					error_msg("expected siginfo size %lu, got %ld",
						  (unsigned long) sizeof(fdsi),
						  (long) size);
					continue;
				}

				switch (fdsi.ssi_signo) {
				case SIGHUP:
				case SIGINT:
				case SIGQUIT:
				case SIGTERM:
					finish_server = 1;
					break;
				case SIGCHLD:
					wait_jobs();
					break;
				default:
					error_msg("unexpected signal %d ignored",
						  fdsi.ssi_signo);
					break;
				}

			}
		}
		for (int i = 0; !finish_server && i < fdcount; ++i) {
			if (!(ev[i].events & EPOLLIN))
				continue;

			if (ev[i].data.fd == sdae->fd_conn) {
				int conn = accept4(sdae->fd_conn, NULL, 0, SOCK_CLOEXEC);
				if (conn < 0) {
					perror_msg("accept4");
					continue;
				}

				if (set_recv_timeout(conn, 3) == 0 &&
				    check_peer_creds(conn) == 0 &&
				    receive_job_request(sdae, conn) == 0) {
					/* reset timer */
					n_seconds = 0;
				}

				xclose(&conn);
			}
		}
	}

	notice_msg("%s/%u:%u: session finished",
		   caller_user, caller_uid, caller_num);
	exit(EXIT_SUCCESS);
}
