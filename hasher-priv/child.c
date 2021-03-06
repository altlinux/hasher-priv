
/*
  Copyright (C) 2004-2019  Dmitry V. Levin <ldv@altlinux.org>

  The chrootuid child handler for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with child privileges. */

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "priv.h"
#include "xmalloc.h"

static void
connect_fds(int pty_fd, int pipe_out, int pipe_err)
{
	if (setsid() < 0)
		error(EXIT_FAILURE, errno, "setsid");

	if (ioctl(pty_fd, (unsigned long) TIOCSCTTY, 0) < 0)
		error(EXIT_FAILURE, errno, "ioctl TIOCSCTTY");

	if (use_pty)
	{
		if (dup2(pty_fd, STDIN_FILENO) < 0)
			error(EXIT_FAILURE, errno, "dup2(%d, %d)",
			      pty_fd, STDIN_FILENO);
	} else
	{
		/* redirect stdin to /dev/null if and only if
		   use_pty is not set and stdin is a tty */
		if (isatty(STDIN_FILENO))
			nullify_stdin();
	}
	if (dup2((use_pty ? pty_fd : pipe_out), STDOUT_FILENO) < 0)
		error(EXIT_FAILURE, errno, "dup2(%d, %d)",
		      (use_pty ? pty_fd : pipe_out), STDOUT_FILENO);
	if (dup2((use_pty ? pty_fd : pipe_err), STDERR_FILENO) < 0)
		error(EXIT_FAILURE, errno, "dup2(%d, %d)",
		      (use_pty ? pty_fd : pipe_err), STDERR_FILENO);

	if (pty_fd > STDERR_FILENO)
		close(pty_fd);
	if (pipe_out > STDERR_FILENO)
		close(pipe_out);
	if (pipe_err > STDERR_FILENO)
		close(pipe_err);
}

static  ssize_t
read_loop(int fd, char *buffer, size_t count)
{
	ssize_t offset = 0;

	while (count > 0)
	{
		ssize_t block = read_retry(fd, &buffer[offset], count);

		if (block <= 0)
			return offset ? : block;
		offset += block;
		count -= (size_t) block;
	}
	return offset;
}

#define PATH_DEVURANDOM "/dev/urandom"

static char *
xauth_gen_fake(void)
{
	int     fd = open(PATH_DEVURANDOM, O_RDONLY);

	if (fd < 0)
	{
		error(EXIT_SUCCESS, errno, "open: %s", PATH_DEVURANDOM);
		return 0;
	}

	char   *x11_fake_data = xmalloc(x11_data_len);

	if (read_loop(fd, x11_fake_data, x11_data_len) !=
	    (ssize_t) x11_data_len)
	{
		error(EXIT_SUCCESS, errno, "read: %s", PATH_DEVURANDOM);
		(void) close(fd);
		free(x11_fake_data);
		return 0;
	}

	(void) close(fd);

	/* Replace original x11_key with fake one. */
	size_t  i, key_len = 2 * x11_data_len + 1;
	char   *new_key = xmalloc(key_len);

	for (i = 0; i < x11_data_len; ++i)
		snprintf(new_key + 2 * i, key_len - 2 * i,
			 "%02x", (unsigned char) x11_fake_data[i]);
	x11_key = new_key;

	return x11_fake_data;
}

static int
xauth_add_entry(char *const *env)
{
	pid_t   pid = fork();

	if (pid < 0)
		return EXIT_FAILURE;

	if (!pid)
	{
		const char *av[] =
			{ "xauth", "add", ":10.0", ".", x11_key, 0 };
		const char *paths[] =
			{ "/usr/bin/xauth", "/usr/X11R6/bin/xauth" };
		size_t  i, paths_size = sizeof(paths) / sizeof(paths[0]);
		int     errors[paths_size];

		for (i = 0; i < paths_size; ++i)
		{
			execve(paths[i], (char *const *) av, env);
			errors[i] = errno;
		}
		for (i = 0; i < paths_size; ++i)
			error(EXIT_SUCCESS, errors[i], "execve: %s",
			      paths[i]);
		_exit(EXIT_FAILURE);
	} else
	{
		int     status = 0;

		if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
			return 1;
		return WEXITSTATUS(status);
	}
}

void
handle_child(char *const *env, int pty_fd, int pipe_out, int pipe_err,
	     int ctl_fd)
{
	if (x11_key)
	{
		/* Child process doesn't need X11 authentication data. */
		memset((char *) x11_key, 0, strlen(x11_key));
		free((char *) x11_key);
		x11_key = 0;
	}
	connect_fds(pty_fd, pipe_out, pipe_err);

	dfl_signal_handler(SIGHUP);
	dfl_signal_handler(SIGPIPE);
	dfl_signal_handler(SIGTERM);

	if (nice(change_nice) < 0)
		error(EXIT_FAILURE, errno, "nice: %d", change_nice);

	if (ctl_fd >= 0)
	{
		int     x11_fd = x11_listen();

		if (x11_fd >= 0)
		{
			char   *data;

			if ((data = xauth_gen_fake())
			    && xauth_add_entry(env) == EXIT_SUCCESS)
				fd_send(ctl_fd, x11_fd, data, x11_data_len);
			(void) close(x11_fd);
			free(data);
		}
	}

	umask(change_umask);

	block_signal_handler(SIGCHLD, SIG_UNBLOCK);

	execve(chroot_argv[0], (char *const *) chroot_argv, env);
	error(EXIT_FAILURE, errno, "chrootuid: execve: %s", chroot_argv[0]);
	exit(EXIT_FAILURE);
}
