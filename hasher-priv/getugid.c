
/*
  Copyright (C) 2003-2019  Dmitry V. Levin <ldv@altlinux.org>

  The getugid actions for the hasher-priv program.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* Code in this file may be executed with root privileges. */

#include <stdio.h>

#include "priv.h"

int
do_getugid1(void)
{
	printf("%u:%u\n", change_uid1, change_gid1);
	return 0;
}

int
do_getugid2(void)
{
	printf("%u:%u\n", change_uid2, change_gid2);
	return 0;
}
