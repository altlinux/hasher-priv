/*
 * waitpid_retry interface for the hasher-priv project.
 *
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_PROCESS_H
# define HASHER_PROCESS_H

# include <sys/types.h>

pid_t waitpid_retry(pid_t pid, int *wstatus, int options);

#endif /* HASHER_PROCESS_H */
