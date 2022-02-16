
/*
  Copyright (C) 2005, 2009-2019  Dmitry V. Levin <ldv@altlinux.org>

  The descriptor passing routines for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with caller or child privileges. */

#include "error_prints.h"
#include "io_loop.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "priv.h"

union cmsg_data_u
{
	int    *i;
	unsigned char *c;
};

/* This function may be executed with child privileges. */
void
fd_send(int ctl, int *fds, unsigned int n_fds,
	const char *data, size_t data_len)
{
	const size_t clen = sizeof(fds[0]) * n_fds;
	union {
		char buf[CMSG_SPACE(clen)];
		struct cmsghdr align;
	} u;

	if (!data_len) {
		/*
		 * We have to send at least a byte of regular data
		 * in order to send some ancillary data.
		 */
		data = u.buf;
		data_len = 1;
	}

	struct iovec iov = {
		.iov_base = (char *) data,
		.iov_len = data_len
	};

	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = u.buf,
		.msg_controllen = sizeof(u.buf)
	};

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(clen);
	memcpy(CMSG_DATA(cmsg), fds, clen);

	ssize_t rc = sendmsg_retry(ctl, &msg, 0);
	if (rc != (ssize_t) data_len) {
		if (rc < 0) {
			perror_msg_and_die("sendmsg");
		} else if (rc) {
			error_msg_and_die("expected size %lu, got %lu",
					  (unsigned long) data_len,
					  (unsigned long) rc);
		} else {
			error_msg_and_die("unexpected EOF");
		}
	}
}

/* This function may be executed with caller privileges. */

int
fd_recv(int ctl, char *data, size_t data_len)
{
	struct iovec vec;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	union cmsg_data_u cmsg_data_p;
	char    buf[CMSG_SPACE(sizeof(int))];

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;

	vec.iov_base = data;
	vec.iov_len = data_len;

	ssize_t rc = recvmsg_retry(ctl, &msg, 0);
	if (rc != (ssize_t) data_len) {
		if (rc < 0) {
			perror_msg("recvmsg");
			fputc('\r', stderr);
		} else {
			if (rc)
				error_msg("expected size %u, got %u\r",
					  (unsigned int) data_len,
					  (unsigned int) rc);
			else
				error_msg("unexpected EOF\r");
		}
		return -1;
	}

	if (!(cmsg = CMSG_FIRSTHDR(&msg)))
	{
		error_msg("no message header\r");
		return -1;
	}

	if (cmsg->cmsg_type != SCM_RIGHTS)
	{
		error_msg("expected type %u, got %u\r",
			  SCM_RIGHTS, cmsg->cmsg_type);
		return -1;
	}

	cmsg_data_p.c = CMSG_DATA(cmsg);
	return *cmsg_data_p.i;
}
