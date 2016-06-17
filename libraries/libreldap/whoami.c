/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS:
 * This program was orignally developed by Kurt D. Zeilenga for inclusion in
 * OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

/*
 * LDAP Who Am I? (Extended) Operation <draft-zeilenga-ldap-authzid-xx.txt>
 */

int ldap_parse_whoami(
	LDAP *ld,
	LDAPMessage *res,
	struct berval **authzid )
{
	int rc;
	char *retoid = NULL;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( res != NULL );
	assert( authzid != NULL );

	*authzid = NULL;

	rc = ldap_parse_extended_result( ld, res, &retoid, authzid, 0 );

	if( rc != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_parse_whoami" );
		return rc;
	}

	ber_memfree( retoid );
	return rc;
}

int
ldap_whoami( LDAP *ld,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	int				*msgidp )
{
	int rc;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msgidp != NULL );

	rc = ldap_extended_operation( ld, LDAP_EXOP_WHO_AM_I,
		NULL, sctrls, cctrls, msgidp );

	return rc;
}

int
ldap_whoami_s(
	LDAP *ld,
	struct berval **authzid,
	LDAPControl **sctrls,
	LDAPControl **cctrls )
{
	int		rc;
	int		msgid;
	LDAPMessage	*res;

	rc = ldap_whoami( ld, sctrls, cctrls, &msgid );
	if ( rc != LDAP_SUCCESS ) return rc;

	if ( ldap_result( ld, msgid, LDAP_MSG_ALL, (struct timeval *) NULL, &res ) == -1 || !res ) {
		return ld->ld_errno;
	}

	rc = ldap_parse_whoami( ld, res, authzid );
	if( rc != LDAP_SUCCESS ) {
		ldap_msgfree( res );
		return rc;
	}

	return( ldap_result2error( ld, res, 1 ) );
}
