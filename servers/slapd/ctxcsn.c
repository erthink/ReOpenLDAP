/* ctxcsn.c -- Context CSN Management Routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2015 The OpenLDAP Foundation.
 * Portions Copyright 2003 IBM Corporation.
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
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "lutil_ldap.h"

struct slap_csn_entry {
	struct berval ce_csn;
	int ce_sid;
#define SLAP_CSN_PENDING	1
#define SLAP_CSN_COMMIT		2
	int ce_state;
	unsigned long ce_opid;
	unsigned long ce_connid;
	LDAP_TAILQ_ENTRY (slap_csn_entry) ce_csn_link;
};

const struct berval slap_ldapsync_bv = BER_BVC("ldapsync");
const struct berval slap_ldapsync_cn_bv = BER_BVC("cn=ldapsync");
int slap_serverID;

/* maxcsn->bv_val must point to a char buf[LDAP_PVT_CSNSTR_BUFSIZE] */
int
slap_get_commit_csn(
	Operation *op,
	struct berval *maxcsn )
{
	struct slap_csn_entry *csne;
	BackendDB *be = op->o_bd->bd_self;
	int sid = -1;

	if ( !BER_BVISEMPTY( &op->o_csn )) {
		sid = slap_parse_csn_sid( &op->o_csn );
		assert( sid >= 0 );
	}

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			assert( sid < 0 || sid == csne->ce_sid );
			csne->ce_state = SLAP_CSN_COMMIT;
			sid = csne->ce_sid;
			break;
		}
	}

	if ( maxcsn ) {
		struct slap_csn_entry *committed_csne = NULL;
		assert( maxcsn->bv_val != NULL );
		assert( maxcsn->bv_len >= LDAP_PVT_CSNSTR_BUFSIZE );
		if (sid >= 0) {
			LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
				if ( sid == csne->ce_sid ) {
					if ( csne->ce_state == SLAP_CSN_COMMIT ) {
						committed_csne = csne;
					} else if ( likely(csne->ce_state == SLAP_CSN_PENDING) ) {
						break;
					} else {
						LDAP_BUG();
					}
				}
			}
		}
		if ( committed_csne ) {
			maxcsn->bv_len = committed_csne->ce_csn.bv_len;
			memcpy( maxcsn->bv_val, committed_csne->ce_csn.bv_val, maxcsn->bv_len+1 );
		} else {
			maxcsn->bv_len = 0;
			maxcsn->bv_val[0] = 0;
		}
	}

	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );
	return sid;
}

int
slap_graduate_commit_csn( Operation *op )
{
	struct slap_csn_entry *csne;
	BackendDB *be;
	int found = 0;

	if ( op == NULL ) return found;
	if ( op->o_bd == NULL ) return found;
	be = op->o_bd->bd_self;
	if (! be->be_pending_csn_list) return found;

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			Debug( LDAP_DEBUG_SYNC, "slap_graduate_commit_csn: removing %p (conn %ld, opid %ld) %s\n",
				csne, csne->ce_connid, csne->ce_opid, csne->ce_csn.bv_val );
			assert( csne->ce_state > 0 );
			assert( slap_csn_match( &op->o_csn, &csne->ce_csn ) );
			found = csne->ce_state;
			LDAP_TAILQ_REMOVE( be->be_pending_csn_list, csne, ce_csn_link );
			if ( op->o_csn.bv_val == csne->ce_csn.bv_val ) {
				BER_BVZERO( &op->o_csn );
			}
			ch_free( csne->ce_csn.bv_val );
			ch_free( csne );
			break;
		}
	}

	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );

	return found;
}

static struct berval ocbva[] = {
	BER_BVC("top"),
	BER_BVC("subentry"),
	BER_BVC("syncProviderSubentry"),
	BER_BVNULL
};

Entry *
slap_create_context_csn_entry(
	Backend *be,
	struct berval *context_csn )
{
	Entry* e;

	struct berval bv;

	e = entry_alloc();

	attr_merge( e, slap_schema.si_ad_objectClass,
		ocbva, NULL );
	attr_merge_one( e, slap_schema.si_ad_structuralObjectClass,
		&ocbva[1], NULL );
	attr_merge_one( e, slap_schema.si_ad_cn,
		(struct berval *)&slap_ldapsync_bv, NULL );

	if ( context_csn ) {
		attr_merge_one( e, slap_schema.si_ad_contextCSN,
			context_csn, NULL );
	}

	BER_BVSTR( &bv, "{}" );
	attr_merge_one( e, slap_schema.si_ad_subtreeSpecification, &bv, NULL );

	build_new_dn( &e->e_name, &be->be_nsuffix[0],
		(struct berval *)&slap_ldapsync_cn_bv, NULL );
	ber_dupbv( &e->e_nname, &e->e_name );

	return e;
}

void
slap_queue_csn(
	Operation *op,
	struct berval *csn )
{
	struct slap_csn_entry *pending;
	BackendDB *be = op->o_bd->bd_self;
	int sid = slap_parse_csn_sid( csn );
	assert(sid > -1);

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	LDAP_TAILQ_FOREACH( pending, be->be_pending_csn_list, ce_csn_link ) {
		if ( pending->ce_opid == op->o_opid && pending->ce_connid == op->o_connid ) {
			assert( pending->ce_sid == sid );
			if ( reopenldap_mode_idkfa() )
				LDAP_BUG();
			break;
		}
	}

	if (likely(pending == NULL)) {
		pending = (struct slap_csn_entry *) ch_calloc( 1,
				sizeof( struct slap_csn_entry ));
		Debug( LDAP_DEBUG_SYNC, "slap_queue_csn: queueing %p (conn %ld, opid %ld) %s\n",
			   pending, op->o_connid, op->o_opid, csn->bv_val );
		ber_dupbv( &pending->ce_csn, csn );
		assert( BER_BVISNULL( &op->o_csn ));
		ber_bvreplace_x( &op->o_csn, &pending->ce_csn, op->o_tmpmemctx );
		pending->ce_sid = sid;
		pending->ce_connid = op->o_connid;
		pending->ce_opid = op->o_opid;
		pending->ce_state = SLAP_CSN_PENDING;
		LDAP_TAILQ_INSERT_TAIL( be->be_pending_csn_list,
			pending, ce_csn_link );
	}

	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );
}

int
slap_get_csn(
	Operation *op,
	struct berval *csn,
	int manage_ctxcsn )
{
	if ( csn == NULL ) return LDAP_OTHER;

	csn->bv_len = ldap_pvt_csnstr( csn->bv_val, csn->bv_len, slap_serverID, 0 );
	if ( manage_ctxcsn )
		slap_queue_csn( op, csn );

	return LDAP_SUCCESS;
}

void slap_free_commit_csn_list( struct be_pcl *list)
{
	struct slap_csn_entry *csne;

	csne = LDAP_TAILQ_FIRST( list );
	while ( csne ) {
		struct slap_csn_entry *tmp_csne = csne;

		LDAP_TAILQ_REMOVE( list, csne, ce_csn_link );
		ch_free( csne->ce_csn.bv_val );
		csne = LDAP_TAILQ_NEXT( csne, ce_csn_link );
		ch_free( tmp_csne );
	}
	ch_free( list );
}
