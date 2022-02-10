/*
  Copyright (C) 2016-2019  Dmitry V. Levin <ldv@altlinux.org>

  The pty opener for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file is executed with root privileges. */

#include "error_prints.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "priv.h"
#include "xmalloc.h"

int open_pty(int *slave_fd, const int chrooted, const int verbose_error)
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

	if (chrooted)
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
		if (verbose_error)
			perror_msg("open: %s", dev_ptmx);
		goto err;
	}

	if (ioctl(ptmx, TIOCGPTN, &num))
	{
		if (verbose_error)
			perror_msg("ioctl TIOCGPTN: %s", dev_ptmx);
		goto err;
	}

	ptsname = xasprintf(pts_fmt, num);

#ifdef TIOCSPTLCK
	num = 0;
	if (ioctl(ptmx, TIOCSPTLCK, &num))
	{
		if (verbose_error)
			perror_msg("ioctl TIOCSPTLCK: %s", dev_ptmx);
		goto err;
	}
#endif

	slave = open(ptsname, dev_open_flags);
	if (slave < 0)
	{
		if (verbose_error)
			perror_msg("open: %s", ptsname);
		goto err;
	}

	goto out;

err:
	close(slave), slave = -1;
	close(ptmx), ptmx = -1;

out:
	free(ptsname);
	if (chrooted && chdir("/"))
		perror_msg_and_die("chdir: %s", "/");
	ch_uid(saved_uid, 0);
	ch_gid(saved_gid, 0);
	*slave_fd = slave;
	return ptmx;
}
