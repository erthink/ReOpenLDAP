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

#include "reldap.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

LDAPMessage *
ldap_first_message( LDAP *ld, LDAPMessage *chain )
{
	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( chain != NULL );

  	return chain;
}

LDAPMessage *
ldap_next_message( LDAP *ld, LDAPMessage *msg )
{
	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msg != NULL );

	return msg->lm_chain;
}

int
ldap_count_messages( LDAP *ld, LDAPMessage *chain )
{
	int	i;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );

	for ( i = 0; chain != NULL; chain = chain->lm_chain ) {
		i++;
	}

	return( i );
}

BerElement*
ldap_get_message_ber( LDAPMessage *ld )
{
	return ld->lm_ber;
}
