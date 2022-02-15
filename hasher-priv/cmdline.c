
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  Command line parser for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include "error_prints.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "priv.h"

static void __attribute__ ((noreturn, format(printf, 1, 2)))
show_usage(const char *fmt, ...)
{
	fprintf(stderr, "%s: ", program_invocation_short_name);

	va_list arg;

	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);

	fprintf(stderr, "\nTry `%s --help' for more information.\n",
		program_invocation_short_name);
	exit(EXIT_FAILURE);
}

static void __attribute__ ((noreturn))
print_help(void)
{
	printf("Privileged helper for the hasher project.\n"
	       "\nUsage: %s [options] <args>\n"
	       "\nValid options are:\n"
	       "  -<number>:\n"
	       "       subconfig identifier;\n"
	       "  --version:\n"
	       "       print program version and exit.\n"
	       "  -h or --help:\n"
	       "       print this help text and exit.\n"
	       "\nValid args are any of:\n\n"
	       "getconf:\n"
	       "       print config file name;\n"
	       "killuid:\n"
	       "       kill all processes of user1 and user2;\n"
	       "getugid1:\n"
	       "       print uid:gid pair for user1;\n"
	       "chrootuid1 <chroot path> <program> [program args]:\n"
	       "       execute program in given chroot with credentials of user1;\n"
	       "getugid2:\n"
	       "       print uid:gid pair for user2;\n"
	       "chrootuid2 <chroot path> <program> [program args]:\n"
	       "       execute program in given chroot with credentials of user2;\n"
	       , program_invocation_short_name);
	exit(EXIT_SUCCESS);
}

static void __attribute__ ((noreturn))
print_version(void)
{
	printf("hasher-priv version %s\n"
	       "\nCopyright (C) 2003-2013  Dmitry V. Levin <ldv@altlinux.org>\n"
	       "\nThis is free software; see the source for copying conditions.\n"
	       "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	       "\nWritten by Dmitry V. Levin <ldv@altlinux.org> et al.\n",
	       PROJECT_VERSION);
	exit(EXIT_SUCCESS);
}

const char *chroot_path;
unsigned caller_num;

static unsigned
get_caller_num(const char *str)
{
	char   *p = 0;
	unsigned long n;

	if (!*str)
		show_usage("-%s: invalid option", str);

	n = strtoul(str, &p, 10);
	if (!p || *p || n > INT_MAX)
		show_usage("-%s: invalid option", str);

	return (unsigned) n;
}

/* Parse command line arguments. */
task_t
parse_cmdline(int argc, const char *argv[], const char ***task_args)
{
	int     ac;
	const char **av;

	if (argc < 2)
		show_usage("insufficient arguments");

	ac = argc - 1;
	av = argv + 1;

	if (av[0][0] == '-')
	{
		/* option */
		if (!strcmp("-h", av[0]) || !strcmp("--help", av[0]))
			print_help();

		if (!strcmp("--version", av[0]))
			print_version();

		caller_num = get_caller_num(&av[0][1]);
		--ac;
		++av;
	}

	if (ac < 1)
		show_usage("insufficient arguments");

	*task_args = NULL;

	if (!strcmp("getconf", av[0]))
	{
		if (ac != 1)
			show_usage("%s: invalid usage", av[0]);
		return TASK_GETCONF;
	} else if (!strcmp("killuid", av[0]))
	{
		if (ac != 1)
			show_usage("%s: invalid usage", av[0]);
		return TASK_KILLUID;
	} else if (!strcmp("getugid1", av[0]))
	{
		if (ac != 1)
			show_usage("%s: invalid usage", av[0]);
		return TASK_GETUGID1;
	} else if (!strcmp("chrootuid1", av[0]))
	{
		if (ac < 3)
			show_usage("%s: invalid usage", av[0]);
		chroot_path = av[1];
		*task_args = av + 2;
		return TASK_CHROOTUID1;
	} else if (!strcmp("getugid2", av[0]))
	{
		if (ac != 1)
			show_usage("%s: invalid usage", av[0]);
		return TASK_GETUGID2;
	} else if (!strcmp("chrootuid2", av[0]))
	{
		if (ac < 3)
			show_usage("%s: invalid usage", av[0]);
		chroot_path = av[1];
		*task_args = av + 2;
		return TASK_CHROOTUID2;
	} else
		show_usage("%s: invalid argument", av[0]);
}
