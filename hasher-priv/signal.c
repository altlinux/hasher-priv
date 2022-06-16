
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The chrootuid signal handling for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "error_prints.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "priv.h"

/* This function may be executed with root privileges. */
void
xsigprocmask(int what, const sigset_t *set, sigset_t *oldset)
{
	if (sigprocmask(what, set, oldset) < 0)
		perror_msg_and_die("sigprocmask");
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

/* This function may be executed with caller or child privileges. */
void
dfl_signal_handler(int no)
{
	if (signal(no, SIG_DFL) == SIG_ERR)
		perror_msg_and_die("signal");

	block_signal_handler(no, SIG_UNBLOCK);
}
