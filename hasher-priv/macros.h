/*
 * The useful macros for the hasher-priv project.
 *
 * Copyright (c) 2001-2022 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef HASHER_MACROS_H
# define HASHER_MACROS_H

# include "cc_compat.h"

/*
 * Evaluates to:
 * a syntax error, if the argument is 0;
 * 0, otherwise.
 */
# define FAIL_BUILD_ON_ZERO(e_)	(sizeof(int[-1 + 2 * !!(e_)]) * 0)

/*
 * Evaluates to:
 * 1, if the given type is known to be a non-array type;
 * 0, otherwise.
 */
# define IS_NOT_ARRAY(a_)	IS_SAME_TYPE((a_), &(a_)[0])

/*
 * Evaluates to:
 * a syntax error, if the argument is not an array;
 * 0, otherwise.
 */
# define MUST_BE_ARRAY(a_)	FAIL_BUILD_ON_ZERO(!IS_NOT_ARRAY(a_))

/* Evaluates to the number of elements in the specified array.  */
# define ARRAY_SIZE(a_)	(sizeof(a_) / sizeof((a_)[0]) + MUST_BE_ARRAY(a_))

#endif /* !HASHER_MACROS_H */
