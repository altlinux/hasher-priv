
/*
  Copyright (C) 2004-2019  Dmitry V. Levin <ldv@altlinux.org>

  The chrootuid child handler for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with child privileges. */

#include "error_prints.h"
#include "io_loop.h"
#include "nullify_stdin.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "priv.h"
#include "macros.h"
#include "xmalloc.h"

static void
connect_fds(int pty_fd, int pipe_out, int pipe_err)
{
	if (setsid() < 0)
		perror_msg_and_die("setsid");

	if (ioctl(pty_fd, (unsigned long) TIOCSCTTY, 0) < 0)
		perror_msg_and_die("ioctl TIOCSCTTY");

	if (use_pty)
	{
		if (dup2(pty_fd, STDIN_FILENO) < 0)
			perror_msg_and_die("dup2(%d, %d)",
					   pty_fd, STDIN_FILENO);
	} else
	{
		/* redirect stdin to /dev/null if and only if
		   use_pty is not set and stdin is a tty */
		if (isatty(STDIN_FILENO))
			nullify_stdin();
	}
	if (dup2((use_pty ? pty_fd : pipe_out), STDOUT_FILENO) < 0)
		perror_msg_and_die("dup2(%d, %d)",
				   (use_pty ? pty_fd : pipe_out),
				   STDOUT_FILENO);
	if (dup2((use_pty ? pty_fd : pipe_err), STDERR_FILENO) < 0)
		perror_msg_and_die("dup2(%d, %d)",
				   (use_pty ? pty_fd : pipe_err),
				   STDERR_FILENO);

	if (pty_fd > STDERR_FILENO)
		close(pty_fd);
	if (pipe_out > STDERR_FILENO)
		close(pipe_out);
	if (pipe_err > STDERR_FILENO)
		close(pipe_err);
}

#define PATH_DEVURANDOM "/dev/urandom"

static char *
xauth_gen_fake(void)
{
	int     fd = open(PATH_DEVURANDOM, O_RDONLY);

	if (fd < 0)
	{
		perror_msg("open: %s", PATH_DEVURANDOM);
		return 0;
	}

	char   *x11_fake_data = xmalloc(x11_data_len);

	if (read_loop(fd, x11_fake_data, x11_data_len) !=
	    (ssize_t) x11_data_len)
	{
		perror_msg("read: %s", PATH_DEVURANDOM);
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
xauth_add_entry(const char *const *env)
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
		size_t  i;
		int     errors[ARRAY_SIZE(paths)];

		for (i = 0; i < ARRAY_SIZE(paths); ++i)
		{
			execve(paths[i], (char *const *) av,
					 (char *const *) env);
			errors[i] = errno;
		}
		for (i = 0; i < ARRAY_SIZE(paths); ++i) {
			errno = errors[i];
			perror_msg("execve: %s", paths[i]);
		}
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
handle_child(const char *const *env,
	     int pty_fd, int pipe_out, int pipe_err, int ctl_fd)
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
		perror_msg_and_die("nice: %d", change_nice);

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

	execve(chroot_argv[0], (char *const *) chroot_argv, (char *const *) env);
	perror_msg_and_die("execve: %s", chroot_argv[0]);
}
