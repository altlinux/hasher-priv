/*
 * The network setup functions for the hasher-privd server program.
 *
 * Copyright (C) 2010  Kirill A. Shutemov <kirill@shutemov.name>
 * Copyright (C) 2011-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "error_prints.h"
#include "fds.h"
#include "net.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <net/if.h>
#include <linux/rtnetlink.h>

void
setup_network(void)
{
	int     rtnetlink_sk;
	struct
	{
		struct nlmsghdr n;
		struct ifinfomsg i;
	} req;

	/* Setup loopback interface */

	rtnetlink_sk = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (rtnetlink_sk < 0)
		perror_msg_and_die("socket");

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));

	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_NEWLINK;
	req.i.ifi_family = AF_UNSPEC;

	req.i.ifi_index = (int) if_nametoindex("lo");
	req.i.ifi_flags = IFF_UP;
	req.i.ifi_change = IFF_UP;

	if (send(rtnetlink_sk, &req, req.n.nlmsg_len, 0) < 0)
		perror_msg_and_die("send");

	if (xclose(&rtnetlink_sk) < 0)
		perror_msg_and_die("close");
}
