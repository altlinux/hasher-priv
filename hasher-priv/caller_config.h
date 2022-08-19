/*
 * The caller configuration interface for the hasher-priv project.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CALLER_CONFIG_H
# define HASHER_CALLER_CONFIG_H

void    configure_caller(void);
void    parse_env(void);

#endif /* !HASHER_CALLER_CONFIG_H */
