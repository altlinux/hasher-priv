/*
 * Server configuration module for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "chdir.h"
#include "error_prints.h"
#include "file_config.h"
#include "opt_parse.h"
#include "server_config.h"
#include "xmalloc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <grp.h>

char *server_loglevel;
char *server_pidfile;
gid_t server_gid;
unsigned long server_session_timeout;

static char *server_access_group;

static void
set_server_access_gid(void)
{
	if (!server_access_group || !*server_access_group)
		error_msg_and_die("undefined option: access_group");

	struct group *gr = getgrnam(server_access_group);

	if (!gr || !gr->gr_name)
		error_msg_and_die("access_group: %s lookup failure",
				  server_access_group);

	server_gid = gr->gr_gid;
}

static void
set_server_name_value(const char *name, const char *value, const char *fname)
{
	if (!strcasecmp("session_timeout", name)) {
		server_session_timeout = opt_str2ul(name, value, fname);
	} else if (!strcasecmp("loglevel", name)) {
		free(server_loglevel);
		server_loglevel = xstrdup(value);
	} else if (!strcasecmp("pidfile", name)) {
		free(server_pidfile);
		server_pidfile = xstrdup(value);
	} else if (!strcasecmp("access_group", name)) {
		free(server_access_group);
		server_access_group = xstrdup(value);
	} else {
		opt_bad_name(name, fname);
	}
}

static void
fread_server_config(FILE *fp, const char *fname)
{
	fread_config_name_value(fp, fname, set_server_name_value);
}

static void
load_server_config(const char *fname)
{
	load_config(fname, fread_server_config);
}

void
configure_server(void)
{
	safe_chdir("/", stat_root_ok_validator);
	safe_chdir("etc/hasher-priv", stat_root_ok_validator);
	load_server_config("daemon.conf");
	safe_chdir("/", stat_root_ok_validator);
	set_server_access_gid();
}
