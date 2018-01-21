/* $ReOpenLDAP$ */
/* Copyright 2001-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>

#include <slap.h>
#include "back-monitor.h"

/*
 * At present, only rootdn can bind with simple bind
 */

int
monitor_back_bind( Operation *op, SlapReply *rs )
{
	Debug(LDAP_DEBUG_ARGS, "==> monitor_back_bind: dn: %s\n",
			op->o_req_dn.bv_val );

	if ( be_isroot_pw( op ) ) {
		return LDAP_SUCCESS;
	}

	rs->sr_err = LDAP_INVALID_CREDENTIALS;
	send_ldap_result( op, rs );

	return rs->sr_err;
}
