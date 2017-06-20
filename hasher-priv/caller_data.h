/*
 * The caller data interface for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CALLER_DATA_H
# define HASHER_CALLER_DATA_H

# include <sys/types.h>

void init_caller_data(uid_t uid, gid_t gid);

extern const char *caller_user;
extern const char *caller_home;
extern uid_t caller_uid;
extern gid_t caller_gid;
extern unsigned int caller_num;

#endif /* !HASHER_CALLER_DATA_H */
