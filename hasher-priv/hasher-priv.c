
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The entry function for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include "fds.h"
#include <stdio.h>
#include <stdlib.h>

#include "priv.h"

int
main(int ac, const char *av[])
{
	/* First, check and sanitize file descriptors. */
	sanitize_fds();

	/* Second, parse command line arguments. */
	const char **task_args;
	task_t task = parse_cmdline(ac, av, &task_args);

	if (chroot_path && *chroot_path != '/')
		error_msg_and_die("%s: invalid chroot path", chroot_path);

	/* Third, initialize data related to caller. */
	init_caller_data();

	/* 4th, parse environment for config options. */
	parse_env();

	/* We don't need environment variables any longer. */
	if (clearenv() != 0)
		perror_msg_and_die("clearenv");

	/* Load config according to caller information. */
	configure_caller();

	/* Finally, execute choosen task. */
	switch (task)
	{
		case TASK_GETCONF:
			return do_getconf();
		case TASK_KILLUID:
			return do_killuid();
		case TASK_GETUGID1:
			return do_getugid1();
		case TASK_CHROOTUID1:
			return do_chrootuid1(task_args);
		case TASK_GETUGID2:
			return do_getugid2();
		case TASK_CHROOTUID2:
			return do_chrootuid2(task_args);
		default:
			error_msg_and_die("unknown task %d", task);
	}

	return EXIT_FAILURE;
}
