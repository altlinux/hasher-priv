
/*
  Copyright (C) 2004-2019  Dmitry V. Levin <ldv@altlinux.org>

  The mount action for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <mntent.h>
#include <sys/mount.h>

#include "priv.h"
#include "xmalloc.h"

int dev_pts_mounted;

static struct mnt_ent
{
	const char *mnt_fsname;
	const char *mnt_dir;
	const char *mnt_type;
	const char *mnt_opts;
} def_fstab[] =
{
	{"dev", "/dev", "tmpfs", "nosuid,noexec,gid=0,mode=755,nr_blocks=0,nr_inodes=256"},
	{"proc", "/proc", "proc", "ro,nosuid,nodev,noexec,gid=proc"},
	{"devpts", "/dev/pts", "devpts", "ro,nosuid,noexec,gid=tty,mode=0620,ptmxmode=0666,newinstance"},
	{"sysfs", "/sys", "sysfs", "ro,nosuid,nodev,noexec"},
	{"shmfs", "/dev/shm", "tmpfs", "nosuid,nodev,noexec,gid=0,mode=1777,nr_blocks=4096,nr_inodes=4096"},
	{"/sys/fs/cgroup", "/sys/fs/cgroup", "rbind", "ro,rbind,nosuid,nodev,noexec"}
};

#define def_fstab_size (sizeof (def_fstab) / sizeof (def_fstab[0]))

#ifndef MS_MANDLOCK
#define MS_MANDLOCK	64
#endif
#ifndef MS_DIRSYNC
#define MS_DIRSYNC	128
#endif
#ifndef MS_NOATIME
#define MS_NOATIME	1024
#endif
#ifndef MS_NODIRATIME
#define MS_NODIRATIME	2048
#endif
#ifndef MS_BIND
#define MS_BIND		4096
#endif
#ifndef MS_MOVE
#define MS_MOVE		8192
#endif
#ifndef MS_REC
#define MS_REC		16384
#endif
#ifndef MS_SLAVE
#define MS_SLAVE	(1 << 19)
#endif

static struct
{
	const char *name;
	int     invert;
	unsigned long value;
} opt_map[] =
{
	{"defaults", 0, 0},
	{"rw", 1, MS_RDONLY},
	{"ro", 0, MS_RDONLY},
	{"suid", 1, MS_NOSUID},
	{"nosuid", 0, MS_NOSUID},
	{"dev", 1, MS_NODEV},
	{"nodev", 0, MS_NODEV},
	{"exec", 1, MS_NOEXEC},
	{"noexec", 0, MS_NOEXEC},
	{"sync", 0, MS_SYNCHRONOUS},
	{"async", 1, MS_SYNCHRONOUS},
	{"mand", 0, MS_MANDLOCK},
	{"nomand", 1, MS_MANDLOCK},
	{"dirsync", 0, MS_DIRSYNC},
	{"dirasync", 1, MS_DIRSYNC},
	{"bind", 0, MS_BIND},
	{"rbind", 0, MS_BIND | MS_REC},
	{"atime", 1, MS_NOATIME},
	{"noatime", 0, MS_NOATIME},
	{"diratime", 1, MS_NODIRATIME},
	{"nodiratime", 0, MS_NODIRATIME}
};

#define opt_map_size (sizeof (opt_map) / sizeof (opt_map[0]))

static void
parse_opt(const char *opt, unsigned long *flags, char **options)
{
	unsigned i;

	for (i = 0; i < opt_map_size; ++i)
		if (!strcmp(opt, opt_map[i].name))
			break;

	if (i < opt_map_size)
	{
		if (opt_map[i].invert)
			*flags &= ~opt_map[i].value;
		else
			*flags |= opt_map[i].value;
		return;
	}

	char   *buf = 0;

	if (!strncmp(opt, "gid=", 4UL) && !isdigit(opt[4]))
	{
		struct group *gr = getgrnam(opt + 4);

		if (gr)
		{
			xasprintf(&buf, "gid=%u", (unsigned) gr->gr_gid);
			opt = buf;
		}
	}

	if (*options)
	{
		*options = xrealloc(*options, 1UL,
				    strlen(*options) + strlen(opt) + 2);
		strcat(*options, ",");
		strcat(*options, opt);
	} else
	{
		*options = xstrdup(opt);
	}

	free(buf);
}

static void
xmount(struct mnt_ent *e)
{
	if (e->mnt_dir[0] != '/')
		error(EXIT_FAILURE, EINVAL, "xmount: %s", e->mnt_dir);

	char   *options = 0, *opt;
	char   *buf = xstrdup(e->mnt_opts);
	unsigned long flags = MS_MGC_VAL | MS_NOSUID;

	for (opt = strtok(buf, ","); opt; opt = strtok(0, ","))
		parse_opt(opt, &flags, &options);

	chdiruid(chroot_path, stat_caller_ok_validator);

	int is_dev_subdir = strncmp(e->mnt_dir + 1, "dev/", 4) == 0;
	chdiruid(e->mnt_dir + 1,
		 is_dev_subdir ? stat_root_ok_validator : stat_caller_ok_validator);

	if (mount(e->mnt_fsname, ".", e->mnt_type, flags, options ? : ""))
		error(EXIT_FAILURE, errno, "mount: %s", e->mnt_dir);

	free(options);
	free(buf);
}

static struct mnt_ent **var_fstab;
size_t var_fstab_size;

static void
load_fstab(void)
{
	safe_chdir("/", stat_root_ok_validator);
	safe_chdir("etc/hasher-priv", stat_root_ok_validator);

	const char *name = "fstab";
	int     fd = open(name, O_RDONLY | O_NOFOLLOW | O_NOCTTY);

	if (fd < 0)
		error(EXIT_FAILURE, errno, "open: %s", name);
	safe_chdir("/", stat_root_ok_validator);

	struct stat st;

	if (fstat(fd, &st) < 0)
		error(EXIT_FAILURE, errno, "fstat: %s", name);

	stat_root_ok_validator(&st, name);

	if (!S_ISREG(st.st_mode))
		error(EXIT_FAILURE, 0, "%s: not a regular file", name);

	if (st.st_size > MAX_CONFIG_SIZE)
		error(EXIT_FAILURE, 0, "%s: file too large: %lu",
		      name, (unsigned long) st.st_size);

	FILE   *fp = fdopen(fd, "r");

	if (!fp)
		error(EXIT_FAILURE, errno, "fdopen: %s", name);

	struct mntent *ent;

	while ((ent = getmntent(fp)))
	{
		struct mnt_ent *e = xmalloc(sizeof(*e));

		e->mnt_fsname = xstrdup(ent->mnt_fsname);
		e->mnt_dir = xstrdup(ent->mnt_dir);
		e->mnt_type = xstrdup(ent->mnt_type);
		e->mnt_opts = xstrdup(ent->mnt_opts);

		var_fstab =
			xrealloc(var_fstab,
				 var_fstab_size + 1, sizeof(*var_fstab));
		var_fstab[var_fstab_size++] = e;
	}

	(void) fclose(fp);
}

static int
is_allowed(const char *item, str_list_t *allowed)
{
	for (size_t i = 0; i < allowed->len; ++i)
		if (allowed->list[i] && !strcmp(item, allowed->list[i]))
			return 1;
	return 0;
}

static struct mnt_ent *
lookup_mount_entry(const char *mpoint)
{
	size_t i;
	struct mnt_ent *e = 0;

	for (i = 0; !e && i < var_fstab_size; ++i)
		if (!strcmp(mpoint, var_fstab[i]->mnt_dir))
			e = var_fstab[i];

	for (i = 0; !e && i < def_fstab_size; ++i)
		if (!strcmp(mpoint, def_fstab[i].mnt_dir))
			e = &def_fstab[i];

	if (!e)
		error(EXIT_FAILURE, 0,
		      "%s: mount point is not supported", mpoint);

	return e;
}

/* called by unshare_mount() after successful CLONE_NEWNS */
void
setup_mountpoints(void)
{
	const char **dev_vec = 0;
	size_t dev_allocated = 0;
	size_t dev_size = 0;

	const char **mpoint_vec = 0;
	size_t mpoint_allocated = 0;
	size_t mpoint_size = 0;

	for (size_t i = 0; i < requested_mountpoints.len; ++i) {
		const char *item = requested_mountpoints.list[i];
		if (!item)
			continue;

		/* /dev and /dev/shm are mounted unconditionally.  */
		if (!strcmp(item, "/dev") || !strcmp(item, "/dev/shm"))
			continue;

		if (item[0] != '/' || item[1] == '/')
			error(EXIT_FAILURE, 0,
			      "%s: mount point is not supported", item);

		int allowed_dev = is_allowed(item, &allowed_devices);
		int allowed_mpoint = is_allowed(item, &allowed_mountpoints);
		if (allowed_dev && allowed_mpoint)
			error(EXIT_FAILURE, 0,
			      "%s: configured as device and mount point"
			      " simultaneously", item);
		if (allowed_dev) {
			if (strncmp(item, "/dev/", 5))
				error(EXIT_FAILURE, 0,
				      "%s: device name is not supported", item);
			if (dev_size >= dev_allocated)
				dev_vec = xgrowarray(dev_vec, &dev_allocated,
						     sizeof(*dev_vec));
			dev_vec[dev_size++] = item;
		} else if (allowed_mpoint) {
			if (mpoint_size >= mpoint_allocated)
				mpoint_vec =
					xgrowarray(mpoint_vec, &mpoint_allocated,
						   sizeof(*mpoint_vec));
			mpoint_vec[mpoint_size++] = item;
			if (!strcmp("/dev/pts", item))
				dev_pts_mounted = 1;
		} else {
			error(EXIT_FAILURE, 0,
			      "%s: mount point is not allowed", item);
		}
	}

	/*
	 * Just in case that some filesystem is mounted as shared,
	 * remount it as slave in our namespace so that
	 * no further mounts show up outside.
	 */
	if (mount("/", "/", NULL, MS_SLAVE | MS_REC, NULL) < 0 &&
	    errno != EINVAL)
		error(EXIT_FAILURE, errno,
		      "mount MS_SLAVE: %s", chroot_path);

	load_fstab();

	xmount(lookup_mount_entry("/dev"));

	setup_devices(dev_vec, dev_size);

	xmount(lookup_mount_entry("/dev/shm"));
	for (size_t i = 0; i < mpoint_size; ++i)
		xmount(lookup_mount_entry(mpoint_vec[i]));

	free(dev_vec);
	free(mpoint_vec);
}
