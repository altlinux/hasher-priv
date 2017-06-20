/*
 * The file level configuration parsing for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "chdir.h"
#include "error_prints.h"
#include "file_config.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_CONFIG_SIZE 16384

void
fread_config_name_value(FILE *fp, const char *fname, name_value_fn_t func)
{
	char buf[BUFSIZ];

	for (unsigned int line = 1; fgets(buf, BUFSIZ, fp); ++line) {
		const char *start, *left;
		char *eq, *right, *end;

		for (start = buf; *start && isspace(*start); ++start)
			;

		if (!*start || '#' == *start)
			continue;

		if (!(eq = strchr(start, '=')))
			error_msg_and_die("%s: syntax error at line %u",
					  fname, line);

		left = start;
		right = eq + 1;

		for (; eq > left; --eq)
			if (!isspace(eq[-1]))
				break;

		if (left == eq)
			error_msg_and_die("%s: syntax error at line %u",
					  fname, line);

		*eq = '\0';
		end = right + strlen(right);

		for (; right < end; ++right)
			if (!isspace(*right))
				break;

		for (; end > right; --end)
			if (!isspace(end[-1]))
				break;

		*end = '\0';
		func(left, right, fname);
	}

	if (ferror(fp))
		perror_msg_and_die("fgets: %s", fname);
}

void
load_config(const char *fname, fread_config_fn_t fread_config_func)
{
	int fd = open(fname, O_RDONLY | O_NOFOLLOW | O_NOCTTY);
	if (fd < 0)
		perror_msg_and_die("open: %s", fname);

	struct stat st;
	if (fstat(fd, &st) < 0)
		perror_msg_and_die("fstat: %s", fname);

	stat_root_ok_validator(&st, fname);

	if (!S_ISREG(st.st_mode))
		error_msg_and_die("%s: not a regular file", fname);

	if (st.st_size > MAX_CONFIG_SIZE)
		error_msg_and_die("%s: file too large: %lu",
				  fname, (unsigned long) st.st_size);

	FILE *fp = fdopen(fd, "r");
	if (!fp)
		perror_msg_and_die("fdopen: %s", fname);

	fread_config_func(fp, fname);

	if (fclose(fp))
		perror_msg_and_die("fclose: %s", fname);
}
