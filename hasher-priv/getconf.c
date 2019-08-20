
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The getconf actions for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include <stdio.h>

#include "priv.h"

int
do_getconf(void)
{
	if (caller_num)
		printf("%s/%s:%u\n", "/etc/hasher-priv/user.d",
		       caller_user, caller_num);
	else
		printf("%s/%s\n", "/etc/hasher-priv/user.d",
		       caller_user);
	return 0;
}
