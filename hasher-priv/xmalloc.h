
/*
  Copyright (C) 2002-2019  Dmitry V. Levin <ldv@altlinux.org>

  Dynamic memory allocation with error checking.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __XMALLOC_H__
#define __XMALLOC_H__

extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern void *xrealloc(void *ptr, size_t nmemb, size_t size);
extern void *xgrowarray(void *ptr, size_t *nmemb, size_t size);
extern char *xstrdup(const char *s);
extern char *xasprintf(char **ptr, const char *fmt, ...)
	__attribute__ ((__format__(__printf__, 2, 3)))
	__attribute__ ((__nonnull__(2)));

#endif /* __XMALLOC_H__ */
