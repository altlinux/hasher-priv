/*
 * The chrootuid child handler interface for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CHILD_H
# define HASHER_CHILD_H

# include "cc_compat.h"

void    handle_child(const char *const *argv, const char *const *env,
		     int pty_fd, int pipe_out, int pipe_err, int ctl_fd)
	ATTRIBUTE_NORETURN;

#endif /* !HASHER_CHILD_H */
