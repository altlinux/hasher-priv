/*
 * The unshare interface for the hasher-priv project.
 *
 * Copyright (C) 2011-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_UNSHARE_H
# define HASHER_UNSHARE_H

void unshare_ipc(void);
void unshare_mount(void);
void unshare_network(void);
void unshare_uts(void);

#endif /* !HASHER_UNSHARE_H */
