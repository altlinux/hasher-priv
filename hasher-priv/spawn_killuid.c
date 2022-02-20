/*
 * Spawn killuid module for the hasher-priv project.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include "spawn_killuid.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "priv.h"

void
spawn_killuid(void)
{
	pid_t pid = fork();
	if (pid < 0)
		perror_msg_and_die("fork");

	if (pid == 0)
		_exit(do_killuid());

	for (;;) {
		int status;
		if (waitpid(pid, &status, 0) < 0) {
			if (errno == EINTR)
				continue;
			perror_msg_and_die("waitpid");
		}
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) == 0)
				break;
			error_msg_and_die("exit status %d",
					  WEXITSTATUS(status));
		}
		if (WIFSIGNALED(status)) {
			error_msg_and_die("terminated by signal %d",
					  WTERMSIG(status));
		}
		error_msg_and_die("unrecognized status %#x", status);
	}
}
