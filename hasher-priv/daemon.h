/*
 * Common server interface for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_DAEMON_H
# define HASHER_DAEMON_H

struct hadaemon {
	/* A pollable pipe to the top-level daemon process. */
	int fd_pipe[2];
	/* A listening socket to accept connects from. */
	int fd_conn;
	/* A descriptor returned by signalfd. */
	int fd_signal;
	/* A descriptor returned by epoll_create1. */
	int fd_ep;
};

#endif /* HASHER_DAEMON_H */
