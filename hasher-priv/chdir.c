/*
 * The chdir-with-validation module for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "caller_config.h"
#include "caller_data.h"
#include "chdir.h"
#include "error_prints.h"
#include "xmalloc.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

/* This function may be executed with root privileges. */
static const char *
is_changed(struct stat *st1, struct stat *st2)
{
	if (st1->st_dev != st2->st_dev)
		return "device number";
	if (st1->st_ino != st2->st_ino)
		return "inode number";
	if (st1->st_rdev != st2->st_rdev)
		return "device type";
	if (st1->st_mode != st2->st_mode)
		return "protection";
	if (st1->st_uid != st2->st_uid || st1->st_gid != st2->st_gid)
		return "ownership";

	return 0;
}

/*
 * Change the current working directory
 * using lstat+validate+chdir+lstat+compare technique.
 *
 * This function may be executed with root privileges.
 */
static void
safe_chdir_component(const char *name, VALIDATE_FPTR validator)
{
	struct stat st1, st2;
	const char *what;

	if (lstat(name, &st1) < 0)
		perror_msg_and_die("lstat: %s", name);

	if (!S_ISDIR(st1.st_mode)) {
		errno = ENOTDIR;
		perror_msg_and_die("%s", name);
	}

	validator(&st1, name);

	if (chdir(name) < 0)
		perror_msg_and_die("chdir: %s", name);

	if (lstat(".", &st2) < 0)
		perror_msg_and_die("lstat: %s", name);

	if ((what = is_changed(&st1, &st2)))
		error_msg_and_die("%s: %s changed during execution",
				  name, what);
}

/*
 * Change the current working directory using
 * lstat+validate+chdir+lstat+compare technique.
 * If the path is relative, chdir to each path element sequentially.
 *
 * This function may be executed with root privileges.
 */
void
safe_chdir(const char *path, VALIDATE_FPTR validator)
{
	if (path[0] == '/' || !strchr(path, '/'))
		safe_chdir_component(path, validator);
	else {
		char *p = xstrdup(path);
		for (char *elem = strtok(p, "/"); elem; elem = strtok(0, "/"))
			safe_chdir_component(elem, validator);
		free(p);
	}
}

/*
 * Ensure that owner is caller_uid:change_gid1,
 * S_IWOTH bit is not set, and
 * S_IWGRP bit is set only when S_ISVTX bit is also set.
 *
 * This function may be executed with caller privileges.
 */
void
stat_caller_ok_validator(struct stat *st, const char *name)
{
	if (st->st_uid != caller_uid)
		error_msg_and_die("%s: expected owner %u, found owner %u",
				  name, caller_uid, st->st_uid);

	if (st->st_gid != change_gid1)
		error_msg_and_die("%s: expected group %u, found group %u",
				  name, change_gid1, st->st_gid);

	if ((st->st_mode & S_IWOTH)
	    || ((st->st_mode & S_IWGRP) && !(st->st_mode & S_ISVTX)))
		error_msg_and_die("%s: bad perms: %o",
				  name, st->st_mode & 07777);
}

/*
 * Ensure that owner is root and permissions contain no
 * group or world writable bits set.
 *
 * This function may be executed with root privileges.
 */
void
stat_root_ok_validator(struct stat *st, const char *name)
{
	if (st->st_uid)
		error_msg_and_die("%s: bad owner: %u", name, st->st_uid);

	if (st->st_mode & (S_IWGRP | S_IWOTH))
		error_msg_and_die("%s: bad perms: %o",
				  name, st->st_mode & 07777);
}
