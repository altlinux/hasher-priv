/*
 * The chrootuid tty interface for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_TTY_H
# define HASHER_TTY_H

int init_tty(void);
int tty_copy_winsize(int master_fd, int slave_fd);
void restore_tty(void);

#endif /* !HASHER_TTY_H */
