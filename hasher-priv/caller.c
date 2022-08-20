/*
 * The caller data initialization module for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_data.h"
#include "error_prints.h"
#include "xmalloc.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>

#include "priv.h"

const char *caller_user;
const char *caller_home;
uid_t caller_uid;
gid_t caller_gid;

/*
 * Initialize caller_user, caller_uid, caller_gid and caller_home.
 */
void
init_caller_data(uid_t uid, gid_t gid)
{
	caller_uid = uid;
	if (caller_uid < MIN_CHANGE_UID)
		error_msg_and_die("caller has invalid uid: %u", caller_uid);

	caller_gid = gid;
	if (caller_gid < MIN_CHANGE_GID)
		error_msg_and_die("caller has invalid gid: %u", caller_gid);

	struct passwd *pw = getpwuid(caller_uid);

	if (!pw || !pw->pw_name)
		error_msg_and_die("caller lookup failure");

	caller_user = xstrdup(pw->pw_name);

	if (caller_uid != pw->pw_uid)
		error_msg_and_die("caller %s: uid mismatch", caller_user);

	if (caller_gid != pw->pw_gid)
		error_msg_and_die("caller %s: gid mismatch", caller_user);

	errno = 0;
	if (pw->pw_dir && pw->pw_dir[0] == '/')
		caller_home = canonicalize_file_name(pw->pw_dir);
	else
		caller_home = 0;

	if (!caller_home || caller_home[0] != '/')
		perror_msg_and_die("caller %s: invalid home", caller_user);
}
