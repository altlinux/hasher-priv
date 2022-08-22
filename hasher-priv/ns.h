/*
 * The namespace setup interface for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_NS_H
# define HASHER_NS_H

# include <sys/types.h>

void setup_ns(pid_t, uid_t);

#endif /* !HASHER_NS_H */
