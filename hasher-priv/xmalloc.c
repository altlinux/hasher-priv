
/*
  Copyright (C) 2002-2007  Dmitry V. Levin <ldv@altlinux.org>

  Dynamic memory allocation with error checking.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <limits.h>

#include "xmalloc.h"

void   *
xmalloc(size_t size)
{
	void   *r = malloc(size);

	if (!r)
		error(EXIT_FAILURE, errno, "malloc: allocating %lu bytes",
		      (unsigned long) size);
	return r;
}

void   *
xcalloc(size_t nmemb, size_t size)
{
	void   *r = calloc(nmemb, size);

	if (!r)
		error(EXIT_FAILURE, errno, "calloc: allocating %lu*%lu bytes",
		      (unsigned long) nmemb, (unsigned long) size);
	return r;
}

#define HALF_SIZE_T	(((size_t) 1) << (sizeof(size_t) * 4))

void   *
xrealloc(void *ptr, size_t nmemb, size_t elem_size)
{
	size_t bytes = nmemb * elem_size;

	if ((nmemb | elem_size) >= HALF_SIZE_T &&
	    elem_size && bytes / elem_size != nmemb)
		error(EXIT_FAILURE, 0, "realloc: nmemb*size too big");

	void *r = realloc(ptr, bytes);

	if (!r)
		error(EXIT_FAILURE, errno, "realloc: allocating %lu*%lu bytes",
		      (unsigned long) nmemb, (unsigned long) elem_size);

	return r;
}

void *
xgrowarray(void *const ptr, size_t *const nmemb, const size_t memb_size)
{
	/* this is the same value as glibc DEFAULT_MXFAST */
	enum { DEFAULT_ALLOC_SIZE = 64 * sizeof(long) / 4 };

	size_t grow_memb;

	if (ptr == NULL)
		grow_memb = *nmemb ? 0 :
			(DEFAULT_ALLOC_SIZE + memb_size - 1) / memb_size;
	else
		grow_memb = (*nmemb >> 1) + 1;

	if ((*nmemb + grow_memb) < *nmemb)
		error(EXIT_FAILURE, 0, "xgrowarray: array too big");

	*nmemb += grow_memb;

	return xrealloc(ptr, *nmemb, memb_size);
}

char   *
xstrdup(const char *s)
{
	size_t  len = strlen(s);
	char   *r = xmalloc(len + 1);

	memcpy(r, s, len + 1);
	return r;
}

char   *
xasprintf(char **ptr, const char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	if (vasprintf(ptr, fmt, arg) < 0)
		error(EXIT_FAILURE, errno, "vasprintf");
	va_end(arg);

	return *ptr;
}
