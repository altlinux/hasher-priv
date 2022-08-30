/*
 * The job running part of the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root or caller privileges. */

#include "caller_config.h"
#include "caller_data.h"
#include "caller_job.h"
#include "caller_runner.h"
#include "cgroup.h"
#include "communication.h"
#include "epoll.h"
#include "error_prints.h"
#include "executors.h"
#include "fds.h"
#include "io_loop.h"
#include "job2str.h"
#include "logging.h"
#include "macros.h"
#include "server_comm.h"
#include "signals.h"
#include "title.h"
#include "xmalloc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>

static int
is_job_spawning(const struct job *job)
{
	switch (job->type) {
		case JOB_CHROOTUID1:
		case JOB_CHROOTUID2:
			return 1;
		default:
			return 0;
	}
}

ATTRIBUTE_NORETURN
static void
job_executor(struct job *job)
{
	setproctitle("%s %s/%u:%u",
		     job2str(job->type), caller_user, caller_uid, caller_num);

	/*
	 * As we are not going to handle signals, unblock them.
	 */
	unblock_all_signals();

	/* Reset standard descriptors. */
	for (unsigned int i = 0; i < ARRAY_SIZE(job->std_fds); ++i)
		move_fd(&job->std_fds[i], (int) i);

	init_log_standalone();

	chroot_fd = job->chroot_fd;

	/* Check and sanitize file descriptors. */
	sanitize_fds();

	/* We don't need environment variables. */
	if (clearenv())
		perror_msg_and_die("clearenv");

	/* Parse job environment for configuration options. */
	parse_env(job->env);

	/* Finally, execute the requested job. */
	int rc;
	switch (job->type) {
		case JOB_GETCONF:
			rc = do_getconf();
			break;
		case JOB_KILLUID:
			rc = do_killuid();
			break;
		case JOB_GETUGID1:
			rc = do_getugid1();
			break;
		case JOB_CHROOTUID1:
			rc = do_chrootuid1((const char *const *) job->argv);
			break;
		case JOB_GETUGID2:
			rc = do_getugid2();
			break;
		case JOB_CHROOTUID2:
			rc = do_chrootuid2((const char *const *) job->argv);
			break;
		default:
			error_msg_and_die("unknown job %d", job->type);
	}
	exit(rc);
}

static int
spawn_job_executor(struct job *job)
{
	pid_t pid = fork();
	if (pid < 0) {
		perror_msg("fork");
		return -1;
	}
	if (pid > 0) {
		return pid;
	}
	job_executor(job);
}

static void
terminate_executor(const struct job *job, pid_t pid)
{
	if (kill(pid, SIGTERM))
		perror_msg("kill");
	else
		wait_job(job, pid);
}

ATTRIBUTE_NORETURN
static void
respond_job_completion(int conn, int rc)
{
	send_response_to_client(conn, rc, NULL);
	exit(EXIT_SUCCESS);
}

ATTRIBUTE_NORETURN
static void
job_runner(struct hadaemon *d ATTRIBUTE_UNUSED, int conn, struct job *job)
{
	setproctitle("runner %s/%u:%u: %s",
		     caller_user, caller_uid, caller_num, job2str(job->type));

	/*
	 * If the job is a chrootuid, the service daemon will spawn
	 * unprivileged processes on behalf of the client, and these
	 * processes can take as much resources as they are allowed to.
	 * Therefore, chrootuid jobs are moved to the cgroup of the client
	 * so the cgroup regulations of the client apply to them.
	 * Other kinds of jobs are short-lived processes that perform very
	 * specific auxiliary tasks, they are parts of the service daemon
	 * and remain in its cgroup.
	 */
	if (is_job_spawning(job))
		join_caller_cgroup(caller_pid);

	/*
	 * Do not wait for daemon_create_signal_fd() invocation and
	 * block SIGCHLD now because it has to be blocked before fork().
	 */
	block_signal_handler(SIGCHLD, SIG_BLOCK);

	int pid = spawn_job_executor(job);
	if (pid < 0)
		exit(EXIT_FAILURE);

	deallocate_job_resources(job);

	/*
	 * Do not assume that fs.suid_dumpable == 0
	 * and clear the dumpable flag explicitly.
	 */
	if (prctl(PR_SET_DUMPABLE, 0))
		perror_msg_and_die("prctl PR_SET_DUMPABLE");

	if (setgroups(0UL, 0) < 0)
		perror_msg_and_die("setgroups");

	if (setgid(caller_gid) < 0)
		perror_msg_and_die("setgid");

	if (setuid(caller_uid) < 0)
		perror_msg_and_die("setuid");

	/* Process is no longer privileged at this point. */

	if ((d->fd_signal = daemon_create_signal_fd()) < 0)
		perror_msg_and_die("signalfd");

	if ((d->fd_ep = epoll_create1(EPOLL_CLOEXEC)) < 0)
		perror_msg_and_die("epoll_create1");

	/*
	 * We do not listen on any socket.
	 * Instead, we poll `conn' for events.
	 */
	if (epoll_add_in(d->fd_ep, d->fd_signal) < 0 ||
	    epoll_add_hup(d->fd_ep, conn) < 0)
		perror_msg_and_die("epoll_add");

	int finish_server = 0;
	const int ep_timeout = -1;

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

		for (int i = 0; i < fdcount; i++) {
			if (!(ev[i].events & (EPOLLHUP | EPOLLERR)))
				continue;

			if (ev[i].data.fd == conn) {
				notice_msg("client disconnected, terminating executor");
				terminate_executor(job, pid);
				exit(EXIT_FAILURE);
			}
		}

		for (int i = 0; i < fdcount; i++) {
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

				int rc;
				switch (fdsi.ssi_signo) {
				case SIGHUP:
				case SIGINT:
				case SIGQUIT:
				case SIGTERM:
					notice_msg("received signal %d",
						   fdsi.ssi_signo);
					finish_server = 1;
					break;
				case SIGCHLD:
					rc = wait_job(job, pid);
					respond_job_completion(conn, rc);
				default:
					error_msg("unexpected signal %d ignored",
						  fdsi.ssi_signo);
					break;
				}
			}
		}
	}

	info_msg("terminating executor");
	terminate_executor(job, pid);
	send_response_to_client(conn, CMD_STATUS_FAILED, NULL);
	exit(EXIT_FAILURE);
}

pid_t
spawn_job_runner(struct hadaemon *d, int conn, struct job *job)
{
	pid_t pid = fork();
	if (pid < 0) {
		perror_msg("fork");
		return -1;
	}
	if (pid > 0) {
		if (is_job_spawning(job))
			return pid;
		/*
		 * Non-chrootuid jobs are short-lived processes that perform very
		 * specific auxiliary tasks, they are parts of the service daemon
		 * and could be trusted to not linger too long.
		 */
		(void) wait_job(job, pid);
		return 0;
	}
	job_runner(d, conn, job);
}
