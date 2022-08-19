/*
 * Purge all SYSV IPC objects for given uid.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with child privileges. */

#include "error_prints.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "priv.h"

union sem_un
{
	int     val;
	struct semid_ds *buf;
	struct seminfo *info;
};

static void
purge_sem(uid_t uid1, uid_t uid2)
{
	int     maxid, id;
	struct seminfo info;
	union sem_un arg;

	arg.info = &info;
	maxid = semctl(0, 0, SEM_INFO, arg);
	if (maxid < 0)
	{
		perror_msg("SEM_INFO");
		return;
	}

	for (id = 0; id <= maxid; ++id)
	{
		int     semid;
		struct semid_ds buf;

		arg.buf = &buf;
		if ((semid = semctl(id, 0, SEM_STAT, arg)) < 0)
			continue;

		if (uid1 != buf.sem_perm.uid &&
		    uid2 != buf.sem_perm.uid)
			continue;

		arg.val = 0;
		(void) semctl(semid, 0, IPC_RMID, arg);
	}
}

union shm_un
{
	struct shmid_ds *buf;
	struct shm_info *info;
};

static void
purge_shm(uid_t uid1, uid_t uid2)
{
	int     maxid, id;
	struct shm_info info;
	union shm_un arg;

	arg.info = &info;
	maxid = shmctl(0, SHM_INFO, arg.buf);
	if (maxid < 0)
	{
		perror_msg("SHM_INFO");
		return;
	}

	for (id = 0; id <= maxid; ++id)
	{
		int     shmid;
		struct shmid_ds buf;

		if ((shmid = shmctl(id, SHM_STAT, &buf)) < 0)
			continue;

		if (uid1 != buf.shm_perm.uid &&
		    uid2 != buf.shm_perm.uid)
			continue;

		(void) shmctl(shmid, IPC_RMID, 0);
	}
}

union msg_un
{
	struct msqid_ds *buf;
	struct msginfo *info;
};

static void
purge_msg(uid_t uid1, uid_t uid2)
{
	int     maxid, id;
	struct msginfo info;
	union msg_un arg;

	arg.info = &info;
	maxid = msgctl(0, MSG_INFO, arg.buf);
	if (maxid < 0)
	{
		perror_msg("MSG_INFO");
		return;
	}

	for (id = 0; id <= maxid; ++id)
	{
		int     msqid;
		struct msqid_ds buf;

		if ((msqid = msgctl(id, MSG_STAT, &buf)) < 0)
			continue;

		if (uid1 != buf.msg_perm.uid &&
		    uid2 != buf.msg_perm.uid)
			continue;

		(void) msgctl(msqid, IPC_RMID, 0);
	}
}

void
purge_ipc(uid_t uid1, uid_t uid2)
{
	purge_sem(uid1, uid2);
	purge_shm(uid1, uid2);
	purge_msg(uid1, uid2);
}
