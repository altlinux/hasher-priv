/*
 * The generic option parsing interface for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_OPT_PARSE_H
# define HASHER_OPT_PARSE_H

# include "cc_compat.h"

void opt_bad_name(const char *name, const char *fname)
	ATTRIBUTE_NONNULL((1, 2))
	ATTRIBUTE_NORETURN;
void opt_bad_value(const char *name, const char *value, const char *fname)
	ATTRIBUTE_NONNULL((1, 2, 3))
	ATTRIBUTE_NORETURN;
unsigned long opt_str2ul(const char *name, const char *value, const char *fname)
	ATTRIBUTE_NONNULL((1, 2, 3));
int opt_str2bool(const char *name, const char *value, const char *fname)
	ATTRIBUTE_NONNULL((1, 2, 3));
int opt_str2int(const char *name, const char *value, const char *fname)
	ATTRIBUTE_NONNULL((1, 2, 3));

#endif /* !HASHER_OPT_PARSE_H */
