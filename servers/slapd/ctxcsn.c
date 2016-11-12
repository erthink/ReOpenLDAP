/* $ReOpenLDAP$ */
/* Copyright 1990-2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* ctxcsn.c -- Context CSN Management Routines */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "lutil_ldap.h"

#define CTXCSN_ORDERING <

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

void slap_op_csn_free( Operation *op )
{
	if ( !BER_BVISNULL( &op->o_csn ) ) {
#if OP_CSN_CHECK
		assert(op->o_csn_master != NULL);
		if (op->o_csn_master == op) {
			op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
		}
		op->o_csn_master = NULL;
#else
		op->o_tmpfree( op->o_csn.bv_val, op->o_tmpmemctx );
#endif
		BER_BVZERO( &op->o_csn );
	}
}

void slap_op_csn_clean( Operation *op )
{
	if ( !BER_BVISNULL( &op->o_csn ) ) {
#if OP_CSN_CHECK
		assert(op->o_csn_master != NULL);
		if (op->o_csn_master == op) {
			op->o_csn.bv_val[0] = 0;
			op->o_csn.bv_len = 0;
		} else {
			BER_BVZERO( &op->o_csn );
			op->o_csn_master = NULL;
		}
#else
		op->o_csn.bv_val[0] = 0;
		op->o_csn.bv_len = 0;
#endif
	}
}

void slap_op_csn_assign( Operation *op, BerValue *csn )
{
#if OP_CSN_CHECK
	assert(op->o_csn_master == op || op->o_csn_master == NULL);
	if (op->o_csn_master != op)
		BER_BVZERO( &op->o_csn );
	op->o_csn_master = op;
#endif
	ber_bvreplace_x( &op->o_csn, csn, op->o_tmpmemctx );
}

/* maxcsn->bv_val must point to a char buf[LDAP_PVT_CSNSTR_BUFSIZE] */
int
slap_get_commit_csn(
	Operation *op,
	struct berval *maxcsn )
{
	struct slap_csn_entry *csne, *self;
	BackendDB *be = op->o_bd->bd_self;
	int sid = -1;

	if ( !BER_BVISEMPTY( &op->o_csn )) {
		sid = slap_csn_get_sid( &op->o_csn );
		assert( sid >= 0 );
	}

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	self = NULL;
	LDAP_TAILQ_FOREACH( csne, be->be_pending_csn_list, ce_csn_link ) {
		if ( csne->ce_opid == op->o_opid && csne->ce_connid == op->o_connid ) {
			assert( sid < 0 || sid == csne->ce_sid );
			self = csne;
			csne->ce_state = SLAP_CSN_COMMIT;
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
#ifdef CTXCSN_ORDERING
					if ( committed_csne
							&& slap_csn_compare_ts( &csne->ce_csn,
								&committed_csne->ce_csn ) CTXCSN_ORDERING 0 ) {
						Debug( LDAP_DEBUG_SYNC, "slap_get_commit_csn:"
							"  next-pending %p (conn %ld, opid %ld) %s\n\t"
							" prev-commited %p (conn %ld, opid %ld) %s\n",
							csne, csne->ce_connid, csne->ce_opid, csne->ce_csn.bv_val,
							committed_csne, committed_csne->ce_connid, committed_csne->ce_opid, committed_csne->ce_csn.bv_val
						);
						assert( 0 );
					}
#endif
					if ( csne->ce_state == SLAP_CSN_COMMIT ) {
						committed_csne = csne;
					} else if ( likely(csne->ce_state == SLAP_CSN_PENDING) ) {
						break;
					} else {
						LDAP_BUG();
					}
					if (csne == self) {
						/* LY: don't go ahead */
						break;
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
			Debug( LDAP_DEBUG_SYNC, "slap_graduate_commit_csn: removing %p (conn %ld, opid %ld) %s, op %p\n",
				csne, csne->ce_connid, csne->ce_opid, csne->ce_csn.bv_val, op );
			assert( csne->ce_state > 0 );
			assert( slap_csn_match( &op->o_csn, &csne->ce_csn ) );
			found = csne->ce_state;
			/* assert(found != 0); LY: op could be cancelled/abandoned before slap_get_commit_csn() */
			LDAP_TAILQ_REMOVE( be->be_pending_csn_list, csne, ce_csn_link );
			slap_op_csn_clean( op );
			ch_free( csne->ce_csn.bv_val );
			ch_free( csne );
#if OP_CSN_CHECK
			assert(be->be_pending_csn_count > 0);
			be->be_pending_csn_count--;
#endif
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
	struct slap_csn_entry *pending, *before;
	BackendDB *be = op->o_bd->bd_self;
	int sid = slap_csn_get_sid( csn );
	assert(sid > -1);

	ldap_pvt_thread_mutex_lock( &be->be_pcl_mutex );

	before = NULL;
	LDAP_TAILQ_FOREACH( pending, be->be_pending_csn_list, ce_csn_link ) {
#ifdef CTXCSN_ORDERING
		if ( pending->ce_sid == sid && ! before
				&& slap_csn_compare_ts( csn, &pending->ce_csn ) CTXCSN_ORDERING 0 )
			before = pending;
#endif
		if ( pending->ce_opid == op->o_opid && pending->ce_connid == op->o_connid ) {
			assert( pending->ce_sid == sid );
			if ( reopenldap_mode_check() )
				LDAP_BUG();
			break;
		}
	}

	if (likely( pending == NULL )) {
		pending = (struct slap_csn_entry *) ch_calloc( 1,
				sizeof( struct slap_csn_entry ));
		ber_dupbv( &pending->ce_csn, csn );
		assert( BER_BVISEMPTY( &op->o_csn ));
		slap_op_csn_assign( op, &pending->ce_csn );
		pending->ce_sid = sid;
		pending->ce_connid = op->o_connid;
		pending->ce_opid = op->o_opid;
		pending->ce_state = SLAP_CSN_PENDING;
		if ( before == NULL ) {
			Debug( LDAP_DEBUG_SYNC, "slap_queue_csn: tail-queueing %p (conn %ld, opid %ld) %s, op %p\n",
				   pending, pending->ce_connid, pending->ce_opid, pending->ce_csn.bv_val, op);
			LDAP_TAILQ_INSERT_TAIL( be->be_pending_csn_list, pending, ce_csn_link );
		} else {
			Debug( LDAP_DEBUG_SYNC, "slap_queue_csn: middle-queueing %p (conn %ld, opid %ld) %s, op %p\n"
				   "\tbefore %p(conn %ld, opid %ld) %s\n",
				   pending, pending->ce_connid, pending->ce_opid, pending->ce_csn.bv_val, op,
				   before, before->ce_connid, before->ce_opid, before->ce_csn.bv_val);
			LDAP_TAILQ_INSERT_BEFORE( before, pending, ce_csn_link );
		}
#if OP_CSN_CHECK
		be->be_pending_csn_count++;
		assert(be->be_pending_csn_count < 42);
#endif
	}

	ldap_pvt_thread_mutex_unlock( &be->be_pcl_mutex );
}

int
slap_get_csn(
	Operation *op,
	struct berval *csn )
{
	char buf[ LDAP_PVT_CSNSTR_BUFSIZE ];
	BerValue local = {sizeof(buf), buf};

	if ( csn == NULL ) {
		csn = &local;
		assert(op != NULL);
	}

	csn->bv_len = ldap_pvt_csnstr( csn->bv_val, csn->bv_len, slap_serverID, 0 );
	if ( op )
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
