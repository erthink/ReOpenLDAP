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
 * Copyright 2005-2014 The OpenLDAP Foundation.
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

/*
 * LDAPv3 Turn Operation Request
 */

#include "portable.h"

#include <stdio.h>
#include <ac/stdlib.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"
#include "ldap_log.h"

int
ldap_turn(
	LDAP *ld,
	int mutual,
	LDAP_CONST char* identifier,
	LDAPControl **sctrls,
	LDAPControl **cctrls,
	int *msgidp )
{
#ifdef LDAP_EXOP_X_TURN
	BerElement *turnvalber = NULL;
	struct berval *turnvalp = NULL;
	int rc;

	turnvalber = ber_alloc_t( LBER_USE_DER );
	if( mutual ) {
		ber_printf( turnvalber, "{bs}", mutual, identifier );
	} else {
		ber_printf( turnvalber, "{s}", identifier );
	}
	ber_flatten( turnvalber, &turnvalp );

	rc = ldap_extended_operation( ld, LDAP_EXOP_X_TURN,
			turnvalp, sctrls, cctrls, msgidp );
	ber_free( turnvalber, 1 );
	return rc;
#else
	return LDAP_CONTROL_NOT_FOUND;
#endif
}

int
ldap_turn_s(
	LDAP *ld,
	int mutual,
	LDAP_CONST char* identifier,
	LDAPControl **sctrls,
	LDAPControl **cctrls )
{
#ifdef LDAP_EXOP_X_TURN
	BerElement *turnvalber = NULL;
	struct berval *turnvalp = NULL;
	int rc;

	turnvalber = ber_alloc_t( LBER_USE_DER );
	if( mutual ) {
		ber_printf( turnvalber, "{bs}", 0xFF, identifier );
	} else {
		ber_printf( turnvalber, "{s}", identifier );
	}
	ber_flatten( turnvalber, &turnvalp );

	rc = ldap_extended_operation_s( ld, LDAP_EXOP_X_TURN,
			turnvalp, sctrls, cctrls, NULL, NULL );
	ber_free( turnvalber, 1 );
	return rc;
#else
	return LDAP_CONTROL_NOT_FOUND;
#endif
}

