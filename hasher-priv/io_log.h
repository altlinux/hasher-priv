/*
 * The chrootuid parent log I/O interface for the hasher-privd server program.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_IO_LOG_H
# define HASHER_IO_LOG_H

# include <sys/select.h>

void    log_handle_new(int log_fd, fd_set *read_fds);
void    fds_add_log(fd_set *read_fds, int *max_fd);
void    log_handle_select(fd_set *read_fds);

#endif /* !HASHER_IO_LOG_H */
