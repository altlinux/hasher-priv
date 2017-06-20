/*
 * The executor interface for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_EXECUTORS_H
# define HASHER_EXECUTORS_H

int     do_getconf(void);
int     do_killuid(void);
int     do_getugid1(void);
int     do_getugid2(void);
int     do_chrootuid1(const char *const *argv);
int     do_chrootuid2(const char *const *argv);

#endif /* !HASHER_EXECUTORS_H */
