/* $ReOpenLDAP$ */
/* Copyright 1990-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/*
 *  free.c - some free routines are included here to avoid having to
 *           link in lots of extra code when not using certain features
 */

#include "reldap.h"

#include <stdio.h>
#include <ac/stdlib.h>

#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

/*
 * C-API deallocator
 */
void
ldap_memfree( void *p )
{
	LDAP_FREE( p );
}

void
ldap_memvfree( void **v )
{
	LDAP_VFREE( v );
}

void *
ldap_memalloc( ber_len_t s )
{
	return LDAP_MALLOC( s );
}

void *
ldap_memcalloc( ber_len_t n, ber_len_t s )
{
	return LDAP_CALLOC( n, s );
}

void *
ldap_memrealloc( void* p, ber_len_t s )
{
	return LDAP_REALLOC( p, s );
}

char *
ldap_strdup( LDAP_CONST char *p )
{
	return LDAP_STRDUP( p );
}

/*
 * free a null-terminated array of pointers to mod structures. the
 * structures are freed, not the array itself, unless the freemods
 * flag is set.
 */

void
ldap_mods_free( LDAPMod **mods, int freemods )
{
	int	i;

	if ( mods == NULL )
		return;

	for ( i = 0; mods[i] != NULL; i++ ) {
		if ( mods[i]->mod_op & LDAP_MOD_BVALUES ) {
			if( mods[i]->mod_bvalues != NULL )
				ber_bvecfree( mods[i]->mod_bvalues );

		} else if( mods[i]->mod_values != NULL ) {
			LDAP_VFREE( mods[i]->mod_values );
		}

		if ( mods[i]->mod_type != NULL ) {
			LDAP_FREE( mods[i]->mod_type );
		}

		LDAP_FREE( (char *) mods[i] );
	}

	if ( freemods ) {
		LDAP_FREE( (char *) mods );
	}
}
