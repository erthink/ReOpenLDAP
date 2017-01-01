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

/* ACKNOWLEDGEMENTS:
 * 	Juan C. Gomez
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

/*
 * A modify rdn request looks like this:
 *	ModifyRDNRequest ::= SEQUENCE {
 *		entry		DistinguishedName,
 *		newrdn		RelativeDistinguishedName,
 *		deleteoldrdn	BOOLEAN
 *		newSuperior	[0] DistinguishedName	[v3 only]
 *	}
 */

BerElement *
ldap_build_moddn_req(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *newrdn,
	LDAP_CONST char *newSuperior,
	int deleteoldrdn,
	LDAPControl **sctrls,
	LDAPControl **cctrls,
	ber_int_t *msgidp )
{
	BerElement	*ber;
	int rc;

	/* create a message to send */
	if ( (ber = ldap_alloc_ber_with_options( ld )) == NULL ) {
		return( NULL );
	}

	LDAP_NEXT_MSGID( ld, *msgidp );
	if( newSuperior != NULL ) {
		/* must be version 3 (or greater) */
		if ( ld->ld_version < LDAP_VERSION3 ) {
			ld->ld_errno = LDAP_NOT_SUPPORTED;
			ber_free( ber, 1 );
			return( NULL );
		}
		rc = ber_printf( ber, "{it{ssbtsN}", /* '}' */
			*msgidp, LDAP_REQ_MODDN,
			dn, newrdn, (ber_int_t) deleteoldrdn,
			LDAP_TAG_NEWSUPERIOR, newSuperior );

	} else {
		rc = ber_printf( ber, "{it{ssbN}", /* '}' */
			*msgidp, LDAP_REQ_MODDN,
			dn, newrdn, (ber_int_t) deleteoldrdn );
	}

	if ( rc < 0 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( NULL );
	}

	/* Put Server Controls */
	if( ldap_int_put_controls( ld, sctrls, ber ) != LDAP_SUCCESS ) {
		ber_free( ber, 1 );
		return( NULL );
	}

	rc = ber_printf( ber, /*{*/ "N}" );
	if ( rc < 0 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( NULL );
	}

	return( ber );
}

/*
 * ldap_rename - initiate an ldap extended modifyDN operation.
 *
 * Parameters:
 *	ld				LDAP descriptor
 *	dn				DN of the object to modify
 *	newrdn			RDN to give the object
 *	deleteoldrdn	nonzero means to delete old rdn values from the entry
 *	newSuperior		DN of the new parent if applicable
 *
 * Returns the LDAP error code.
 */

int
ldap_rename(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *newrdn,
	LDAP_CONST char *newSuperior,
	int deleteoldrdn,
	LDAPControl **sctrls,
	LDAPControl **cctrls,
	int *msgidp )
{
	BerElement	*ber;
	int rc;
	ber_int_t id;

	Debug( LDAP_DEBUG_TRACE, "ldap_rename\n" );

	/* check client controls */
	rc = ldap_int_client_controls( ld, cctrls );
	if( rc != LDAP_SUCCESS ) return rc;

	ber = ldap_build_moddn_req( ld, dn, newrdn, newSuperior,
		deleteoldrdn, sctrls, cctrls, &id );
	if( !ber )
		return ld->ld_errno;

	/* send the message */
	*msgidp = ldap_send_initial_request( ld, LDAP_REQ_MODRDN, dn, ber, id );

	if( *msgidp < 0 ) {
		return( ld->ld_errno );
	}

	return LDAP_SUCCESS;
}

int
ldap_rename_s(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *newrdn,
	LDAP_CONST char *newSuperior,
	int deleteoldrdn,
	LDAPControl **sctrls,
	LDAPControl **cctrls )
{
	int rc;
	int msgid = 0;
	LDAPMessage *res;

	rc = ldap_rename( ld, dn, newrdn, newSuperior,
		deleteoldrdn, sctrls, cctrls, &msgid );

	if( rc != LDAP_SUCCESS ) {
		return rc;
	}

	rc = ldap_result( ld, msgid, LDAP_MSG_ALL, NULL, &res );

	if( rc == -1 || !res ) {
		return ld->ld_errno;
	}

	return ldap_result2error( ld, res, 1 );
}
