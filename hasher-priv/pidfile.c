/*
 * The pidfiles module for the hasher-privd server program.
 *
 * Copyright (c) 1995  Martin Schulze <Martin.Schulze@Linux.DE>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "fds.h"
#include "pidfile.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Reads the specified pidfile and returns the read pid.
 * 0 is returned if either there's no pidfile, it's empty
 * or no pid can be read.
 */
static int
read_pid(const char *pidfile)
{
	FILE *fp = fopen(pidfile, "r");
	if (!fp)
		return 0;
	int pid;
	if (fscanf(fp, "%d", &pid) != 1)
		pid = 0;

	(void) fclose(fp);
	return pid;
}

/*
 * Reads the pid using read_pid and checks whether a process corresponding
 * to that pid exists.  If so the pid is returned, otherwise 0.
 */
int
check_pid(const char *pidfile)
{
	int pid = read_pid(pidfile);

	/* Amazing ! _I_ am already holding the pid file... */
	if (!pid || pid == getpid())
		return 0;

	if (kill(pid, 0) && errno == ESRCH)
		return 0;

	return pid;
}

/*
 * Writes the pid to the specified file.
 * If that fails 0 is returned, otherwise the pid.
 */
int
write_pid(const char *pidfile)
{
	int fd = open(pidfile, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	if (fd < 0) {
		perror_msg("open: %s", pidfile);
		return 0;
	}

	FILE *fp = fdopen(fd, "r+");
	if (!fp) {
		perror_msg("fdopen: %s", pidfile);
		xclose(&fd);
		return 0;
	}

	int pid;
	if (flock(fd, LOCK_EX | LOCK_NB)) {
		if (fscanf(fp, "%d", &pid) != 1)
			pid = 0;
		error_msg("%s: locked by pid %d", pidfile, pid);
		goto err;
	}

	pid = getpid();
	if (fprintf(fp, "%d\n", pid) < 0) {
		perror_msg("fprintf: %s", pidfile);
		goto err;
	}

	if (fclose(fp)) {
		perror_msg("fclose: %s", pidfile);
		return 0;
	}

	return pid;

err:
	(void) fclose(fp);
	return 0;
}

/*
 * Remove the specified file.
 * The result from unlink(2) is returned.
 */
int
remove_pid(const char *pidfile)
{
	if (unlink(pidfile)) {
		perror_msg("unlink: %s", pidfile);
		return -1;
	}
	return 0;
}
