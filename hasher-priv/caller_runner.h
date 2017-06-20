/*
 * The job running interface of the hasher-privd server program.
 *
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CALLER_RUNNER_H
# define HASHER_CALLER_RUNNER_H

# include "daemon.h"
# include "caller_job.h"
# include <sys/types.h>

pid_t spawn_job_runner(struct hadaemon *, int conn, struct job *);

#endif /* HASHER_CALLER_RUNNER_H */
