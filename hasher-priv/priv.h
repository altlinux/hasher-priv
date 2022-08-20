/*
 * The main include header for the hasher-priv project.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PKG_BUILD_PRIV_H
#define PKG_BUILD_PRIV_H

#include "cc_compat.h"
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>

#define	MIN_CHANGE_UID	34
#define	MIN_CHANGE_GID	34

typedef enum
{
	JOB_NONE = 0,
	JOB_GETCONF,
	JOB_KILLUID,
	JOB_GETUGID1,
	JOB_CHROOTUID1,
	JOB_GETUGID2,
	JOB_CHROOTUID2,
} job_enum_t;

typedef struct
{
	const char *name;
	int     resource;
	rlim_t *hard, *soft;
} change_rlimit_t;

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

typedef void (*VALIDATE_FPTR)(struct stat *, const char *);

void    xwrite_all(int fd, const char *buffer, size_t count);
int     init_tty(void);
void    restore_tty(void);
int     tty_copy_winsize(int master_fd, int slave_fd);
int     open_pty(int *slave_fd, int chrooted, int verbose_error);
job_enum_t parse_cmdline(int ac, const char *av[], const char ***job_args);
void    init_caller_data(uid_t uid, gid_t gid);
void    chdiruid(const char *path, VALIDATE_FPTR validator);
void    handle_child(const char *const *argv, const char *const *env,
		     int pty_fd, int pipe_out, int pipe_err, int ctl_fd)
	ATTRIBUTE_NORETURN;
int     handle_parent(pid_t pid, int pty_fd, int pipe_out, int pipe_err, int ctl_fd);
void    safe_chdir(const char *name, VALIDATE_FPTR validator);
void    stat_caller_ok_validator(struct stat *st, const char *name);
void    stat_root_ok_validator(struct stat *st, const char *name);
void    fd_send(int ctl, int *fds, unsigned int n_fds,
		const char *data, size_t data_len) ATTRIBUTE_NONNULL((2));
int     fd_recv(int ctl, int *fds, unsigned int n_fds,
		char *data, size_t data_len) ATTRIBUTE_NONNULL((2));
int     unix_accept(int fd);
int     log_listen(void);

void    setup_devices(const char **vec, size_t len);
void	setup_mountpoints(void);
void	setup_network(void);
void	unshare_ipc(void);
void	unshare_mount(void);
void	unshare_network(void);
void	unshare_uts(void);

int     do_getconf(void);
int     do_killuid(void);
int     do_getugid1(void);
int     do_chrootuid1(const char *const *argv);
int     do_getugid2(void);
int     do_chrootuid2(const char *const *argv);

extern const char *chroot_path;

extern str_list_t allowed_devices;
extern str_list_t allowed_mountpoints;
extern str_list_t requested_mountpoints;

extern const char *term;

extern int makedev_console;
extern int use_pty;
extern int share_caller_network;
extern int share_ipc;
extern int share_network;
extern int share_uts;

extern int dev_pts_mounted;

extern const char *const *chroot_prefix_list;
extern const char *chroot_prefix_path;
extern const char *caller_config_file_name;
extern const char *caller_user, *caller_home;
extern uid_t caller_uid;
extern gid_t caller_gid;
extern unsigned caller_num;

extern const char *change_user1, *change_user2;
extern uid_t change_uid1, change_uid2;
extern gid_t change_gid1, change_gid2;
extern mode_t change_umask;
extern int change_nice;
extern change_rlimit_t change_rlimit[];
extern work_limit_t wlimit;

#endif /* PKG_BUILD_PRIV_H */
