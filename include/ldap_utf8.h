/* $ReOpenLDAP$ */
/* Copyright 1992-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#ifndef _LDAP_UTF8_H
#define _LDAP_UTF8_H

#include <lber_types.h>	/* get ber_*_t */

/*
 * UTF-8 Utility Routines
 */

LDAP_BEGIN_DECL

#define LDAP_UCS4_INVALID (0x80000000U)
typedef ber_int_t ldap_ucs4_t;


/* LDAP_MAX_UTF8_LEN is 3 or 6 depending on size of wchar_t */
#define LDAP_MAX_UTF8_LEN  ( sizeof(wchar_t) * 3/2 )

/* Unicode conversion routines  */
LDAP_F( ldap_ucs4_t ) ldap_x_utf8_to_ucs4( LDAP_CONST char * p );
LDAP_F( int ) ldap_x_ucs4_to_utf8( ldap_ucs4_t c, char *buf );


/*
 * Wide Char / UTF-8 Conversion Routines
 */

/* UTF-8 character to Wide Char */
LDAP_F(int) ldap_x_utf8_to_wc LDAP_P((
	wchar_t *wchar, LDAP_CONST char *utf8char ));

/* UTF-8 string to Wide Char string */
LDAP_F(int) ldap_x_utf8s_to_wcs LDAP_P((
	wchar_t *wcstr, LDAP_CONST char *utf8str, size_t count ));

/* Wide Char to UTF-8 character */
LDAP_F(int) ldap_x_wc_to_utf8 LDAP_P((
	char *utf8char, wchar_t wchar, size_t count ));

/* Wide Char string to UTF-8 string */
LDAP_F(int) ldap_x_wcs_to_utf8s LDAP_P((
	char *utf8str, LDAP_CONST wchar_t *wcstr, size_t count ));

/*
 * MultiByte Char / UTF-8 Conversion Routines
 */

/* UTF-8 character to MultiByte character */
LDAP_F(int) ldap_x_utf8_to_mb LDAP_P((
	char *mbchar, LDAP_CONST char *utf8char,
	int (*ldap_f_wctomb)( char *mbchar, wchar_t wchar )));

/* UTF-8 string to MultiByte string */
LDAP_F(int) ldap_x_utf8s_to_mbs LDAP_P((
	char *mbstr, LDAP_CONST char *utf8str, size_t count,
	size_t (*ldap_f_wcstombs)( char *mbstr,
		LDAP_CONST wchar_t *wcstr, size_t count) ));

/* MultiByte character to UTF-8 character */
LDAP_F(int) ldap_x_mb_to_utf8 LDAP_P((
	char *utf8char, LDAP_CONST char *mbchar, size_t mbsize,
	int (*ldap_f_mbtowc)( wchar_t *wchar,
		LDAP_CONST char *mbchar, size_t count) ));

/* MultiByte string to UTF-8 string */
LDAP_F(int) ldap_x_mbs_to_utf8s LDAP_P((
	char *utf8str, LDAP_CONST char *mbstr, size_t count,
	size_t (*ldap_f_mbstowcs)( wchar_t *wcstr,
		LDAP_CONST char *mbstr, size_t count) ));

LDAP_END_DECL

#endif /* _LDAP_UTF8_H */
