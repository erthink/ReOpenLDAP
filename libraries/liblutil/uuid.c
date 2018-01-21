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

#include "reldap.h"

#include <limits.h>
#include <stdio.h>
#include <sys/types.h>

#include <ac/stdlib.h>
#include <ac/string.h>	/* get memcmp() */

#include <uuid/uuid.h>
#include <lutil.h>

size_t
lutil_uuidstr( char *buf, size_t len )
{
	uuid_t uu;

	uuid_generate( uu );
	uuid_unparse_lower( uu, buf );
	return strlen( buf );
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
