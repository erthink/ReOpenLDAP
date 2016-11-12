/* $ReOpenLDAP$ */
/* Copyright 1992-2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
 * All rights reserved.
 *
 * This file is part of ReOpenLDAP.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
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
