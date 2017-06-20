/*
 * A collection of helpers to simplify the use of sockets
 * for the hasher-priv project.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_SOCKETS_H_
#define HASHER_SOCKETS_H_

#include "cc_compat.h"
#include <sys/types.h>

int srv_listen(const char *)  ATTRIBUTE_NONNULL((1));
int srv_connect(const char *, const char *) ATTRIBUTE_NONNULL((1, 2));
int srv_try_connect(const char *, const char *) ATTRIBUTE_NONNULL((1, 2));

int get_peercred(int, pid_t *, uid_t *, gid_t *);
int set_recv_timeout(int fd, int secs);

int xsendmsg(int conn, void *data, size_t len) ATTRIBUTE_NONNULL((2));
int xrecvmsg(int conn, void *data, size_t len) ATTRIBUTE_NONNULL((2));

#endif /* HASHER_SOCKETS_H_ */
