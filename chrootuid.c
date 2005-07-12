
/*
  $Id$
  Copyright (C) 2003-2005  Dmitry V. Levin <ldv@altlinux.org>

  The chrootuid actions for the hasher-priv program.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/* Code in this file may be executed with root privileges. */

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <pty.h>

#include "priv.h"
#include "xmalloc.h"

static void
set_rlimits (void)
{
	change_rlimit_t *p;

	for (p = change_rlimit; p->name; ++p)
	{
		struct rlimit rlim;

		if (!p->hard && !p->soft)
			continue;

		if (getrlimit (p->resource, &rlim) < 0)
			error (EXIT_FAILURE, errno, "getrlimit: %s", p->name);

		if (p->hard)
			rlim.rlim_max = *(p->hard);

		if (p->soft)
			rlim.rlim_cur = *(p->soft);

		if ((unsigned long) rlim.rlim_max <
		    (unsigned long) rlim.rlim_cur)
			rlim.rlim_cur = rlim.rlim_max;

		if (setrlimit (p->resource, &rlim) < 0)
			error (EXIT_FAILURE, errno, "setrlimit: %s", p->name);
	}
}

static int
chrootuid (uid_t uid, gid_t gid, const char *ehome,
	   const char *euser, const char *epath)
{
	int     master = -1, slave = -1, x11_fd;
	int     out[2] = { -1, -1 };
	pid_t   pid;

	if (uid < MIN_CHANGE_UID || uid == getuid ())
		error (EXIT_FAILURE, 0, "chrootuid: invalid uid: %u", uid);

	chdiruid (chroot_path);

	endpwent ();
	endgrent ();

	/* Check and sanitize file descriptors again. */
	sanitize_fds ();

	if (!use_pty && pipe (out) < 0)
		error (EXIT_FAILURE, errno, "pipe");

	/* Always create pty, necessary for ioctl TIOCSCTTY in the child. */
	if (openpty (&master, &slave, 0, 0, 0) < 0)
		error (EXIT_FAILURE, errno, "openpty");

	x11_fd = x11_socket ();

	if (chroot (".") < 0)
		error (EXIT_FAILURE, errno, "chroot: %s", chroot_path);

	if (setgroups (0, 0) < 0)
		error (EXIT_FAILURE, errno, "setgroups");

	set_rlimits ();

	block_signal_handler (SIGCHLD, SIG_BLOCK);

	if ((pid = fork ()) < 0)
		error (EXIT_FAILURE, errno, "fork");

	if (pid)
	{
		if (setgid (caller_gid) < 0)
			error (EXIT_FAILURE, errno, "setgid");

		if (setuid (caller_uid) < 0)
			error (EXIT_FAILURE, errno, "setuid");

		/* Process is no longer privileged at this point. */

		if (close (slave) || (!use_pty && close (out[1])))
			error (EXIT_FAILURE, errno, "close");

		return handle_parent (pid, master, out[0], x11_fd);
	} else
	{
		if (setgid (gid) < 0)
			error (EXIT_FAILURE, errno, "setgid");

		if (setuid (uid) < 0)
			error (EXIT_FAILURE, errno, "setuid");

		/* Process is no longer privileged at this point. */

		if (close (master) || (!use_pty && close (out[0])))
			error (EXIT_FAILURE, errno, "close");

		char   *term_env;

		xasprintf (&term_env, "TERM=%s", term ? : "dumb");
		const char *x11_env = (x11_display && x11_key)
				       ? "DISPLAY=:10.0" : 0;
		const char *const env[] = {
			ehome, euser, epath, term_env, x11_env,
				"SHELL=/bin/sh", 0
		};

		return handle_child ((char *const *) env, slave, out[1],
				     x11_fd);
	}
}

int
do_chrootuid1 (void)
{
	return chrootuid (change_uid1, change_gid1,
			  "HOME=/root", "USER=root",
			  "PATH=/sbin:/usr/sbin:/bin:/usr/bin");
}

int
do_chrootuid2 (void)
{
	return chrootuid (change_uid2, change_gid2,
			  "HOME=/usr/src", "USER=builder",
			  "PATH=/bin:/usr/bin:/usr/X11R6/bin");
}
