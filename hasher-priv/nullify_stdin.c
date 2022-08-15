/*
 * nullify_stdin function for the hasher-priv project.
 *
 * Copyright (C) 2004-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller or child privileges. */

#include "error_prints.h"
#include "fds.h"
#include "nullify_stdin.h"
#include <stdlib.h>
#include <unistd.h>

void
nullify_stdin(void)
{
	int pipe_fds[2];

	if (pipe(pipe_fds))
		perror_msg_and_die("pipe");
	if (xclose(&pipe_fds[1]))
		perror_msg_and_die("close");

	if (pipe_fds[0] != STDIN_FILENO) {
		if (dup2(pipe_fds[0], STDIN_FILENO) != STDIN_FILENO)
			perror_msg_and_die("dup2");
		if (xclose(&pipe_fds[0]))
			perror_msg_and_die("close");
	}

	/*
	 * At this point STDIN_FILENO is an O_RDONLY descriptor,
	 * a read attempt from such descriptor ends with EOF,
	 * and a write attempt is rejected with EBADF.
	 */
}
