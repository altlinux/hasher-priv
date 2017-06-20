/*
 * The signal handling interface for the hasher-privd server program.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_SIGNALS_H
# define HASHER_SIGNALS_H

# include <signal.h>

void    xsigprocmask(int what, const sigset_t *set, sigset_t *oldset);
void    block_signal_handler(int no, int what);
void    dfl_signal_handler(int no);
void    unblock_all_signals(void);
int     daemon_create_signal_fd(void);

#endif /* !HASHER_SIGNALS_H */
