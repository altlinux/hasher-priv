/*
 * The file level configuration parsing interface
 * for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_FILE_CONFIG_H
# define HASHER_FILE_CONFIG_H

#include "cc_compat.h"
#include <stdio.h>

typedef void (*name_value_fn_t)(const char *name, const char *value,
				const char *fname);

typedef void (*fread_config_fn_t)(FILE *, const char *fname);

void fread_config_name_value(FILE *fp, const char *fname, name_value_fn_t)
	ATTRIBUTE_NONNULL((1, 2, 3));

void load_config(const char *fname, fread_config_fn_t)
	ATTRIBUTE_NONNULL((1, 2));

#endif /* !HASHER_FILE_CONFIG_H */
