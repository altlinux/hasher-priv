/*
 * unblock_fd function for the hasher-priv project.
 *
 * Copyright (C) 2004-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller privileges. */

#include "error_prints.h"
#include <fcntl.h>

#include "priv.h"

void
unblock_fd(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		perror_msg_and_die("fcntl F_GETFL");

	int newflags = flags | O_NONBLOCK;

	if (flags != newflags && fcntl(fd, F_SETFL, newflags) < 0)
		perror_msg_and_die("fcntl F_SETFL");
}
