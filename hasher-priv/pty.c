/*
 * The pty opener for the hasher-privd server program.
 *
 * Copyright (C) 2016-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file is executed with root privileges. */

#include "caller_data.h"
#include "chdir.h"
#include "chid.h"
#include "error_prints.h"
#include "fds.h"
#include "mount.h"
#include "pty.h"
#include "xmalloc.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

int
open_pty(int *slave_fd, const enum open_pty_chrootedness chrooted,
	 const enum open_pty_verbosity verbose_error)
{
	const char *dev_ptmx = "/dev/ptmx";
	const char *pts_fmt = "/dev/pts/%u";
	const int dev_open_flags = O_RDWR | O_NOCTTY;
	unsigned int num = 0;
	int     ptmx = -1;
	int     slave = -1;
	gid_t   saved_gid = (gid_t) - 1;
	uid_t   saved_uid = (uid_t) - 1;
	char   *ptsname = NULL;

	ch_gid(caller_gid, &saved_gid);
	ch_uid(caller_uid, &saved_uid);

	if (chrooted == OPEN_PTY_CHROOTED)
	{
		static const char dev_pts_ptmx[] = "pts/ptmx";
		const mode_t rwdev = S_IFCHR | 0666;
		struct stat st;

		if (!dev_pts_mounted)
			goto err;

		safe_chdir("dev", stat_root_ok_validator);
		dev_ptmx = "ptmx";
		pts_fmt = "pts/%u";

		if (stat(dev_pts_ptmx, &st) ||
		    (st.st_mode & rwdev) != rwdev)
			goto err;
	}

	ptmx = open(dev_ptmx, dev_open_flags);
	if (ptmx < 0)
	{
		if (verbose_error == OPEN_PTY_VERBOSE)
			perror_msg("open: %s", dev_ptmx);
		goto err;
	}

	if (ioctl(ptmx, TIOCGPTN, &num))
	{
		if (verbose_error == OPEN_PTY_VERBOSE)
			perror_msg("ioctl TIOCGPTN: %s", dev_ptmx);
		goto err;
	}

	ptsname = xasprintf(pts_fmt, num);

#ifdef TIOCSPTLCK
	num = 0;
	if (ioctl(ptmx, TIOCSPTLCK, &num))
	{
		if (verbose_error == OPEN_PTY_VERBOSE)
			perror_msg("ioctl TIOCSPTLCK: %s", dev_ptmx);
		goto err;
	}
#endif

	slave = open(ptsname, dev_open_flags);
	if (slave < 0)
	{
		if (verbose_error == OPEN_PTY_VERBOSE)
			perror_msg("open: %s", ptsname);
		goto err;
	}

	goto out;

err:
	xclose(&slave);
	xclose(&ptmx);

out:
	free(ptsname);
	if ((chrooted == OPEN_PTY_CHROOTED) && chdir("/"))
		perror_msg_and_die("chdir: %s", "/");
	ch_uid(saved_uid, 0);
	ch_gid(saved_gid, 0);
	*slave_fd = slave;
	return ptmx;
}
