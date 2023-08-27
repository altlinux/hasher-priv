/*
 * The entry point for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022-2023  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "caller_data.h"
#include "caller_server.h"
#include "communication.h"
#include "epoll.h"
#include "error_prints.h"
#include "fds.h"
#include "io_loop.h"
#include "logging.h"
#include "macros.h"
#include "pidfile.h"
#include "process.h"
#include "server_comm.h"
#include "server_config.h"
#include "signals.h"
#include "sockets.h"
#include "xmalloc.h"
#include "xstring.h"
#include "title.h"

#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/un.h>

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

struct session {
	struct session *next;

	uid_t caller_uid;
	gid_t caller_gid;

	unsigned int caller_num;

	pid_t server_pid;
};

static struct hadaemon dn = { {-1, -1}, -1, -1, -1, };
static struct session *pool;

static void
create_socket_node(struct hadaemon* d)
{
	mode_t m = umask(017);

	char socketpath[UNIX_PATH_MAX];
	xsprintf(socketpath, "%s/%s", SOCKETDIR, MAIN_SOCKET_BASE_NAME);

	if ((d->fd_conn = srv_listen(socketpath)) < 0)
		perror_msg_and_die("srv_listen");

	umask(m);

	if (chown(socketpath, 0, server_gid))
		perror_msg_and_die("chown: %s", socketpath);

	notice_msg("listening on %s", socketpath);
}

ATTRIBUTE_NORETURN
static void
run_caller_server(struct hadaemon *d, int cl_conn, const struct session *a)
{
	xclose(&d->fd_pipe[1]);
	xclose(&d->fd_ep);
	xclose(&d->fd_signal);
	xclose(&d->fd_conn);

	caller_num = a->caller_num;
	init_caller_data(a->caller_uid, a->caller_gid);

	setproctitle("server %s/%u:%u",
		     caller_user, caller_uid, caller_num);

	struct hadaemon sh = {
		.fd_pipe = { d->fd_pipe[0], -1 }
	};

	if (caller_server_listener_init(&sh) < 0) {
		send_response_to_client(cl_conn, CMD_STATUS_FAILED, NULL);
		exit(EXIT_FAILURE);
	}

	notice_msg("%s/%u:%u: session started",
		   caller_user, caller_uid, caller_num);

	/* Notify the client that the session server is ready. */
	send_response_to_client(cl_conn, CMD_STATUS_DONE, NULL);
	xclose(&cl_conn);

	caller_server(&sh);
}

static int
start_session(struct hadaemon *d, int conn, unsigned num)
{
	uid_t uid;
	gid_t gid;
	if (get_peercred(conn, NULL, &uid, &gid) < 0)
		return -1;

	if (min_uid >= 0) {
		if (!valid_uid(uid) || uid == getuid()) {
			error_msg("invalid uid: %u", uid);
			return -1;
		}
	}

	if (min_gid >= 0) {
		if (!valid_gid(gid) || gid == getgid()) {
			error_msg("invalid gid: %u", gid);
			return -1;
		}
	}

	struct session **sp = &pool;
	while (*sp) {
		if ((*sp)->caller_uid == uid && (*sp)->caller_num == num) {
			/* Session exists and will be reused. */
			send_response_to_client(conn, CMD_STATUS_DONE, NULL);
			return 0;
		}
		sp = &(*sp)->next;
	}

	struct session *s = calloc(1, sizeof(*s));
	if (!s) {
		perror_msg("calloc");
		return -1;
	}

	s->caller_uid = uid;
	s->caller_gid = gid;
	s->caller_num = num;

	notice_msg("starting session for user %d:%u", uid, num);

	if ((s->server_pid = fork()) < 0) {
		perror_msg("fork");
		free(s);
		return -1;
	}
	if (s->server_pid == 0)
		run_caller_server(d, conn, s);

	*sp = s;

	/*
	 * The successful response is delayed until the session server is ready
	 * or an error happens.
	 */
	return 0;
}

static void
process_request(struct hadaemon *d, int conn)
{
	if (set_recv_timeout(conn, 3) < 0)
		return;

	cmd_header_t hdr = { 0 };
	if (xrecvmsg(conn, &hdr, sizeof(hdr)) < 0)
		return;

	switch (hdr.type) {
		case CMD_OPEN_SESSION:
			if (start_session(d, conn, hdr.len) < 0) {
				send_response_to_client(conn, CMD_STATUS_FAILED,
							"command failed");
			}
			break;
		default:
			error_msg("unknown command: %d", hdr.type);
			send_response_to_client(conn, CMD_STATUS_FAILED,
						"unknown command");
	}
}

static void
free_session(const pid_t pid)
{
	struct session **sp = &pool;

	while (*sp) {
		if (pid == (*sp)->server_pid) {
			struct session *s = *sp;
			*sp = (*sp)->next;
			free(s);
			break;
		}
		sp = &(*sp)->next;
	}
}

static void ATTRIBUTE_NORETURN
show_usage(void)
{
	fprintf(stderr, "\nTry `%s --help' for more information.\n",
		program_invocation_short_name);
	exit(EXIT_FAILURE);
}

static void ATTRIBUTE_NORETURN
print_help(void)
{
	printf("Usage: %s [options]\n"
	       " -D, --daemonize        run in the background;\n"
	       " -l, --loglevel=LEVEL   set the minimal log level;\n"
	       " -p, --pidfile=FILE     set the pid file location;\n"
	       " -V, --version          print program version and exit;\n"
	       " -h, --help             show this text and exit.\n"
	       "\n",
	       program_invocation_short_name);
	exit(EXIT_SUCCESS);
}

static void ATTRIBUTE_NORETURN
print_version(void)
{
	printf("%s version %s\n"
	       "Copyright (c) 2003-2023  Dmitry V. Levin <ldv@altlinux.org>\n"
	       "Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>\n"
	       "Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>\n"
	       "Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>\n"
	       "\nAll rights reserved.\n"
	       "\nThis is free software; see the source for copying conditions.  There is NO\n"
	       "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	       "\nWritten by Dmitry V. Levin <ldv@altlinux.org> et al.\n",
	       program_invocation_short_name, PROJECT_VERSION);
	exit(EXIT_SUCCESS);
}

static void
wait_sessions(int waitpid_options)
{
	for (;;) {
		int status;
		pid_t pid = waitpid_retry(-1, &status, waitpid_options);
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

		free_session(pid);
	}
}

int
main(int argc, char **argv)
{
	static const struct option long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'V' },
		{ "daemonize", no_argument, 0, 'D' },
		{ "loglevel", required_argument, 0, 'l' },
		{ "pidfile", required_argument, 0, 'p' },
		{ 0, 0, 0, 0 }
	};

	sanitize_fds();

#ifdef PR_SET_NO_NEW_PRIVS
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0))
		perror_msg("PR_SET_NO_NEW_PRIVS");
#endif

	const char *loglevel = NULL;
	const char *pidfile = NULL;
	int daemonize = 0;
	int i;
	while ((i = getopt_long(argc, argv, "hVDl:p:", long_options, NULL)) != -1) {
		switch (i) {
			case 'D':
				daemonize = 1;
				break;
			case 'l':
				loglevel = optarg;
				break;
			case 'p':
				pidfile = optarg;
				break;
			case 'V':
				print_version();
			case 'h':
				print_help();
			default:
				show_usage();
		}
	}

	configure_server();
	if (!loglevel)
		loglevel = server_loglevel;
	if (loglevel)
		set_log_level(loglevel);

	if (daemonize && !pidfile && server_pidfile && *server_pidfile)
		pidfile = server_pidfile;

	if (pidfile && check_pid(pidfile))
		error_msg_and_die("already running");

	if (daemonize && daemon(0, 0) < 0)
		perror_msg_and_die("daemon");

	init_log_daemon(!daemonize);

	if (pidfile && write_pid(pidfile) == 0)
		return EXIT_FAILURE;

	struct hadaemon *d = &dn;

	if (pipe(d->fd_pipe))
		perror_msg_and_die("pipe");

	create_socket_node(d);

	if ((d->fd_signal = daemon_create_signal_fd()) < 0)
		perror_msg_and_die("signalfd");

	if ((d->fd_ep = epoll_create1(EPOLL_CLOEXEC)) < 0)
		perror_msg_and_die("epoll_create1");

	if (epoll_add_in(d->fd_ep, d->fd_signal) < 0 ||
	    epoll_add_in(d->fd_ep, d->fd_conn) < 0)
		perror_msg_and_die("epoll_add_in");

	/*
	 * As we use waitpid, SIGCHLD should not be ignored.
	 */
	signal(SIGCHLD, SIG_DFL);

	/*
	 * Ignore SIGPIPE for now as we are going to write into various
	 * descriptors and are prepared to handle EPIPE ourselves.
	 * The disposition will be reset to the default later in chrootuid
	 * so it won't leak into other executables.
	 */
	signal(SIGPIPE, SIG_IGN);

	notice_msg("accepting connections");

	int ep_timeout = -1;
	int finish_server = 0;

	while (!finish_server) {
		errno = 0;
		struct epoll_event ev[16];
		int fdcount = epoll_wait(d->fd_ep, ev, ARRAY_SIZE(ev), ep_timeout);
		if (fdcount < 0) {
			if (errno == EINTR)
				continue;
			perror_msg("epoll_wait");
			break;
		}

		for (i = 0; i < fdcount; i++) {
			if (!(ev[i].events & EPOLLIN))
				continue;

			if (ev[i].data.fd == d->fd_signal) {
				struct signalfd_siginfo fdsi;
				ssize_t size;

				size = read_retry(d->fd_signal, &fdsi, sizeof(fdsi));
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
					wait_sessions(WNOHANG);
					break;
				default:
					error_msg("unexpected signal %d ignored",
						  fdsi.ssi_signo);
					break;
				}
			}
		}
		for (i = 0; !finish_server && i < fdcount; i++) {
			if (!(ev[i].events & EPOLLIN))
				continue;

			if (ev[i].data.fd == d->fd_conn) {
				int conn = accept4(d->fd_conn, NULL, 0, SOCK_CLOEXEC);
				if (conn < 0) {
					perror_msg("accept4");
					continue;
				}

				process_request(d, conn);
				xclose(&conn);
			}
		}
	}

	notice_msg("shutting down");

	/* Notify child processes. */
	xclose(&d->fd_pipe[1]);

	wait_sessions(0);

	if (pidfile)
		remove_pid(pidfile);

	return EXIT_SUCCESS;
}
