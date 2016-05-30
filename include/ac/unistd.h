/* Generic unistd.h */
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

#ifndef _AC_UNISTD_H
#define _AC_UNISTD_H

#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif

#ifdef HAVE_PROCESS_H
#	include <process.h>
#endif

/* note: callers of crypt(3) should include <ac/crypt.h> */

#if defined(HAVE_GETPASSPHRASE)
LDAP_LIBC_F(char*)(getpassphrase)();

#else
#define getpassphrase(p) lutil_getpass(p)
LDAP_LUTIL_F(char*)(lutil_getpass) LDAP_P((const char *getpass));
#endif

/* getopt() defines may be in separate include file */
#ifdef HAVE_GETOPT_H
#	include <getopt.h>

#elif !defined(HAVE_GETOPT)
	/* no getopt, assume we need getopt-compat.h */
#	include <getopt-compat.h>

#else
	/* assume we need to declare these externs */
	LDAP_LIBC_V (char *) optarg;
	LDAP_LIBC_V (int) optind, opterr, optopt;
#endif

/* use lutil file locking */
#define ldap_lockf(x)	lutil_lockf(x)
#define ldap_unlockf(x)	lutil_unlockf(x)
#include <lutil_lockf.h>

/*
 * Windows: although sleep() will be resolved by both MSVC and Mingw GCC
 * linkers, the function is not declared in header files. This is
 * because Windows' version of the function is called _sleep(), and it
 * is declared in stdlib.h
 */

#endif /* _AC_UNISTD_H */
