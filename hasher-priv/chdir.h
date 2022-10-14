/*
 * The chdir-with-validation interface for the hasher-privd server program.
 *
 * Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (C) 2022  Gleb Fotengauer-Malinovskiy <glebfm@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HASHER_CHDIR_H
# define HASHER_CHDIR_H

struct stat;

typedef void (*VALIDATE_FPTR)(struct stat *, const char *);

void stat_caller_ok_validator(struct stat *, const char *);
void stat_caller_rooter_ok_validator(struct stat *, const char *);
void stat_root_ok_validator(struct stat *, const char *);

void safe_chdir(const char *, VALIDATE_FPTR);
void safe_fchdir(int, VALIDATE_FPTR);
void chdiruid(const char *, VALIDATE_FPTR);
void fchdiruid(int, VALIDATE_FPTR);

#endif /* !HASHER_CHDIR_H */
