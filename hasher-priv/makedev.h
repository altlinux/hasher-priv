/*
 * The device setup interface for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_MAKEDEV_H
# define HASHER_MAKEDEV_H

# include <sys/types.h>

void setup_devices(const char **vec, size_t len);

#endif /* !HASHER_MAKEDEV_H */
