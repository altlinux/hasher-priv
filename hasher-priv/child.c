/*
 * The chrootuid child handler for the hasher-privd server program.
 *
 * Copyright (C) 2004-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with child privileges. */

#include "caller_config.h"
#include "child.h"
#include "error_prints.h"
#include "fds.h"
#include "io_loop.h"
#include "macros.h"
#include "nullify_stdin.h"
#include "pass.h"
#include "process.h"
#include "signals.h"
#include "x11.h"
#include "xmalloc.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>

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

	if (pty_fd > STDERR_FILENO && xclose(&pty_fd))
		perror_msg_and_die("close pty_fd");
	if (pipe_out > STDERR_FILENO && xclose(&pipe_out))
		perror_msg_and_die("close pipe_out");
	if (pipe_err > STDERR_FILENO && xclose(&pipe_err))
		perror_msg_and_die("close pipe_err");
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
		free(x11_fake_data);
		xclose(&fd);
		return 0;
	}

	xclose(&fd);

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

		if (waitpid_retry(pid, &status, 0) != pid || !WIFEXITED(status))
			return 1;
		return WEXITSTATUS(status);
	}
}

static void
cpu_set_swap_bits(size_t x, size_t y, cpu_set_t *set)
{
	int a = CPU_ISSET(x, set);
	int b = CPU_ISSET(y, set);

	if (a)
		CPU_SET(y, set);
	else
		CPU_CLR(y, set);
	if (b)
		CPU_SET(x, set);
	else
		CPU_CLR(x, set);
}

static int
set_affinity_nproc(size_t nproc)
{
	cpu_set_t set;
	cpu_set_t tmp;
	size_t cpucount;
	size_t i, j;

	if (!nproc)
		return 0;
	if (sched_getaffinity(0, sizeof(cpu_set_t), &set) == -1)
		return -1;
	cpucount = (size_t) CPU_COUNT(&set);
	if (cpucount <= nproc)
		return 0;
	CPU_ZERO(&tmp);
	for (i = 0; i < nproc; i++)
		CPU_SET(i, &tmp);
	srandom((unsigned int) getpid());
	for (i = 0; i < nproc; i++)
		cpu_set_swap_bits(i, (size_t) random() % cpucount, &tmp);
	/*
	 * At this point, 'tmp' is shuffled and expanded to the width of
	 * cpucount, and it now contains nproc set bits. After the completion
	 * of the subsequent cycle, 'set' will contain nproc set bits.
	 */
	for (i = 0, j = 0; i < CPU_SETSIZE && j < cpucount; i++) {
		if (CPU_ISSET(i, &set)) {
			if (!CPU_ISSET(j, &tmp))
				CPU_CLR(i, &set);
			j++;
		}
	}
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) == -1)
		return -1;
	return 0;
}

void
handle_child(const char *const *argv, const char *const *env,
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

	if (set_affinity_nproc(change_nproc) < 0)
		perror_msg_and_die("nproc: %zu", change_nproc);

	if (ctl_fd >= 0)
	{
		int     x11_fd = x11_listen();

		if (x11_fd >= 0)
		{
			char   *data;

			if ((data = xauth_gen_fake())
			    && xauth_add_entry(env) == EXIT_SUCCESS)
				fd_send(ctl_fd, &x11_fd, 1, data, x11_data_len);
			free(data);
			xclose(&x11_fd);
		}
	}

	umask(change_umask);

	block_signal_handler(SIGCHLD, SIG_UNBLOCK);

	execve(argv[0], (char *const *) argv, (char *const *) env);
	perror_msg_and_die("execve: %s", argv[0]);
}
