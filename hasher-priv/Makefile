#
# Makefile for the hasher-priv project.
#
# Copyright (C) 2003-2022  Dmitry V. Levin <ldv@altlinux.org>
# All rights reserved.
#
# SPDX-License-Identifier: GPL-2.0-or-later
#

PROJECT = hasher-priv
VERSION = $(shell sed '/^Version: */!d;s///;q' hasher-priv.spec)
HELPERS = getconf.sh getugid1.sh chrootuid1.sh getugid2.sh chrootuid2.sh
MAN5PAGES = $(PROJECT).conf.5
MAN8PAGES = $(PROJECT).8 hasher-useradd.8
TARGETS = $(PROJECT) hasher-privd hasher-useradd $(HELPERS) $(MAN5PAGES) $(MAN8PAGES)

have-cc-function = $(shell echo 'extern void $(1)(void); int main () { $(1)(); return 0; }' |$(CC) -o /dev/null -xc - > /dev/null 2>&1 && echo "-D$(2)")

sysconfdir = /etc
initdir=$(sysconfdir)/rc.d/init.d
systemd_unitdir=/lib/systemd/system
libexecdir = /usr/lib
sbindir = /usr/sbin
mandir = /usr/share/man
man5dir = $(mandir)/man5
man8dir = $(mandir)/man8
configdir = $(sysconfdir)/$(PROJECT)
helperdir = $(libexecdir)/$(PROJECT)
runstatedir = /run
socketdir = $(runstatedir)/hasher-priv
DESTDIR =

MKDIR_P = mkdir -p
INSTALL = install
HELP2MAN8 = help2man -N -s8
LFS_CFLAGS = $(shell getconf LFS_CFLAGS)
CHDIRUID_FLAGS = -DENABLE_SETFSUGID -DENABLE_SUPPLEMENTARY_GROUPS
WARNINGS = -Wall -W -Werror \
	   -Waggregate-return \
	   -Wconversion \
	   -Wdisabled-optimization \
	   -Wmissing-declarations \
	   -Wmissing-format-attribute \
	   -Wmissing-noreturn \
	   -Wmissing-prototypes \
	   -Wpointer-arith \
	   -Wredundant-decls \
	   -Wshadow \
	   -Wstrict-prototypes \
	   -Wwrite-strings \
	   #
CPPFLAGS = -std=gnu99 -D_GNU_SOURCE $(CHDIRUID_FLAGS) \
	$(call have-cc-function,close_range,HAVE_CLOSE_RANGE) \
	$(LFS_CFLAGS) -DPROJECT_VERSION=\"$(VERSION)\" \
	-DSOCKETDIR=\"$(socketdir)\" -DPROJECT=\"$(PROJECT)\"
CFLAGS = -pipe -O2
override CFLAGS += $(WARNINGS)
LDLIBS =
server_LDLIBS = -lsetproctitle

SRC_client =		\
	cmdline.c	\
	die.c		\
	error_prints.c	\
	fds.c		\
	hasher-priv.c	\
	io_loop.c	\
	pass.c		\
	sockets.c	\
	xmalloc.c	\
	#
OBJ_client = $(SRC_client:.c=.o)

SRC_server =		\
	caller.c	\
	caller_config.c	\
	caller_job.c	\
	caller_runner.c	\
	caller_server.c	\
	chdir.c		\
	chdiruid.c	\
	chid.c		\
	child.c		\
	chrootuid.c	\
	die.c		\
	epoll.c		\
	error_prints.c	\
	fd_set.c	\
	fds.c		\
	file_config.c	\
	getconf.c	\
	getugid.c	\
	hasher-privd.c	\
	io_log.c	\
	io_loop.c	\
	io_x11.c	\
	ipc.c		\
	job2str.c	\
	killuid.c	\
	makedev.c	\
	mount.c		\
	net.c		\
	nullify_stdin.c	\
	opt_parse.c	\
	parent.c	\
	pass.c		\
	pidfile.c	\
	process.c	\
	procfd.c	\
	pty.c		\
	server_comm.c	\
	server_config.c	\
	signal.c	\
	sockets.c	\
	spawn_killuid.c	\
	tty.c		\
	unblock_fd.c	\
	unix.c		\
	unshare.c	\
	x11.c		\
	xmalloc.c	\
	#
OBJ_server = $(SRC_server:.c=.o)

DEP = $(SRC_client:.c=.d) $(SRC_server:.c=.d)

.PHONY:	all install clean indent

all: $(TARGETS)

$(PROJECT): $(OBJ_client)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

hasher-privd: $(OBJ_server)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(server_LDLIBS) -o $@

install: all
	$(MKDIR_P) -m710 $(DESTDIR)$(configdir)/user.d
	$(INSTALL) -p -m640 fstab $(DESTDIR)$(configdir)/fstab
	$(INSTALL) -p -m640 system.conf $(DESTDIR)$(configdir)/system
	$(INSTALL) -p -m640 daemon.conf $(DESTDIR)$(configdir)/daemon.conf
	$(MKDIR_P) -m750 $(DESTDIR)$(helperdir)
	$(INSTALL) -p -m700 $(PROJECT) $(DESTDIR)$(helperdir)/
	$(INSTALL) -p -m755 $(HELPERS) $(DESTDIR)$(helperdir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(systemd_unitdir)
	$(INSTALL) -p -m644 hasher-privd.service $(DESTDIR)$(systemd_unitdir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(initdir)
	$(INSTALL) -p -m755 hasher-privd.sysvinit $(DESTDIR)$(initdir)/hasher-privd
	$(MKDIR_P) -m755 $(DESTDIR)$(sbindir)
	$(INSTALL) -p -m755 hasher-privd $(DESTDIR)$(sbindir)/
	$(INSTALL) -p -m755 hasher-useradd $(DESTDIR)$(sbindir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(man5dir)
	$(INSTALL) -p -m644 $(MAN5PAGES) $(DESTDIR)$(man5dir)/
	$(MKDIR_P) -m755 $(DESTDIR)$(man8dir)
	$(INSTALL) -p -m644 $(MAN8PAGES) $(DESTDIR)$(man8dir)/

clean:
	$(RM) $(TARGETS) $(DEP) $(OBJ_client) $(OBJ_server) core *~

indent:
	indent *.h *.c

%1.sh: %.sh.in Makefile
	sed -e 's|@N@|1|g' -e 's|@helper@|$(helperdir)/$(PROJECT)|g' <$< >$@
	chmod --reference=$< $@

%2.sh: %.sh.in Makefile
	sed -e 's|@N@|2|g' -e 's|@helper@|$(helperdir)/$(PROJECT)|g' <$< >$@
	chmod --reference=$< $@

%.sh: %.sh.in Makefile
	sed -e 's|@helper@|$(helperdir)/$(PROJECT)|g' <$< >$@
	chmod --reference=$< $@

%.5: %.5.in
	sed -e 's/@VERSION@/$(VERSION)/g' <$< >$@
	chmod --reference=$< $@

hasher-useradd: hasher-useradd.in
	sed -e 's/@VERSION@/$(VERSION)/g' <$< >$@
	chmod 755 $@

%.8: % %.8.inc Makefile
	$(HELP2MAN8) -i $@.inc -o $@ ./$<

# We need dependencies only if goal isn't "indent" or "clean".
ifneq ($(MAKECMDGOALS),indent)
ifneq ($(MAKECMDGOALS),clean)

%.d:	%.c Makefile
	@echo Making dependences for $<
	@$(CC) -MM $(CPPFLAGS) $< |sed -e 's,\($*\)\.o[ :]*,\1.o $@: Makefile ,g' >$@

ifneq ($(DEP),)
-include $(DEP)
endif

endif # clean
endif # indent
