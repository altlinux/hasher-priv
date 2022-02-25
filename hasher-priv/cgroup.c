/*
 * The cgroup module for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "caller_data.h"
#include "cgroup.h"
#include "error_prints.h"
#include "fds.h"
#include "io_loop.h"
#include "logging.h"
#include "procfd.h"
#include "xmalloc.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
join_cgroup(const char *cgroup_path) {
	char *fname = xasprintf("%s/%s/%s", "/sys/fs/cgroup",
				cgroup_path, "cgroup.procs");
	int fd = open(fname, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		perror_msg_and_die("open: %s", fname);

	debug_msg("joining %s", cgroup_path);

	if (dprintf(fd, "%d\n", getpid()) < 0)
		perror_msg_and_die("dprintf: %s", fname);

	if (xclose(&fd) < 0)
		perror_msg_and_die("close: %s", fname);

	free(fname);
}

void
join_caller_cgroup(pid_t pid)
{
	int proc_fd = open_proc_fd(pid, caller_uid);

	static const char cgroup_name[] = "cgroup";
	int cgroup_fd = openat(proc_fd, cgroup_name, O_RDONLY | O_CLOEXEC);
	if (cgroup_fd < 0)
		perror_msg_and_die("open: %s", cgroup_name);

	static const char prefix[] = "0::";
	enum { prefix_len = sizeof(prefix) - 1 };
	char line[PATH_MAX + prefix_len];

	ssize_t sz = read_loop(cgroup_fd, line, sizeof(line) - 1);
	if (sz < 0)
		perror_msg_and_die("read: %s", cgroup_name);

	xclose(&cgroup_fd);
	xclose(&proc_fd);

	if (sz == 0) {
		debug_msg("%s file is empty", cgroup_name);
		return;
	}

	switch (line[sz - 1]) {
		case '\0':
			break;
		case '\n':
			line[--sz] = '\0';
			break;
		default:
			line[sz] = '\0';
			break;
	}

	if (strncmp(line, prefix, prefix_len))
		error_msg_and_die("%s: not version 2", cgroup_name);

	join_cgroup(&line[prefix_len]);
}
