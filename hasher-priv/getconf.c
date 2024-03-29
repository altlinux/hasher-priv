/*
 * The getconf actions for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with root privileges. */

#include "caller_config.h"
#include "executors.h"
#include <stdio.h>

int
do_getconf(void)
{
	printf("%s/%s\n", "/etc/hasher-priv/user.d", caller_config_file_name);
	return 0;
}
