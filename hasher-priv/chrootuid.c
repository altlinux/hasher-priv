/*
 * The chrootuid actions for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "caller_data.h"
#include "change_rlimit.h"
#include "chdir.h"
#include "child.h"
#include "error_prints.h"
#include "executors.h"
#include "fds.h"
#include "mount.h"
#include "ns.h"
#include "parent.h"
#include "pty.h"
#include "signals.h"
#include "spawn_killuid.h"
#include "unshare.h"
#include "x11.h"
#include "xmalloc.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>
#include <sys/socket.h>

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
chrootuid(uid_t uid, gid_t gid, const char *user_name, const char *const *argv,
	  const char *ehome, const char *euser, const char *epath)
{
	int     master = -1, slave = -1;
	int     pipe_out[2] = { -1, -1 };
	int     pipe_err[2] = { -1, -1 };
	int     ctl[2] = { -1, -1 };
	pid_t   pid;

	spawn_killuid();

	/*
	 * Obtain the supplementary group access list for the target user,
	 * clear the current supplementary group access list.
	 */
	int ngroups;
	gid_t *groups = NULL;

	if (initgroups(user_name, gid) != 0)
	    perror_msg_and_die("initgroups");

	if ((ngroups = getgroups(0, NULL)) == -1)
	    perror_msg_and_die("getgroups");

	if (ngroups > 0) {
	    groups = (gid_t *)xcalloc((size_t)ngroups, sizeof(gid_t));

	    if ((ngroups = getgroups(ngroups, groups)) == -1)
		perror_msg_and_die("getgroups");
	}

	if (setgroups(0UL, 0) < 0)
		perror_msg_and_die("setgroups");

	/* Check and setup namespaces.  */
	setup_ns(caller_pid, caller_uid);

	/*
	 * chdir to the chroot directory,
	 * unshare the mount namespace,
	 * reopen the chroot directory in the new mount namespace.
	 */
	fchdiruid(chroot_fd, stat_caller_ok_validator);
	unshare_mount();
	chroot_fd = open(".", O_RDONLY);
	if (chroot_fd < 0)
		perror_msg_and_die("open: .");

	/* Mount all requested mountpoints and setup devices. */
	setup_mountpoints();

	/* chdir back to the chroot directory after setup_mountpoints. */
	fchdiruid(chroot_fd, stat_caller_ok_validator);
	xclose(&chroot_fd);

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

	/* Always create pty, necessary for ioctl TIOCSCTTY in the child. */
	master = open_pty(&slave, OPEN_PTY_UNCHROOTED, OPEN_PTY_VERBOSE);

	if (chroot(".") < 0)
		perror_msg_and_die("chroot");

	/* Try to create another pty inside chroot. */
	{
		int slave2 = -1;
		int master2 =
			open_pty(&slave2, OPEN_PTY_CHROOTED,
				 master < 0 ? OPEN_PTY_VERBOSE : OPEN_PTY_SILENT);
		if (master2 > master)
		{
			xclose(&master), master = master2;
			xclose(&slave), slave = slave2;
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

		free(groups);

		if (xclose(&slave)
		    || (!use_pty
			&& (xclose(&pipe_out[1]) || xclose(&pipe_err[1])))
		    || (x11_display && xclose(&ctl[1])))
			perror_msg_and_die("close");

		/*
		 * Do not assume that fs.suid_dumpable == 0
		 * and clear the dumpable flag explicitly.
		 */
		if (prctl(PR_SET_DUMPABLE, 0))
			perror_msg_and_die("prctl PR_SET_DUMPABLE");

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

		if (xclose(&master)
		    || (!use_pty
			&& (xclose(&pipe_out[0]) || xclose(&pipe_err[0])))
		    || (x11_display && xclose(&ctl[0])))
			perror_msg_and_die("close");

		if (share_caller_network)
			unshare_network();

		/*
		 * Do not assume that fs.suid_dumpable == 0
		 * and clear the dumpable flag explicitly.
		 */
		if (prctl(PR_SET_DUMPABLE, 0))
			perror_msg_and_die("prctl PR_SET_DUMPABLE");

		setgroups((size_t) ngroups, groups);
		free(groups);

		if (setgid(gid) < 0)
			perror_msg_and_die("setgid");

		if (setuid(uid) < 0)
			perror_msg_and_die("setuid");

		/* Process is no longer privileged at this point. */

		char   *term_env = xasprintf("TERM=%s", term ? : "dumb");
		const char *x11_env = x11_display ? "DISPLAY=:10.0" : 0;
		const char *const env[] = {
			ehome, euser, epath, term_env, x11_env, 0
		};

		handle_child(argv, env, slave,
			     pipe_out[1], pipe_err[1], ctl[1]);
	}
}

int
do_chrootuid1(const char *const *argv)
{
	return chrootuid(change_uid1, change_gid1, change_user1, argv,
			 "HOME=/root", "USER=root",
			 "PATH=/sbin:/usr/sbin:/bin:/usr/bin");
}

int
do_chrootuid2(const char *const *argv)
{
	return chrootuid(change_uid2, change_gid2, change_user2, argv,
			 "HOME=/usr/src", "USER=builder",
			 "PATH=/bin:/usr/bin:/usr/X11R6/bin");
}
