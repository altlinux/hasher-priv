/*
 * The I/O retry and loop interface for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_IO_LOOP_H
# define HASHER_IO_LOOP_H

#include <sys/types.h>

struct msghdr;

ssize_t read_retry(int fd, void *buf, size_t count);
ssize_t write_retry(int fd, const void *buf, size_t count);
ssize_t sendmsg_retry(int fd, const struct msghdr *, int flags);
ssize_t recvmsg_retry(int fd, struct msghdr *, int flags);
ssize_t read_loop(int fd, char *buffer, size_t count);
ssize_t write_loop(int fd, const char *buffer, size_t count);

#endif /* !HASHER_IO_LOOP_H */
