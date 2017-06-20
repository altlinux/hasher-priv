/*
 * The descriptor passing routines for the hasher-priv project.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller or child privileges. */

#include "error_prints.h"
#include "io_loop.h"
#include "pass.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

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
fd_recv(int ctl, int *fds, unsigned int n_fds, char *data, size_t data_len)
{
	const size_t clen = sizeof(fds[0]) * n_fds;
	union {
		char buf[CMSG_SPACE(clen)];
		struct cmsghdr align;
	} u;

	char dummy;
	if (!data_len) {
		/*
		 * We have to receive at least a byte of regular data
		 * in order to receive some ancillary data.
		 */
		data = &dummy;
		data_len = sizeof(dummy);
	}

	struct iovec iov = {
		.iov_base = data,
		.iov_len = data_len
	};

	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = u.buf,
		.msg_controllen = sizeof(u.buf)
	};

	ssize_t rc = recvmsg_retry(ctl, &msg, 0);
	if (rc != (ssize_t) data_len) {
		if (rc < 0) {
			perror_msg("recvmsg");
		} else if (rc) {
			error_msg("expected size %lu, got %lu",
				  (unsigned long) data_len,
				  (unsigned long) rc);
		} else {
			error_msg("unexpected EOF");
		}
		return -1;
	}

	/*
	 * Since the peer is expected to follow the protocol,
	 * be defensive and bail out in case of any irregularities.
	 * This means up to n_fds - 1 descriptors might be received
	 * and leaked in the worst case scenario.
	 * Call callers are prepared to handle that situation properly.
	 */
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	if (!cmsg) {
		error_msg("no ancillary data");
		return -1;
	}

	if (cmsg->cmsg_level != SOL_SOCKET) {
		error_msg("expected SOL_SOCKET, got %d", cmsg->cmsg_level);
		return -1;
	}

	if (cmsg->cmsg_type != SCM_RIGHTS) {
		error_msg("expected SCM_RIGHTS, got %d", cmsg->cmsg_type);
		return -1;
	}

	size_t recv_len = cmsg->cmsg_len - CMSG_LEN(0);
	if (recv_len != clen) {
		error_msg("SCM_RIGHTS: expected size %lu, got %lu",
			  (unsigned long) clen, (unsigned long) recv_len);
		return -1;
	}

	memcpy(fds, CMSG_DATA(cmsg), clen);

	if (CMSG_NXTHDR(&msg, cmsg)) {
		error_msg("stray ancillary data");
		return -1;
	}

	return 0;
}
