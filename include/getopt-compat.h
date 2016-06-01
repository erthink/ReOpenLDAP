/* getopt-compat.h -- getopt(3) compatibility header */
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

/*
 * getopt(3) declarations
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
