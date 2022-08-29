.\" Documentation for the hasher-privd service daemon.
.\"
.\" Copyright (C) 2004-2022  Dmitry V. Levin <ldv@altlinux.org>
.\" All rights reserved.
.\"
.\" SPDX-License-Identifier: GPL-2.0-or-later

[NAME]
\fBhasher-privd\fR \- service daemon for the hasher project

[FILES]
.TP
.I /etc/hasher\-priv/daemon.conf
service daemon configuration file
.TP
.I /etc/hasher\-priv/system
systemwide hasher\-priv configuration file
.TP
\fI/etc/hasher\-priv/user.d/\fBUSER\fR
per-user hasher\-priv configuration files
.TP
\fI/etc/hasher\-priv/user.d/\fBUSER\fI:\fBNUMBER\fR
per-user per-number subconfig files

[SEE ALSO]
.BR hasher (7),
.BR hasher\-priv (8),
.BR hasher\-priv.conf (5),
.BR hasher\-useradd (8).
