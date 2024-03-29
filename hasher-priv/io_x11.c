/*
 * The chrootuid parent X11 I/O handler for the hasher-privd server program.
 *
 * Copyright (C) 2005-2022  Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Code in this file may be executed with caller privileges. */

#include "error_prints.h"
#include "fds.h"
#include "fd_set.h"
#include "io_loop.h"
#include "io_x11.h"
#include "unblock_fd.h"
#include "unix.h"
#include "x11.h"
#include "xmalloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct io_x11
{
	int     master_fd, slave_fd;
	size_t  master_avail, slave_avail;
	int     authenticated;
	char    master_buf[BUFSIZ], slave_buf[BUFSIZ];
};

typedef struct io_x11 *io_x11_t;

static io_x11_t *io_x11_list;
static size_t io_x11_count;

static  io_x11_t
io_x11_new(int master_fd, int slave_fd)
{
	size_t i;

	for (i = 0; i < io_x11_count; ++i)
		if (!io_x11_list[i])
			break;

	if (i == io_x11_count)
		io_x11_list = xreallocarray(io_x11_list, ++io_x11_count,
					    sizeof(*io_x11_list));

	io_x11_t io = io_x11_list[i] = xzalloc(sizeof(*io_x11_list[i]));

	io->master_fd = master_fd;
	io->slave_fd = slave_fd;
	unblock_fd(master_fd);
	unblock_fd(slave_fd);

	return io;
}

static void
io_x11_free(io_x11_t io)
{
	size_t i;

	for (i = 0; i < io_x11_count; ++i)
		if (io_x11_list[i] == io)
			break;
	if (i == io_x11_count)
		error_msg_and_die("entry %p not found, count=%lu",
				  io, (unsigned long) io_x11_count);
	io_x11_list[i] = 0;

	xclose(&io->master_fd);
	xclose(&io->slave_fd);
	memset(io, 0, sizeof(*io));
	free(io);
}

void
x11_handle_new(const int x11_fd, fd_set *read_fds)
{
	if (!fds_isset(read_fds, x11_fd))
		return;

	int     accept_fd = unix_accept(x11_fd);

	if (accept_fd < 0)
		return;

	int     connect_fd = x11_connect();

	if (connect_fd >= 0)
		io_x11_new(connect_fd, accept_fd);
	else
		xclose(&accept_fd);
}

void
fds_add_x11(fd_set *read_fds, fd_set *write_fds, int *max_fd)
{
	size_t i;

	for (i = 0; i < io_x11_count; ++i)
	{
		io_x11_t io;

		if (!(io = io_x11_list[i]))
			continue;

		if (io->slave_avail)
			fds_add_fd(write_fds, max_fd, io->master_fd);
		else
			fds_add_fd(read_fds, max_fd, io->slave_fd);

		if (io->master_avail)
			fds_add_fd(write_fds, max_fd, io->slave_fd);
		else
			fds_add_fd(read_fds, max_fd, io->master_fd);
	}
}

static void
io_check_auth_data(io_x11_t io, const char *x11_saved_data,
		   const char *x11_fake_data)
{
	if (io->authenticated)
		return;
	io->authenticated = 1;

	size_t avail = io->slave_avail, expected = 12;

	if (avail < expected)
	{
		error_msg("Initial X11 packet too short, expected length = %lu\r",
			  (unsigned long) expected);
		return;
	}
	unsigned proto_len = 0, data_len = 0;
	unsigned char *p = (unsigned char *) io->slave_buf;

	if (p[0] == 0x42)
	{			/* Byte order MSB first. */
		proto_len = (unsigned) (p[7] | (p[6] << 8));
		data_len = (unsigned) (p[9] | (p[8] << 8));
	} else if (p[0] == 0x6c)
	{			/* Byte order LSB first. */
		proto_len = (unsigned) (p[6] | (p[7] << 8));
		data_len = (unsigned) (p[8] | (p[9] << 8));
	} else
	{
		error_msg("Initial X11 packet contains unrecognized order byte: %#x\r",
			  p[0]);
		return;
	}

	expected =
		12 + ((proto_len + 3) & (unsigned) ~3) +
		((data_len + 3) & (unsigned) ~3);
	if (avail < expected)
	{
		error_msg("Initial X11 packet too short, expected length = %lu\r",
			  (unsigned long) expected);
		return;
	}

	if (!proto_len && !data_len)
		return;

	if (data_len != x11_data_len ||
	    memcmp(p + 12 + ((proto_len + 3) & (unsigned) ~3),
		   x11_fake_data, x11_data_len) != 0)
	{
		error_msg("X11 auth data does not match fake data\r");
		return;
	}

	memcpy(p + 12 + ((proto_len + 3) & (unsigned) ~3),
	       x11_saved_data, x11_data_len);
}

void
x11_handle_select(fd_set *read_fds, fd_set *write_fds,
		  const char *x11_saved_data, const char *x11_fake_data)
{
	size_t i;
	ssize_t n;

	for (i = 0; i < io_x11_count; ++i)
	{
		io_x11_t io;

		if (!(io = io_x11_list[i]))
			continue;

		if (io->master_avail && fds_isset(write_fds, io->slave_fd))
		{
			n = write_loop(io->slave_fd,
				       io->master_buf, io->master_avail);
			if (n <= 0)
			{
				io_x11_free(io);
				continue;
			}

			if ((size_t) n < io->master_avail)
			{
				memmove(io->master_buf,
					io->master_buf + (size_t) n,
					io->master_avail - (size_t) n);
			}
			io->master_avail -= (size_t) n;
		}

		if (!io->master_avail && fds_isset(read_fds, io->master_fd))
		{
			n = read_retry(io->master_fd,
				       io->master_buf, sizeof io->master_buf);
			if (n <= 0)
			{
				io_x11_free(io);
				continue;
			}

			io->master_avail = (size_t) n;
		}

		if (io->slave_avail && fds_isset(write_fds, io->master_fd))
		{
			n = write_loop(io->master_fd,
				       io->slave_buf, io->slave_avail);
			if (n <= 0)
			{
				io_x11_free(io);
				continue;
			}

			if ((size_t) n < io->slave_avail)
			{
				memmove(io->slave_buf,
					io->slave_buf + (size_t) n,
					io->slave_avail - (size_t) n);
			}
			io->slave_avail -= (size_t) n;
		}

		if (!io->slave_avail && fds_isset(read_fds, io->slave_fd))
		{
			n = read_retry(io->slave_fd,
				       io->slave_buf, sizeof io->slave_buf);
			if (n <= 0)
			{
				io_x11_free(io);
				continue;
			}

			io->slave_avail = (size_t) n;
			io_check_auth_data(io, x11_saved_data, x11_fake_data);
		}
	}
}
