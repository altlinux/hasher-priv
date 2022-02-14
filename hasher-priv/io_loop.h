/*
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_IO_LOOP_H
# define HASHER_IO_LOOP_H

#include <sys/types.h>

ssize_t read_retry(int fd, void *buf, size_t count);
ssize_t write_retry(int fd, const void *buf, size_t count);
ssize_t read_loop(int fd, char *buffer, size_t count);
ssize_t write_loop(int fd, const char *buffer, size_t count);

#endif /* !HASHER_IO_LOOP_H */
