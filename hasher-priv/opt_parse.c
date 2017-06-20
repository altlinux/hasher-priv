/*
 * The generic option parsing module for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include "opt_parse.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

void
opt_bad_name(const char *name, const char *fname)
{
	error_msg_and_die("%s: unrecognized option: %s", fname, name);
}

void
opt_bad_value(const char *name, const char *value, const char *fname)
{
	error_msg_and_die("%s: invalid value for \"%s\" option: %s",
			  fname, name, value);
}

unsigned long
opt_str2ul(const char *name, const char *value, const char *fname)
{
	if (!*value)
		opt_bad_value(name, value, fname);

	errno = 0;
	char *p = 0;
	unsigned long long n = strtoull(value, &p, 10);
	if (!p || *p || n > ULONG_MAX || (n == ULLONG_MAX && errno == ERANGE))
		opt_bad_value(name, value, fname);

	return (unsigned long) n;
}

int
opt_str2bool(const char *name, const char *value, const char *fname)
{
	if (value[0] == '\0' || !strcasecmp(value, "no")
	    || !strcasecmp(value, "false") || !strcasecmp(value, "0"))
		return 0;
	if (!strcasecmp(value, "yes") || !strcasecmp(value, "true")
	    || !strcasecmp(value, "1"))
		return 1;

	error_msg_and_die("%s: invalid value \"%s\" for \"%s\" option",
			  fname, value, name);
	return 0;
}
