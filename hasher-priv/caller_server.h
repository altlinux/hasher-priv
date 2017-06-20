/*
 * The per-calling-user server interface of the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CALLER_SERVER_H
# define HASHER_CALLER_SERVER_H

# include "cc_compat.h"
# include "daemon.h"

int caller_server_listener_init(struct hadaemon *);
void caller_server(struct hadaemon *) ATTRIBUTE_NORETURN;

#endif /* !HASHER_CALLER_SERVER_H */
