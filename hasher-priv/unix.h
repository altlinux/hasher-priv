/*
 * The unix domain listening interface for the hasher-priv project.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_UNIX_H
# define HASHER_UNIX_H

int unix_listen(const char *);
int unix_accept(int fd);
int log_listen(void);

#endif /* !HASHER_UNIX_H */
