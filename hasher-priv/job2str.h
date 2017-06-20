/*
 * The job2str interface for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_JOB2STR_H
# define HASHER_JOB2STR_H

# include "communication.h"

const char *job2str(job_enum_t type);

#endif /* !HASHER_JOB2STR_H */
