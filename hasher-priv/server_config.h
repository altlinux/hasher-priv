/*
 * Server configuration interface for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_SERVER_CONFIG_H
# define HASHER_SERVER_CONFIG_H

# include <sys/types.h>

void configure_server(void);

extern unsigned long server_session_timeout;
extern char *server_loglevel;
extern char *server_pidfile;
extern gid_t server_gid;

extern int min_uid;
extern int min_gid;

inline int valid_uid(uid_t uid)
{
	return (min_uid < 0 || (uid_t) min_uid <= uid);
}

inline int valid_gid(gid_t gid)
{
	return (min_gid < 0 || (gid_t) min_gid <= gid);
}

#endif /* !HASHER_SERVER_CONFIG_H */
