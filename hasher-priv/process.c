/*
 * waitpid_retry function for the hasher-priv project.
 *
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "process.h"

pid_t
waitpid_retry(pid_t pid, int *wstatus, int options)
{
	return (pid_t) TEMP_FAILURE_RETRY(waitpid(pid, wstatus, options));
}
