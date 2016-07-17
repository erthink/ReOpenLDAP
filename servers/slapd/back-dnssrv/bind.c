/* bind.c - DNS SRV backend bind function */
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
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
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
 * This work was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */


#include "reldap.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "proto-dnssrv.h"

int
dnssrv_back_bind(
	Operation	*op,
	SlapReply	*rs )
{
	Debug( LDAP_DEBUG_TRACE, "DNSSRV: bind dn=\"%s\" (%d)\n",
		BER_BVISNULL( &op->o_req_dn ) ? "" : op->o_req_dn.bv_val,
		op->orb_method );

	/* allow rootdn as a means to auth without the need to actually
 	 * contact the proxied DSA */
	switch ( be_rootdn_bind( op, NULL ) ) {
	case LDAP_SUCCESS:
		/* frontend will send result */
		return rs->sr_err;

	default:
		/* treat failure and like any other bind, otherwise
		 * it could reveal the DN of the rootdn */
		break;
	}

	if ( !BER_BVISNULL( &op->orb_cred ) &&
		!BER_BVISEMPTY( &op->orb_cred ) )
	{
		/* simple bind */
		Statslog( LDAP_DEBUG_STATS,
		   	"%s DNSSRV BIND dn=\"%s\" provided cleartext passwd\n",
	   		op->o_log_prefix,
			BER_BVISNULL( &op->o_req_dn ) ? "" : op->o_req_dn.bv_val  );

		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"you shouldn't send strangers your password" );

	} else {
		/* unauthenticated bind */
		/* NOTE: we're not going to get here anyway:
		 * unauthenticated bind is dealt with by the frontend */
		Debug( LDAP_DEBUG_TRACE, "DNSSRV: BIND dn=\"%s\"\n",
			BER_BVISNULL( &op->o_req_dn ) ? "" : op->o_req_dn.bv_val );

		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"anonymous bind expected" );
	}

	return 1;
}
