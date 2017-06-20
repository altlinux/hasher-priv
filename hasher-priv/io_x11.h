/*
 * The chrootuid parent X11 I/O interface for the hasher-privd server program.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_IO_X11_H
# define HASHER_IO_X11_H

# include <sys/select.h>

void    x11_handle_new(int x11_fd, fd_set *read_fds);
void    fds_add_x11(fd_set *read_fds, fd_set *write_fds, int *max_fd);
void    x11_handle_select(fd_set *read_fds, fd_set *write_fds,
			  const char *x11_saved_data,
			  const char *x11_fake_data);

#endif /* !HASHER_IO_X11_H */
