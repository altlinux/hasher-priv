/*
 * The switch-user-and-chdir-with-validation module for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "caller_config.h"
#include "caller_data.h"
#include "chdir.h"
#include "chid.h"
#include "error_prints.h"
#include "xmalloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>

/*
 * Check whether the file path PREFIX is prefix of the file path SAMPLE.
 * Return zero if prefix check succeeded, and non-zero otherwise.
 *
 * This function may be executed with caller privileges.
 */
static int
is_prefix(const char *prefix, const char *sample)
{
	size_t  len = strlen(prefix);

	return !strncmp(sample, prefix, len)
		&& ((sample[len] == '\0') || (sample[len] == '/'));
}

/*
 * Check the current working directory against the chroot prefix path.
 *
 * This function may be executed with caller privileges.
 */
static void
check_cwd(void)
{
	char *cwd = getcwd(0, 0UL);
	if (!cwd)
		perror_msg_and_die("getcwd");

	int invalid = !!chroot_prefix_list;
	for (const char *const *prefix = chroot_prefix_list;
	     prefix && *prefix; ++prefix) {
		if (is_prefix(*prefix, cwd)) {
			invalid = 0;
			break;
		}
	}

	if (invalid) {
		error_msg_and_die("%s: prefix mismatch, working directory"
				  " should start with one of directories"
				  " listed in colon-separated prefix list (%s)",
				  cwd, chroot_prefix_path);
	}

	free(cwd);
}

/*
 * Temporary change credentials to caller_user during this operation.
 *
 * This function may be executed with root privileges.
 */
static void
change_creds(uid_t *saved_uid, gid_t *saved_gid)
{
	*saved_uid = (uid_t) -1;
	*saved_gid = (gid_t) -1;
#ifdef ENABLE_SUPPLEMENTARY_GROUPS
	if (initgroups(caller_user, caller_gid) < 0)
		perror_msg_and_die("initgroups(%s, %u)",
				   caller_user, caller_gid);
#endif /* ENABLE_SUPPLEMENTARY_GROUPS */
	ch_gid(caller_gid, saved_gid);
	ch_uid(caller_uid, saved_uid);
}

/*
 * Restore credentials saved by change_creds.
 *
 * This function may be executed with caller privileges.
 */
static void
restore_creds(uid_t saved_uid, gid_t saved_gid)
{
	ch_uid(saved_uid, 0);
	ch_gid(saved_gid, 0);
#ifdef ENABLE_SUPPLEMENTARY_GROUPS
	if (setgroups(0UL, 0) < 0)
		perror_msg_and_die("setgroups");
#endif /* ENABLE_SUPPLEMENTARY_GROUPS */
}

/*
 * Change the current working directory to the given path.
 * Temporary change credentials to caller_user during this operation.
 * If the path is relative, chdir to each path element sequentially.
 * If chroot prefix path is set, ensure that it matches given path.
 *
 * This function may be executed with root privileges.
 */
void
chdiruid(const char *path, VALIDATE_FPTR validator)
{
	if (!path)
		error_msg_and_die("invalid chroot path");

	/* Change credentials. */
	uid_t saved_uid;
	gid_t saved_gid;
	change_creds(&saved_uid, &saved_gid);

	/* Change and verify directory. */
	if (path[0] == '/' || !strchr(path, '/')) {
		safe_chdir(path, validator);
	} else {
		char *p = xstrdup(path);
		for (char *elem = strtok(p, "/"); elem; elem = strtok(0, "/"))
			safe_chdir(elem, validator);
		free(p);
	}

	/* Check the current working directory against the chroot prefix path. */
	check_cwd();

	/* Restore credentials. */
	restore_creds(saved_uid, saved_gid);
}
