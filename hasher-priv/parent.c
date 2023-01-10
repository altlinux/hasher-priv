/*
 * The chrootuid parent handler for the hasher-privd server program.
 *
 * Copyright (C) 2003-2023  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller privileges. */

#include "caller_config.h"
#include "error_prints.h"
#include "fd_set.h"
#include "fds.h"
#include "io_log.h"
#include "io_loop.h"
#include "io_x11.h"
#include "parent.h"
#include "pass.h"
#include "process.h"
#include "signals.h"
#include "tty.h"
#include "unblock_fd.h"
#include "x11.h"
#include "xmalloc.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>

static volatile pid_t child_pid;

static volatile sig_atomic_t sigalrm_arrived;
static volatile sig_atomic_t sigwinch_arrived;

static int
xselect(int nfds, fd_set *read_fds, fd_set *write_fds,
	const unsigned long int timeout)
{
	const struct timespec tmout = {.tv_sec = (time_t) timeout };
	const sigset_t sigmask = {};

	return pselect(nfds, read_fds, write_fds, NULL,
		       (timeout ? &tmout : NULL), &sigmask);
}

static void
sigalrm_handler(int signo ATTRIBUTE_UNUSED)
{
	sigalrm_arrived = 1;
}

static void
setup_timer(void)
{
	block_signal_handler(SIGALRM, SIG_BLOCK);
	struct sigaction act = { .sa_handler = sigalrm_handler };
	if (sigaction(SIGALRM, &act, 0))
		perror_msg_and_die("sigaction");

	timer_t timer_id;
	if (timer_create(CLOCK_MONOTONIC, NULL, &timer_id))
		perror_msg_and_die("timer_create");

	const struct itimerspec its = {
		.it_value = { .tv_sec = (time_t) wlimit.time_elapsed }
	};
	if (timer_settime(timer_id, 0, &its, NULL))
		perror_msg_and_die("timer_settime");
}

static void
sigwinch_handler(int signo ATTRIBUTE_UNUSED)
{
	sigwinch_arrived = 1;
}

static void
setup_sigwinch_handler(void)
{
	block_signal_handler(SIGWINCH, SIG_BLOCK);
	struct sigaction act = {
		.sa_handler = sigwinch_handler,
		.sa_flags = SA_RESTART
	};
	if (sigaction(SIGWINCH, &act, 0))
		perror_msg_and_die("sigaction");
}

static int child_rc;

static void
sigchld_handler(int signo ATTRIBUTE_UNUSED)
{
	int     status;
	pid_t   child = child_pid;

	/* handle only one child */
	if (!child)
		return;
	child_pid = 0;

	if (waitpid_retry(child, &status, 0) != child)
		perror_msg_and_die("waitpid");

	if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status))
		{
			child_rc = WEXITSTATUS(status);
		}
	} else if (WIFSIGNALED(status))
	{
		child_rc = 128 + WTERMSIG(status);
	} else
	{
		/* quite strange condition */
		child_rc = 255;
	}
}

static void
setup_sigchld_handler(void)
{
	struct sigaction act = {
		.sa_handler = sigchld_handler,
		.sa_flags = (int) (SA_NOCLDSTOP | SA_RESETHAND)
	};
	if (sigaction(SIGCHLD, &act, 0))
		perror_msg_and_die("sigaction");
}

static void
forget_child(void)
{
	if (child_pid)
	{
		child_rc = 128 + SIGTERM;

		/* no need to kill, we have no perms,
		   and it will receive HUP anyway. */
		child_pid = 0;
	}
}

static void
wait_child(void)
{
	unsigned i;

	block_signal_handler(SIGCHLD, SIG_UNBLOCK);
	for (i = 0; i < 10; ++i)
		if (child_pid)
			usleep(100000);
}

#define limit_exceeded(...)		\
	do {				\
		forget_child();		\
		restore_tty();		\
		fputc('\n', stderr);	\
		error_msg(__VA_ARGS__);	\
		exit(128 + SIGTERM);	\
	} while (0)

static int
work_limits_ok(unsigned long bytes_read, unsigned long bytes_written)
{
	if (sigalrm_arrived)
		limit_exceeded("time elapsed limit (%lu seconds) exceeded",
			       wlimit.time_elapsed);

	if (wlimit.bytes_read
	    && bytes_read >= (unsigned long) wlimit.bytes_read)
		limit_exceeded("bytes read limit (%lu bytes) exceeded",
			       wlimit.bytes_read);

	if (wlimit.bytes_written
	    && bytes_written >= (unsigned long) wlimit.bytes_written)
		limit_exceeded("bytes written limit (%lu bytes) exceeded",
			       wlimit.bytes_written);

	return 1;
}

struct io_std
{
	int     master_read_fd, master_write_out_fd, master_write_err_fd;
	int     slave_read_out_fd, slave_read_err_fd, slave_write_fd;
	size_t  master_avail, slave_avail;
	char    master_buf[BUFSIZ], slave_buf[BUFSIZ];
};

typedef struct io_std *io_std_t;

static int pty_fd = -1, ctl_fd = -1, x11_fd = -1;
static unsigned long total_bytes_read, total_bytes_written;

static char *x11_saved_data, *x11_fake_data;

static int
handle_x11_ctl(void)
{
	x11_saved_data = xmalloc(x11_data_len);

	unsigned i;

	for (i = 0; i < x11_data_len; i++)
	{
		unsigned value = 0;

		if (sscanf(x11_key + 2 * i, "%2x", &value) != 1)
		{
			error_msg("Invalid X11 authentication data\r");
			free(x11_saved_data);
			x11_saved_data = 0;
			return -1;
		}
		x11_saved_data[i] = (char) value;
	}

	x11_fake_data = xmalloc(x11_data_len);

	int fd = -1;
	if (fd_recv(ctl_fd, &fd, 1, x11_fake_data, x11_data_len) < 0) {
		fputc('\r', stderr);
	}
	if (fd >= 0)
		fd = x11_check_listen(fd);

	if (!memcmp(x11_saved_data, x11_fake_data, x11_data_len))
	{
		error_msg("Invalid X11 fake authentication data\r");
		free(x11_saved_data);
		free(x11_fake_data);
		x11_saved_data = x11_fake_data = 0;
		xclose(&fd);
	}

	return fd;
}

static int
handle_io(io_std_t io)
{
	ssize_t n;
	int     rc;
	int     max_fd = 0;
	fd_set  read_fds, write_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	if (sigwinch_arrived)
	{
		sigwinch_arrived = 0;
		(void) tty_copy_winsize(STDIN_FILENO, pty_fd);
	}

	/* Select child output, error, log and x11 descriptors
	   even after child process completion. */
	fds_add_fd(&read_fds, &max_fd, io->slave_read_out_fd);
	fds_add_fd(&read_fds, &max_fd, io->slave_read_err_fd);
	fds_add_log(&read_fds, &max_fd);
	fds_add_x11(&read_fds, &write_fds, &max_fd);

	if (child_pid)
	{
		/* Select child input, tty input and listeners
		   only if child process is alive. */
		if (io->master_avail)
			fds_add_fd(&write_fds, &max_fd, io->slave_write_fd);
		else
			fds_add_fd(&read_fds, &max_fd, io->master_read_fd);

		fds_add_fd(&read_fds, &max_fd, log_fd);
		fds_add_fd(&read_fds, &max_fd, ctl_fd);
		fds_add_fd(&read_fds, &max_fd, x11_fd);
	} else
	{
		/* No child process and no descriptors to handle? */
		if (max_fd <= 1)
			return EXIT_FAILURE;
	}

	rc = xselect(max_fd + 1, &read_fds, &write_fds, wlimit.time_idle);
	if (!rc)
		limit_exceeded("idle time limit (%lu seconds) exceeded",
			       wlimit.time_idle);
	else if (rc < 0)
		return (errno == EINTR) ? EXIT_SUCCESS : EXIT_FAILURE;

	if (fds_isset(&read_fds, io->slave_read_err_fd))
	{
		/* handle child stderr */
		n = read_retry(io->slave_read_err_fd,
			       io->slave_buf, sizeof io->slave_buf);
		if (n <= 0)
		{
			io->slave_read_err_fd = -1;
		} else
		{
			xwrite_all(io->master_write_err_fd, io->slave_buf,
				   (size_t) n);
		}
	}

	if (fds_isset(&read_fds, io->slave_read_out_fd))
	{
		/* handle child stdout */
		n = read_retry(io->slave_read_out_fd,
			       io->slave_buf, sizeof io->slave_buf);
		if (n <= 0)
		{
			io->slave_read_out_fd = -1;
		} else
		{
			xwrite_all(io->master_write_out_fd, io->slave_buf,
				   (size_t) n);
		}
	}

	if (io->master_avail && fds_isset(&write_fds, io->slave_write_fd))
	{
		/* handle child input */
		n = write_loop(io->slave_write_fd,
			       io->master_buf, io->master_avail);
		if (n <= 0)
			return EXIT_FAILURE;

		if ((size_t) n < io->master_avail)
		{
			memmove(io->master_buf,
				io->master_buf + (size_t) n,
				io->master_avail - (size_t) n);
		}

		total_bytes_read += io->master_avail;
		io->master_avail -= (size_t) n;
	}

	if (!io->master_avail && fds_isset(&read_fds, io->master_read_fd))
	{
		/* handle tty input */
		n = read_retry(io->master_read_fd,
			       io->master_buf, sizeof io->master_buf);
		if (n > 0)
			io->master_avail = (size_t) n;
		else if (n == 0)
		{
			io->master_buf[0] = 4;
			io->master_avail = 1;
		} else
			io->master_read_fd = -1;
	}

	x11_handle_select(&read_fds, &write_fds, x11_saved_data,
			  x11_fake_data);
	x11_handle_new(x11_fd, &read_fds);

	log_handle_select(&read_fds);
	log_handle_new(log_fd, &read_fds);

	if (fds_isset(&read_fds, ctl_fd))
	{
		if ((x11_fd = handle_x11_ctl()) < 0)
		{
			x11_closedir();
			error_msg("X11 forwarding disabled\r");
		}
		xclose(&ctl_fd);
	}

	return EXIT_SUCCESS;
}

void
xwrite_all(int fd, const char *buffer, size_t count)
{
	if (write_loop(fd, buffer, count) != (ssize_t) count)
		perror_msg_and_die("write");

	total_bytes_written += count;
}

int
handle_parent(pid_t a_child_pid, int a_pty_fd, int pipe_out, int pipe_err,
	      int a_ctl_fd)
{
	io_std_t io;

	pty_fd = a_pty_fd;
	ctl_fd = a_ctl_fd;

	child_pid = a_child_pid;

	setup_sigchld_handler();

	signal(SIGPIPE, SIG_IGN);

	io = xzalloc(sizeof(*io));
	io->master_read_fd = use_pty ? STDIN_FILENO : -1;
	io->master_write_out_fd = STDOUT_FILENO;
	io->master_write_err_fd = use_pty ? -1 : STDERR_FILENO;
	io->slave_read_out_fd = use_pty ? pty_fd : pipe_out;
	io->slave_read_err_fd = use_pty ? -1 : pipe_err;
	io->slave_write_fd = use_pty ? pty_fd : -1;

	if (pty_fd >= 0)
		unblock_fd(pty_fd);
	if (pipe_out >= 0)
		unblock_fd(pipe_out);
	if (pipe_err >= 0)
		unblock_fd(pipe_err);

	/* redirect standard descriptors, init tty if necessary */
	if (init_tty() && tty_copy_winsize(STDIN_FILENO, pty_fd) == 0)
		setup_sigwinch_handler();

	if (wlimit.time_elapsed)
		setup_timer();

	while (work_limits_ok(total_bytes_read, total_bytes_written))
		if (handle_io(io) != EXIT_SUCCESS)
			break;

	/* Close master pty descriptor, thus sending HUP to child session. */
	xclose(&pty_fd);

	dfl_signal_handler(SIGWINCH);
	wait_child();
	dfl_signal_handler(SIGCHLD);
	forget_child();

	return child_rc;
}
