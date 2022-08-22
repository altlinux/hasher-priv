/*
 * The procfd module for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "procfd.h"
#include "xmalloc.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int
open_proc_fd(pid_t pid, uid_t uid)
{
	/* Open /proc/pid directory.  */
	char *fname = xasprintf("/proc/%u", (unsigned int) pid);
	int fd = open(fname, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (fd < 0)
		perror_msg_and_die("open: %s", fname);

	/* Check that the directory belongs to the specified uid.  */
	struct stat st;
	if (fstat(fd, &st))
		perror_msg_and_die("fstat: %s", fname);
	if (st.st_uid != uid)
		error_msg_and_die("%s: expected owner %u, found owner %u",
				  fname, uid, st.st_uid);

	free(fname);

	return fd;
}
