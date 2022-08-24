/*
 * The namespace setup module for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include "fds.h"
#include "macros.h"
#include "ns.h"
#include "procfd.h"
#include "xmalloc.h"
#include <dirent.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static int
open_proc_ns_fd(pid_t pid, uid_t uid)
{
	int proc_fd = open_proc_fd(pid, uid);
	int ns_fd = openat(proc_fd, "ns", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (ns_fd < 0)
		perror_msg_and_die("open: /proc/%u/%s",
				   (unsigned int) pid, "ns");
	xclose(&proc_fd);
	return ns_fd;
}

void
setup_ns(pid_t pid, uid_t uid)
{
	/* Open /proc/pid/ns directory.  */
	int pid_ns_fd = open_proc_ns_fd(pid, uid);

	/* Open /proc/self/ns directory stream.  */
	static const char self_ns_name[] = "/proc/self/ns";
	DIR *self_ns_dir = opendir(self_ns_name);
	if (!self_ns_dir)
		perror_msg_and_die("opendir: %s", self_ns_name);

	int self_ns_fd = dirfd(self_ns_dir);
	if (self_ns_fd < 0)
		perror_msg_and_die("dirfd: %s", self_ns_name);

	struct {
		const char *name;
		int nstype;
		int fd;
	} enter_ns[] = {
		/*
		 * The list of namespaces that are allowed to differ
		 * should contain at least those namespace types
		 * that are supported by unshare_* functions.
		 */
		{ "mnt", CLONE_NEWNS, -1 },
		{ "ipc", CLONE_NEWIPC, -1 },
		{ "uts", CLONE_NEWUTS, -1 },
		{ "net", CLONE_NEWNET, -1 },
	};

	struct dirent *dent;
	while ((dent = readdir(self_ns_dir))) {
		if (dent->d_type != DT_LNK)
			continue;

		/* Check whether links in these two ns directories match.  */
		struct stat self_st, pid_st;
		if (fstatat(self_ns_fd, dent->d_name, &self_st, 0))
			perror_msg_and_die("fstatat: %s/%s",
					   self_ns_name, dent->d_name);
		if (fstatat(pid_ns_fd, dent->d_name, &pid_st, 0))
			perror_msg_and_die("fstatat: /proc/%u/%s/%s",
					   (unsigned int) pid, "ns", dent->d_name);

		if (self_st.st_dev == pid_st.st_dev &&
		    self_st.st_ino == pid_st.st_ino)
			continue;

		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(enter_ns); ++i) {
			if (strcmp(enter_ns[i].name, dent->d_name))
				continue;
			enter_ns[i].fd = openat(pid_ns_fd, dent->d_name,
						O_RDONLY | O_CLOEXEC);
			break;
		}

		if (i >= ARRAY_SIZE(enter_ns))
			perror_msg_and_die("%s namespace mismatch", dent->d_name);
	}

	closedir(self_ns_dir);
	xclose(&pid_ns_fd);

	for (unsigned int i = 0; i < ARRAY_SIZE(enter_ns); ++i) {
		if (enter_ns[i].fd < 0)
			continue;
		if (setns(enter_ns[i].fd, enter_ns[i].nstype))
			perror_msg_and_die("setns: %s", enter_ns[i].name);
		xclose(&enter_ns[i].fd);
	}
}
