/*
 * Caller configuration module for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "error_prints.h"
#include "file_config.h"
#include "opt_parse.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>

#include "priv.h"
#include "xmalloc.h"

const char *caller_config_file_name;
const char *const *chroot_prefix_list;
const char *chroot_prefix_path;
const char *change_user1, *change_user2;
const char *term;
const char *x11_display, *x11_key;
str_list_t allowed_devices;
str_list_t allowed_mountpoints;
str_list_t requested_mountpoints;
uid_t   change_uid1, change_uid2;
gid_t   change_gid1, change_gid2;
mode_t  change_umask = 022;
int change_nice = 8;
int     makedev_console;
int     use_pty;
size_t  x11_data_len;
int share_caller_network = 0;
int share_ipc = -1;
int share_network = -1;
int share_uts = -1;
change_rlimit_t change_rlimit[] = {

/* Per-process CPU limit, in seconds.  */
	{"cpu", RLIMIT_CPU, 0, 0},

/* Largest file that can be created, in bytes.  */
	{"fsize", RLIMIT_FSIZE, 0, 0},

/* Maximum size of data segment, in bytes.  */
	{"data", RLIMIT_DATA, 0, 0},

/* Maximum size of stack segment, in bytes.  */
	{"stack", RLIMIT_STACK, 0, 0},

/* Largest core file that can be created, in bytes.  */
	{"core", RLIMIT_CORE, 0, 0},

/* Largest resident set size, in bytes.  */
	{"rss", RLIMIT_RSS, 0, 0},

/* Number of processes.  */
	{"nproc", RLIMIT_NPROC, 0, 0},

/* Number of open files.  */
	{"nofile", RLIMIT_NOFILE, 0, 0},

/* Locked-in-memory address space.  */
	{"memlock", RLIMIT_MEMLOCK, 0, 0},

/* Address space limit.  */
	{"as", RLIMIT_AS, 0, 0},

/* Maximum number of file locks.  */
	{"locks", RLIMIT_LOCKS, 0, 0},

#ifdef RLIMIT_SIGPENDING
/* Maximum number of pending signals.  */
	{"sigpending", RLIMIT_SIGPENDING, 0, 0},
#endif

#ifdef RLIMIT_MSGQUEUE
/* Maximum number of bytes in POSIX mqueues.  */
	{"msgqueue", RLIMIT_MSGQUEUE, 0, 0},
#endif

#ifdef RLIMIT_NICE
/* Maximum nice priority allowed to raise to.  */
	{"nice", RLIMIT_NICE, 0, 0},
#endif

#ifdef RLIMIT_RTPRIO
/* Maximum realtime priority.  */
	{"rtprio", RLIMIT_RTPRIO, 0, 0},
#endif

/* End of limits.  */
	{0, 0, 0, 0}
};

work_limit_t wlimit;

static  mode_t
str2umask(const char *name, const char *value, const char *filename)
{
	char   *p = 0;
	unsigned long n;

	if (!*value)
		opt_bad_value(name, value, filename);

	n = strtoul(value, &p, 8);
	if (!p || *p || n > 0777)
		opt_bad_value(name, value, filename);

	return (mode_t) n;
}

static int
str2nice(const char *name, const char *value, const char *filename)
{
	char   *p = 0;
	unsigned long n;

	if (!*value)
		opt_bad_value(name, value, filename);

	n = strtoul(value, &p, 10);
	if (!p || *p || n > 19)
		opt_bad_value(name, value, filename);

	return (int) n;
}

static  rlim_t
str2rlim(const char *name, const char *value, const char *filename)
{
	char   *p = 0;
	unsigned long long n;

	if (!*value)
		opt_bad_value(name, value, filename);

	if (!strcasecmp(value, "inf"))
		return RLIM_INFINITY;

	errno = 0;
	n = strtoull(value, &p, 10);
	if (!p || *p || n > ULONG_MAX || (n == ULLONG_MAX && errno == ERANGE))
		opt_bad_value(name, value, filename);

	return n;
}

static void
set_rlim(const char *name, const char *value, int hard,
	 const char *optname, const char *filename)
{
	change_rlimit_t *p;

	for (p = change_rlimit; p->name; ++p)
		if (!strcasecmp(name, p->name))
		{
			rlim_t **limit = hard ? &(p->hard) : &(p->soft);

			free(*limit);
			*limit = xmalloc(sizeof(**limit));
			**limit = str2rlim(optname, value, filename);
			return;
		}

	opt_bad_name(optname, filename);
}

static void
parse_rlim(const char *name, const char *value, const char *optname,
	   const char *filename)
{
	const char hard_prefix[] = "hard_";
	const char soft_prefix[] = "soft_";

	if (!strncasecmp(hard_prefix, name, sizeof(hard_prefix) - 1))
		set_rlim(name + sizeof(hard_prefix) - 1, value, 1,
			 optname, filename);
	else if (!strncasecmp(soft_prefix, name, sizeof(soft_prefix) - 1))
		set_rlim(name + sizeof(soft_prefix) - 1, value, 0,
			 optname, filename);
	else
		opt_bad_name(optname, filename);
}

static void
modify_wlim(unsigned long *pval, const char *value,
	    const char *optname, const char *filename, int is_system)
{
	unsigned long val = opt_str2ul(optname, value, filename);

	if (is_system || *pval == 0 || (val > 0 && val < *pval))
		*pval = val;
}

static void
parse_wlim(const char *name, const char *value,
	   const char *optname, const char *filename)
{
	unsigned long *pval;

	if (!strcasecmp("time_elapsed", name))
		pval = &wlimit.time_elapsed;
	else if (!strcasecmp("time_idle", name))
		pval = &wlimit.time_idle;
	else if (!strcasecmp("bytes_written", name))
		pval = &wlimit.bytes_written;
	else
		opt_bad_name(optname, filename);

	modify_wlim(pval, value, optname, filename, 1);
}

static int
strp_cmp(const void *a, const void *b)
{
	return strcmp(* (char *const *) a, * (char *const *) b);
}

static void
parse_str_list(const char *value, str_list_t *s)
{
	if (s->len) {
		memset(s->list, 0, s->len * sizeof(*s->list));
		s->len = 0;
	}
	if (s->buf)
		free(s->buf);

	s->buf = xstrdup(value);
	char *ctx = 0;
	char *item = strtok_r(s->buf, " \t,", &ctx);

	for (; item; item = strtok_r(0, " \t,", &ctx)) {
		if (s->len >= s->allocated)
			s->list = xgrowarray(s->list, &s->allocated,
					     sizeof(*s->list));
		s->list[s->len++] = item;
	}

	if (s->len)
		qsort(s->list, s->len, sizeof(*s->list), strp_cmp);
	/* clear duplicate entries */
	for (size_t i = 1; i < s->len; ++i)
		if (!strcmp(s->list[i - 1], s->list[i]))
			s->list[i - 1] = 0;
}

static char *
parse_prefix(const char *name, const char *value, const char *filename)
{
	char   *prefix = xstrdup(strcmp(value, "~") ? value : caller_home);
	size_t  n = strlen(prefix);

	/* Strip trailing slashes. */
	for (; n > 0; --n)
	{
		if (prefix[n - 1] == '/')
			prefix[n - 1] = '\0';
		else
			break;
	}

	if (prefix[0] == '\0' || prefix[0] == '/')
		return prefix;

	error_msg_and_die("%s: invalid value \"%s\" for \"%s\" option",
			  filename, value, name);
	return 0;
}

static void
parse_prefix_list(const char *name, const char *value, const char *filename)
{
	char   *paths = xstrdup(value);
	char   *path = strtok(paths, ":");
	const char **list = 0;
	size_t  size = 0;

	for (; path; path = strtok(0, ":"))
	{
		path = parse_prefix(name, path, filename);
		list = xreallocarray(list, size + 2, sizeof(*list));
		list[size++] = path;
	}

	free(paths);
	if (size)
		list[size] = 0;

	free((char *) chroot_prefix_path);
	chroot_prefix_path = xstrdup(value);

	if (chroot_prefix_list)
	{
		char  **prefix = (char **) chroot_prefix_list;

		for (; prefix && *prefix; ++prefix)
		{
			free(*prefix);
			*prefix = 0;
		}
		free((char **) chroot_prefix_list);
	}
	chroot_prefix_list = list;
}

static void
set_caller_name_value(const char *name, const char *value, const char *filename)
{
	const char rlim_prefix[] = "rlimit_";
	const char wlim_prefix[] = "wlimit_";

	if (!strcasecmp("user1", name))
	{
		free((char *) change_user1);
		change_user1 = xstrdup(value);
	} else if (!strcasecmp("user2", name))
	{
		free((char *) change_user2);
		change_user2 = xstrdup(value);
	} else if (!strcasecmp("prefix", name))
		parse_prefix_list(name, value, filename);
	else if (!strcasecmp("umask", name))
		change_umask = str2umask(name, value, filename);
	else if (!strcasecmp("nice", name))
		change_nice = str2nice(name, value, filename);
	else if (!strcasecmp("allowed_devices", name))
		parse_str_list(value, &allowed_devices);
	else if (!strcasecmp("allowed_mountpoints", name))
		parse_str_list(value, &allowed_mountpoints);
	else if (!strcasecmp("allow_ttydev", name))
		(void) opt_str2bool(name, value, filename);	/* obsolete */
	else if (!strncasecmp(rlim_prefix, name, sizeof(rlim_prefix) - 1))
		parse_rlim(name + sizeof(rlim_prefix) - 1, value, name,
			   filename);
	else if (!strncasecmp(wlim_prefix, name, sizeof(wlim_prefix) - 1))
		parse_wlim(name + sizeof(wlim_prefix) - 1, value, name,
			   filename);
	else
		opt_bad_name(name, filename);
}

static void
fread_caller_config(FILE *fp, const char *fname)
{
	fread_config_name_value(fp, fname, set_caller_name_value);
}

static void
load_caller_config(const char *fname)
{
	load_config(fname, fread_caller_config);
	caller_config_file_name = fname;
}

static void
check_user(const char *user_name, uid_t * user_uid, gid_t * user_gid,
	   const char *name)
{
	struct passwd *pw;

	if (!user_name || !*user_name)
		error_msg_and_die("undefined: %s", name);

	pw = getpwnam(user_name);

	if (!pw || !pw->pw_name)
		error_msg_and_die("%s: %s lookup failure",
				  name, user_name);

	if (strcmp(user_name, pw->pw_name))
		error_msg_and_die("%s: %s: name mismatch",
				  name, user_name);

	if (pw->pw_uid < MIN_CHANGE_UID)
		error_msg_and_die("%s: %s: invalid uid: %u",
				  name, user_name, pw->pw_uid);
	*user_uid = pw->pw_uid;

	if (pw->pw_gid < MIN_CHANGE_GID)
		error_msg_and_die("%s: %s: invalid gid: %u",
				  name, user_name, pw->pw_gid);
	*user_gid = pw->pw_gid;

	if (!strcmp(caller_user, user_name))
		error_msg_and_die("%s: %s: name coincides with caller",
				  name, user_name);

	if (caller_uid == *user_uid)
		error_msg_and_die("%s: %s: uid coincides with caller",
				  name, user_name);

	if (caller_gid == *user_gid)
		error_msg_and_die("%s: %s: gid coincides with caller",
				  name, user_name);
}

void
configure_caller(void)
{
	safe_chdir("/", stat_root_ok_validator);
	safe_chdir("etc/hasher-priv", stat_root_ok_validator);
	load_caller_config("system");

	safe_chdir("user.d", stat_root_ok_validator);
	load_caller_config(caller_user);

	if (caller_num) {
		/* Discard user1 and user2. */
		free((void *) change_user1);
		change_user1 = 0;

		free((void *) change_user2);
		change_user2 = 0;

		load_caller_config(xasprintf("%s:%u", caller_user, caller_num));
	}

	safe_chdir("/", stat_root_ok_validator);

	check_user(change_user1, &change_uid1, &change_gid1, "user1");
	check_user(change_user2, &change_uid2, &change_gid2, "user2");

	if (!strcmp(change_user1, change_user2))
		error_msg_and_die("user1 coincides with user2");

	if (change_uid1 == change_uid2)
		error_msg_and_die("uid of user1 coincides with uid of user2");

	if (change_gid1 == change_gid2)
		error_msg_and_die("gid of user1 coincides with gid of user2");
}

void
parse_env(void)
{
	const char *e;

	if ((e = getenv("wlimit_time_elapsed")) && *e)
		modify_wlim(&wlimit.time_elapsed, e, "wlimit_time_elapsed",
			    "environment", 0);

	if ((e = getenv("wlimit_time_idle")) && *e)
		modify_wlim(&wlimit.time_idle, e, "wlimit_time_idle",
			    "environment", 0);

	if ((e = getenv("wlimit_bytes_written")) && *e)
		modify_wlim(&wlimit.bytes_written, e, "wlimit_bytes_written",
			    "environment", 0);

	if ((e = getenv("makedev_console")))
		makedev_console = opt_str2bool("makedev_console", e, "environment");

	if ((e = getenv("use_pty")))
		use_pty = opt_str2bool("use_pty", e, "environment");

	if (use_pty && (e = getenv("TERM")) && *e)
		term = xstrdup(e);

	if ((e = getenv("XAUTH_DISPLAY")) && *e)
		x11_display = xstrdup(e);

	if ((e = getenv("XAUTH_KEY")) && *e)
		x11_key = xstrdup(e);

	if (x11_display && x11_key)
	{
		x11_data_len = strlen(x11_key);
		if (x11_data_len & 1)
		{
			error_msg("Invalid X11 authentication data");
			x11_data_len = 0;
		} else
		{
			x11_data_len /= 2;
			if (x11_parse_display() != EXIT_SUCCESS)
				x11_data_len = 0;
		}
	}

	if (x11_data_len == 0)
		x11_drop_display();

	if ((e = getenv("share_ipc")))
		share_ipc = opt_str2bool("share_ipc", e, "environment");

	if ((e = getenv("share_network")))
		share_network = opt_str2bool("share_network", e, "environment");

	if ((e = getenv("share_uts")))
		share_uts = opt_str2bool("share_uts", e, "environment");

	if ((e = getenv("requested_mountpoints")))
		parse_str_list(e, &requested_mountpoints);
}
