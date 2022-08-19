/*
 * The dynamic memory allocation interface.
 *
 * Copyright (c) 2001-2021 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef HASHER_XMALLOC_H
# define HASHER_XMALLOC_H

# include "cc_compat.h"
# include <stddef.h>

/** Allocate memory, die if the allocation has failed. */
void *xmalloc(size_t size) ATTRIBUTE_MALLOC ATTRIBUTE_ALLOC_SIZE((1));

/**
 * Allocate an array and zero it out (similar to calloc), die if the allocation
 * has failed or if the product of nmemb and size is too big.
 */
void *xcalloc(size_t nmemb, size_t size)
	ATTRIBUTE_MALLOC ATTRIBUTE_ALLOC_SIZE((1, 2));

/** Wrapper for xcalloc(1, size) with xmalloc-like interface. */
ATTRIBUTE_MALLOC ATTRIBUTE_ALLOC_SIZE((1))
static inline void *
xzalloc(size_t size)
{
	return xcalloc(1, size);
}

/**
 * Reallocate memory for the array, die if the allocation has failed or
 * if the product of nmemb and size is too big.
 */
void *xreallocarray(void *ptr, size_t nmemb, size_t size)
	ATTRIBUTE_ALLOC_SIZE((2, 3));

/** Wrapper for xreallocarray(ptr, 1, size) with realloc-like interface. */
ATTRIBUTE_ALLOC_SIZE((2))
static inline void *
xrealloc(void *ptr, size_t size)
{
        return xreallocarray(ptr, 1, size);
}

/**
 * Utility function for the simplification of managing various dynamic arrays.
 * Knows better how to resize arrays. Dies if there's no enough memory.
 *
 * @param[in]      ptr       Pointer to the array to be resized. If ptr is NULL,
 *                           new array is allocated.
 * @param[in, out] nmemb     Pointer to the current member count. If ptr is
 *                           NULL, it specifies number of members in the newly
 *                           created array. If ptr is NULL and nmemb is 0,
 *                           number of members in the new array is decided by
 *                           the function. Member count is updated by the
 *                           function to the new value.
 * @param[in]      memb_size Size of array member in bytes.
 * @return                   Pointer to the (re)allocated array.
 */
void *xgrowarray(void *ptr, size_t *nmemb, size_t memb_size);

/*
 * Note that the following function returns NULL when NULL is specified
 * and not when allocation is failed, since, as the "x" prefix implies,
 * the allocation failure leads to program termination, so we may re-purpose
 * this return value and simplify the idiom "str ? xstrdup(str) : NULL".
 */
char *xstrdup(const char *str) ATTRIBUTE_MALLOC;

/**
 * Analogous to asprintf, die in case of an error.
 */
char *xasprintf(const char *fmt, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2)) ATTRIBUTE_MALLOC;

#endif /* !HASHER_XMALLOC_H */
