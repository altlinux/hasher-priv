
/*
  $Id$
  Copyright (C) 2002  Dmitry V. Levin <ldv@altlinux.org>

  Dynamic memory allocation with error checking.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

#include "xmalloc.h"

void   *
xmalloc (size_t size)
{
	void   *r = malloc (size);

	if (!r)
		error (EXIT_FAILURE, errno, "malloc");
	return r;
}

void   *
xrealloc (void *ptr, size_t size)
{
	void   *r = realloc (ptr, size);

	if (!r)
		error (EXIT_FAILURE, errno, "realloc");
	return r;
}

char   *
xstrdup (const char *s)
{
	char   *r = strdup (s);

	if (!r)
		error (EXIT_FAILURE, errno, "strdup");
	return r;
}

char   *
xasprintf (char **ptr, const char *fmt, ...)
{
	va_list arg;

	va_start (arg, fmt);
	if (vasprintf (ptr, fmt, arg) < 0)
		error (EXIT_FAILURE, errno, "vasprintf");
	va_end (arg);

	return *ptr;
}
