/*
 * The command line parser interface for the hasher-priv program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CMDLINE_H
# define HASHER_CMDLINE_H

# include "priv.h"

job_enum_t parse_cmdline(int ac, const char *av[], const char ***job_args);

#endif /* !HASHER_CMDLINE_H */
