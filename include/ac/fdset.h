/* redefine FD_SET */
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
 * This header is to be included by portable.h to ensure
 * tweaking of FD_SETSIZE is done early enough to be effective.
 */

#ifndef _AC_FDSET_H
#define _AC_FDSET_H

#if !defined( OPENLDAP_FD_SETSIZE ) && !defined( FD_SETSIZE )
#  define OPENLDAP_FD_SETSIZE 4096
#endif

#ifdef OPENLDAP_FD_SETSIZE
    /* assume installer desires to enlarge fd_set */
#  ifdef HAVE_BITS_TYPES_H
#    include <bits/types.h>
#  endif
#  ifdef __FD_SETSIZE
#    undef __FD_SETSIZE
#    define __FD_SETSIZE OPENLDAP_FD_SETSIZE
#  else
#    define FD_SETSIZE OPENLDAP_FD_SETSIZE
#  endif
#endif

#endif /* _AC_FDSET_H */
