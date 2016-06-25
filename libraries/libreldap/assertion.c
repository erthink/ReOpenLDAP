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

#include "reldap.h"

#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

int
ldap_create_assertion_control_value(
	LDAP		*ld,
	char		*assertion,
	struct berval	*value )
{
	BerElement		*ber = NULL;
	int			err;

	if ( assertion == NULL || assertion[ 0 ] == '\0' ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return ld->ld_errno;
	}

	if ( value == NULL ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return ld->ld_errno;
	}

	BER_BVZERO( value );

	ber = ldap_alloc_ber_with_options( ld );
	if ( ber == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	err = ldap_pvt_put_filter( ber, assertion );
	if ( err < 0 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		goto done;
	}

	err = ber_flatten2( ber, value, 1 );
	if ( err < 0 ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		goto done;
	}

done:;
	if ( ber != NULL ) {
		ber_free( ber, 1 );
	}

	return ld->ld_errno;
}

int
ldap_create_assertion_control(
	LDAP		*ld,
	char		*assertion,
	int		iscritical,
	LDAPControl	**ctrlp )
{
	struct berval	value;

	if ( ctrlp == NULL ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return ld->ld_errno;
	}

	ld->ld_errno = ldap_create_assertion_control_value( ld,
		assertion, &value );
	if ( ld->ld_errno == LDAP_SUCCESS ) {
		ld->ld_errno = ldap_control_create( LDAP_CONTROL_ASSERT,
			iscritical, &value, 0, ctrlp );
		if ( ld->ld_errno != LDAP_SUCCESS ) {
			LDAP_FREE( value.bv_val );
		}
	}

	return ld->ld_errno;
}

