/*
 * The signal handling module for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "signals.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/signalfd.h>

/* This function may be executed with root privileges. */
void
xsigprocmask(int what, const sigset_t *set, sigset_t *oldset)
{
	if (sigprocmask(what, set, oldset) < 0)
		perror_msg_and_die("sigprocmask");
}

/* This function may be executed with root privileges. */
int
daemon_create_signal_fd(void)
{
	sigset_t mask;

	sigemptyset(&mask);

	/*
	 * These five signals are handled by the appropriate epoll_wait loop.
	 */
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGCHLD);

	int fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (fd >= 0)
		xsigprocmask(SIG_SETMASK, &mask, NULL);

	return fd;
}

/* This function may be executed with root privileges. */
void
block_signal_handler(int no, int what)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, no);
	xsigprocmask(what, &set, 0);
}

/* This function may be executed with root privileges. */
void
unblock_all_signals(void)
{
	sigset_t mask;
	sigemptyset(&mask);
	xsigprocmask(SIG_SETMASK, &mask, NULL);
}

/* This function may be executed with caller or child privileges. */
void
dfl_signal_handler(int no)
{
	if (signal(no, SIG_DFL) == SIG_ERR)
		perror_msg_and_die("signal");

	block_signal_handler(no, SIG_UNBLOCK);
}
