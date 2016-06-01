/* str2filter.c - parse an RFC 4515 string filter */
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/socket.h>

#include "slap.h"


Filter *
str2filter_x( Operation *op, const char *str )
{
	int rc;
	Filter	*f = NULL;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	const char *text = NULL;

	Debug( LDAP_DEBUG_FILTER, "str2filter \"%s\"\n", str );

	if ( str == NULL || *str == '\0' ) {
		return NULL;
	}

	ber_init2( ber, NULL, LBER_USE_DER );
	if ( op->o_tmpmemctx ) {
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );
	}

	rc = ldap_pvt_put_filter( ber, str );
	if( rc < 0 ) {
		goto done;
	}

	ber_reset( ber, 1 );

	rc = get_filter( op, ber, &f, &text );

done:
	ber_free_buf( ber );

	return f;
}

Filter *
str2filter( const char *str )
{
	Operation op = {0};
	Opheader ohdr = {0};

	op.o_hdr = &ohdr;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;

	return str2filter_x( &op, str );
}
