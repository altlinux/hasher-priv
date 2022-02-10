/*
 * Copyright (c) 1999-2022 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#define error_msg error_msg
#include "error_prints.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void
error_msg(const char *fmt, ...)
{
	va_list p;
	char *msg = NULL;
	int saved_errno = errno;

	va_start(p, fmt);

	fflush(NULL);

	/*
	 * We want to print the entire message with a single fprintf to ensure
	 * the message integrity if stderr is shared with other programs.
	 * Thus we use vasprintf + single fprintf.
	 */
	errno = saved_errno;
	if (vasprintf(&msg, fmt, p) >= 0) {
		fprintf(stderr, "%s: %s\n",
			program_invocation_short_name, msg);
		free(msg);
	} else {
		/* malloc in vasprintf failed, try it without malloc */
		fprintf(stderr, "%s: ", program_invocation_short_name);
		errno = saved_errno;
		vfprintf(stderr, fmt, p);
		putc('\n', stderr);
	}
	/*
	 * We don't switch stderr to buffered, thus fprintf(stderr)
	 * always flushes its output and this is not necessary:
	 * fflush(stderr);
	 */

	va_end(p);
}
