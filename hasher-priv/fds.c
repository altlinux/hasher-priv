
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The file descriptor sanitizer for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root or caller privileges. */

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <limits.h>

#ifndef CLOSE_RANGE_CLOEXEC
# define CLOSE_RANGE_CLOEXEC	(1U << 2)
#endif

#include "priv.h"

/* This function may be executed with root privileges. */
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

/* This function may be executed with root privileges. */
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

/* This function may be executed with root privileges. */
static void
close_range_brutely(int from, int to)
{
	for (; from < to; ++from) {
		(void) close(from);
	}
}

/* This function may be executed with root privileges. */
static void
cloexec_range_brutely(int from, int to)
{
	for (; from < to; ++from) {
		int     flags = fcntl(from, F_GETFD, 0);

		if (flags < 0)
			continue;

		int     newflags = flags | FD_CLOEXEC;

		if (flags != newflags && fcntl(from, F_SETFD, newflags))
			error(EXIT_FAILURE, errno, "fcntl F_SETFD");
	}
}

/* This function may be executed with root privileges. */
static int
reorder_fd(int start_fd, int *target_fdp)
{
	if (*target_fdp < start_fd) {
		return start_fd;
	}

	if (*target_fdp > start_fd) {
		if (dup2(*target_fdp, start_fd) != start_fd) {
			error(EXIT_FAILURE, errno, "dup2(%d, %d)",
			      *target_fdp, start_fd);
		}
		/* Old *target_fdp will be closed by close_range. */
		*target_fdp = start_fd;
	}

	return ++start_fd;
}

/* This function may be executed with root privileges. */
static int
reorder_fds(int start_fd)
{
	/* Reorder log_fd, other descriptors may be added later. */
	return reorder_fd(start_fd, &log_fd);
}

/* This function may be executed with root privileges. */
void
sanitize_fds(void)
{
	int     fd;

	/* Set safe umask, just in case. */
	umask(077);

	/* Check for stdin, stdout and stderr: they should be present. */
	for (fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd)
	{
		struct stat st;

		if (fstat(fd, &st) < 0)
			/* At this stage, we shouldn't even report error. */
			exit(EXIT_FAILURE);
	}

	/* Reorder descriptors that should be kept open. */
	fd = reorder_fds(fd);

	/* Close all the rest. */
	if (sys_close_range((unsigned int) fd, -1U, 0) < 0) {
		close_range_brutely(fd, get_open_max());
	}

	errno = 0;
}

/* This function may be executed with root privileges. */
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

/* This function may be executed with caller or child privileges. */
void
nullify_stdin(void)
{
	int     fd = open(_PATH_DEVNULL, O_RDONLY);

	if (fd < 0)
		error(EXIT_FAILURE, errno, "open: %s", _PATH_DEVNULL);
	if (fd != STDIN_FILENO)
	{
		if (dup2(fd, STDIN_FILENO) != STDIN_FILENO)
			error(EXIT_FAILURE, errno, "dup2(%d, %d)",
			      fd, STDIN_FILENO);
		if (close(fd) < 0)
			error(EXIT_FAILURE, errno, "close(%d)", fd);
	}
}

/* This function may be executed with caller privileges. */
void
unblock_fd(int fd)
{
	int     flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		error(EXIT_FAILURE, errno, "fcntl F_GETFL");

	int     newflags = flags | O_NONBLOCK;

	if (flags != newflags && fcntl(fd, F_SETFL, newflags) < 0)
		error(EXIT_FAILURE, errno, "fcntl F_SETFL");
}

/* This function may be executed with caller privileges. */
ssize_t
read_retry(int fd, void *buf, size_t count)
{
	return TEMP_FAILURE_RETRY(read(fd, buf, count));
}

/* This function may be executed with caller privileges. */
ssize_t
write_retry(int fd, const void *buf, size_t count)
{
	return TEMP_FAILURE_RETRY(write(fd, buf, count));
}

/* This function may be executed with caller privileges. */
ssize_t
write_loop(int fd, const char *buffer, size_t count)
{
	ssize_t offset = 0;

	while (count > 0)
	{
		ssize_t block = write_retry(fd, &buffer[offset], count);

		if (block <= 0)
			return offset ? : block;
		offset += block;
		count -= (size_t) block;
	}
	return offset;
}

/* This function may be executed with caller privileges. */
void
fds_add_fd(fd_set *fds, int *max_fd, const int fd)
{
	if (fd < 0)
		return;

	FD_SET(fd, fds);
	if (fd > *max_fd)
		*max_fd = fd;
}

/* This function may be executed with caller privileges. */
int
fds_isset(fd_set *fds, const int fd)
{
	return (fd >= 0) && FD_ISSET(fd, fds);
}
