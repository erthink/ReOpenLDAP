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
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lber.h>
#include <ldif.h>
#include <lutil.h>

#include "slapcommon.h"

int
slapdn( int argc, char **argv )
{
	int			rc = 0;
	const char		*progname = "slapdn";

	slap_tool_init( progname, SLAPDN, argc, argv );

	argv = &argv[ optind ];
	argc -= optind;

	for ( ; argc--; argv++ ) {
		struct berval	dn,
				pdn = BER_BVNULL,
				ndn = BER_BVNULL;

		ber_str2bv( argv[ 0 ], 0, 0, &dn );

		switch ( dn_mode ) {
		case SLAP_TOOL_LDAPDN_PRETTY:
			rc = dnPretty( NULL, &dn, &pdn, NULL );
			break;

		case SLAP_TOOL_LDAPDN_NORMAL:
			rc = dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL );
			break;

		default:
			rc = dnPrettyNormal( NULL, &dn, &pdn, &ndn, NULL );
			break;
		}

		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "DN: <%s> check failed %d (%s)\n",
					dn.bv_val, rc,
					ldap_err2string( rc ) );
			if ( !continuemode ) {
				rc = -1;
				break;
			}

		} else {
			switch ( dn_mode ) {
			case SLAP_TOOL_LDAPDN_PRETTY:
				printf( "%s\n", pdn.bv_val );
				break;

			case SLAP_TOOL_LDAPDN_NORMAL:
				printf( "%s\n", ndn.bv_val );
				break;

			default:
				printf( "DN: <%s> check succeeded\n"
						"normalized: <%s>\n"
						"pretty:     <%s>\n",
						dn.bv_val,
						ndn.bv_val, pdn.bv_val );
				break;
			}

			ch_free( ndn.bv_val );
			ch_free( pdn.bv_val );
		}
	}

	if ( slap_tool_destroy())
		rc = EXIT_FAILURE;

	return rc;
}
