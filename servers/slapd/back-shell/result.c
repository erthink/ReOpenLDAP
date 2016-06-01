/* result.c - shell backend result reading function */
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
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "slap.h"
#include "shell.h"

int
read_and_send_results(
    Operation	*op,
    SlapReply	*rs,
    FILE	*fp )
{
	int	bsize, len;
	char	*buf, *bp;
	char	line[BUFSIZ];
	char	ebuf[128];

	/* read in the result and send it along */
	buf = (char *) ch_malloc( BUFSIZ );
	buf[0] = '\0';
	bsize = BUFSIZ;
	bp = buf;
	while ( !feof(fp) ) {
		errno = 0;
		if ( fgets( line, sizeof(line), fp ) == NULL ) {
			if ( errno == EINTR ) continue;

			Debug( LDAP_DEBUG_ANY, "shell: fgets failed: %s (%d)\n",
				AC_STRERROR_R(errno, ebuf, sizeof ebuf), errno );
			break;
		}

		Debug( LDAP_DEBUG_SHELL, "shell search reading line (%s)\n",
		    line );

		/* ignore lines beginning with # (LDIFv1 comments) */
		if ( *line == '#' ) {
			continue;
		}

		/* ignore lines beginning with DEBUG: */
		if ( strncasecmp( line, "DEBUG:", 6 ) == 0 ) {
			continue;
		}

		len = strlen( line );
		while ( bp + len + 1 - buf > bsize ) {
			size_t offset = bp - buf;
			bsize += BUFSIZ;
			buf = (char *) ch_realloc( buf, bsize );
			bp = &buf[offset];
		}
		strcpy( bp, line );
		bp += len;

		/* line marked the end of an entry or result */
		if ( *line == '\n' ) {
			if ( strncasecmp( buf, "RESULT", 6 ) == 0 ) {
				break;
			}

			if ( (rs->sr_entry = str2entry( buf )) == NULL ) {
				Debug( LDAP_DEBUG_ANY, "str2entry(%s) failed\n",
				    buf );
			} else {
				rs->sr_attrs = op->oq_search.rs_attrs;
				rs->sr_flags = REP_ENTRY_MODIFIABLE;
				send_search_entry( op, rs );
				entry_free( rs->sr_entry );
				rs->sr_attrs = NULL;
			}

			bp = buf;
		}
	}
	(void) str2result( buf, &rs->sr_err, (char **)&rs->sr_matched, (char **)&rs->sr_text );

	/* otherwise, front end will send this result */
	if ( rs->sr_err != 0 || op->o_tag != LDAP_REQ_BIND ) {
		send_ldap_result( op, rs );
	}

	free( buf );

	return( rs->sr_err );
}

void
print_suffixes(
    FILE	*fp,
    Backend	*be
)
{
	int	i;

	for ( i = 0; be->be_suffix[i].bv_val != NULL; i++ ) {
		fprintf( fp, "suffix: %s\n", be->be_suffix[i].bv_val );
	}
}
