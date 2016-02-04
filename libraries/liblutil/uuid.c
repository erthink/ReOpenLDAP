/* uuid.c -- Universally Unique Identifier routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2016 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright 2000, John E. Schimmel, All rights reserved.
 * This software is not subject to any license of Mirapoint, Inc.
 *
 * This is free software; you can redistribute and use it
 * under the same terms as OpenLDAP itself.
 */
/* This work was initially developed by John E. Schimmel and adapted
 * for inclusion in OpenLDAP Software by Kurt D. Zeilenga.
 */

/*
 * Sorry this file is so scary, but it needs to run on a wide range of
 * platforms.  The only exported routine is lutil_uuidstr() which is all
 * that LDAP cares about.  It generates a new uuid and returns it in
 * in string form.
 */
#include "portable.h"

#include <limits.h>
#include <stdio.h>
#include <sys/types.h>

#include <ac/stdlib.h>
#include <ac/string.h>	/* get memcmp() */

#ifdef HAVE_UUID_TO_STR
#  include <sys/uuid.h>
#elif defined( HAVE_UUID_GENERATE )
#  include <uuid/uuid.h>
#else
# error "libuuid-dev installed?"
#endif

#include <lutil.h>

/*
** All we really care about is an ISO UUID string.  The format of a UUID is:
**	field			octet		note
**	time_low		0-3		low field of the timestamp
**	time_mid		4-5		middle field of timestamp
**	time_hi_and_version	6-7		high field of timestamp and
**						version number
**	clock_seq_hi_and_resv	8		high field of clock sequence
**						and variant
**	clock_seq_low		9		low field of clock sequence
**	node			10-15		spacially unique identifier
**
** We use DCE version one, and the DCE variant.  Our unique identifier is
** the first ethernet address on the system.
*/
size_t
lutil_uuidstr( char *buf, size_t len )
{
#ifdef HAVE_UUID_TO_STR
	uuid_t uu = {0};
	unsigned rc;
	char *s;
	size_t l;

	uuid_create( &uu, &rc );
	if ( rc != uuid_s_ok ) {
		return 0;
	}

	uuid_to_str( &uu, &s, &rc );
	if ( rc != uuid_s_ok ) {
		return 0;
	}

	l = strlen( s );
	if ( l >= len ) {
		free( s );
		return 0;
	}

	strncpy( buf, s, len );
	free( s );

	return l;

#elif defined( HAVE_UUID_GENERATE )
	uuid_t uu;

	uuid_generate( uu );
	uuid_unparse_lower( uu, buf );
	return strlen( buf );
#else
	return buf[0] = 0;
#endif
}

int
lutil_uuidstr_from_normalized(
	char		*uuid,
	size_t		uuidlen,
	char		*buf,
	size_t		buflen )
{
	unsigned char nibble;
	int i, d = 0;

	assert( uuid != NULL );
	assert( buf != NULL );

	if ( uuidlen != 16 ) return -1;
	if ( buflen < 36 ) return -1;

	for ( i = 0; i < 16; i++ ) {
		if ( i == 4 || i == 6 || i == 8 || i == 10 ) {
			buf[(i<<1)+d] = '-';
			d += 1;
		}

		nibble = (uuid[i] >> 4) & 0xF;
		if ( nibble < 10 ) {
			buf[(i<<1)+d] = nibble + '0';
		} else {
			buf[(i<<1)+d] = nibble - 10 + 'a';
		}

		nibble = (uuid[i]) & 0xF;
		if ( nibble < 10 ) {
			buf[(i<<1)+d+1] = nibble + '0';
		} else {
			buf[(i<<1)+d+1] = nibble - 10 + 'a';
		}
	}

	if ( buflen > 36 ) buf[36] = '\0';
	return 36;
}

#ifdef TEST
int
main(int argc, char **argv)
{
	char buf1[8], buf2[64];

#ifndef HAVE_UUID_TO_STR
	unsigned char *p = lutil_eaddr();

	if( p ) {
		printf( "Ethernet Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			(unsigned) p[0], (unsigned) p[1], (unsigned) p[2],
			(unsigned) p[3], (unsigned) p[4], (unsigned) p[5]);
	}
#endif

	if ( lutil_uuidstr( buf1, sizeof( buf1 ) ) ) {
		printf( "UUID: %s\n", buf1 );
	} else {
		fprintf( stderr, "too short: %ld\n", (long) sizeof( buf1 ) );
	}

	if ( lutil_uuidstr( buf2, sizeof( buf2 ) ) ) {
		printf( "UUID: %s\n", buf2 );
	} else {
		fprintf( stderr, "too short: %ld\n", (long) sizeof( buf2 ) );
	}

	if ( lutil_uuidstr( buf2, sizeof( buf2 ) ) ) {
		printf( "UUID: %s\n", buf2 );
	} else {
		fprintf( stderr, "too short: %ld\n", (long) sizeof( buf2 ) );
	}

	return 0;
}
#endif
