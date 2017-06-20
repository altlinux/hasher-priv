/*
 * Purge all SYSV IPC objects for the given pair of user ids
 * for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_IPC_H
# define HASHER_IPC_H

# include <sys/types.h>

void purge_ipc(uid_t uid1, uid_t uid2);

#endif /* !HASHER_IPC_H */
