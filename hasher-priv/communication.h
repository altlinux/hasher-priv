/*
 * This file offers some helper functions for client-server communication
 * for the hasher-priv project.
 *
 * Copyright (C) 2019  Alexey Gladkov <legion@altlinux.org>
 * Copyright (C) 2022  Arseny Maslennikov <arseny@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_COMMUNICATION_H_
#define HASHER_COMMUNICATION_H_

#define MAIN_SOCKET_BASE_NAME "daemon"

typedef enum {
	/* not a command */
	CMD_NONE = 0,

	/* main service commands */
	CMD_OPEN_SESSION	= 1U << 0,

	/* job service commands */
	CMD_JOB_TYPE		= 1U << 1,
	CMD_JOB_FDS		= 1U << 2,
	CMD_JOB_ARGUMENTS	= 1U << 3,
	CMD_JOB_ENVIRON		= 1U << 4,
	CMD_JOB_CHROOT_FD	= 1U << 5,
	CMD_JOB_PERSONALITY	= 1U << 6,
	CMD_JOB_RUN		= 1U << 7,
} cmd_enum_t;

enum {
	CMD_STATUS_DONE = 0,
	CMD_STATUS_FAILED = -1,
};

typedef enum {
	JOB_NONE = 0,
	JOB_GETCONF,
	JOB_KILLUID,
	JOB_GETUGID1,
	JOB_CHROOTUID1,
	JOB_GETUGID2,
	JOB_CHROOTUID2,
} job_enum_t;

typedef struct {
	cmd_enum_t   type;
	unsigned int len;
} cmd_header_t;

typedef struct {
        int rc;
        unsigned int len;
} srv_cmd_resp_t;

#endif /* HASHER_COMMUNICATION_H_ */
