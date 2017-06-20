/*
 * The server communication interface for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_SERVER_COMM_H
# define HASHER_SERVER_COMM_H

# include "cc_compat.h"

int send_response_to_client(int conn, int rc, const char *fmt, ...)
	ATTRIBUTE_FORMAT((printf, 3, 4));

#endif /* !HASHER_SERVER_COMM_H */
