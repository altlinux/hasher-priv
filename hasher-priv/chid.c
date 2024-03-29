/*
 * The change uid/gid module for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "chid.h"
#include "error_prints.h"
#include <unistd.h>

#ifdef ENABLE_SETFSUGID
#include <sys/fsuid.h>

/*
 * Two setfsuid() in a row - stupid, but
 * how the hell am I supposed to check
 * whether setfsuid() succeeded or not?
 */

/* This function may be executed with root privileges. */
void
ch_uid(uid_t uid, uid_t *save)
{
	uid_t   tmp = (uid_t) setfsuid(uid);

	if (save)
		*save = tmp;
	if ((uid_t) setfsuid(uid) != uid)
		error_msg_and_die("failed to change fsuid to %u", uid);
}

/* This function may be executed with root privileges. */
void
ch_gid(gid_t gid, gid_t *save)
{
	gid_t   tmp = (gid_t) setfsgid(gid);

	if (save)
		*save = tmp;
	if ((gid_t) setfsgid(gid) != gid)
		error_msg_and_die("failed to change fsgid to %u", gid);
}

#else /* ! ENABLE_SETFSUGID */

/* This function may be executed with root privileges. */
void
ch_uid(uid_t uid, uid_t *save)
{
	if (save)
		*save = geteuid();
	if (setresuid((uid_t)-1, uid, 0) < 0)
		perror_msg_and_die("failed to change euid to %u", uid);
}

/* This function may be executed with root privileges. */
void
ch_gid(gid_t gid, gid_t *save)
{
	if (save)
		*save = getegid();
	if (setresgid((gid_t)-1, gid, 0) < 0)
		perror_msg_and_die("failed to change egid to %u", gid);
}

#endif /* ENABLE_SETFSUGID */
