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

extern const char *chroot_path;

extern unsigned int caller_num;

#endif /* PKG_BUILD_PRIV_H */
