/*
 * The logging interface for the hasher-priv project.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef HASHER_LOGGING_H
# define HASHER_LOGGING_H

void init_log_standalone(void);
void init_log_daemon(int is_foreground);
void set_log_level(const char *name);

#endif /* !HASHER_LOGGING_H */
