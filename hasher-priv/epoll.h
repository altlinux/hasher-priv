/*
 * The helpers for epoll API for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_EPOLL_H_
#define HASHER_EPOLL_H_

#include <sys/epoll.h>

int epoll_add_in(int fd_ep, int fd);
int epoll_add_hup(int fd_ep, int fd);

#endif /* HASHER_EPOLL_H_ */
