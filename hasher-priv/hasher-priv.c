/*
 * The hasher-priv client program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller privileges. */

#include "caller_config.h"
#include "cmdline.h"
#include "communication.h"
#include "error_prints.h"
#include "executors.h"
#include "fds.h"
#include "pass.h"
#include "sockets.h"
#include "xmalloc.h"
#include "xstring.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/un.h>

/* Handle response from the session server. */
static int
recv_response(int conn, const char *name)
{
	srv_cmd_resp_t rs;

	if (xrecvmsg(conn, &rs, sizeof(rs)) < 0) {
		error_msg_and_die("failed to receive header in response"
				  " to %s", name);
	}

	if (rs.len) {
		char *data = xzalloc(rs.len);

		if (xrecvmsg(conn, data, rs.len) < 0) {
			error_msg_and_die("failed to receive data in response"
					  " to %s", name);
		}

		if (*data)
			error_msg("%s: %s", name, data);

		free(data);
	}

	if (rs.rc == CMD_STATUS_FAILED)
		error_msg_and_die("failed");

	return rs.rc;
}

/* Connect to the main server and request a session server. */
static void
request_session(const char *dir_name, const char *file_name, unsigned num)
{
	int conn = srv_connect(dir_name, file_name);

	cmd_header_t hdr = {
		.type = CMD_OPEN_SESSION,
		.len = num,
	};

	if (xsendmsg(conn, &hdr, sizeof(hdr)) < 0)
		error_msg_and_die("failed to send header");

	(void) recv_response(conn, "session request");

	if (xclose(&conn))
		perror_msg_and_die("close");
}

/* Open a user session. */
static int
connect_to_session(void)
{
	char socketname[UNIX_PATH_MAX];
	xsprintf(socketname, "%d:%u", geteuid(), caller_num);

	/* Try to connect directly to the session server. */
	int fd = srv_try_connect(SOCKETDIR, socketname);
	if (fd >= 0)
		return fd;

	/* Connect to remote server and request a session. */
	request_session(SOCKETDIR, MAIN_SOCKET_BASE_NAME, caller_num);

	/* Connect to the session server. */
	return srv_connect(SOCKETDIR, socketname);
}

static void
send_type(int conn, job_enum_t type)
{
	cmd_header_t hdr = {
		.type = CMD_JOB_TYPE,
		.len = type,
	};

	if (xsendmsg(conn, &hdr, sizeof(hdr)) < 0)
		error_msg_and_die("failed to send header");

	(void) recv_response(conn, "job type");
}

static void
send_fds(int conn, cmd_enum_t cmd, const char *name,
	 int *fds, unsigned int n_fds)
{
	cmd_header_t hdr = {
		.type = cmd,
		.len = sizeof(fds[0]) * n_fds,
	};

	if (xsendmsg(conn, &hdr, sizeof(hdr)) < 0)
		error_msg_and_die("%s: failed to send header", name);

	fd_send(conn, fds, n_fds, 0, 0);

	(void) recv_response(conn, name);
}

static void
send_strings(int conn, cmd_enum_t cmd, const char *name, const char **argv)
{
	size_t size = 0;
	for (const char **args = argv; args && *args; ++args)
		size += strlen(*args) + 1;

	cmd_header_t hdr = {
		.type = cmd,
		.len = (unsigned int) size
	};
	if (hdr.len != size)
		error_msg_and_die("%s: too big to send", name);

	if (xsendmsg(conn, &hdr, sizeof(hdr)) < 0)
		error_msg_and_die("%s: error sending header", name);

	for (const char **args = argv; args && *args; ++args)
		if (xsendmsg(conn, (char *) *args, strlen(*args) + 1) < 0)
			error_msg_and_die("%s: error sending data", name);

	(void) recv_response(conn, name);
}

static int
send_run(int conn)
{
	cmd_header_t hdr = {
		.type = CMD_JOB_RUN,
	};

	if (xsendmsg(conn, &hdr, sizeof(hdr)) < 0)
		error_msg_and_die("failed to send header");

	return recv_response(conn, "run");
}

int
main(int ac, const char *av[], const char *ev[])
{
	/* Check and sanitize file descriptors. */
	sanitize_fds();

	/* Parse command line arguments. */
	const char **args;
	job_enum_t job = parse_cmdline(ac, av, &args);

	/* Open a user session */
	int conn = connect_to_session();

	int fds[] = {
		STDIN_FILENO,
		STDOUT_FILENO,
		STDERR_FILENO,
	};
	send_type(conn, job);
	send_fds(conn, CMD_JOB_FDS, "stdio", fds, ARRAY_SIZE(fds));
	if (args)
		send_strings(conn, CMD_JOB_ARGUMENTS, "arguments", args);
	if (job == JOB_CHROOTUID1 || job == JOB_CHROOTUID2) {
		send_strings(conn, CMD_JOB_ENVIRON, "environment", ev);
		send_fds(conn, CMD_JOB_CHROOT_FD, "chroot descriptor",
			 &chroot_fd, 1);
	}

	return send_run(conn);
}
