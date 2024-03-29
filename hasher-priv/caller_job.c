/*
 * The job handling part of the hasher-privd server program.
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
#include "pass.h"
#include "process.h"
#include "server_comm.h"
#include "signals.h"
#include "sockets.h"
#include "title.h"
#include "xmalloc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

static int
validate_arguments(job_enum_t job, char **argv)
{
	int required_args = 0, more_args = 0;
	int rc = -1;
	int argc = 0;

	while (argv && argv[argc])
		argc++;

	switch (job) {
	case JOB_GETCONF:
	case JOB_KILLUID:
	case JOB_GETUGID1:
	case JOB_GETUGID2:
		required_args = 0;
		break;
	case JOB_CHROOTUID1:
	case JOB_CHROOTUID2:
		more_args = 1;
		required_args = 1;
		break;
	default:
		error_msg("unknown job type: %u", job);
		return rc;
	}

	if (argc < 0)
		error_msg("number of arguments must be a positive but got %d", argc);

	else if (argc < required_args)
		error_msg("%s job requires at least %d arguments but got %d",
		    job2str(job), required_args, argc);

	else if (argc > required_args && !more_args)
		error_msg("%s job requires exactly %d arguments but got %d",
		    job2str(job), required_args, argc);

	else
		rc = 0;

	return rc;
}

void
deallocate_job_resources(struct job *job)
{
	xclose(&job->chroot_fd);

	for (unsigned int i = 0; i < ARRAY_SIZE(job->std_fds); ++i) {
		xclose(&job->std_fds[i]);
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(job->pipe_fds); ++i) {
		xclose(&job->pipe_fds[i]);
	}

	if (job->env) {
		free(job->env[0]);
		free(job->env);
	}

	if (job->argv) {
		free(job->argv[0]);
		free(job->argv);
	}
}

ATTRIBUTE_NORETURN
static void
cancel_job(int conn, struct job *job, const char *reason)
{
	deallocate_job_resources(job);
	send_response_to_client(conn, CMD_STATUS_FAILED, "%s", reason);
	exit(EXIT_FAILURE);
}

ATTRIBUTE_NORETURN
static void
respond_server_error(int conn, struct job *job)
{
	/* Internal server error. */
	cancel_job(conn, job, "command failed");
}

ATTRIBUTE_NORETURN
static void
respond_bad_request(int conn, struct job *job)
{
	/* The client has violated the protocol. */
	cancel_job(conn, job, "bad request");
}

static int
recv_strings_from_client(int conn, char ***argv, unsigned int len)
{
	enum {
		MAX_ARGS_SIZE = 0x20000	/* ARG_MAX */
	};
	if (len > MAX_ARGS_SIZE) {
		error_msg("strings of total size %u rejected", len);
		return -1;
	}

	/* A buffer for strings. */
	char *args = malloc(len + 1);
	if (!args) {
		perror_msg("malloc");
		return -1;
	}

	if (len > 0 && xrecvmsg(conn, args, len) < 0) {
		goto err;
	}

	/* Just in case args is not NUL-terminated. */
	args[len] = '\0';

	unsigned int n = 0;
	for (char *p = args; p < args + len; p = (char *) rawmemchr(p, '\0') + 1)
		++n;

	/* A buffer for pointers to strings. */
	char **av = calloc(n + 1, sizeof(char *));
	if (!av) {
		perror_msg("calloc");
		goto err;
	}

	n = 0;
	for (char *p = args; p < args + len; p = (char *) rawmemchr(p, '\0') + 1)
		av[n++] = p;

	*argv = av;

	return 0;

err:
	free(args);
	return -1;
}

int
wait_job(const struct job *job, pid_t pid)
{
	int rc = EXIT_FAILURE;
	int status;

	pid = waitpid_retry(pid, &status, 0);
	if (pid < 0) {
		perror_msg("waitpid");
	} else if (WIFEXITED(status)) {
		rc = WEXITSTATUS(status);
		if (rc) {
			notice_msg("%s/%u:%u: "
				   "%s: process %d exited, status=%d",
				   caller_user, caller_uid, caller_num,
				   job2str(job->type), pid, rc);
		} else {
			info_msg("%s/%u:%u: "
				 "%s: process %d exited",
				 caller_user, caller_uid, caller_num,
				 job2str(job->type), pid);
		}
	} else if (WIFSIGNALED(status)) {
		int signo = WTERMSIG(status);
		rc = 128 + signo;
		notice_msg("%s/%u:%u: "
			   "%s: process %d terminated by signal %d",
			   caller_user, caller_uid, caller_num,
			   job2str(job->type), pid, signo);
	}

	return rc;
}

/**
 * Returns < 0 if called with an invalid job, 0 otherwise.
 */
static int
validate_job(const struct job *job)
{
	/* Check command correctness. */
	switch (job->type) {
	case JOB_CHROOTUID1:
	case JOB_CHROOTUID2:
		if (!(job->mask & CMD_JOB_CHROOT_FD)) {
			error_msg("no root directory");
			return -1;
		}
		if (!(job->mask & CMD_JOB_ARGUMENTS)) {
			error_msg("no arguments");
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

ATTRIBUTE_NORETURN
static void
receive_job_request(struct hadaemon *d, int conn, struct job *job)
{
	setproctitle("job %s/%u:%u", caller_user, caller_uid, caller_num);

	xclose(&d->fd_ep);
	xclose(&d->fd_signal);
	xclose(&d->fd_conn);
	xclose(&d->fd_pipe[0]);

	for (;;) {
		cmd_header_t hdr = { 0 };

		if (xrecvmsg(conn, &hdr, sizeof(hdr)) < 0)
			respond_server_error(conn, job);

		if (job->mask & hdr.type) {
			error_msg("repeated command: %d", hdr.type);
			respond_bad_request(conn, job);
		}

		job->mask |= hdr.type;

		switch (hdr.type) {
		case CMD_JOB_TYPE:
			job->type = hdr.len;
			break;

		case CMD_JOB_PERSONALITY:
			job->persona = hdr.len;
			break;

		case CMD_JOB_CHROOT_FD:
			if ((hdr.len != sizeof(job->chroot_fd)) ||
			    fd_recv(conn, &job->chroot_fd, 1, 0, 0) < 0)
				respond_bad_request(conn, job);
			break;

		case CMD_JOB_FDS:
			if ((hdr.len !=
			     sizeof(job->std_fds[0]) * ARRAY_SIZE(job->std_fds)) ||
			    fd_recv(conn, job->std_fds,
				    ARRAY_SIZE(job->std_fds), 0, 0) < 0)
				respond_bad_request(conn, job);
			break;

		case CMD_JOB_ARGUMENTS:
			if (recv_strings_from_client(conn, &job->argv, hdr.len) < 0 ||
			    validate_arguments(job->type, job->argv) < 0)
				respond_bad_request(conn, job);
			break;

		case CMD_JOB_ENVIRON:
			if (recv_strings_from_client(conn, &job->env, hdr.len) < 0)
				respond_bad_request(conn, job);
			break;

		case CMD_JOB_RUN:
			if (hdr.len || validate_job(job) < 0)
				respond_bad_request(conn, job);
			if (spawn_job_runner(d, conn, job) < 0)
				respond_server_error(conn, job);
			/* spawn_job_runner() sends a response by itself and exits. */
			exit(EXIT_SUCCESS);

		default:
			error_msg("unknown command: %d", hdr.type);
			respond_bad_request(conn, job);
		}

		send_response_to_client(conn, CMD_STATUS_DONE, NULL);
	}
}

int
spawn_job_request_handler(struct hadaemon *d, int conn)
{
	struct job job = {
		.persona = -1U,
		.chroot_fd = -1,
		.std_fds = { -1, -1, -1 },
		.pipe_fds = { -1, -1 }
	};

	/*
	 * Move the job request handling into a subprocess
	 * to simplify resource handling.
	 */
	pid_t pid = fork();
	if (pid < 0) {
		perror_msg("fork");
		send_response_to_client(conn, CMD_STATUS_FAILED,
					"command failed");
		return EXIT_FAILURE;
	}
	if (pid > 0) {
		return wait_job(&job, pid);
	}

	receive_job_request(d, conn, &job);
}
