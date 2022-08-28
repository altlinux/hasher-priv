/*
 * The main include header for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PKG_BUILD_PRIV_H
#define PKG_BUILD_PRIV_H

#include "cc_compat.h"
#include <sys/types.h>

#define	MIN_CHANGE_UID	34
#define	MIN_CHANGE_GID	34

typedef enum
{
	JOB_NONE = 0,
	JOB_GETCONF,
	JOB_KILLUID,
	JOB_GETUGID1,
	JOB_CHROOTUID1,
	JOB_GETUGID2,
	JOB_CHROOTUID2,
} job_enum_t;

void    xwrite_all(int fd, const char *buffer, size_t count);
void    handle_child(const char *const *argv, const char *const *env,
		     int pty_fd, int pipe_out, int pipe_err, int ctl_fd)
	ATTRIBUTE_NORETURN;
int     handle_parent(pid_t pid, int pty_fd, int pipe_out, int pipe_err, int ctl_fd);

extern const char *chroot_path;

extern int dev_pts_mounted;

extern unsigned int caller_num;

#endif /* PKG_BUILD_PRIV_H */
