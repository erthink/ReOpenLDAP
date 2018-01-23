/* $ReOpenLDAP$ */
/* Copyright 1992-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#ifndef _GETOPT_COMPAT_H
#define _GETOPT_COMPAT_H

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

/* change symbols to avoid clashing */
#define optarg lutil_optarg
#define optind lutil_optind
#define opterr lutil_opterr
#define optopt lutil_optopt
#define getopt lutil_getopt

LDAP_LUTIL_V (char *) optarg;
LDAP_LUTIL_V (int) optind, opterr, optopt;
LDAP_LUTIL_F (int) getopt LDAP_P(( int, char * const [], const char *));

LDAP_END_DECL

#endif /* _GETOPT_COMPAT_H */
