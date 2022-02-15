
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The chrootuid actions for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include "fds.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>

#include "priv.h"
#include "xmalloc.h"

static void
set_rlimits(void)
{
	change_rlimit_t *p;

	for (p = change_rlimit; p->name; ++p)
	{
		struct rlimit rlim;

		if (!p->hard && !p->soft)
			continue;

		if (getrlimit(p->resource, &rlim) < 0)
			perror_msg_and_die("getrlimit: %s", p->name);

		if (p->hard)
			rlim.rlim_max = *(p->hard);

		if (p->soft)
			rlim.rlim_cur = *(p->soft);

		if ((unsigned long) rlim.rlim_max <
		    (unsigned long) rlim.rlim_cur)
			rlim.rlim_cur = rlim.rlim_max;

		if (setrlimit(p->resource, &rlim) < 0)
			perror_msg_and_die("setrlimit: %s", p->name);
	}
}

static int
chrootuid(uid_t uid, gid_t gid, const char *ehome,
	  const char *euser, const char *epath)
{
	int     master = -1, slave = -1;
	int     pipe_out[2] = { -1, -1 };
	int     pipe_err[2] = { -1, -1 };
	int     ctl[2] = { -1, -1 };
	pid_t   pid;

	if (uid < MIN_CHANGE_UID || uid == getuid())
		error_msg_and_die("invalid uid: %u", uid);

	/*
	 * Unshare mount namespace,
	 * mount all requested mountpoints,
	 * setup devices.
	 */
	unshare_mount();

	chdiruid(chroot_path, stat_caller_ok_validator);

	endpwent();
	endgrent();

	/* Check and sanitize file descriptors again. */
	sanitize_fds();

	/* Create pipes only if use_pty is not set. */
	if (!use_pty && (pipe(pipe_out) || pipe(pipe_err)))
		perror_msg_and_die("pipe");

	/* Create socketpair only if X11 forwarding is enabled. */
	if (x11_prepare_connect() == EXIT_SUCCESS
	    && socketpair(AF_UNIX, SOCK_STREAM, 0, ctl))
		perror_msg_and_die("socketpair AF_UNIX");

	unshare_ipc();
	unshare_uts();
	if (!share_caller_network)
		unshare_network();

	if (setgroups(0UL, 0) < 0)
		perror_msg_and_die("setgroups");

	/* Always create pty, necessary for ioctl TIOCSCTTY in the child. */
	master = open_pty(&slave, 0, 1);

	if (chroot(".") < 0)
		perror_msg_and_die("chroot: %s", chroot_path);

	/* Try to create another pty inside chroot. */
	{
		int     slave2 = -1;
		int     master2 = open_pty(&slave2, 1, master < 0);
		if (master2 > master)
		{
			close(master), master = master2;
			close(slave), slave = slave2;
		}
	}

	if (master < 0)
		error_msg_and_die("failed to create pty");

	set_rlimits();

	/* Set close-on-exec flag on all non-standard descriptors. */
	cloexec_fds();

	block_signal_handler(SIGCHLD, SIG_BLOCK);

	if ((pid = fork()) < 0)
		perror_msg_and_die("fork");

	if (pid)
	{
		program_invocation_short_name =
			xasprintf("%s: %s",
				  program_invocation_short_name, "parent");

		if (close(slave)
		    || (!use_pty
			&& (close(pipe_out[1]) || close(pipe_err[1])))
		    || (x11_display && close(ctl[1])))
			perror_msg_and_die("close");

		if (setgid(caller_gid) < 0)
			perror_msg_and_die("setgid");

		if (setuid(caller_uid) < 0)
			perror_msg_and_die("setuid");

		/* Process is no longer privileged at this point. */

		return handle_parent(pid, master, pipe_out[0], pipe_err[0],
				     ctl[0]);
	} else
	{
		program_invocation_short_name =
			xasprintf("%s: %s",
				  program_invocation_short_name, "child");

		if (close(master)
		    || (!use_pty
			&& (close(pipe_out[0]) || close(pipe_err[0])))
		    || (x11_display && close(ctl[0])))
			perror_msg_and_die("close");

		if (share_caller_network)
			unshare_network();

		if (setgid(gid) < 0)
			perror_msg_and_die("setgid");

		if (setuid(uid) < 0)
			perror_msg_and_die("setuid");

		/* Process is no longer privileged at this point. */

		char   *term_env = xasprintf("TERM=%s", term ? : "dumb");
		const char *x11_env = x11_display ? "DISPLAY=:10.0" : 0;
		const char *const env[] = {
			ehome, euser, epath, term_env, x11_env,
			"SHELL=/bin/sh", 0
		};

		handle_child(env, slave,
			     pipe_out[1], pipe_err[1], ctl[1]);
	}
}

int
do_chrootuid1(void)
{
	return chrootuid(change_uid1, change_gid1,
			 "HOME=/root", "USER=root",
			 "PATH=/sbin:/usr/sbin:/bin:/usr/bin");
}

int
do_chrootuid2(void)
{
	return chrootuid(change_uid2, change_gid2,
			 "HOME=/usr/src", "USER=builder",
			 "PATH=/bin:/usr/bin:/usr/X11R6/bin");
}
