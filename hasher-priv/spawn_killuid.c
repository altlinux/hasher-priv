/*
 * The spawn killuid module for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include "executors.h"
#include "spawn_killuid.h"
#include "process.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void
spawn_killuid(void)
{
	pid_t pid = fork();
	if (pid < 0)
		perror_msg_and_die("fork");

	if (pid == 0)
		_exit(do_killuid());

	int status;
	if (waitpid_retry(pid, &status, 0) < 0)
		perror_msg_and_die("waitpid");

	if (WIFEXITED(status)) {
		int rc = WEXITSTATUS(status);
		if (rc == 0)
			return;
		error_msg_and_die("exit status %d", rc);
	}

	if (WIFSIGNALED(status)) {
		error_msg_and_die("terminated by signal %d", WTERMSIG(status));
	}

	error_msg_and_die("unrecognized status %#x", status);
}
