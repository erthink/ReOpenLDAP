/* $ReOpenLDAP$ */
/* Copyright 1990-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* ch_malloc.c - malloc routines that test returns from malloc and friends */

/* LY: one more rebus - this turnoff 'free' to 'ch_free' substitution. */
#define CH_FREE 1

#include "reldap.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

BerMemoryFunctions ch_mfuncs = {
	(BER_MEMALLOC_FN *)ch_malloc,
	(BER_MEMCALLOC_FN *)ch_calloc,
	(BER_MEMREALLOC_FN *)ch_realloc,
	(BER_MEMFREE_FN *)ch_free
};

void *
ch_malloc(
    ber_len_t	size
)
{
	void	*new;

	if ( (new = (void *) ber_memalloc_x( size, NULL )) == NULL ) {
		Debug( LDAP_DEBUG_ANY, "ch_malloc of %lu bytes failed\n",
			(long) size );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}

	return( new );
}

void *
ch_realloc(
    void		*block,
    ber_len_t	size
)
{
	void	*new, *ctx;

	if ( block == NULL ) {
		return( ch_malloc( size ) );
	}

	if( size == 0 ) {
		ch_free( block );
		return NULL;
	}

	ctx = slap_sl_context( block );
	if ( ctx ) {
		return slap_sl_realloc( block, size, ctx );
	}

	if ( (new = (void *) ber_memrealloc_x( block, size, NULL )) == NULL ) {
		Debug( LDAP_DEBUG_ANY, "ch_realloc of %lu bytes failed\n",
			(long) size );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}

	return( new );
}

void *
ch_calloc(
    ber_len_t	nelem,
    ber_len_t	size
)
{
	void	*new;

	if ( (new = (void *) ber_memcalloc_x( nelem, size, NULL )) == NULL ) {
		Debug( LDAP_DEBUG_ANY, "ch_calloc of %lu elems of %lu bytes failed\n",
		  (long) nelem, (long) size );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}

	return( new );
}

char *
ch_strdup(
    const char *string
)
{
	char	*new;

	if ( (new = ber_strdup_x( string, NULL )) == NULL ) {
		Debug( LDAP_DEBUG_ANY, "ch_strdup(%s) failed\n", string );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}

	return( new );
}

void
ch_free( void *ptr )
{
	void *ctx;

	ctx = slap_sl_context( ptr );
	if (ctx) {
		slap_sl_free( ptr, ctx );
	} else {
		ber_memfree_x( ptr, NULL );
	}
}
