/* extended.c - bdb backend extended routines */
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
 * Copyright 2000-2014 The OpenLDAP Foundation.
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

#include <stdio.h>
#include <ac/string.h>

#include "back-bdb.h"
#include "lber_pvt.h"

static struct exop {
	struct berval *oid;
	BI_op_extended	*extended;
} exop_table[] = {
	{ NULL, NULL }
};

int
bdb_extended( Operation *op, SlapReply *rs )
/*	struct berval		*reqoid,
	struct berval	*reqdata,
	char		**rspoid,
	struct berval	**rspdata,
	LDAPControl *** rspctrls,
	const char**	text,
	BerVarray	*refs
) */
{
	int i;

	for( i=0; exop_table[i].extended != NULL; i++ ) {
		if( ber_bvcmp( exop_table[i].oid, &op->oq_extended.rs_reqoid ) == 0 ) {
			return (exop_table[i].extended)( op, rs );
		}
	}

	rs->sr_text = "not supported within naming context (bdb)";
	return rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
}

