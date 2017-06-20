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

#endif /* !HASHER_SERVER_CONFIG_H */
