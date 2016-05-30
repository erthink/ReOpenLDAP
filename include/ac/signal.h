/* Generic signal.h */
/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _AC_SIGNAL_H
#define _AC_SIGNAL_H

#include <signal.h>

#undef SIGNAL

#if defined( HAVE_SIGACTION )
#define SIGNAL lutil_sigaction
typedef void (*lutil_sig_t)(int);
LDAP_LUTIL_F(lutil_sig_t) lutil_sigaction( int sig, lutil_sig_t func );
#define SIGNAL_REINSTALL(sig,act)	(void)0
#elif defined( HAVE_SIGSET )
#define SIGNAL sigset
#define SIGNAL_REINSTALL sigset
#else
#define SIGNAL signal
#define SIGNAL_REINSTALL signal
#endif

#if !defined( LDAP_SIGUSR1 ) || !defined( LDAP_SIGUSR2 )
#undef LDAP_SIGUSR1
#undef LDAP_SIGUSR2
#define LDAP_SIGUSR1	SIGUSR1
#define LDAP_SIGUSR2	SIGUSR2
#endif

#ifndef LDAP_SIGCHLD
#ifdef SIGCHLD
#define LDAP_SIGCHLD SIGCHLD
#elif SIGCLD
#define LDAP_SIGCHLD SIGCLD
#endif
#endif

#endif /* _AC_SIGNAL_H */
