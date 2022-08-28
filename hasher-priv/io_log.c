/*
 * The chrootuid parent log I/O handler for the hasher-priv project.
 *
 * Copyright (C) 2008-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller privileges. */

#include "error_prints.h"
#include "fds.h"
#include "fd_set.h"
#include "io_log.h"
#include "io_loop.h"
#include "parent.h"
#include "unblock_fd.h"
#include "unix.h"
#include "xmalloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static int *fd_list;
static size_t fd_count;

static void
fd_new(int fd)
{
	if (fd < 0)
		return;

	size_t  i;

	for (i = 0; i < fd_count; ++i)
		if (fd_list[i] < 0)
			break;

	if (i == fd_count)
		fd_list = xreallocarray(fd_list, ++fd_count, sizeof(*fd_list));

	fd_list[i] = fd;
	unblock_fd(fd);
}

static void
fd_free(const int fd)
{
	size_t  i;

	for (i = 0; i < fd_count; ++i) {
		if (fd == fd_list[i]) {
			xclose(&fd_list[i]);
			return;
		}
	}

	error_msg_and_die("descriptor %d not found, count=%lu",
			  fd, (unsigned long) fd_count);
}

void
log_handle_new(const int fd, fd_set *fds)
{
	if (fds_isset(fds, fd))
		fd_new(unix_accept(fd));
}

void
fds_add_log(fd_set *fds, int *max_fd)
{
	size_t  i;

	for (i = 0; i < fd_count; ++i)
		fds_add_fd(fds, max_fd, fd_list[i]);
}

static void
copy_log(const int fd)
{
	ssize_t i;
	char    buf[BUFSIZ];

	i = read_retry(fd, buf, sizeof(buf) - 2);
	if (i <= 0)
	{
		fd_free(fd);
		return;
	}

	buf[i] = '\0';

	size_t  n = strlen(buf);

	if (n > 0 && buf[n - 1] != '\n')
	{
		buf[n++] = '\r';
		buf[n++] = '\n';
	}

	xwrite_all(STDERR_FILENO, buf, n);
}


void
log_handle_select(fd_set *fds)
{
	size_t  i;

	for (i = 0; i < fd_count; ++i)
		if (fds_isset(fds, fd_list[i]))
			copy_log(fd_list[i]);
}
