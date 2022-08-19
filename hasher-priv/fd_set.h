/*
 * The fd_set interface for the hasher-priv project.
 *
 * Copyright (C) 2008-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_FD_SET_H
# define HASHER_FD_SET_H

#include <sys/select.h>

void fds_add_fd(fd_set *fds, int *max_fd, const int fd);
int fds_isset(fd_set *fds, const int fd);

#endif /* !HASHER_FD_SET_H */
