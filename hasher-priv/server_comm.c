/*
 * The server communication module for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "communication.h"
#include "error_prints.h"
#include "io_loop.h"
#include "server_comm.h"
#include "sockets.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int
send_response_to_client(int conn, int rc, const char *fmt, ...)
{
	char *data = 0;
	unsigned int size = 0;

	if (fmt && *fmt) {
		va_list ap;
		va_start(ap, fmt);
		int len = vsnprintf(NULL, 0, fmt, ap);
		va_end(ap);

		if (len < 0) {
			error_msg("unable to calculate message size");
			return -1;
		}

		size = (unsigned int) len + 1;
		data = malloc(size);
		if (data) {
			va_start(ap, fmt);
			vsnprintf(data, size, fmt, ap);
			va_end(ap);
		} else {
			perror_msg("malloc");
			size = 0;
		}
	}

	srv_cmd_resp_t rs = {
		.rc = rc,
		.len = size
	};

	struct iovec iov = {
		.iov_base = &rs,
		.iov_len = sizeof(rs)
	};

	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1
	};

	/* First, send the header. */

	errno = 0;
	if (sendmsg_retry(conn, &msg, MSG_NOSIGNAL) < 0) {
		free(data);

		/* The client left without waiting for an answer. */
		if (errno == EPIPE)
			return 0;

		perror_msg("sendmsg");
		return -1;
	}

	if (!data)
		return 0;

	/* Second, send the data. */

	msg.msg_iov[0].iov_base = data;
	msg.msg_iov[0].iov_len = size;

	rc = 0;
	errno = 0;
	if (sendmsg_retry(conn, &msg, MSG_NOSIGNAL) < 0 && errno != EPIPE) {
		perror_msg("sendmsg");
		rc = -1;
	}

	free(data);

	return rc;
}
