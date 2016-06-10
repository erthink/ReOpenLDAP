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
 * This program was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */

/*
 * LDAPv3 Cancel Operation Request
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
ldap_cancel(
	LDAP		*ld,
	int		cancelid,
	LDAPControl	**sctrls,
	LDAPControl	**cctrls,
	int		*msgidp )
{
	BerElement *cancelidber = NULL;
	struct berval *cancelidvalp = NULL;
	int rc;

	cancelidber = ber_alloc_t( LBER_USE_DER );
	ber_printf( cancelidber, "{i}", cancelid );
	ber_flatten( cancelidber, &cancelidvalp );
	rc = ldap_extended_operation( ld, LDAP_EXOP_CANCEL,
		cancelidvalp, sctrls, cctrls, msgidp );
	ber_free( cancelidber, 1 );
	return rc;
}

int
ldap_cancel_s(
	LDAP		*ld,
	int		cancelid,
	LDAPControl	**sctrls,
	LDAPControl	**cctrls )
{
	BerElement *cancelidber = NULL;
	struct berval *cancelidvalp = NULL;
	int rc;

	cancelidber = ber_alloc_t( LBER_USE_DER );
	ber_printf( cancelidber, "{i}", cancelid );
	ber_flatten( cancelidber, &cancelidvalp );
	rc = ldap_extended_operation_s( ld, LDAP_EXOP_CANCEL,
		cancelidvalp, sctrls, cctrls, NULL, NULL );
	ber_free( cancelidber, 1 );
	return rc;
}

