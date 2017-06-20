/*
 * The pty opener interface for the hasher-privd server program.
 *
 * Copyright (C) 2016-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_PTY_H
# define HASHER_PTY_H

enum open_pty_chrootedness {
	OPEN_PTY_UNCHROOTED = 0,
	OPEN_PTY_CHROOTED = 1
};

enum open_pty_verbosity {
	OPEN_PTY_SILENT = 0,
	OPEN_PTY_VERBOSE = 1
};

int open_pty(int *slave_fd, enum open_pty_chrootedness, enum open_pty_verbosity);

#endif /* !HASHER_PTY_H */
