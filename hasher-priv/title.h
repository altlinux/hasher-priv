/*
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_TITLE_H
# define HASHER_TITLE_H

# include "error_prints.h"
# include <setproctitle.h>

# ifndef setproctitle
#  define setproctitle(fmt_, ...)					\
	do {								\
		setproctitle(fmt_, ##__VA_ARGS__);			\
		debug_msg("process \"" fmt_ "\"", ##__VA_ARGS__);	\
	} while (0)
# endif

#endif /* HASHER_TITLE_H */
