/* proxyOld.c - module for supporting obsolete (rev 05) proxyAuthz control */
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
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions Copyright 2005 by Howard Chu, Symas Corp.
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

#include <reldap.h>

#include <slap.h>

#include <lber.h>
/*
#include <lber_pvt.h>
#include <lutil.h>
*/

/* This code is based on draft-weltman-ldapv3-proxy-05. There are a lot
 * of holes in that draft, it doesn't specify that the control is legal
 * for Add operations, and it makes no mention of Extended operations.
 * It also doesn't specify whether an empty LDAPDN is allowed in the
 * control value.
 *
 * For usability purposes, we're copying the op / exop behavior from the
 * newer -12 draft.
 */
#define LDAP_CONTROL_PROXY_AUTHZ05	"2.16.840.1.113730.3.4.12"

static const char* const proxyOld_extops[] = {
	LDAP_EXOP_MODIFY_PASSWD,
	LDAP_EXOP_X_WHO_AM_I,
	NULL
};

static int
proxyOld_parse(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	int rc;
	BerElement	*ber;
	ber_tag_t	tag;
	struct berval dn = BER_BVNULL;
	struct berval authzDN = BER_BVNULL;


	/* We hijack the flag for the new control. Clearly only one or the
	 * other can be used at any given time.
	 */
	if ( op->o_proxy_authz != SLAP_CONTROL_NONE ) {
		rs->sr_text = "proxy authorization control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_proxy_authz = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	/* Parse the control value
	 *  proxyAuthzControlValue ::= SEQUENCE {
	 *		proxyDN	LDAPDN
	 *	}
	 */
	ber = ber_init( &ctrl->ldctl_value );
	if ( ber == NULL ) {
		rs->sr_text = "ber_init failed";
		return LDAP_OTHER;
	}

	tag = ber_scanf( ber, "{m}", &dn );

	if ( tag == LBER_ERROR ) {
		rs->sr_text = "proxyOld control could not be decoded";
		rc = LDAP_OTHER;
		goto done;
	}
	if ( BER_BVISEMPTY( &dn )) {
		Debug( LDAP_DEBUG_TRACE,
			"proxyOld_parse: conn=%lu anonymous\n",
				op->o_connid );
		authzDN.bv_val = ch_strdup("");
	} else {
		Debug( LDAP_DEBUG_ARGS,
			"proxyOld_parse: conn %lu ctrl DN=\"%s\"\n",
				op->o_connid, dn.bv_val );
		rc = dnNormalize( 0, NULL, NULL, &dn, &authzDN, op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			goto done;
		}
		rc = slap_sasl_authorized( op, &op->o_ndn, &authzDN );
		if ( rc ) {
			op->o_tmpfree( authzDN.bv_val, op->o_tmpmemctx );
			rs->sr_text = "not authorized to assume identity";
			/* new spec uses LDAP_PROXY_AUTHZ_FAILURE */
			rc = LDAP_INSUFFICIENT_ACCESS;
			goto done;
		}
	}
	free( op->o_ndn.bv_val );
	free( op->o_dn.bv_val );
	op->o_ndn = authzDN;
	ber_dupbv( &op->o_dn, &authzDN );

	Statslog( LDAP_DEBUG_STATS, "conn=%lu op=%lu PROXYOLD dn=\"%s\"\n",
		op->o_connid, op->o_opid,
		authzDN.bv_len ? authzDN.bv_val : "anonymous" );
	rc = LDAP_SUCCESS;
done:
	ber_free( ber, 1 );
	return rc;
}

SLAP_OVERLAY_ENTRY(proxyOld, modinit) ( int argc, char *argv[] )
{
	return register_supported_control( LDAP_CONTROL_PROXY_AUTHZ05,
		SLAP_CTRL_GLOBAL|SLAP_CTRL_HIDE|SLAP_CTRL_ACCESS, proxyOld_extops,
		proxyOld_parse, NULL );
}
