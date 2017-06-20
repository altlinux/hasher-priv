/*
 * The job2str function for the hasher-privd server program.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "job2str.h"
#include "macros.h"

static const struct {
	const char *name;
	job_enum_t value;
} jobmap[] = {
	{ "none",        JOB_NONE },
	{ "getconf",     JOB_GETCONF },
	{ "killuid",     JOB_KILLUID },
	{ "getugid1",    JOB_GETUGID1 },
	{ "getugid2",    JOB_GETUGID2 },
	{ "chrootuid1",  JOB_CHROOTUID1 },
	{ "chrootuid2",  JOB_CHROOTUID2 }
};

static const unsigned int jobmap_size = ARRAY_SIZE(jobmap);

const char *
job2str(job_enum_t type)
{
	for (unsigned int i = 0; i < jobmap_size; ++i) {
		if (jobmap[i].value == type)
			return jobmap[i].name;
	}
	return 0;
}
