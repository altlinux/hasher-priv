/*
 * The file descriptor sanitizer for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <limits.h>

#ifndef CLOSE_RANGE_CLOEXEC
# define CLOSE_RANGE_CLOEXEC	(1U << 2)
#endif

#include "priv.h"

int log_fd = -1;

static int
get_open_max(void)
{
	long int i;

	i = sysconf(_SC_OPEN_MAX);
	if (i < NR_OPEN)
		i = NR_OPEN;
	if (i > INT_MAX)
		i = INT_MAX;

	return (int) i;
}

#if defined HAVE_CLOSE_RANGE
static int
sys_close_range(unsigned int from, unsigned int to, unsigned int flags)
{
	return close_range(from, to, (int) flags);
}
#elif defined __NR_close_range
static int
sys_close_range(unsigned int from, unsigned int to, unsigned int flags)
{
	return syscall(__NR_close_range, from, to, flags) < 0 ? -1 : 0;
}
#else
static int
sys_close_range(unsigned int __attribute__((unused)) from,
		unsigned int __attribute__((unused)) to,
		unsigned int __attribute__((unused)) flags)
{
	errno = ENOSYS;
	return -1;
}
#endif

static void
close_range_brutely(int from, int to)
{
	for (; from < to; ++from) {
		(void) close(from);
	}
}

static void
cloexec_range_brutely(int from, int to)
{
	for (; from < to; ++from) {
		int     flags = fcntl(from, F_GETFD, 0);

		if (flags < 0)
			continue;

		int     newflags = flags | FD_CLOEXEC;

		if (flags != newflags && fcntl(from, F_SETFD, newflags))
			perror_msg_and_die("fcntl F_SETFD");
	}
}

static int
reorder_fd(int start_fd, int *target_fdp)
{
	if (*target_fdp < start_fd) {
		return start_fd;
	}

	if (*target_fdp > start_fd) {
		if (dup2(*target_fdp, start_fd) != start_fd) {
			perror_msg_and_die("dup2(%d, %d)",
					   *target_fdp, start_fd);
		}
		/* Old *target_fdp will be closed by close_range. */
		*target_fdp = start_fd;
	}

	return ++start_fd;
}

static int
reorder_fds(int start_fd)
{
	/* Reorder log_fd, other descriptors may be added later. */
	return reorder_fd(start_fd, &log_fd);
}

void
sanitize_fds(void)
{
	int     fd;

	/* Set safe umask, just in case. */
	umask(077);

	/* Check for stdin, stdout and stderr: they should be present. */
	for (fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd) {
		struct stat st;

		if (fstat(fd, &st) < 0)
			/* At this stage, we shouldn't even report error. */
			die();
	}

	/* Reorder descriptors that should be kept open. */
	fd = reorder_fds(fd);

	/* Close all the rest. */
	if (sys_close_range((unsigned int) fd, -1U, 0) < 0) {
		close_range_brutely(fd, get_open_max());
	}

	errno = 0;
}

void
cloexec_fds(void)
{
	/* Set close-on-exec flag on all non-standard descriptors. */
	int     fd = STDERR_FILENO + 1;

	if (sys_close_range((unsigned int) fd, -1U, CLOSE_RANGE_CLOEXEC) < 0) {
		cloexec_range_brutely(fd, get_open_max());
	}

	errno = 0;
}
