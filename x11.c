
/*
  $Id$
  Copyright (C) 2005  Dmitry V. Levin <ldv@altlinux.org>

  The X11 forwarding support_str for the hasher-priv program.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/* Code in this file may be executed with root, caller or child privileges. */

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "priv.h"
#include "xmalloc.h"

#define X11_UNIX_DIR "/tmp/.X11-unix"

typedef int (*x11_connect_method_t) (const char *, unsigned);
static x11_connect_method_t x11_connect_method;
static const char *x11_connect_name;
static unsigned long x11_connect_port;

static int x11_dir_fd = -1;

/* This function may be executed with child privileges. */

int
xauth_add_entry (char *const *env)
{
	if (!x11_display || !x11_key)
		return 0;

	const char *av[] = { "xauth", "add", ":10.0", ".", x11_key, 0 };
	const char *path = "/usr/X11R6/bin/xauth";

	pid_t   pid = fork ();

	if (pid < 0)
		return 1;

	if (!pid)
	{
		execve (path, (char *const *) av, env);
		error (EXIT_SUCCESS, errno, "execve: %s", path);
		_exit (1);
	} else
	{
		int     status = 0;

		if (waitpid (pid, &status, 0) != pid || !WIFEXITED (status))
			return 1;
		return WEXITSTATUS (status);
	}
}

/* This function may be executed with child privileges. */

int
x11_bind (int fd)
{
	struct sockaddr_un sun;

	memset (&sun, 0, sizeof (sun));
	sun.sun_family = AF_UNIX;
	snprintf (sun.sun_path, sizeof sun.sun_path, "%s/%s",
		  X11_UNIX_DIR, "X10");

	if (unlink (sun.sun_path) && errno != ENOENT)
	{
		error (EXIT_SUCCESS, errno, "unlink: %s", sun.sun_path);
		return -1;
	}

	if (mkdir (X11_UNIX_DIR, 0700) && errno != EEXIST)
	{
		error (EXIT_SUCCESS, errno, "mkdir: %s", X11_UNIX_DIR);
		return -1;
	}

	if (bind (fd, (struct sockaddr *) &sun, sizeof sun))
	{
		error (EXIT_SUCCESS, errno, "bind: %s", sun.sun_path);
		return -1;
	}

	if (listen (fd, 16) < 0)
	{
		error (EXIT_SUCCESS, errno, "listen: %s", sun.sun_path);
		return -1;
	}
	return 0;
}

/* This function may be executed with caller privileges. */

static int
x11_connect_unix ( __attribute__ ((unused)) const char *name,
		  unsigned display_number)
{
	int     fd = -1;

	for (;;)
	{
		if (fchdir (x11_dir_fd))
		{
			error (EXIT_SUCCESS, errno, "fchdir (%d)",
			       x11_dir_fd);
			fputc('\r', stderr);
			break;
		}

		if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
		{
			error (EXIT_SUCCESS, errno, "socket");
			fputc('\r', stderr);
			break;
		}

		struct sockaddr_un sun;

		memset (&sun, 0, sizeof (sun));
		sun.sun_family = AF_UNIX;
		snprintf (sun.sun_path, sizeof sun.sun_path, "X%u",
			  display_number);

		if (!connect (fd, (struct sockaddr *) &sun, sizeof sun))
			break;

		error (EXIT_SUCCESS, errno, "connect: %s", sun.sun_path);
		fputc('\r', stderr);
		close (fd);
		fd = -1;
		break;
	}
	(void) chdir ("/");
	return fd;
}

/* This function may be executed with caller privileges. */

static int
x11_connect_inet (const char *name, unsigned display_number)
{
	int     rc, fd = -1;
	unsigned port_num = 6000 + display_number;
	struct addrinfo hints, *ai, *aitop;
	char    port_str[NI_MAXSERV];

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	snprintf (port_str, sizeof port_str, "%u", port_num);
	if ((rc = getaddrinfo (name, port_str, &hints, &aitop)) != 0)
	{
		error (EXIT_SUCCESS, errno, "getaddrinfo: %s:%u", name,
		       port_num);
		fputc('\r', stderr);
		return -1;
	}
	for (ai = aitop; ai; ai = ai->ai_next)
	{
		fd = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0)
			continue;

		if (connect (fd, ai->ai_addr, ai->ai_addrlen) < 0)
		{
			close (fd);
			continue;
		}

		break;
	}

	freeaddrinfo (aitop);
	if (!ai)
	{
		error (EXIT_SUCCESS, errno, "connect: %s:%u", name, port_num);
		fputc('\r', stderr);
	}
	return fd;
}

/* This function may be executed with caller privileges. */

int
x11_connect (void)
{
	return x11_connect_method
		? x11_connect_method (x11_connect_name,
				      x11_connect_port) : -1;
}

/* This function may be executed with caller privileges. */

int
x11_accept (int fd)
{
	struct sockaddr_un sun;
	socklen_t len = sizeof (sun);

	return accept (fd, (struct sockaddr *) &sun, &len);
}

/* This function may be executed with root privileges. */

static x11_connect_method_t
x11_parse_display (void)
{
	static char *display;

	if (!x11_display || !x11_key || x11_connect_method)
		return 0;

	display = xstrdup (x11_display);

	char   *colon = strchr (display, ':');
	const char *number = strrchr (display, ':');

	if (!colon || !number)
	{
		error (EXIT_SUCCESS, 0,
		       "Unrecognized DISPLAY=%s, X11 forwarding disabled",
		       display);
		x11_display = 0;
		return 0;
	}

	x11_connect_name = display;
	*colon = '\0';
	++number;

	char   *endp;

	x11_connect_port = strtoul (number, &endp, 10);
	if (!endp || (*endp && *endp != '.') || x11_connect_port > 100)
	{
		error (EXIT_SUCCESS, 0,
		       "Unrecognized DISPLAY=%s, X11 forwarding disabled",
		       display);
		x11_display = 0;
		return 0;
	}

	if (x11_connect_name[0] == '\0')
		return x11_connect_unix;

	const char *slash = strrchr (x11_connect_name, '/');

	if (slash && !strcmp (slash + 1, "unix"))
		return x11_connect_unix;
	else
		return x11_connect_inet;
}

/* This function may be executed with root privileges. */

int
x11_socket (void)
{
	if (!(x11_connect_method = x11_parse_display ()))
		return -1;

	int     fd = socket (AF_UNIX, SOCK_STREAM, 0);

	if (fd < 0)
		error (EXIT_SUCCESS, errno, "socket");

	if (x11_dir_fd < 0)
	{
		if ((x11_dir_fd = open (X11_UNIX_DIR, O_RDONLY)) < 0)
		{
			error (EXIT_SUCCESS, errno, "open: %s", X11_UNIX_DIR);
			(void) close (fd);
			fd = -1;
		} else
			set_cloexec (x11_dir_fd);
	}

	return fd;
}
