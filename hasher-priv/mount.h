/*
 * The mount setup interface for the hasher-privd server program.
 *
 * Copyright (C) 2004-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_MOUNT_H
# define HASHER_MOUNT_H

void setup_mountpoints(void);

extern int dev_pts_mounted;

#endif /* !HASHER_MOUNT_H */
