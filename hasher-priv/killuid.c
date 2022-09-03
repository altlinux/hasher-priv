/*
 * The killuid actions for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "error_prints.h"
#include "executors.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/resource.h>

static void
raise_rlimit_nproc(void)
{
	struct rlimit rlim;

	if (getrlimit(RLIMIT_NPROC, &rlim))
		perror_msg_and_die("getrlimit");

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;

	(void) setrlimit(RLIMIT_NPROC, &rlim);
}

int
do_killuid(void)
{
	uid_t u = getuid();

	if (change_uid1 < MIN_CHANGE_UID || change_uid1 == u)
		error_msg_and_die("invalid uid: %u", change_uid1);
	if (change_uid2 < MIN_CHANGE_UID || change_uid2 == u)
		error_msg_and_die("invalid uid: %u", change_uid2);

	raise_rlimit_nproc();

	/*
	 * Do not assume that fs.suid_dumpable == 0
	 * and clear the dumpable flag explicitly.
	 */
	if (prctl(PR_SET_DUMPABLE, 0))
		perror_msg_and_die("prctl PR_SET_DUMPABLE");

	if (setreuid(change_uid1, change_uid2) < 0)
		perror_msg_and_die("setreuid");

	if (kill(-1, SIGKILL))
		perror_msg_and_die("kill");

	purge_ipc(change_uid1, change_uid2);

	if (setreuid(change_uid2, change_uid1) < 0)
		perror_msg_and_die("setreuid");

	purge_ipc(change_uid1, change_uid2);

	return 0;
}
