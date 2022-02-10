
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The file descriptor sanitizer for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root or caller privileges. */

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
			perror_msg_and_die("fcntl F_SETFD");
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
			perror_msg_and_die("dup2(%d, %d)",
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
	int pipe_fds[2];

	if (pipe(pipe_fds))
		perror_msg_and_die("pipe");
	if (close(pipe_fds[1]))
		perror_msg_and_die("close");

	if (pipe_fds[0] != STDIN_FILENO) {
		if (dup2(pipe_fds[0], STDIN_FILENO) != STDIN_FILENO)
			perror_msg_and_die("dup2");
		if (close(pipe_fds[0]))
			perror_msg_and_die("close");
	}

	/*
	 * At this point STDIN_FILENO is an O_RDONLY descriptor,
	 * a read attempt from such descriptor ends with EOF,
	 * and a write attempt is rejected with EBADF.
	 */
}

/* This function may be executed with caller privileges. */
void
unblock_fd(int fd)
{
	int     flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		perror_msg_and_die("fcntl F_GETFL");

	int     newflags = flags | O_NONBLOCK;

	if (flags != newflags && fcntl(fd, F_SETFL, newflags) < 0)
		perror_msg_and_die("fcntl F_SETFL");
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
