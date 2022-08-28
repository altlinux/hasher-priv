/*
 * The chrootuid parent handler interface for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_PARENT_H
# define HASHER_PARENT_H

# include <sys/types.h>

void xwrite_all(int fd, const char *buffer, size_t count);
int handle_parent(pid_t pid, int pty_fd, int pipe_out, int pipe_err, int ctl_fd);

#endif /* !HASHER_PARENT_H */
