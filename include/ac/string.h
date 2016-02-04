/* Generic string.h */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2016 The OpenLDAP Foundation.
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

#if LDAP_SAFEMEMCPY
#	undef memcpy
	__extern_C void* ber_memcpy_safe(void* dest, const void* src, size_t n);
#	define memcpy ber_memcpy_safe
#endif

/* use ldap_pvt_strtok instead of strtok or strtok_r! */
LDAP_F(char *) ldap_pvt_strtok LDAP_P(( char *str,
	const char *delim, char **pos ));

#ifndef HAVE_STRDUP
	/* strdup() is missing, declare our own version */
#	undef strdup
#	define strdup(s) ber_strdup(s)
#elif !defined(_WIN32)
	/* some systems fail to declare strdup */
	/* Windows does not require this declaration */
	LDAP_LIBC_F(char *) (strdup)();
#endif

#ifdef NEED_MEMCMP_REPLACEMENT
	int (lutil_memcmp)(const void *b1, const void *b2, size_t len);
#define memcmp lutil_memcmp
#endif

void *(lutil_memrchr)(const void *b, int c, size_t n);
/* GNU extension (glibc >= 2.1.91), only declared when defined(_GNU_SOURCE) */
#if defined(HAVE_MEMRCHR) && defined(_GNU_SOURCE)
#define lutil_memrchr(b, c, n) memrchr(b, c, n)
#endif /* ! HAVE_MEMRCHR */

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
