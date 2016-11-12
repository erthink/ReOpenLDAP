/* $ReOpenLDAP$ */
/* Copyright 1999-2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

#ifdef ENABLE_REWRITE
int
ldap_back_dn_massage(
	dncookie	*dc,
	struct berval	*dn,
	struct berval	*res )
{
	int		rc = 0;
	static char	*dmy = "";

	switch ( rewrite_session( dc->target->mt_rwmap.rwm_rw, dc->ctx,
				( dn->bv_val ? dn->bv_val : dmy ),
				dc->conn, &res->bv_val ) )
	{
	case REWRITE_REGEXEC_OK:
		if ( res->bv_val != NULL ) {
			res->bv_len = strlen( res->bv_val );
		} else {
			*res = *dn;
		}
		Debug( LDAP_DEBUG_ARGS,
			"[rw] %s: \"%s\" -> \"%s\"\n",
			dc->ctx,
			BER_BVISNULL( dn ) ? "" : dn->bv_val,
			BER_BVISNULL( res ) ? "" : res->bv_val );
		rc = LDAP_SUCCESS;
		break;

 	case REWRITE_REGEXEC_UNWILLING:
		if ( dc->rs ) {
			dc->rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			dc->rs->sr_text = "Operation not allowed";
		}
		rc = LDAP_UNWILLING_TO_PERFORM;
		break;

	case REWRITE_REGEXEC_ERR:
		if ( dc->rs ) {
			dc->rs->sr_err = LDAP_OTHER;
			dc->rs->sr_text = "Rewrite error";
		}
		rc = LDAP_OTHER;
		break;
	}

	if ( res->bv_val == dmy ) {
		BER_BVZERO( res );
	}

	return rc;
}

#else
/*
 * ldap_back_dn_massage
 *
 * Aliases the suffix; based on suffix_alias (servers/slapd/suffixalias.c).
 */
int
ldap_back_dn_massage(
	dncookie *dc,
	struct berval *odn,
	struct berval *res
)
{
	int     i, src, dst;
	struct berval pretty = {0,NULL}, *dn = odn;

	assert( res != NULL );

	if ( dn == NULL ) {
		res->bv_val = NULL;
		res->bv_len = 0;
		return 0;
	}
	if ( dc->target->mt_rwmap.rwm_suffix_massage == NULL ) {
		*res = *dn;
		return 0;
	}

	if ( dc->tofrom ) {
		src = 0 + dc->normalized;
		dst = 2 + dc->normalized;
	} else {
		src = 2 + dc->normalized;
		dst = 0 + dc->normalized;
		/* DN from remote server may be in arbitrary form.
		 * Pretty it so we can parse reliably.
		 */
		dnPretty( NULL, dn, &pretty, NULL );
		if (pretty.bv_val) dn = &pretty;
	}

	for ( i = 0;
		dc->target->mt_rwmap.rwm_suffix_massage[i].bv_val != NULL;
		i += 4 ) {
		int aliasLength = dc->target->mt_rwmap.rwm_suffix_massage[i+src].bv_len;
		int diff = dn->bv_len - aliasLength;

		if ( diff < 0 ) {
			/* alias is longer than dn */
			continue;
		} else if ( diff > 0 && ( !DN_SEPARATOR(dn->bv_val[diff-1]))) {
			/* boundary is not at a DN separator */
			continue;
			/* At a DN Separator */
		}

		if ( !strcmp( dc->target->mt_rwmap.rwm_suffix_massage[i+src].bv_val, &dn->bv_val[diff] ) ) {
			res->bv_len = diff + dc->target->mt_rwmap.rwm_suffix_massage[i+dst].bv_len;
			res->bv_val = ch_malloc( res->bv_len + 1 );
			strncpy( res->bv_val, dn->bv_val, diff );
			strcpy( &res->bv_val[diff], dc->target->mt_rwmap.rwm_suffix_massage[i+dst].bv_val );
			Debug( LDAP_DEBUG_ARGS,
				"ldap_back_dn_massage:"
				" converted \"%s\" to \"%s\"\n",
				BER_BVISNULL( dn ) ? "" : dn->bv_val,
				BER_BVISNULL( res ) ? "" : res->bv_val );
			break;
		}
	}
	if (pretty.bv_val) {
		ch_free(pretty.bv_val);
		dn = odn;
	}
	/* Nothing matched, just return the original DN */
	if (res->bv_val == NULL) {
		*res = *dn;
	}

	return 0;
}
#endif /* !ENABLE_REWRITE */
