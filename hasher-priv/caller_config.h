/*
 * The caller configuration interface for the hasher-privd server program.
 *
 * Copyright (C) 2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CALLER_CONFIG_H
# define HASHER_CALLER_CONFIG_H

# include <sys/types.h>

# define MIN_CHANGE_UID	34
# define MIN_CHANGE_GID	34

typedef struct
{
	unsigned long time_elapsed;
	unsigned long time_idle;
	unsigned long bytes_read;
	unsigned long bytes_written;
} work_limit_t;

typedef struct {
	const char **list;
	size_t len;
	size_t allocated;
	char *buf;
} str_list_t;

void configure_caller(void);
void parse_env(char **env);

extern const char *term;

extern int makedev_console;
extern int use_pty;

extern int share_ipc;
extern int share_network;
extern int share_uts;

extern const char *caller_config_file_name;
extern const char *chroot_prefix_path;
extern const char *const *chroot_prefix_list;

extern const char *change_user1, *change_user2;
extern uid_t change_uid1, change_uid2;
extern gid_t change_gid1, change_gid2;
extern mode_t change_umask;
extern int change_nice;

extern str_list_t allowed_devices;
extern str_list_t allowed_mountpoints;
extern str_list_t requested_mountpoints;

extern work_limit_t wlimit;

#endif /* !HASHER_CALLER_CONFIG_H */
