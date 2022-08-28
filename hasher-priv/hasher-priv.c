/*
 * The entry function for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_data.h"
#include "caller_config.h"
#include "cmdline.h"
#include "error_prints.h"
#include "executors.h"
#include "fds.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int ac, const char *av[])
{
	/* First, check and sanitize file descriptors. */
	sanitize_fds();

	/* Second, parse command line arguments. */
	const char **job_args;
	job_enum_t job = parse_cmdline(ac, av, &job_args);

	if (chroot_path && *chroot_path != '/')
		error_msg_and_die("%s: invalid chroot path", chroot_path);

	/* Third, initialize data related to caller. */
	init_caller_data(getuid(), getgid());

	/* 4th, parse environment for config options. */
	parse_env();

	/* We don't need environment variables any longer. */
	if (clearenv() != 0)
		perror_msg_and_die("clearenv");

	/* Load config according to caller information. */
	configure_caller();

	/* Finally, execute chosen job. */
	switch (job)
	{
		case JOB_GETCONF:
			return do_getconf();
		case JOB_KILLUID:
			return do_killuid();
		case JOB_GETUGID1:
			return do_getugid1();
		case JOB_CHROOTUID1:
			return do_chrootuid1(job_args);
		case JOB_GETUGID2:
			return do_getugid2();
		case JOB_CHROOTUID2:
			return do_chrootuid2(job_args);
		default:
			error_msg_and_die("unknown job %d", job);
	}

	return EXIT_FAILURE;
}
