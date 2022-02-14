/*
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_FDS_H
# define HASHER_FDS_H

void sanitize_fds(void);
void cloexec_fds(void);

extern int log_fd;

#endif /* !HASHER_FDS_H */
