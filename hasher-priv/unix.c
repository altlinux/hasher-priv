/*
 * The unix domain listening module for the hasher-privd server program.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "fds.h"
#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

/* This function may be executed with root or child privileges. */

int
unix_listen(const char *file_name)
{
	struct sockaddr_un sun = { .sun_family = AF_UNIX };
	strncat(sun.sun_path, file_name, sizeof(sun.sun_path) - 1);

	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror_msg("socket");
		return -1;
	}

	if (bind(fd, (struct sockaddr *) &sun, (socklen_t) sizeof sun)) {
		perror_msg("bind: %s", sun.sun_path);
		xclose(&fd);
		return -1;
	}

	if (listen(fd, 16) < 0) {
		perror_msg("listen: %s", sun.sun_path);
		xclose(&fd);
		return -1;
	}

	return fd;
}

/* This function may be executed with root privileges. */

int
log_listen(void)
{
	static const char log_path[] = "log";

	int fd = unix_listen(log_path);

	if (fd >= 0 && chmod(log_path, 0622)) {
		perror_msg("chmod: %s", log_path);
		xclose(&fd);
	}

	return fd;
}

/* This function may be executed with caller privileges. */

int
unix_accept(int fd)
{
	struct sockaddr_un sun;
	socklen_t len = sizeof(sun);

	int rc = accept(fd, (struct sockaddr *) &sun, &len);

	if (rc < 0) {
		perror_msg("accept");
		fputc('\r', stderr);
	}

	return rc;
}
