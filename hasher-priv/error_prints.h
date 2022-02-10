/*
 * Copyright (c) 2001-2022 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef HASHER_ERROR_PRINTS_H
# define HASHER_ERROR_PRINTS_H

# include "cc_compat.h"

void die(void) ATTRIBUTE_NORETURN;

void error_msg(const char *fmt, ...)
	ATTRIBUTE_FORMAT((printf, 1, 2));

/* A simple wrapper for providing function name in error messages. */
# ifndef error_msg
#  define error_msg(fmt_, ...)				\
	error_msg("%s: " fmt_, __func__, ##__VA_ARGS__)
# endif

# define error_msg_and_die(fmt_, ...)			\
	do {						\
		error_msg(fmt_,	##__VA_ARGS__);		\
		die();					\
	} while (0)

# define perror_msg(fmt_, ...)				\
	error_msg(fmt_ ": %m", ##__VA_ARGS__)

# define perror_msg_and_die(fmt_, ...)			\
	error_msg_and_die(fmt_ ": %m", ##__VA_ARGS__)

#endif /* !HASHER_ERROR_PRINTS_H */
