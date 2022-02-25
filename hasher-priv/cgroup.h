/*
 * The cgroup interface for the hasher-privd server program.
 *
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CGROUP_H
# define HASHER_CGROUP_H

# include <sys/types.h>

void join_caller_cgroup(pid_t client_pid);

#endif /* HASHER_CGROUP_H */
