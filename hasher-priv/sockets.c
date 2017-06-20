/*
 * A collection of helpers to simplify the use of sockets
 * for the hasher-priv project.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "fds.h"
#include "io_loop.h"
#include "sockets.h"
#include "xstring.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * All necessary path components are assumed to be created
 * with proper permissions.
 *
 * This function may be executed with root privileges.
 */
int
srv_listen(const char *file_name)
{
	struct sockaddr_un sun = { .sun_family = AF_UNIX };
	xsprintf(sun.sun_path, "%s", file_name);

	if (unlink(sun.sun_path) && errno != ENOENT) {
		perror_msg("unlink: %s", sun.sun_path);
		return -1;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (fd < 0) {
		perror_msg("socket");
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&sun, (socklen_t) sizeof(sun))) {
		perror_msg("bind: %s", sun.sun_path);
		xclose(&fd);
		return -1;
	}

	if (listen(fd, 16)) {
		perror_msg("listen: %s", sun.sun_path);
		xclose(&fd);
		return -1;
	}

	return fd;
}

/* This function may be executed with caller privileges. */
int
srv_connect(const char *dir_name, const char *file_name)
{
	int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		perror_msg_and_die("socket");

	struct sockaddr_un sun = { .sun_family = AF_UNIX };
	xsprintf(sun.sun_path, "%s/%s", dir_name, file_name);

	if (connect(fd, (const struct sockaddr *)&sun, sizeof(sun)))
		perror_msg_and_die("%s", sun.sun_path);

	return fd;
}

/* This function may be executed with caller privileges. */
int
srv_try_connect(const char *dir_name, const char *file_name)
{
	int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		perror_msg_and_die("socket");

	struct sockaddr_un sun = { .sun_family = AF_UNIX };
	xsprintf(sun.sun_path, "%s/%s", dir_name, file_name);

	if (connect(fd, (const struct sockaddr *)&sun, sizeof(sun))) {
		int saved_errno = errno;
		xclose(&fd);
		errno = saved_errno;
	}

	return fd;
}

/* This function may be executed with root privileges. */
int
get_peercred(int fd, pid_t *pid, uid_t *uid, gid_t *gid)
{
	struct ucred uc;
	socklen_t len = sizeof(struct ucred);

	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &uc, &len) < 0) {
		perror_msg("getsockopt");
		return -1;
	}

	if (len != sizeof(struct ucred)) {
		error_msg("returned length does not match expectations");
		return -1;
	}

	if (pid)
		*pid = uc.pid;

	if (uid)
		*uid = uc.uid;

	if (gid)
		*gid = uc.gid;

	return 0;
}

/* This function may be executed with root privileges. */
int
set_recv_timeout(int fd, int secs)
{
	struct timeval tv = { .tv_sec = secs };

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror_msg("setsockopt");
		return -1;
	}
	return 0;
}

/* This function may be executed with root privileges. */
int
xsendmsg(int fd, void *data, size_t len)
{
	struct iovec iov = { 0 };
	struct msghdr msg = { 0 };

	iov.iov_base = data;
	iov.iov_len  = len;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ssize_t n = sendmsg_retry(fd, &msg, MSG_NOSIGNAL);

	if (n != (ssize_t) len) {
		if (n < 0)
			perror_msg("sendmsg");
		else if (n)
			error_msg("sendmsg: expected size %lu, got %lu",
				  (unsigned long) len, (unsigned long) n);
		else
			error_msg("sendmsg: unexpected EOF");
		return -1;
	}

	return 0;
}

/* This function may be executed with root privileges. */
int
xrecvmsg(int fd, void *data, size_t len)
{
	struct iovec iov = { 0 };
	struct msghdr msg = { 0 };

	iov.iov_base = data;
	iov.iov_len  = len;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ssize_t n = recvmsg_retry(fd, &msg, MSG_WAITALL);

	if (n != (ssize_t) len) {
		if (n < 0)
			perror_msg("recvmsg");
		else if (n)
			error_msg("recvmsg: expected size %lu, got %lu",
				  (unsigned long) len, (unsigned long) n);
		else
			error_msg("recvmsg: unexpected EOF");
		return -1;
	}

	return 0;
}
