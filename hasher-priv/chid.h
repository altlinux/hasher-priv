/*
 * The change uid/gid interface for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CHID_H
# define HASHER_CHID_H

# include <sys/types.h>

void    ch_uid(uid_t uid, uid_t *save);
void    ch_gid(gid_t gid, gid_t *save);

#endif /* !HASHER_CHID_H */
