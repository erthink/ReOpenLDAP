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

/* str2filter.c - parse an RFC 4515 string filter */

#include "reldap.h"

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
