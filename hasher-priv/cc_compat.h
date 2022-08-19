/*
 * The C language compatibility interface for the hasher-priv project.
 *
 * Copyright (c) 2015 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2015-2022 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef HASHER_CC_COMPAT_H
# define HASHER_CC_COMPAT_H

# if defined __GNUC__ && defined __GNUC_MINOR__
#  define GNUC_PREREQ(maj, min)	\
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
# else
#  define GNUC_PREREQ(maj, min)	0
# endif

# if defined __clang__ && defined __clang_major__ && defined __clang_minor__
#  define CLANG_PREREQ(maj, min)	\
	((__clang_major__ << 16) + __clang_minor__ >= ((maj) << 16) + (min))
# else
#  define CLANG_PREREQ(maj, min)	0
# endif

# if !(GNUC_PREREQ(2, 0) || CLANG_PREREQ(1, 0))
#  define __attribute__(x)	/* empty */
# endif

# if GNUC_PREREQ(2, 5)
#  define ATTRIBUTE_NORETURN	__attribute__((__noreturn__))
# else
#  define ATTRIBUTE_NORETURN	/* empty */
# endif

# if GNUC_PREREQ(2, 7)
#  define ATTRIBUTE_FORMAT(args)	__attribute__((__format__ args))
# else
#  define ATTRIBUTE_FORMAT(args)	/* empty */
# endif

# if GNUC_PREREQ(2, 8)
#  define ATTRIBUTE_UNUSED	__attribute__((__unused__))
# else
#  define ATTRIBUTE_UNUSED	/* empty */
# endif

/*
 * Evaluates to:
 * 1, if the given two types are known to be the same;
 * 0, otherwise.
 */
# if GNUC_PREREQ(3, 0)
#  define IS_SAME_TYPE(x_, y_)						\
	__builtin_types_compatible_p(__typeof__(x_), __typeof__(y_))
# else
/* Cannot tell whether these types are the same.  */
#  define IS_SAME_TYPE(x_, y_)	0
# endif

# if GNUC_PREREQ(3, 0)
#  define ATTRIBUTE_MALLOC	__attribute__((__malloc__))
# else
#  define ATTRIBUTE_MALLOC	/* empty */
# endif

# if GNUC_PREREQ(3, 1)
#  define ATTRIBUTE_NOINLINE	__attribute__((__noinline__))
# else
#  define ATTRIBUTE_NOINLINE	/* empty */
# endif

# if GNUC_PREREQ(3, 3)
#  define ATTRIBUTE_NONNULL(args)	__attribute__((__nonnull__ args))
# else
#  define ATTRIBUTE_NONNULL(args)	/* empty */
# endif

# if GNUC_PREREQ(4, 0)
#  define ATTRIBUTE_SENTINEL	__attribute__((__sentinel__))
# else
#  define ATTRIBUTE_SENTINEL	/* empty */
# endif

# if GNUC_PREREQ(4, 1)
#  define ALIGNOF(t_)	__alignof__(t_)
# else
#  define ALIGNOF(t_)	(sizeof(struct { char x_; t_ y_; }) - sizeof(t_))
# endif

# if GNUC_PREREQ(4, 3)
#  define ATTRIBUTE_ALLOC_SIZE(args)	__attribute__((__alloc_size__ args))
# else
#  define ATTRIBUTE_ALLOC_SIZE(args)	/* empty */
# endif

# if GNUC_PREREQ(7, 0)
#  define ATTRIBUTE_FALLTHROUGH	__attribute__((__fallthrough__))
# else
#  define ATTRIBUTE_FALLTHROUGH	((void) 0)
# endif

#endif /* !HASHER_CC_COMPAT_H */
