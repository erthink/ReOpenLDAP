/* operational.c - routines to deal with on-the-fly operational attrs */
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
 * Copyright 2001-2014 The OpenLDAP Foundation.
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

#include "portable.h"

#include "slap.h"

/*
 * helpers for on-the-fly operational attribute generation
 */

Attribute *
slap_operational_subschemaSubentry( Backend *be )
{
	Attribute	*a;

	/* The backend wants to take care of it */
	if ( be && !SLAP_FRONTEND(be) && be->be_schemadn.bv_val ) return NULL;

	a = attr_alloc( slap_schema.si_ad_subschemaSubentry );

	a->a_numvals = 1;
	a->a_vals = ch_malloc( 2 * sizeof( struct berval ) );
	ber_dupbv( a->a_vals, &frontendDB->be_schemadn );
	a->a_vals[1].bv_len = 0;
	a->a_vals[1].bv_val = NULL;

	a->a_nvals = ch_malloc( 2 * sizeof( struct berval ) );
	ber_dupbv( a->a_nvals, &frontendDB->be_schemandn );
	a->a_nvals[1].bv_len = 0;
	a->a_nvals[1].bv_val = NULL;

	return a;
}

Attribute *
slap_operational_entryDN( Entry *e )
{
	Attribute	*a;

	assert( e != NULL );
	assert( !BER_BVISNULL( &e->e_name ) );
	assert( !BER_BVISNULL( &e->e_nname ) );

	a = attr_alloc( slap_schema.si_ad_entryDN );

	a->a_numvals = 1;
	a->a_vals = ch_malloc( 2 * sizeof( struct berval ) );
	ber_dupbv( &a->a_vals[ 0 ], &e->e_name );
	BER_BVZERO( &a->a_vals[ 1 ] );

	a->a_nvals = ch_malloc( 2 * sizeof( struct berval ) );
	ber_dupbv( &a->a_nvals[ 0 ], &e->e_nname );
	BER_BVZERO( &a->a_nvals[ 1 ] );

	return a;
}

Attribute *
slap_operational_hasSubordinate( int hs )
{
	Attribute	*a;
	struct berval	val;

	val = hs ? slap_true_bv : slap_false_bv;

	a = attr_alloc( slap_schema.si_ad_hasSubordinates );
	a->a_numvals = 1;
	a->a_vals = ch_malloc( 2 * sizeof( struct berval ) );

	ber_dupbv( &a->a_vals[0], &val );
	a->a_vals[1].bv_val = NULL;

	a->a_nvals = a->a_vals;

	return a;
}
