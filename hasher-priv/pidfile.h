/*
 * The pidfiles interface for the hasher-privd server program.
 *
 * Copyright (c) 1995  Martin Schulze <Martin.Schulze@Linux.DE>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_PIDFILE_H
# define HASHER_PIDFILE_H

# include "cc_compat.h"

int check_pid(const char *) ATTRIBUTE_NONNULL((1));
int write_pid(const char *) ATTRIBUTE_NONNULL((1));
int remove_pid(const char *) ATTRIBUTE_NONNULL((1));

#endif /* !HASHER_PIDFILE_H */
