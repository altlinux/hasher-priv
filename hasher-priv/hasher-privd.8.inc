.\" Documentation for the hasher-privd service daemon.
.\"
.\" Copyright (C) 2004-2022  Dmitry V. Levin <ldv@altlinux.org>
.\" All rights reserved.
.\"
.\" SPDX-License-Identifier: GPL-2.0-or-later

[NAME]
\fBhasher-privd\fR \- service daemon for the hasher project

[OVERVIEW]
As described in
.BR hasher (7),
hasher runs isolated workloads in a Unix-like
three-user environment; processes can run as
.IR caller ,
.I builder
and
.IR pseudoroot .
This program performs various actions that require privileges on behalf
of unprivileged processes run by the
.IR caller .
It accepts requests via a Unix stream socket and fulfills or rejects them.
.B hasher-privd
is intended to be run as a system-wide service as part of the startup sequence.
Consumer programs like
.BR hsh (1)
invoke a client program, which can talk to
.BR hasher-privd .
The communication protocol is not in any way stable and is expected to change
as needed in future versions of
.BR hasher-priv .
It is only guaranteed to match across a client and a server of the same release.

The main daemon process only exists to spawn a "session" process for the
client's peercreds and subconfig ID.

The session process then listens on a different Unix socket and accepts job
requests from clients of a particular UID and subconfig ID.
A
.I job
is a single logical action for the server to do on the client's request,
coupled with optional metadata like textual parameters and passed file
descriptors.
Contrary to its name, the session process does not track job runners and the
subprocesses they spawn at all.
Since a job request consists of many messages, and each message can invalidate
the entire request (and the server will have to drop all received state), the
current implementation passes the connection socket for a request to a child
subprocess, the
.IR "job handler" ,
and proceeds to poll socket events or reap children.

If the request is invalid, the
.I job handler
sends an error reply with an optional message and exits.
Otherwise, it spawns yet another child, the job runner, and exits immediately,
reparenting it to pid 1 or a subreaper.
(This design relieves the session process from having to call
.BR wait (2)
on runners.)

The job runner's only purpose is to join the client's cgroup
(so the resources it consumes are attributed to the client), spawn the
.I job executor
and wait for one of the following, whatever happens first:
.RS
.PP
\- the client disconnects from the runner, so the job must be aborted;
.PP
\- the job finishes, so the runner sends a reply to the client with the job's
exit status.
.RE

If the service is restarted, runners are not killed and are expected
to continue running.

The executor process, in turn, closes unknown file descriptors and does the
privileged action requested.

All the processes appear in e.g.
.BR ps (1)
output with description strings denoting their role and credentials.

[NOTES]
Since 2.0.2, before a program is executed as part of a chrootuid job,
the process it will run in calls prctl(PR_SET_NO_NEW_PRIVS, 1).
This may or may not impact unaware workloads.

[FILES]
.TP
.I /etc/hasher\-priv/daemon.conf
service daemon configuration file
.TP
.I /etc/hasher\-priv/system
systemwide hasher\-priv configuration file
.TP
.IB /etc/hasher\-priv/user.d/ USER
per-user hasher\-priv configuration files
.TP
.IB /etc/hasher\-priv/user.d/ USER : NUMBER
per-user per-number subconfig files

[SEE ALSO]
.BR hasher (7),
.BR hasher\-priv (8),
.BR hasher\-priv.conf (5),
.BR hasher\-useradd (8).
