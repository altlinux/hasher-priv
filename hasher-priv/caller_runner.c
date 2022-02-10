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

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "caller_data.h"
#include "caller_job.h"
#include "caller_runner.h"
#include "communication.h"
#include "error_prints.h"
#include "executors.h"
#include "fds.h"
#include "job2str.h"
#include "logging.h"
#include "macros.h"
#include "server_comm.h"
#include "signals.h"
#include "title.h"
#include "xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

ATTRIBUTE_NORETURN
static void
job_executor(struct job *job)
{
	setproctitle("%s %s/%u:%u",
		     job2str(job->type), caller_user, caller_uid, caller_num);

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

ATTRIBUTE_NORETURN
static void
job_runner(struct hadaemon *d, int conn, struct job *job)
{
	setproctitle("runner %s/%u:%u: %s",
		     caller_user, caller_uid, caller_num, job2str(job->type));

	xclose(&d->fd_pipe[0]);
	xclose(&d->fd_ep);
	xclose(&d->fd_signal);
	xclose(&d->fd_conn);

	/*
	 * As we are not going to handle signals, unblock them.
	 */
	unblock_all_signals();

	int rc;
	int exit_status;
	int pid = spawn_job_executor(job);
	if (pid > 0) {
		deallocate_job_resources(job);
		rc = wait_job(job, pid);
		exit_status = EXIT_SUCCESS;
	} else {
		rc = exit_status = EXIT_FAILURE;
	}

	/* Notify the client about the results. */
	send_response_to_client(conn, rc, NULL);

	exit(exit_status);
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
		return pid;
	}
	job_runner(d, conn, job);
}
