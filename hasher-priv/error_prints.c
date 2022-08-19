/*
 * The error printing module.
 *
 * Copyright (c) 1999-2022 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#define error_msg error_msg
#define warn_msg warn_msg
#define notice_msg notice_msg
#define info_msg info_msg
#define debug_msg debug_msg

#include "error_prints.h"
#include "logging.h"
#include "macros.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

static enum {
	LOGGING_STANDALONE,
	LOGGING_STDERR,
	LOGGING_SYSLOG
} logging = LOGGING_STANDALONE;

void
init_log_standalone(void)
{
	if (logging == LOGGING_SYSLOG)
		closelog();
	logging = LOGGING_STANDALONE;
}

void
init_log_daemon(int is_foreground)
{
	if (is_foreground) {
		if (logging == LOGGING_SYSLOG)
			closelog();
		logging = LOGGING_STDERR;
	} else {
		openlog(program_invocation_short_name,
			LOG_PID | LOG_NDELAY, LOG_DAEMON);
		logging = LOGGING_SYSLOG;
	}
}

static int log_level = LOG_INFO;

static int
translate_log_level(const char *name)
{
	static const struct {
		const char *name;
		int level;
	} table[] = {
		{ "debug", LOG_DEBUG },
		{ "info", LOG_INFO },
		{ "notice", LOG_NOTICE },
		{ "warning", LOG_WARNING },
		{ "error", LOG_ERR },
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(table); ++i) {
		if (strcasecmp(name, table[i].name) == 0)
			return table[i].level;
	}

	error_msg_and_die("unrecognized log level: %s", name);
}

void
set_log_level(const char *name)
{
	log_level = translate_log_level(name);
}

static void
ATTRIBUTE_FORMAT((__printf__, 2, 0))
vprint_msg(int prio, const char *fmt, va_list p)
{
	char *msg = NULL;
	int saved_errno = errno;

	fflush(NULL);

	/*
	 * We want to print the entire message with a single fprintf to ensure
	 * the message integrity if stderr is shared with other programs.
	 * Thus we use vasprintf + single fprintf.
	 */
	errno = saved_errno;
	if (vasprintf(&msg, fmt, p) >= 0) {
		if (logging == LOGGING_STDERR)
			fprintf(stderr, "<%d>%s\n", prio, msg);
		else
			fprintf(stderr, "%s: %s\n",
				program_invocation_short_name, msg);
		free(msg);
	} else {
		/* malloc in vasprintf failed, try it without malloc */
		if (logging == LOGGING_STDERR)
			fprintf(stderr, "<%d>", prio);
		else
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
}

static void
ATTRIBUTE_FORMAT((__printf__, 2, 0))
vprint_or_log_msg(int prio, const char *fmt, va_list p)
{
	if (prio > log_level)
		return;

	switch (logging) {
		case LOGGING_SYSLOG:
			vsyslog(prio, fmt, p);
			break;
		default:
			vprint_msg(prio, fmt, p);
	}
}

void
error_msg(const char *fmt, ...)
{
	va_list p;
	va_start(p, fmt);
	vprint_or_log_msg(LOG_ERR, fmt, p);
	va_end(p);
}

void
warn_msg(const char *fmt, ...)
{
	va_list p;
	va_start(p, fmt);
	vprint_or_log_msg(LOG_WARNING, fmt, p);
	va_end(p);
}

void
notice_msg(const char *fmt, ...)
{
	va_list p;
	va_start(p, fmt);
	vprint_or_log_msg(LOG_NOTICE, fmt, p);
	va_end(p);
}

void
info_msg(const char *fmt, ...)
{
	va_list p;
	va_start(p, fmt);
	vprint_or_log_msg(LOG_INFO, fmt, p);
	va_end(p);
}

void
debug_msg(const char *fmt, ...)
{
	va_list p;
	va_start(p, fmt);
	vprint_or_log_msg(LOG_DEBUG, fmt, p);
	va_end(p);
}
