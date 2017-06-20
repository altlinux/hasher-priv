/*
 * The change rlimit interface for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CHANGE_RLIMIT_H
# define HASHER_CHANGE_RLIMIT_H

# include <sys/resource.h>

typedef struct
{
	const char *name;
	int     resource;
	rlim_t *hard, *soft;
} change_rlimit_t;

extern change_rlimit_t change_rlimit[];

#endif /* !HASHER_CHANGE_RLIMIT_H */
