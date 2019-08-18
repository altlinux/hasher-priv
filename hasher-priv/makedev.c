
/*
  Copyright (C) 2003-2006  Dmitry V. Levin <ldv@altlinux.org>

  Setup devices for the hasher-priv program.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Code in this file may be executed with root privileges. */

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysmacros.h>

#include "priv.h"

static void
xmknod(const char *name, mode_t mode, unsigned major, unsigned minor)
{
	if (mknod(name, mode, makedev(major, minor)))
		error(EXIT_FAILURE, errno, "mknod: %s", name);
}

static void
xmkdir(const char *name, mode_t mode)
{
	if (mkdir(name, mode))
		error(EXIT_FAILURE, errno, "mkdir: %s", name);
}

static void
xsymlink(const char *target, const char *linkpath)
{
	if (symlink(target, linkpath))
		error(EXIT_FAILURE, errno, "symlink: %s", linkpath);
}

void
setup_devices(void)
{
	gid_t   saved_gid = (gid_t) - 1;
	mode_t  m;

	chdiruid(chroot_path, stat_caller_ok_validator);
	chdiruid("dev", stat_root_ok_validator);

	ch_gid(0, &saved_gid);
	m = umask(0);

	xmkdir("pts", 0755);
	xmkdir("shm", 0755);

	xsymlink("../proc/self/fd", "fd");
	xsymlink("../proc/self/fd/0", "stdin");
	xsymlink("../proc/self/fd/1", "stdout");
	xsymlink("../proc/self/fd/2", "stderr");

	xmknod("null", S_IFCHR | 0666, 1, 3);
	xmknod("zero", S_IFCHR | 0666, 1, 5);
	xmknod("full", S_IFCHR | 0666, 1, 7);
	xmknod("urandom", S_IFCHR | 0644, 1, 9);
	xmknod("random", S_IFCHR | 0644, 1, 9);	/* pseudo random. */

	if (makedev_console) {
		xmknod("console", S_IFCHR | 0600, 5, 1);
		xmknod("tty0", S_IFCHR | 0600, 4, 0);
		xmknod("fb0", S_IFCHR | 0600, 29, 0);
	}

	if (dev_pts_mounted) {
		xmknod("tty", S_IFCHR | 0666, 5, 0);
		xsymlink("pts/ptmx", "ptmx");
	}

	log_fd = log_listen();

	umask(m);
	ch_gid(saved_gid, 0);
}
