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

/* Mostly copied from sasl.c */

#include "reldap.h"

#include <stdlib.h>
#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/errno.h>

#include "ldap-int.h"

int
ldap_ntlm_bind(
	LDAP		*ld,
	LDAP_CONST char	*dn,
	ber_tag_t	tag,
	struct berval	*cred,
	LDAPControl	**sctrls,
	LDAPControl	**cctrls,
	int		*msgidp )
{
	BerElement	*ber;
	int rc;
	ber_int_t id;

	Debug( LDAP_DEBUG_TRACE, "ldap_ntlm_bind\n" );

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msgidp != NULL );

	if( msgidp == NULL ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return ld->ld_errno;
	}

	/* create a message to send */
	if ( (ber = ldap_alloc_ber_with_options( ld )) == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	assert( LBER_VALID( ber ) );

	LDAP_NEXT_MSGID( ld, id );
	rc = ber_printf( ber, "{it{istON}" /*}*/,
			 id, LDAP_REQ_BIND,
			 ld->ld_version, dn, tag,
			 cred );

	/* Put Server Controls */
	if( rc != LDAP_SUCCESS
			|| ldap_int_put_controls( ld, sctrls, ber ) != LDAP_SUCCESS ) {
		ber_free( ber, 1 );
		return ld->ld_errno;
	}

	if ( ber_printf( ber, /*{*/ "N}" ) == -1 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return ld->ld_errno;
	}

	/* send the message */
	*msgidp = ldap_send_initial_request( ld, LDAP_REQ_BIND, dn, ber, id );

	if(*msgidp < 0)
		return ld->ld_errno;

	return LDAP_SUCCESS;
}

int
ldap_parse_ntlm_bind_result(
	LDAP		*ld,
	LDAPMessage	*res,
	struct berval	*challenge)
{
	ber_int_t	errcode;
	ber_tag_t	tag;
	BerElement	*ber;

	Debug( LDAP_DEBUG_TRACE, "ldap_parse_ntlm_bind_result\n" );

	if ( ld == NULL || res == NULL ) {
		return LDAP_PARAM_ERROR;
	}

        assert( ld != NULL );
        assert( LDAP_VALID( ld ) );
        assert( res != NULL );

	if( res->lm_msgtype != LDAP_RES_BIND ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return ld->ld_errno;
	}

	if ( ld->ld_error ) {
		LDAP_FREE( ld->ld_error );
		ld->ld_error = NULL;
	}
	if ( ld->ld_matched ) {
		LDAP_FREE( ld->ld_matched );
		ld->ld_matched = NULL;
	}

	/* parse results */

	ber = ber_dup( res->lm_ber );

	if( ber == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	tag = ber_scanf( ber, "{ioa" /*}*/,
			 &errcode, challenge, &ld->ld_error );
	ber_free( ber, 0 );

	if( tag == LBER_ERROR ) {
		ld->ld_errno = LDAP_DECODING_ERROR;
		return ld->ld_errno;
	}

	ld->ld_errno = errcode;

	return( ld->ld_errno );
}
