/*
 * The X11 forwarding interface for the hasher-privd server program.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_X11_H
# define HASHER_X11_H

# include <sys/types.h>

void    x11_drop_display(void);
int     x11_parse_display(void);
int     x11_prepare_connect(void);
void    x11_closedir(void);
int     x11_listen(void);
int     x11_connect(void);
int     x11_check_listen(int fd);

extern const char *x11_display, *x11_key;
extern size_t x11_data_len;

extern int share_caller_network;

#endif /* !HASHER_X11_H */
