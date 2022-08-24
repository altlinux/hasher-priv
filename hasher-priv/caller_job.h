/*
 * The job handling interface of the hasher-privd server program.
 *
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CALLER_JOB_H
# define HASHER_CALLER_JOB_H

# include "communication.h"
# include "daemon.h"
# include <sys/types.h>

struct job {
	job_enum_t type;
	unsigned int mask;
	unsigned int num;
	int chroot_fd;
	int std_fds[3];
	char **argv;
	char **env;
};

int spawn_job_request_handler(struct hadaemon *, int conn);
int wait_job(const struct job *, pid_t);
void deallocate_job_resources(struct job *);

#endif /* HASHER_CALLER_JOB_H */
