/*
 * The procfd interface for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_PROCFD_H
# define HASHER_PROCFD_H

# include <sys/types.h>

/*
 * Open /proc/pid directory for the specified pid,
 * check that the directory belongs to the specified uid,
 * return the checked descriptor.
 */
int open_proc_fd(pid_t, uid_t);

#endif /* !HASHER_PROCFD_H */
