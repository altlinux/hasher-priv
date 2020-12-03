
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
sys_close_range(unsigned int from, unsigned int to)
{
	return close_range(from, to, 0);
}
#elif defined __NR_close_range
static int
sys_close_range(unsigned int from, unsigned int to)
{
	return syscall(__NR_close_range, from, to, 0) < 0 ? -1 : 0;
}
#else
static int
sys_close_range(unsigned int __attribute__((unused)) from, unsigned int __attribute__((unused)) to)
{
	errno = ENOSYS;
	return -1;
}
#endif

/* This function may be executed with root privileges. */
static int
attempt_efficient_close_range(int from, int to)
{
	int r;

	unsigned int u_from = (unsigned int)from;
	unsigned int u_to = (unsigned int)to;
	unsigned int u_log_fd = (unsigned int)log_fd;

	if (u_log_fd < u_from || u_log_fd > u_to) {
		/* Close the whole range. */
		r = sys_close_range(u_from, u_to);
		if (r >= 0)
			return 1;
	}
	else if (u_from == u_log_fd) {
		r = sys_close_range(u_from + 1, u_to);
		if (r >= 0)
			return 1;
	}
	else if (u_log_fd == u_to) {
		r = sys_close_range(u_from, u_to - 1);
		if (r >= 0)
			return 1;
	}
	else if (u_from < u_log_fd && u_log_fd < u_to) {
		/* Close two split ranges. Both calls must succeed. */
		r = sys_close_range(u_from, u_log_fd - 1);
		if (r < 0) /* reverse! */
			return 0;
		r = sys_close_range(u_log_fd + 1, u_to);
		if (r < 0)
			return 0;
		return 1;
	}
	return 0;
}

/* This function may be executed with root privileges. */
static void
close_range_brutely(int from, int to)
{
	for (; from < to; ++from)
		if (log_fd < 0 || from != log_fd)
			(void) close(from);
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

	/* Close all the rest. */
	if (!attempt_efficient_close_range(fd, -1)) {
		close_range_brutely(fd, get_open_max());
	}

	errno = 0;
}

/* This function may be executed with root privileges. */
void
cloexec_fds(void)
{
	int     fd, max_fd = get_open_max();

	/* Set close-on-exec flag on all non-standard descriptors. */
	for (fd = STDERR_FILENO + 1; fd < max_fd; ++fd)
	{
		int     flags = fcntl(fd, F_GETFD, 0);

		if (flags < 0)
			continue;

		int     newflags = flags | FD_CLOEXEC;

		if (flags != newflags && fcntl(fd, F_SETFD, newflags))
			error(EXIT_FAILURE, errno, "fcntl F_SETFD");
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
