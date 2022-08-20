/*
 * The descriptor passing interface for the hasher-priv project.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_PASS_H
# define HASHER_PASS_H

# include "cc_compat.h"
# include <sys/types.h>

void    fd_send(int ctl, int *fds, unsigned int n_fds,
		const char *data, size_t data_len) ATTRIBUTE_NONNULL((2));
int     fd_recv(int ctl, int *fds, unsigned int n_fds,
		char *data, size_t data_len) ATTRIBUTE_NONNULL((2));

#endif /* !HASHER_PASS_H */
