/* Generic string.h */
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

#ifndef _AC_STRING_H
#define _AC_STRING_H

#include <string.h>

/* use ldap_pvt_strtok instead of strtok or strtok_r! */
__extern_C LDAP_F(char*) ldap_pvt_strtok(char *str, const char *delim, char **pos);

/* LY: engaging overlap checking for memcpy */
#ifndef LDAP_SAFEMEMCPY
#	if LDAP_ASSERT_CHECK || LDAP_DEBUG || ! defined(NDEBUG)
#		define LDAP_SAFEMEMCPY 1
#	else
#		define LDAP_SAFEMEMCPY 0
#	endif
#endif

#if LDAP_SAFEMEMCPY
#	undef memcpy
#	define memcpy ber_memcpy_safe
	/* LY: memcpy with checking for overlap */
	__extern_C LDAP_F(void*) ber_memcpy_safe(void* dest, const void* src, size_t n);
#endif

#define STRLENOF(s)	(sizeof(s)-1)

#if defined( HAVE_NONPOSIX_STRERROR_R )
#	define AC_STRERROR_R(e,b,l)		(strerror_r((e), (b), (l)))
#elif defined( HAVE_STRERROR_R )
#	define AC_STRERROR_R(e,b,l)		(strerror_r((e), (b), (l)) == 0 ? (b) : "Unknown error")
#elif defined( HAVE_SYS_ERRLIST )
#	define AC_STRERROR_R(e,b,l)		((e) > -1 && (e) < sys_nerr \
							? sys_errlist[(e)] : "Unknown error" )
#elif defined( HAVE_STRERROR )
#	define AC_STRERROR_R(e,b,l)		(strerror(e))	/* NOTE: may be NULL */
#else
#	define AC_STRERROR_R(e,b,l)		("Unknown error")
#endif

#endif /* _AC_STRING_H */
