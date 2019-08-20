
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The switch-user-and-chdir-with-validation module for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>

#include "priv.h"
#include "xmalloc.h"

/*
 * Check whether the file path PREFIX is prefix of the file path SAMPLE.
 * Return zero if prefix check succeeded, and non-zero otherwise.
 */

/* This function may be executed with caller privileges. */
static int
is_prefix(const char *prefix, const char *sample)
{
	size_t  len = strlen(prefix);

	return !strncmp(sample, prefix, len)
		&& ((sample[len] == '\0') || (sample[len] == '/'));
}

/*
 * Change the current work directory to the given path.
 * If chroot prefix path is set, ensure that it matches given path.
 */

/* This function may be executed with caller privileges. */
static void
chdiruid_simple(const char *path, VALIDATE_FPTR validator)
{
	/* Change and verify directory. */
	safe_chdir(path, validator);

	char   *cwd;

	if (!(cwd = getcwd(0, 0UL)))
		error(EXIT_FAILURE, errno, "getcwd");

	/* Check for chroot prefix path. */
	const char *const *prefix;
	int invalid = !!chroot_prefix_list;

	for (prefix = chroot_prefix_list; prefix && *prefix; ++prefix)
	{
		if (is_prefix(*prefix, cwd))
		{
			invalid = 0;
			break;
		}
	}

	if (invalid)
		error(EXIT_FAILURE, 0,
		      "%s: prefix mismatch, working directory should start with one of directories listed in colon-separated prefix list (%s)",
		      cwd, chroot_prefix_path);

	free(cwd);
}

/*
 * Change the current work directory to the given path.
 * Temporary change credentials to caller_user during this operation.
 * If the path is relative, chdir to each path element sequentially.
 * If chroot prefix path is set, ensure that it matches given path.
 */

/* This function may be executed with root privileges. */
void
chdiruid(const char *path, VALIDATE_FPTR validator)
{
	uid_t   saved_uid = (uid_t) - 1;
	gid_t   saved_gid = (gid_t) - 1;

	if (!path)
		error(EXIT_FAILURE, 0, "chdiruid: invalid chroot path");

	/* Set credentials. */
#ifdef ENABLE_SUPPLEMENTARY_GROUPS
	if (initgroups(caller_user, caller_gid) < 0)
		error(EXIT_FAILURE, errno, "chdiruid: initgroups: %s", caller_user);
#endif /* ENABLE_SUPPLEMENTARY_GROUPS */
	ch_gid(caller_gid, &saved_gid);
	ch_uid(caller_uid, &saved_uid);

	/* Change and verify directory, check for chroot prefix path. */
	if (path[0] == '/')
		chdiruid_simple(path, validator);
	else
	{
		if (!strchr(path, '/'))
			chdiruid_simple(path, validator);
		else
		{
			char   *elem, *p = xstrdup(path);

			for (elem = strtok(p, "/"); elem; elem = strtok(0, "/"))
				chdiruid_simple(elem, validator);
			free(p);
		}
	}

	/* Restore credentials. */
	ch_uid(saved_uid, 0);
	ch_gid(saved_gid, 0);
#ifdef ENABLE_SUPPLEMENTARY_GROUPS
	if (setgroups(0UL, 0) < 0)
		error(EXIT_FAILURE, errno, "chdiruid: setgroups");
#endif /* ENABLE_SUPPLEMENTARY_GROUPS */
}
