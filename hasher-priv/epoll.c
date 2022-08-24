/*
 * The helpers for epoll API for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "fds.h"
#include "epoll.h"

#include <string.h>
#include <unistd.h>

int
epoll_add_in(int fd_ep, int fd)
{
	struct epoll_event ev = {
		.events = EPOLLIN,
		.data.fd = fd,
	};

	if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd, &ev) < 0) {
		perror_msg("epoll_ctl");
		return -1;
	}

	return 0;
}

int
epoll_add_hup(int fd_ep, int fd)
{
	struct epoll_event ev = {
		.events = 0, /* HUP and ERR are implicit. */
		.data.fd = fd,
	};

	if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd, &ev) < 0) {
		perror_msg("epoll_ctl");
		return -1;
	}

	return 0;
}
