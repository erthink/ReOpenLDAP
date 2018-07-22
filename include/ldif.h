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

#ifndef _LDIF_H
#define _LDIF_H

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

/* This is NOT a bogus extern declaration (unlike ldap_debug_mask) */
LDAP_LDIF_V (int) ldif_debug;

#define LDIF_LINE_WIDTH      76      /* default maximum length of LDIF lines */
#define LDIF_LINE_WIDTH_MAX  ((ber_len_t)-1) /* maximum length of LDIF lines */
#define LDIF_LINE_WIDTH_WRAP(wrap) ((wrap) == 0 ? LDIF_LINE_WIDTH : (wrap))

/*
 * Macro to calculate maximum number of bytes that the base64 equivalent
 * of an item that is "len" bytes long will take up.  Base64 encoding
 * uses one byte for every six bits in the value plus up to two pad bytes.
 */
#define LDIF_BASE64_LEN(len)	(((len) * 4 / 3 ) + 3)

/*
 * Macro to calculate maximum size that an LDIF-encoded type (length
 * tlen) and value (length vlen) will take up:  room for type + ":: " +
 * first newline + base64 value + continued lines.  Each continued line
 * needs room for a newline and a leading space character.
 */
#define LDIF_SIZE_NEEDED(nlen,vlen) \
    ((nlen) + 4 + LDIF_BASE64_LEN(vlen) \
    + ((LDIF_BASE64_LEN(vlen) + (nlen) + 3) / (LDIF_LINE_WIDTH-1) * 2 ))

#define LDIF_SIZE_NEEDED_WRAP(nlen,vlen,wrap) \
    ((nlen) + 4 + LDIF_BASE64_LEN(vlen) \
    + ((wrap) == 0 ? ((LDIF_BASE64_LEN(vlen) + (nlen) + 3) / ( LDIF_LINE_WIDTH-1 ) * 2 ) : \
	((wrap) == LDIF_LINE_WIDTH_MAX ? 0 : ((LDIF_BASE64_LEN(vlen) + (nlen) + 3) / (wrap-1) * 2 ))))

LDAP_LDIF_F( int )
ldif_parse_line LDAP_P((
	const char *line,
	char **name,
	char **value,
	ber_len_t *vlen ));

LDAP_LDIF_F( int )
ldif_parse_line2 LDAP_P((
	char *line,
	struct berval *type,
	struct berval *value,
	int *freeval ));

LDAP_LDIF_F( FILE * )
ldif_open_url LDAP_P(( const char *urlstr ));

LDAP_LDIF_F( int )
ldif_fetch_url LDAP_P((
	const char *line,
	char **value,
	ber_len_t *vlen ));

LDAP_LDIF_F( char * )
ldif_getline LDAP_P(( char **next ));

LDAP_LDIF_F( int )
ldif_countlines LDAP_P(( const char *line ));

/* ldif_ropen, rclose, read_record - just for reading LDIF files,
 * no special open/close needed to write LDIF files.
 */
typedef struct LDIFFP {
	FILE *fp;
	struct LDIFFP *prev;
} LDIFFP;

LDAP_LDIF_F( LDIFFP * )
ldif_open LDAP_P(( const char *file, const char *mode ));

/* ldif_open equivalent that opens ldif stream in memory rather than from file */
LDAP_LDIF_F( LDIFFP * )
ldif_open_mem LDAP_P(( char *ldif, size_t size, const char *mode ));

LDAP_LDIF_F( void )
ldif_close LDAP_P(( LDIFFP * ));

LDAP_LDIF_F( int )
ldif_read_record LDAP_P((
	LDIFFP *fp,
	unsigned long *lineno,
	char **bufp,
	int *buflen ));

LDAP_LDIF_F( int )
ldif_must_b64_encode_register LDAP_P((
	const char *name,
	const char *oid ));

LDAP_LDIF_F( void )
ldif_must_b64_encode_release LDAP_P(( void ));

#define LDIF_PUT_NOVALUE	0x0000	/* no value */
#define LDIF_PUT_VALUE		0x0001	/* value w/ auto detection */
#define LDIF_PUT_TEXT		0x0002	/* assume text */
#define	LDIF_PUT_BINARY		0x0004	/* assume binary (convert to base64) */
#define LDIF_PUT_B64		0x0008	/* pre-converted base64 value */

#define LDIF_PUT_COMMENT	0x0010	/* comment */
#define LDIF_PUT_URL		0x0020	/* url */
#define LDIF_PUT_SEP		0x0040	/* separator */

LDAP_LDIF_F( void )
ldif_sput LDAP_P((
	char **out,
	int type,
	const char *name,
	const char *val,
	ber_len_t vlen ));

LDAP_LDIF_F( void )
ldif_sput_wrap LDAP_P((
	char **out,
	int type,
	const char *name,
	const char *val,
	ber_len_t vlen,
        ber_len_t wrap ));

LDAP_LDIF_F( char * )
ldif_put LDAP_P((
	int type,
	const char *name,
	const char *val,
	ber_len_t vlen ));

LDAP_LDIF_F( char * )
ldif_put_wrap LDAP_P((
	int type,
	const char *name,
	const char *val,
	ber_len_t vlen,
	ber_len_t wrap ));

LDAP_LDIF_F( int )
ldif_is_not_printable LDAP_P((
	const char *val,
	ber_len_t vlen ));

LDAP_END_DECL

#endif /* _LDIF_H */
