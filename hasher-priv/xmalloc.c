/*
 * The dynamic memory allocation with error checking
 * for the hasher-priv project.
 *
 * Copyright (C) 2002-2019  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2015-2021 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "error_prints.h"
#include "macros.h"
#include "xmalloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ATTRIBUTE_NORETURN
static void
die_out_of_memory(void)
{
	error_msg_and_die("Out of memory");
}

void *
xmalloc(size_t size)
{
	void *p = malloc(size);

	if (!p)
		die_out_of_memory();

	return p;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);

	if (!p)
		die_out_of_memory();

	return p;
}

#define HALF_SIZE_T	(((size_t) 1) << (sizeof(size_t) * 4))

void *
xreallocarray(void *ptr, size_t nmemb, size_t size)
{
	size_t bytes = nmemb * size;

	if ((nmemb | size) >= HALF_SIZE_T &&
	    size && bytes / size != nmemb)
		die_out_of_memory();

	void *p = realloc(ptr, bytes);

	if (!p)
		die_out_of_memory();

	return p;
}

void *
xgrowarray(void *const ptr, size_t *const nmemb, const size_t memb_size)
{
	/* This is the same value as glibc DEFAULT_MXFAST. */
	enum { DEFAULT_ALLOC_SIZE = 64 * sizeof(long) / 4 };

	size_t grow_memb;

	if (ptr == NULL)
		grow_memb = *nmemb ? 0 :
			(DEFAULT_ALLOC_SIZE + memb_size - 1) / memb_size;
	else
		grow_memb = (*nmemb >> 1) + 1;

	if ((*nmemb + grow_memb) < *nmemb)
		die_out_of_memory();

	*nmemb += grow_memb;

	return xreallocarray(ptr, *nmemb, memb_size);
}

char *
xstrdup(const char *str)
{
	if (!str)
		return NULL;

	char *p = strdup(str);

	if (!p)
		die_out_of_memory();

	return p;
}

char *
xasprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char *res;
	if (vasprintf(&res, fmt, ap) < 0)
		die_out_of_memory();

	va_end(ap);
	return res;
}
