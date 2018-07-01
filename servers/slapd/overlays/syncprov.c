/* $ReOpenLDAP$ */
/* Copyright 2004-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

/* syncprov.c - syncrepl provider */

#include "reldap.h"

#ifdef SLAPD_OVER_SYNCPROV

#include <ac/string.h>
#include "lutil.h"
#include "slap.h"
#include "slapconfig.h"
#include "ldap_rq.h"

/* A modify request on a particular entry */
typedef struct modinst {
	struct modinst *mi_next;
	Operation *mi_op;
} modinst;

typedef struct modtarget {
	struct modinst *mt_mods;
	struct modinst *mt_tail;
	struct berval mt_dn;
	ldap_pvt_thread_mutex_t mt_mutex;
} modtarget;

/* All the info of a psearch result that's shared between
 * multiple queues
 */
typedef struct resinfo {
	struct reslink *ri_list;
	Entry *ri_e;
	struct berval ri_dn;
	struct berval ri_ndn;
	struct berval ri_uuid;
	struct berval ri_csn;
	struct berval ri_cookie;
	char ri_isref;
	ldap_pvt_thread_mutex_t ri_mutex;
} resinfo;

/* A queued result of a persistent search */
typedef struct reslink {
	struct reslink *rl_next;	/* list of results on this psearch queue */
	struct reslink *rl_list;	/* list of psearches using this result */
	resinfo *rl_info;
	char rl_mode;
} reslink;

/* Record of a persistent search */
typedef struct syncops {
	struct syncops *s_next;
	struct syncprov_info_t *s_si;
	Operation	*s_op;		/* search op */
	int		s_rid;
	int		s_sid;
	struct berval	s_base;		/* ndn of search base */
	ID		s_eid;		/* entryID of search base */
	struct berval s_filterstr;

	ldap_pvt_thread_mutex_t	s_mutex;
	int		s_flags;	/* search status and reference bits */
#define	PS_IS_REFRESHING	0x01
#define	PS_IS_DETACHED		0x02
#define	PS_WROTE_BASE		0x04
#define	PS_FIND_BASE		0x08
#define	PS_FIX_FILTER		0x10
#define	PS_LOST_BASE		0x20
#define	PS_DEAD				0x40

#define	OS_REF_PLAYBACK		0x0100
#define	OS_REF_OP_SEARCH	0x0200
#define	OS_REF_OP_MATCH		0x0400
#define	OS_REF_PREPARE		0x0800
#define	OS_REF_ABANDON		0x1000
#define	OS_REF_UNLINK		0x2000
#define	OS_REF_CLOSE		0x4000
#define	OS_REF_MASK			0x7F00

#define	PS_TASK_QUEUED		OS_REF_PLAYBACK

	int		s_matchops_inuse;	/* reference count from matchops */
	struct reslink *s_rl;
	struct reslink *s_rltail;
	void *s_pool_cookie;

	Operation *s_op_safe, s_slap_op_copy;
} syncops;

#ifdef SLAP_NO_SL_MALLOC
#	warning "syncprov: is unstable without ls-malloc and has a few memory leaks"
#endif /* SLAP_NO_SL_MALLOC */

/* LY: safely check is it abandoned, without deref so->s_op */
static ATTRIBUTE_NO_SANITIZE_THREAD_INLINE
int is_syncops_abandoned(const syncops *so)
{
	return so->s_next == so || slap_get_op_abandon(so->s_op);
}

/* A received sync control */
typedef struct sync_control {
	struct sync_cookie sr_state;
	int sr_rhint;
	int sr_jammed;
} sync_control;

#define SLAP_SYNC_NONE					(LDAP_SYNC_NONE<<SLAP_CONTROL_SHIFT)
#define SLAP_SYNC_REFRESH				(LDAP_SYNC_REFRESH_ONLY<<SLAP_CONTROL_SHIFT)
#define SLAP_SYNC_PERSIST				(LDAP_SYNC_RESERVED<<SLAP_CONTROL_SHIFT)
#define SLAP_SYNC_REFRESH_AND_PERSIST	(LDAP_SYNC_REFRESH_AND_PERSIST<<SLAP_CONTROL_SHIFT)

/* Record of which searches matched at premodify step */
typedef struct syncmatches {
	struct syncmatches *sm_next;
	syncops *sm_op;
} syncmatches;

/* Session log data */
typedef struct slog_entry {
	struct slog_entry *se_next;
	struct berval se_uuid;
	struct berval se_csn;
	int	se_sid;
	ber_tag_t	se_tag;
} slog_entry;

typedef struct sessionlog {
	struct sync_cookie sl_cookie;
	int		sl_num;
	int		sl_size;
	int		sl_playing;
	slog_entry *sl_head;
	slog_entry *sl_tail;
	ldap_pvt_thread_mutex_t sl_mutex;
} sessionlog;

/* The main state for this overlay */
typedef struct syncprov_info_t {
	syncops		*si_ops;
	struct berval	si_contextdn;
	struct sync_cookie si_cookie;
	int		si_chkops;	/* checkpointing info */
	int		si_chktime;
	int		si_numops;	/* number of ops since last checkpoint */
	char	si_nopres;	/* Skip present phase */
	char	si_usehint;	/* use reload hint */
	char	si_showstatus;	/* add a mock contextCSN if sync is not perfect */
	int		si_active;	/* >0 if there are active mods */
	int		si_dirty;	/* True if the context is dirty, i.e changes
						 * have been made without updating the csn. */
	time_t	si_chklast;	/* time of last checkpoint */
	Avlnode	*si_mods;	/* entries being modified */
	sessionlog	*si_logs;
	volatile int si_psearches;
	volatile int si_prefresh;
	ldap_pvt_thread_rdwr_t	si_csn_rwlock;
	ldap_pvt_thread_mutex_t	si_ops_mutex;
	ldap_pvt_thread_mutex_t	si_mods_mutex;
	ldap_pvt_thread_mutex_t	si_resp_mutex;
} syncprov_info_t;

typedef struct opcookie {
	slap_overinst *son;
	syncmatches *smatches;
	modtarget *smt;
	Entry *se;
	struct berval sdn;	/* DN of entry, for deletes */
	struct berval sndn;
	struct berval suuid;	/* UUID of entry */
	struct berval sctxcsn;
	short osid;	/* sid of op csn */
	short rsid;	/* sid of relay */
	short sreference;	/* Is the entry a reference? */
	reslink srl;
} opcookie;

typedef struct fbase_cookie {
	struct berval *fdn;	/* DN of a modified entry, for scope testing */
	syncops *fss;	/* persistent search we're testing against */
	int fbase;	/* if TRUE we found the search base and it's still valid */
	int fscope;	/* if TRUE then fdn is within the psearch scope */
} fbase_cookie;

static AttributeName csn_anlist[3];
static AttributeName uuid_anlist[2];

static void syncprov_compose_cookie( Operation *op,
		struct berval *cookie,
		BerVarray csns,
		int rid )
{
	slap_cookie_compose( cookie, csns, rid,
		(reopenldap_mode_righteous() || slap_serverID) ? slap_serverID : -1,
		op ? op->o_tmpmemctx : NULL );
}

/* Build a LDAPsync intermediate state control */
static int
syncprov_state_ctrl(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e,
	int		entry_sync_state,
	LDAPControl	**ctrls,
	int		num_ctrls,
	BerValue	*csn,
	int		rid)
{
	Attribute* a;
	struct berval entryuuid_bv = BER_BVNULL;

	if ( csn && BER_BVISEMPTY( csn ) )
		csn = NULL;

	a = attr_find( e->e_attrs, slap_schema.si_ad_entryCSN );
	if ( a ) {
		if ( csn ) {
			assert( slap_csn_match( csn, a->a_nvals ));
		} else {
			assert( entry_sync_state == LDAP_SYNC_ADD || entry_sync_state == LDAP_SYNC_MODIFY );
		}
	}

	a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
	if ( a )
		entryuuid_bv = a->a_nvals[0];

	if (entryuuid_bv.bv_val == NULL || entryuuid_bv.bv_len != 16) {
		Debug( LDAP_DEBUG_TRACE,
			"syncprov_state_ctrl: DN-UUID is empty or invalid (%s)\n", e->e_nname.bv_val );
		send_ldap_error( op, rs, LDAP_OTHER, "DN-UUID is empty or invalid" );
		return LDAP_OTHER;
	}

	unsigned char *u = (void*) entryuuid_bv.bv_val;
	Debug( LDAP_DEBUG_TRACE,
		"syncprov_state_ctrl: rid=%03d, %s, %s, "
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x, %s\n",
		rid, ldap_sync_state2str(entry_sync_state), e->e_nname.bv_val,
		u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15],
		csn ? csn->bv_val : "no-CSN" );

	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPControl *cp;
	struct berval bv;

	ber_init2( ber, 0, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	if ( csn ) {
		struct berval cookie = BER_BVNULL;
		struct berval csns[2];
		csns[0] = *csn;
		BER_BVZERO( &csns[1] );
		syncprov_compose_cookie( op, &cookie, csns, rid );
		ber_printf( ber, "{eOON}",
			entry_sync_state, &entryuuid_bv, &cookie );
		op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
	} else {
		ber_printf( ber, "{eON}",
			entry_sync_state, &entryuuid_bv );
	}

	int ret = ber_flatten2( ber, &bv, 0 );
	if ( ret == 0 ) {
		cp = op->o_tmpalloc( sizeof( LDAPControl ) + bv.bv_len, op->o_tmpmemctx );
		cp->ldctl_oid = LDAP_CONTROL_SYNC_STATE;
		cp->ldctl_iscritical = (op->o_sync == SLAP_CONTROL_CRITICAL);
		cp->ldctl_value.bv_val = (char *)&cp[1];
		cp->ldctl_value.bv_len = bv.bv_len;
		memcpy( cp->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrls[num_ctrls] = cp;
	}
	ber_free_buf( ber );

	if ( ret < 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"syncprov_state_ctrl: ber_flatten2 failed (%d)\n", ret );
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return LDAP_OTHER;
	}

	return LDAP_SUCCESS;
}

/* Build a LDAPsync final state control */
static int
syncprov_done_ctrl(
	Operation	*op,
	SlapReply	*rs,
	LDAPControl	**ctrls,
	int			num_ctrls,
	struct berval *cookie,
	int			refreshDeletes )
{
	int ret;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPControl *cp;
	struct berval bv;

	Debug( LDAP_DEBUG_SYNC,
		"syncprov_done_ctrl: %s, refreshDeletes=%d\n",
		cookie ? cookie->bv_val : "no-cookie", refreshDeletes );

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	ber_printf( ber, "{" );
	if ( cookie ) {
		ber_printf( ber, "O", cookie );
	}
	if ( refreshDeletes == LDAP_SYNC_REFRESH_DELETES ) {
		ber_printf( ber, "b", refreshDeletes );
	}
	ber_printf( ber, "N}" );

	ret = ber_flatten2( ber, &bv, 0 );
	if ( ret == 0 ) {
		cp = op->o_tmpalloc( sizeof( LDAPControl ) + bv.bv_len, op->o_tmpmemctx );
		cp->ldctl_oid = LDAP_CONTROL_SYNC_DONE;
		cp->ldctl_iscritical = (op->o_sync == SLAP_CONTROL_CRITICAL);
		cp->ldctl_value.bv_val = (char *)&cp[1];
		cp->ldctl_value.bv_len = bv.bv_len;
		memcpy( cp->ldctl_value.bv_val, bv.bv_val, bv.bv_len );
		ctrls[num_ctrls] = cp;
	}

	ber_free_buf( ber );

	if ( ret < 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"syncprov_done_ctrl: ber_flatten2 failed (%d)\n", ret );
		send_ldap_error( op, rs, LDAP_OTHER, "internal error" );
		return LDAP_OTHER;
	}

	return LDAP_SUCCESS;
}

static int
syncprov_sendinfo(
	Operation	*op,
	int			type,
	struct berval *cookie,
	int			refreshDone,
	BerVarray	syncUUIDs,
	int			refreshDeletes )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	struct berval rspdata;
	SlapReply rs = { REP_INTERMEDIATE };

	int ret;
	int n = 0;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	if ( type ) {
		switch ( type ) {
		case LDAP_TAG_SYNC_NEW_COOKIE:
			Debug( LDAP_DEBUG_SYNC,
				"syncprov_sendinfo: LDAP_TAG_SYNC_NEW_COOKIE, %s\n",
				cookie ? cookie->bv_val : "no-cookie!?" );
			ber_printf( ber, "tO", type, cookie );
			break;
		case LDAP_TAG_SYNC_REFRESH_DELETE:
		case LDAP_TAG_SYNC_REFRESH_PRESENT:
			Debug( LDAP_DEBUG_SYNC,
				"syncprov_sendinfo: LDAP_TAG_SYNC_REFRESH_%s, %s, refreshDone=%d\n",
				(type == LDAP_TAG_SYNC_REFRESH_DELETE) ? "DELETE" : "PRESENT",
				cookie ? cookie->bv_val : "no-cookie", refreshDone );
			ber_printf( ber, "t{", type );
			if ( cookie ) {
				ber_printf( ber, "O", cookie );
			}
			if ( refreshDone == 0 ) {
				ber_printf( ber, "b", refreshDone );
			}
			ber_printf( ber, "N}" );
			break;
		case LDAP_TAG_SYNC_ID_SET:
			if (syncUUIDs)
				while (syncUUIDs[n].bv_len)
					++n;
			Debug( LDAP_DEBUG_SYNC,
				"syncprov_sendinfo: LDAP_TAG_SYNC_ID_SET, %s, syncUUIDs=#%d, refreshDeletes=%d\n",
				cookie ? cookie->bv_val : "no-cookie", n, refreshDeletes );
			ber_printf( ber, "t{", type );
			if ( cookie ) {
				ber_printf( ber, "O", cookie );
			}
			if ( refreshDeletes ) {
				ber_printf( ber, "b", 1 );
			}
			ber_printf( ber, "[W]", syncUUIDs );
			ber_printf( ber, "N}" );
			break;
		default:
			Debug( LDAP_DEBUG_TRACE,
				"syncprov_sendinfo: invalid syncinfo type (%d)\n",
				type );
			return LDAP_OTHER;
		}
	}

	ret = ber_flatten2( ber, &rspdata, 0 );

	if ( ret < 0 ) {
		Debug( LDAP_DEBUG_TRACE,
			"syncprov_sendinfo: ber_flatten2 failed (%d)\n",
			ret );
		send_ldap_error( op, &rs, LDAP_OTHER, "internal error" );
		return LDAP_OTHER;
	}

	rs.sr_rspoid = LDAP_SYNC_INFO;
	rs.sr_rspdata = &rspdata;
	send_ldap_intermediate( op, &rs );
	ber_free_buf( ber );

	return LDAP_SUCCESS;
}

/* Find a modtarget in an AVL tree */
static int
sp_avl_cmp( const void *c1, const void *c2 )
{
	const modtarget *m1, *m2;
	int rc;

	m1 = c1; m2 = c2;
	rc = m1->mt_dn.bv_len - m2->mt_dn.bv_len;

	if ( rc ) return rc;
	return ber_bvcmp( &m1->mt_dn, &m2->mt_dn );
}

/* syncprov_findbase:
 *   finds the true DN of the base of a search (with alias dereferencing) and
 * checks to make sure the base entry doesn't get replaced with a different
 * entry (e.g., swapping trees via ModDN, or retargeting an alias). If a
 * change is detected, any persistent search on this base must be terminated /
 * reloaded.
 *   On the first call, we just save the DN and entryID. On subsequent calls
 * we compare the DN and entryID with the saved values.
 */
static int
findbase_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;

	if ( rs->sr_type == REP_SEARCH && rs->sr_err == LDAP_SUCCESS ) {
		fbase_cookie *fc = sc->sc_private;

		/* If no entryID, we're looking for the first time.
		 * Just store whatever we got.
		 */
		if ( fc->fss->s_eid == NOID ) {
			fc->fbase = 2;
			fc->fss->s_eid = rs->sr_entry->e_id;
			ber_dupbv( &fc->fss->s_base, &rs->sr_entry->e_nname );
		} else if ( rs->sr_entry->e_id == fc->fss->s_eid &&
			dn_match( &rs->sr_entry->e_nname, &fc->fss->s_base )) {
			/* OK, the DN is the same and the entryID is the same. */
			fc->fbase = 1;
		}
	}
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_SYNC, "findbase failed! %d\n", rs->sr_err );
	}
	return LDAP_SUCCESS;
}

static Filter generic_filter = { LDAP_FILTER_PRESENT, { 0 }, NULL };
static struct berval generic_filterstr = BER_BVC("(objectclass=*)");

static int
syncprov_findbase( Operation *op, fbase_cookie *fc )
{
	/* Use basic parameters from syncrepl search, but use
	 * current op's threadctx / tmpmemctx
	 */
	ldap_pvt_thread_mutex_lock( &fc->fss->s_mutex );
	if ( fc->fss->s_flags & PS_FIND_BASE ) {
		slap_callback cb = {0};
		Operation fop;
		SlapReply frs = { REP_RESULT };
		int rc __maybe_unused;

		fc->fss->s_flags ^= PS_FIND_BASE;
		ldap_pvt_thread_mutex_unlock( &fc->fss->s_mutex );

		slap_op_copy(fc->fss->s_op_safe, &fop, NULL, NULL);

		fop.o_bd = fop.o_bd->bd_self;
		fop.o_hdr = op->o_hdr;
		fop.o_time = op->o_time;
		fop.o_tincr = op->o_tincr;
		fop.o_extra = op->o_extra;

		cb.sc_response = findbase_cb;
		cb.sc_private = fc;

		fop.o_sync = 0;	/* turn off sync mode */
		fop.o_managedsait = SLAP_CONTROL_CRITICAL;
		fop.o_callback = &cb;
		fop.o_tag = LDAP_REQ_SEARCH;
		fop.ors_scope = LDAP_SCOPE_BASE;
		fop.ors_limit = NULL;
		fop.ors_slimit = 1;
		fop.ors_tlimit = SLAP_NO_LIMIT;
		fop.ors_attrs = slap_anlist_no_attrs;
		fop.ors_attrsonly = 1;
		fop.ors_filter = &generic_filter;
		fop.ors_filterstr = generic_filterstr;

		rc = fop.o_bd->be_search( &fop, &frs );
		/* ignore result */
	} else {
		ldap_pvt_thread_mutex_unlock( &fc->fss->s_mutex );
		fc->fbase = 1;
	}

	/* After the first call, see if the fdn resides in the scope */
	if ( fc->fbase == 1 ) {
		switch ( fc->fss->s_op_safe->ors_scope ) {
		case LDAP_SCOPE_BASE:
			fc->fscope = dn_match( fc->fdn, &fc->fss->s_base );
			break;
		case LDAP_SCOPE_ONELEVEL: {
			struct berval pdn;
			dnParent( fc->fdn, &pdn );
			fc->fscope = dn_match( &pdn, &fc->fss->s_base );
			break; }
		case LDAP_SCOPE_SUBTREE:
			fc->fscope = dnIsSuffix( fc->fdn, &fc->fss->s_base );
			break;
		case LDAP_SCOPE_SUBORDINATE:
			fc->fscope = dnIsSuffix( fc->fdn, &fc->fss->s_base ) &&
				!dn_match( fc->fdn, &fc->fss->s_base );
			break;
		}
	}

	if ( fc->fbase )
		return LDAP_SUCCESS;

	/* If entryID has changed, then the base of this search has
	 * changed. Invalidate the psearch.
	 */
	return LDAP_NO_SUCH_OBJECT;
}

/* syncprov_findcsn:
 *   This function has three different purposes, but they all use a search
 * that filters on entryCSN so they're combined here.
 * 1: at startup time, after a contextCSN has been read from the database,
 * we search for all entries with CSN >= contextCSN in case the contextCSN
 * was not checkpointed at the previous shutdown.
 *
 * 2: when the current contextCSN is known and we have a sync cookie, we search
 * for one entry with CSN = the cookie CSN. If not found, try <= cookie CSN.
 * If an entry is found, the cookie CSN is valid, otherwise it is stale.
 *
 * 3: during a refresh phase, we search for all entries with CSN <= the cookie
 * CSN, and generate Present records for them. We always collect this result
 * in SyncID sets, even if there's only one match.
 */
typedef enum find_csn_t {
	FIND_MAXCSN	= 1,
	FIND_CSN	= 2,
	FIND_PRESENT	= 3
} find_csn_t;

static int
findmax_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH && rs->sr_err == LDAP_SUCCESS ) {
		struct berval *maxcsn = op->o_callback->sc_private;
		Attribute *a = attr_find( rs->sr_entry->e_attrs,
			slap_schema.si_ad_entryCSN );

		if ( a && slap_csn_compare_ts( &a->a_vals[0], maxcsn ) > 0 &&
			slap_csn_get_sid( &a->a_vals[0] ) == slap_serverID ) {
			maxcsn->bv_len = a->a_vals[0].bv_len;
			strcpy( maxcsn->bv_val, a->a_vals[0].bv_val );
		}
	}
	return LDAP_SUCCESS;
}

static int
findcsn_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;

	/* We just want to know that at least one exists, so it's OK if
	 * we exceed the unchecked limit.
	 */
	if ( rs->sr_err == LDAP_ADMINLIMIT_EXCEEDED ||
		(rs->sr_type == REP_SEARCH && rs->sr_err == LDAP_SUCCESS )) {
		sc->sc_private = (void *)1;
	}
	return LDAP_SUCCESS;
}

/* Build a list of entryUUIDs for sending in a SyncID set */

#define UUID_LEN	16

typedef struct fpres_cookie {
	int num, total, rc;
	BerVarray uuids;
	char *last;
} fpres_cookie;

static int
findpres_cb( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	fpres_cookie *pc = sc->sc_private;
	Attribute *a;

	if ( pc->rc != LDAP_SUCCESS ) {
		return pc->rc;
	}
	if ( rs->sr_err != LDAP_SUCCESS ) {
		pc->rc = rs->sr_err;
		Debug( LDAP_DEBUG_SYNC,
			"find-present: search failed, rc %d\n", pc->rc );
		return pc->rc;
	}

	switch ( rs->sr_type ) {
	case REP_SEARCH:
		a = attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_entryUUID );
		if ( a && a->a_nvals[0].bv_len == UUID_LEN ) {
			pc->uuids[pc->num].bv_val = pc->last;
			pc->uuids[pc->num].bv_len = UUID_LEN;
			memcpy( pc->last, a->a_nvals[0].bv_val, UUID_LEN );
			pc->last += UUID_LEN;
			pc->num++;
			pc->total++;
		}
		if ( pc->num != SLAP_SYNCUUID_SET_SIZE )
			return LDAP_SUCCESS;
		/* FALLTHRU */

	case REP_RESULT:
		if ( pc->num ) {
			pc->uuids[pc->num].bv_val = NULL;
			pc->uuids[pc->num].bv_len = 0;
			pc->rc = syncprov_sendinfo( op, LDAP_TAG_SYNC_ID_SET, NULL,
				0, pc->uuids, 0 );
			pc->num = 0;
			pc->last = (char *)(pc->uuids + SLAP_SYNCUUID_SET_SIZE+1);
			if ( pc->rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_SYNC,
					"find-present: send failed, rc %d\n", pc->rc );
				return pc->rc;
			}
		}
		return LDAP_SUCCESS;

	default:
		return SLAP_CB_CONTINUE;
	}
}

static int
syncprov_findcsn( Operation *op, find_csn_t mode, struct berval *pivot )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;

	slap_callback cb = {0};
	Operation fop;
	SlapReply frs = { REP_RESULT };
	char buf[LDAP_PVT_CSNSTR_BUFSIZE + STRLENOF("(entryCSN<=)")];
	char cbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	struct berval maxcsn;
	Filter cf;
	AttributeAssertion eq = ATTRIBUTEASSERTION_INIT;
	fpres_cookie pcookie;
	sync_control *srs __maybe_unused = NULL;
	struct slap_limits_set fc_limits;
	int i, rc = LDAP_SUCCESS, findcsn_retry = 1;
	int maxid = -1;

	if ( mode != FIND_MAXCSN ) {
		srs = op->o_controls[slap_cids.sc_LDAPsync];
	}

	slap_op_copy(op, &fop, NULL, NULL);
	fop.o_sync &= SLAP_CONTROL_MASK;	/* turn off sync_mode */
	/* We want pure entries, not referrals */
	fop.o_managedsait = SLAP_CONTROL_CRITICAL;

	cf.f_ava = &eq;
	cf.f_av_desc = slap_schema.si_ad_entryCSN;
	BER_BVZERO( &cf.f_av_value );
	cf.f_next = NULL;

	fop.o_callback = &cb;
	fop.ors_limit = NULL;
	fop.ors_tlimit = SLAP_NO_LIMIT;
	fop.ors_filter = &cf;
	fop.ors_filterstr.bv_val = buf;
	fop.o_dont_replicate = 1;

again:
	switch( mode ) {
	case FIND_MAXCSN:
		ldap_pvt_thread_rdwr_wlock( &si->si_csn_rwlock );
		if (reopenldap_mode_check())
			slap_cookie_verify( &si->si_cookie );
		cf.f_choice = LDAP_FILTER_GE;
		/* If there are multiple CSNs, use the one with our serverID */
		for ( i=0; i<si->si_cookie.numcsns; i++) {
			if ( slap_serverID == si->si_cookie.sids[i] ) {
				maxid = i;
				break;
			}
		}
		if ( maxid < 0 ) {
			/* No match: this is multimaster, and none of the content in the DB
			 * originated locally. Treat like no CSN.
			 */
			ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );
			return LDAP_NO_SUCH_OBJECT;
		}
		cf.f_av_value = si->si_cookie.ctxcsn[maxid];
		fop.ors_filterstr.bv_len = snprintf( buf, sizeof( buf ),
			"(entryCSN>=%s)", cf.f_av_value.bv_val );
		if ( fop.ors_filterstr.bv_len >= sizeof( buf ) ) {
			ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );
			return LDAP_OTHER;
		}
		fop.ors_attrsonly = 0;
		fop.ors_attrs = csn_anlist;
		fop.ors_slimit = SLAP_NO_LIMIT;
		cb.sc_private = &maxcsn;
		cb.sc_response = findmax_cb;
		strcpy( cbuf, cf.f_av_value.bv_val );
		maxcsn.bv_val = cbuf;
		maxcsn.bv_len = cf.f_av_value.bv_len;
		break;
	case FIND_CSN:
		if ( BER_BVISEMPTY( &cf.f_av_value )) {
			cf.f_av_value = *pivot;
		}
		fop.o_dn = op->o_bd->be_rootdn;
		fop.o_ndn = op->o_bd->be_rootndn;
		fop.o_req_dn = op->o_bd->be_suffix[0];
		fop.o_req_ndn = op->o_bd->be_nsuffix[0];
		/* Look for exact match the first time */
		if ( findcsn_retry ) {
			cf.f_choice = LDAP_FILTER_EQUALITY;
			fop.ors_filterstr.bv_len = snprintf( buf, sizeof( buf ),
				"(entryCSN=%s)", cf.f_av_value.bv_val );
		/* On retry, look for <= */
		} else {
			cf.f_choice = LDAP_FILTER_LE;
			fop.ors_limit = &fc_limits;
			memset( &fc_limits, 0, sizeof( fc_limits ));
			fc_limits.lms_s_unchecked = 1;
			fop.ors_filterstr.bv_len = snprintf( buf, sizeof( buf ),
				"(entryCSN<=%s)", cf.f_av_value.bv_val );
		}
		if ( fop.ors_filterstr.bv_len >= sizeof( buf ) ) {
			return LDAP_OTHER;
		}
		fop.ors_attrsonly = 1;
		fop.ors_attrs = slap_anlist_no_attrs;
		fop.ors_slimit = 1;
		cb.sc_private = NULL;
		cb.sc_response = findcsn_cb;
		break;
	case FIND_PRESENT:
		fop.ors_filter = op->ors_filter;
		fop.ors_filterstr = op->ors_filterstr;
		fop.ors_attrsonly = 0;
		fop.ors_attrs = uuid_anlist;
		fop.ors_slimit = SLAP_NO_LIMIT;
		cb.sc_private = &pcookie;
		cb.sc_response = findpres_cb;
		pcookie.num = 0;
		pcookie.total = 0;
		pcookie.rc = LDAP_SUCCESS;

		/* preallocate storage for a full set */
		pcookie.uuids = op->o_tmpalloc( (SLAP_SYNCUUID_SET_SIZE+1) *
			sizeof(struct berval) + SLAP_SYNCUUID_SET_SIZE * UUID_LEN,
			op->o_tmpmemctx );
		pcookie.last = (char *)(pcookie.uuids + SLAP_SYNCUUID_SET_SIZE+1);
		break;
	}

	fop.o_bd->bd_info = (BackendInfo *)on->on_info;
	rc = fop.o_bd->be_search( &fop, &frs );
	fop.o_bd->bd_info = (BackendInfo *)on;

	switch( mode ) {
	case FIND_MAXCSN:
		if ( rc == LDAP_BUSY ) {
			Debug( LDAP_DEBUG_SYNC,
				"syncprov-findmax: got %d, retry\n", rc );
			rs_reinit( &frs, REP_RESULT );
			goto again;
		}
		if ( rc == LDAP_SUCCESS ) {
			assert( slap_csn_verify_full( &maxcsn ) );
			if ( slap_csn_compare_ts( &si->si_cookie.ctxcsn[maxid], &maxcsn ) < 0 ) {
				ber_bvreplace( &si->si_cookie.ctxcsn[maxid], &maxcsn );
				si->si_numops++;	/* ensure a checkpoint */
			}
			if (reopenldap_mode_check())
				slap_cookie_verify( &si->si_cookie );
		}
		ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );
		break;
	case FIND_CSN:
		if ( rc == LDAP_BUSY ) {
			Debug( LDAP_DEBUG_SYNC,
				"syncprov-findcsn: got %d, retry\n", rc );
			rs_reinit( &frs, REP_RESULT );
			goto again;
		}
		/* If matching CSN was not found, invalidate the context. */
		if ( !cb.sc_private ) {
			/* If we didn't find an exact match, then try for <= */
			if ( findcsn_retry ) {
				findcsn_retry = 0;
				rs_reinit( &frs, REP_RESULT );
				goto again;
			}
			Debug( LDAP_DEBUG_SYNC,
				"syncprov-findcsn: sid %d, pivot %s, not-found\n",
				srs->sr_state.sid, pivot->bv_val );
			if (rc == LDAP_SUCCESS)
				rc = LDAP_NO_SUCH_OBJECT;
		} else {
			rc = LDAP_SUCCESS;
		}
		break;
	case FIND_PRESENT:
		if ( rc == LDAP_SUCCESS )
			rc = pcookie.rc;
		if ( DebugTest( LDAP_DEBUG_SYNC ) ) {
			ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
			Debug( LDAP_DEBUG_SYNC,
				"syncprov-findpresent: sid %d, pivot %s, find-present %u, rc %d\n",
				srs->sr_state.sid, pivot->bv_val, pcookie.total, rc );
			slap_cookie_debug_pair(
				"syncprov-findpresent:",
				"provider", &si->si_cookie,
				"consumer", &srs->sr_state, -1 );
			ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
		}
		op->o_tmpfree( pcookie.uuids, op->o_tmpmemctx );
		break;
	}

	return rc;
}

static void reslink_detach( reslink *rl )
{
	reslink **p;
	resinfo *ri = rl->rl_info;

	ldap_pvt_thread_mutex_lock( &ri->ri_mutex );
	for (p = &ri->ri_list; *p; p = &(*p)->rl_list) {
		if (*p == rl) {
			*p = rl->rl_list;
			rl->rl_list = NULL;
			rl->rl_info = NULL;
			break;
		}
	}

	assert(rl->rl_info == NULL);
	if (ri->ri_list) {
		ldap_pvt_thread_mutex_unlock( &ri->ri_mutex );
	} else {
		ldap_pvt_thread_mutex_unlock( &ri->ri_mutex );
		ldap_pvt_thread_mutex_destroy( &ri->ri_mutex );
		if ( ri->ri_e )
			entry_free( ri->ri_e );
		if ( !BER_BVISNULL( &ri->ri_cookie ))
			ch_free( ri->ri_cookie.bv_val );
		ch_free( ri );
	}
}

static void
syncprov_refresh_end(syncprov_info_t *si, syncops *so);

#define SO_LOCKED_NONE 0
#define SO_LOCKED_SIOP 1
#define SO_LOCKED_CONN 2

static void
syncprov_unlink_syncop( syncops *so, int unlink_flags, int locked_flags )
{
	syncprov_info_t *si = so->s_si;
	assert(unlink_flags && !(unlink_flags & ~OS_REF_MASK));
	assert((slap_tsan__read_int(&so->s_flags) & unlink_flags) > 0);

	if (locked_flags & SO_LOCKED_SIOP)
		assert(ldap_pvt_thread_mutex_trylock( &si->si_ops_mutex ) != 0);
	else if (unlink_flags & OS_REF_OP_SEARCH)
		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );

	ldap_pvt_thread_mutex_lock( &so->s_mutex );
	LDAP_ENSURE(so->s_flags & unlink_flags);

	if (unlink_flags & OS_REF_OP_MATCH)  {
		assert(so->s_matchops_inuse > 0);
		if (--so->s_matchops_inuse > 0)
			unlink_flags -= OS_REF_OP_MATCH;
	}

	if (so->s_flags & unlink_flags & OS_REF_OP_SEARCH)  {
		if (so != so->s_next) {
			syncops **sop;

			if (so != so->s_next) {
				for ( sop = &si->si_ops; *sop; sop = &(*sop)->s_next ) {
					if ( *sop == so ) {
						*sop = so->s_next;
						so->s_next = so; /* LY: safely mark it as unlinked */
						break;
					}
				}
				assert(so == so->s_next);
			}
		}

		if (so->s_flags & PS_IS_REFRESHING)
			syncprov_refresh_end( si, so );

		if ( (locked_flags & SO_LOCKED_SIOP) == 0)
			ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

		if ( (so->s_flags & PS_IS_DETACHED) == 0 )
			so->s_op = NULL;
		else if ( so->s_op->o_conn && (so->s_flags & OS_REF_UNLINK) == 0 ) {
			Connection* conn = so->s_op->o_conn;
			if ( (locked_flags & SO_LOCKED_CONN) == 0 ) {
				if (ldap_pvt_thread_mutex_trylock( &conn->c_mutex )) {
					if (slap_get_op_abandon(so->s_op)) {
						/* LY: connection_abandon() is running, and we are
						 * clashed with it. So, we could skip OS_REF_OP_SEARCH
						 * and connection unlinkin. */
						unlink_flags -= OS_REF_OP_SEARCH;
						goto skip_conn_unlink;
					}
					/* LY: avoid lock-order reversal */
					so->s_flags |= OS_REF_UNLINK;
					unlink_flags |= OS_REF_UNLINK;
					ldap_pvt_thread_mutex_unlock( &so->s_mutex );
					ldap_pvt_thread_mutex_lock( &conn->c_mutex );
					ldap_pvt_thread_mutex_lock( &so->s_mutex );
					assert( (so->s_flags & OS_REF_UNLINK) != 0 );
				}
			}
			if (conn == so->s_op->o_conn) {
				assert(conn->c_n_ops_executing > 0);
				conn->c_n_ops_executing--;
				conn->c_n_ops_completed++;
				LDAP_STAILQ_REMOVE( &conn->c_ops, so->s_op, Operation, o_next );
				LDAP_STAILQ_NEXT(so->s_op, o_next) = NULL;
				so->s_op->o_conn = NULL;
			} else {
				assert(so->s_op->o_conn == NULL);
			}
			if ( (so->s_flags & OS_REF_ABANDON) == 0
					&& LDAP_STAILQ_EMPTY(&conn->c_ops)
					&& LDAP_STAILQ_EMPTY(&conn->c_pending_ops) ) {
				/* LY: If ops-queues are empty
				 *        and we called NOT from connection_abandon(),
				 *     then kicks the connection to close it immediately.
				 * For instance, when syncprov_unlink_syncop() called
				 * from syncprov_playback_dequeue(). */
				connection_closing( conn, NULL );
			}
			if ( (locked_flags & SO_LOCKED_CONN) == 0 )
				ldap_pvt_thread_mutex_unlock( &conn->c_mutex );
		}
	} else if ((unlink_flags & OS_REF_OP_SEARCH) != 0
			&& (locked_flags & SO_LOCKED_SIOP) == 0) {
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
	}

skip_conn_unlink:
	so->s_flags &= ~unlink_flags;
	if (so->s_flags & OS_REF_MASK) {
		ldap_pvt_thread_mutex_unlock( &so->s_mutex );
		return;
	}

	assert(so->s_matchops_inuse == 0);
	assert(so == so->s_next);
	reslink *rl, *rlnext;

	if ( so->s_flags & PS_IS_DETACHED ) {
		GroupAssertion *ga, *gnext;

		assert(so->s_op->o_conn == NULL);
		filter_free( so->s_op->ors_filter );
		for ( ga = so->s_op->o_groups; ga; ga=gnext ) {
			gnext = ga->ga_next;
			ch_free( ga );
		}
		ch_free( so->s_op);
	}

	ch_free( so->s_base.bv_val );
	for ( rl = so->s_rl; rl; rl = rlnext ) {
		rlnext = rl->rl_next;
		reslink_detach( rl );
		ch_free( rl );
	}

	so->s_si = NULL;
	so->s_op = NULL;

	ldap_pvt_thread_mutex_unlock( &so->s_mutex );
	ldap_pvt_thread_mutex_destroy( &so->s_mutex );
	ch_free( so );
	LDAP_ENSURE(__sync_fetch_and_sub(&si->si_psearches, 1) > 0 );
}

/* Send a persistent search response */
static int
syncprov_sendresp( Operation *op, resinfo *ri, syncops *so, int mode )
{
	SlapReply rs = { REP_SEARCH };
	Entry e_uuid = {0};
	Attribute a_uuid = {0};

	if ( is_syncops_abandoned(so)
			|| (slap_tsan__read_int(&so->s_flags) & PS_DEAD) )
		return SLAPD_ABANDON;

	rs.sr_ctrls = op->o_tmpalloc( sizeof(LDAPControl *)*2, op->o_tmpmemctx );
	rs.sr_ctrls[1] = NULL;
	rs.sr_flags = REP_CTRLS_MUSTBEFREED;
	unsigned char *u = (void*) ri->ri_uuid.bv_val;

	Debug( LDAP_DEBUG_SYNC, "syncprov_sendresp: %s, %s, to=%03x, "
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x, "
		"%s\n",
		ldap_sync_state2str(mode),
		ri->ri_e ? ri->ri_e->e_nname.bv_val : op->o_bd->be_nsuffix->bv_val,
		so->s_sid,
		u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15],
		ri->ri_csn.bv_val );

	if (mode != LDAP_SYNC_DELETE && ! ri->ri_isref && ri->ri_csn.bv_len) {
		Attribute* a = attr_find( ri->ri_e->e_attrs, slap_schema.si_ad_entryCSN );

		if (a && ! slap_csn_match( &ri->ri_csn, a->a_nvals )) {
			Debug( LDAP_DEBUG_ANY, "syncprov: send_resp: out.entryCSN %s != out.cookieCSN %s, is biglock enabled?!\n",
				  a->a_nvals->bv_val, ri->ri_csn.bv_val );
			if (reopenldap_mode_strict()) {
				send_ldap_error(op, &rs,
					LDAP_OTHER, "syncprov: out.entryCSN != out.cookieCSN, is biglock enabled?!");
				return SLAPD_DISCONNECT;
			}
			if (reopenldap_mode_righteous()) {
				Debug( LDAP_DEBUG_SYNC, "syncprov_sendresp: override wrong out.cookieCSN %s by out.entryCSN %s\n",
					  ri->ri_csn.bv_val, a->a_nvals->bv_val );
				LDAP_ENSURE(a->a_nvals->bv_len == ri->ri_csn.bv_len);
				memcpy(ri->ri_csn.bv_val, a->a_nvals->bv_val, a->a_nvals->bv_len);
			}
		}
	}

	e_uuid.e_attrs = &a_uuid;
	a_uuid.a_desc = slap_schema.si_ad_entryUUID;
	a_uuid.a_nvals = &ri->ri_uuid;
	rs.sr_err = syncprov_state_ctrl( op, &rs, &e_uuid,
		mode, rs.sr_ctrls, 0, &ri->ri_csn, so->s_rid );

	rs.sr_entry = &e_uuid;
	if ( mode == LDAP_SYNC_ADD || mode == LDAP_SYNC_MODIFY ) {
		e_uuid = *ri->ri_e;
		e_uuid.e_private = NULL;
	}

	switch( mode ) {
	case LDAP_SYNC_ADD:
		if ( ri->ri_isref && so->s_op_safe->o_managedsait <= SLAP_CONTROL_IGNORED ) {
			rs.sr_ref = get_entry_referrals( op, rs.sr_entry );
			rs.sr_err = send_search_reference( op, &rs );
			ber_bvarray_free( rs.sr_ref );
			break;
		}
		/* fallthru */
	case LDAP_SYNC_MODIFY:
		rs.sr_attrs = op->ors_attrs;
		rs.sr_err = send_search_entry( op, &rs );
		break;
	case LDAP_SYNC_DELETE:
		e_uuid.e_attrs = NULL;
		e_uuid.e_name = ri->ri_dn;
		e_uuid.e_nname = ri->ri_ndn;
		if ( ri->ri_isref && so->s_op_safe->o_managedsait <= SLAP_CONTROL_IGNORED ) {
			struct berval bv = BER_BVNULL;
			rs.sr_ref = &bv;
			rs.sr_err = send_search_reference( op, &rs );
		} else {
			rs.sr_err = send_search_entry( op, &rs );
		}
		break;
	default:
		LDAP_BUG();
	}
	return rs.sr_err;
}

static void
syncprov_playback_enqueue( syncops *so );

/* Play back queued responses */
static int
syncprov_playback_locked( Operation *op, syncops *so )
{
	reslink *rl;
	int rc = LDAP_SUCCESS;

	while(1) {
		rl = so->s_rl;
		if ( !rl )
			break;
		so->s_rl = rl->rl_next;
		if ( !so->s_rl )
			so->s_rltail = NULL;
		ldap_pvt_thread_mutex_unlock( &so->s_mutex );

		if ( is_syncops_abandoned(so)
				|| (slap_tsan__read_int(&so->s_flags) & PS_DEAD)) {
			rc = SLAPD_ABANDON;
		} else if ( rl->rl_mode == LDAP_SYNC_NEW_COOKIE ) {
			rc = syncprov_sendinfo( op, LDAP_TAG_SYNC_NEW_COOKIE,
				&rl->rl_info->ri_cookie, 0, NULL, 0 );
		} else {
			rc = syncprov_sendresp( op, rl->rl_info, so, rl->rl_mode );
		}

		reslink_detach( rl );
		ch_free( rl );

		ldap_pvt_thread_mutex_lock( &so->s_mutex );
		if ( rc != LDAP_SUCCESS )
			so->s_flags |= PS_DEAD;
		if ( (so->s_flags & PS_DEAD) == 0 )
			break;
	}

	/* We now only send one change at a time, to prevent one
	 * psearch from hogging all the CPU. Resubmit this task if
	 * there are more responses queued and no errors occurred.
	 */
	if ( so->s_rl != NULL )
		return 1;

	if ( (so->s_flags & PS_DEAD) == 0 )
		return 0;

	if ( (so->s_flags & PS_LOST_BASE) && ! is_syncops_abandoned(so) ) {
		SlapReply rs = {REP_RESULT};
		send_ldap_error( so->s_op, &rs, LDAP_SYNC_REFRESH_REQUIRED,
			"search base has changed" );
	}

	return -1;
}

/* task for playing back queued responses */
static void *
syncprov_playback_dequeue( void *ctx, void *arg )
{
	syncops *so = arg;
	OperationBuffer opbuf;
	Operation *op;
	BackendDB be;
	int resubmit;

	ldap_pvt_thread_mutex_lock( &so->s_mutex );

	slap_op_copy(so->s_op, op = &opbuf.ob_op, &opbuf.ob_hdr, &be);
	op->o_controls = opbuf.ob_controls;
	memset( op->o_controls, 0, sizeof(opbuf.ob_controls) );
	op->o_sync = SLAP_CONTROL_IGNORED;

	op->o_tmpmemctx = slap_sl_mem_create(SLAP_SLAB_SIZE, SLAP_SLAB_STACK, ctx, 1);
	op->o_tmpmfuncs = &slap_sl_mfuncs;
	op->o_threadctx = ctx;

	/* syncprov_playback expects a fake db */
	be.be_flags |= SLAP_DBFLAG_OVERLAY;
	LDAP_SLIST_FIRST(&op->o_extra) = NULL;

	resubmit = syncprov_playback_locked( op, so );

	ldap_pvt_thread_mutex_unlock( &so->s_mutex );

	if (resubmit > 0)
		ldap_pvt_thread_pool_submit2( &connection_pool,
			syncprov_playback_dequeue, so, &so->s_pool_cookie );
	else
		syncprov_unlink_syncop( so,
				(resubmit < 0)
				? OS_REF_PLAYBACK | OS_REF_OP_SEARCH : OS_REF_PLAYBACK,
			SO_LOCKED_NONE );

	return NULL;
}

/* Start the task to play back queued psearch responses */
static void
syncprov_playback_enqueue( syncops *so )
{
	assert(!(so->s_flags & OS_REF_PLAYBACK));
	so->s_flags |= OS_REF_PLAYBACK;
	ldap_pvt_thread_pool_submit2( &connection_pool,
		syncprov_playback_dequeue, so, &so->s_pool_cookie );
}

/* Queue a persistent search response */
static void
syncprov_qresp( opcookie *opc, syncops *so, int mode )
{
	reslink *rl;
	resinfo *ri;
	struct berval csn = opc->sctxcsn;

	if (unlikely( is_syncops_abandoned(so)
			|| (slap_tsan__read_int(&so->s_flags) & PS_DEAD) ))
		return;

	rl = ch_malloc( sizeof( reslink ));
	rl->rl_next = NULL;
	rl->rl_mode = mode;
	if ( !opc->srl.rl_info ) {
		int risize = sizeof( resinfo );
		if ( csn.bv_len )
			risize += csn.bv_len + 1;

		if ( opc->se ) {
			Attribute *a;
			ri = ch_malloc( risize );
			ri->ri_dn = opc->se->e_name;
			ri->ri_ndn = opc->se->e_nname;
			a = attr_find( opc->se->e_attrs, slap_schema.si_ad_entryUUID );
			if ( a ) {
				ri->ri_uuid = a->a_nvals[0];
				assert( ri->ri_uuid.bv_len == UUID_LEN );
			} else
				ri->ri_uuid.bv_len = 0;
			if ( csn.bv_len ) {
				ri->ri_csn.bv_val = (char *)(ri + 1);
				ri->ri_csn.bv_len = csn.bv_len;
				memcpy( ri->ri_csn.bv_val, csn.bv_val, csn.bv_len );
				ri->ri_csn.bv_val[csn.bv_len] = '\0';
			} else {
				assert( mode == LDAP_SYNC_ADD || mode == LDAP_SYNC_MODIFY
						|| mode == LDAP_SYNC_NEW_COOKIE );
				ri->ri_csn.bv_val = NULL;
			}
		} else {
			risize += opc->suuid.bv_len +
				opc->sdn.bv_len + 1 + opc->sndn.bv_len + 1;
			ri = ch_malloc( risize );
			ri->ri_dn.bv_val = (char *)(ri + 1);
			ri->ri_dn.bv_len = opc->sdn.bv_len;
			ri->ri_ndn.bv_val = lutil_strcopy( ri->ri_dn.bv_val,
				opc->sdn.bv_val ) + 1;
			ri->ri_ndn.bv_len = opc->sndn.bv_len;
			ri->ri_uuid.bv_val = lutil_strcopy( ri->ri_ndn.bv_val,
				opc->sndn.bv_val ) + 1;
			ri->ri_uuid.bv_len = opc->suuid.bv_len;
			memcpy( ri->ri_uuid.bv_val, opc->suuid.bv_val, opc->suuid.bv_len );
			if ( csn.bv_len ) {
				ri->ri_csn.bv_val = ri->ri_uuid.bv_val + ri->ri_uuid.bv_len;
				memcpy( ri->ri_csn.bv_val, csn.bv_val, csn.bv_len );
				ri->ri_csn.bv_val[csn.bv_len] = '\0';
			} else {
				assert( mode == LDAP_SYNC_NEW_COOKIE || mode == LDAP_SYNC_DELETE );
				ri->ri_csn.bv_val = NULL;
			}
		}
		ri->ri_list = &opc->srl;
		ri->ri_e = opc->se;
		ri->ri_csn.bv_len = csn.bv_len;
		ri->ri_isref = opc->sreference;
		BER_BVZERO( &ri->ri_cookie );
		ldap_pvt_thread_mutex_init( &ri->ri_mutex );
		opc->se = NULL;
		opc->srl.rl_info = ri;
	}
	ri = opc->srl.rl_info;

	ldap_pvt_thread_mutex_lock( &ri->ri_mutex );
	rl->rl_info = ri;
	rl->rl_list = ri->ri_list;
	ri->ri_list = rl;
	if ( mode == LDAP_SYNC_NEW_COOKIE && BER_BVISNULL( &ri->ri_cookie )) {
		syncprov_info_t	*si = opc->son->on_bi.bi_private;
		ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
		if (reopenldap_mode_check()) {
			slap_cookie_verify( &si->si_cookie );
			if (opc->sctxcsn.bv_len) {
				int i;
				for( i = si->si_cookie.numcsns; --i >= 0; )
					if (strcmp(si->si_cookie.ctxcsn[i].bv_val, opc->sctxcsn.bv_val) == 0)
						break;
				assert(i >= 0);
			}
		}
		syncprov_compose_cookie( NULL, &ri->ri_cookie,
			si->si_cookie.ctxcsn, so->s_rid );
		ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
	}
	ldap_pvt_thread_mutex_unlock( &ri->ri_mutex );

	ldap_pvt_thread_mutex_lock( &so->s_mutex );
	if ( !so->s_rl ) {
		so->s_rl = rl;
	} else {
		so->s_rltail->rl_next = rl;
	}
	so->s_rltail = rl;

	/* If the base of the psearch was modified, check it next time round */
	if ( so->s_flags & PS_WROTE_BASE ) {
		so->s_flags ^= PS_WROTE_BASE;
		so->s_flags |= PS_FIND_BASE;
	}
	if (( so->s_flags & (PS_IS_DETACHED|PS_TASK_QUEUED)) == PS_IS_DETACHED ) {
		syncprov_playback_enqueue( so );
	}
	ldap_pvt_thread_mutex_unlock( &so->s_mutex );
}

static int
syncprov_abandon_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *sc = op->o_callback;
	op->o_callback = sc->sc_next;
	syncprov_unlink_syncop( sc->sc_private,
		OS_REF_OP_SEARCH | OS_REF_ABANDON, SO_LOCKED_CONN );
	op->o_tmpfree( sc, op->o_tmpmemctx );
	return 0;
}

typedef struct searchstate {
	slap_overinst *ss_on;
	syncops *ss_so;
	BerVarray ss_ctxcsn;
	int *ss_sids;
	int ss_numcsns;
#define	SS_PRESENT	0x01
#define	SS_CHANGED	0x02
	int ss_flags;
} searchstate;

static int
syncprov_op_abandon( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = si = on->on_bi.bi_private;
	syncops *so, **pso;

	if (! si)
		/* LY: workaround for https://github.com/leo-yuriev/ReOpenLDAP/issues/45 */
		return SLAP_CB_CONTINUE;

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	for (pso = &si->si_ops; (so = *pso) != NULL; pso = &so->s_next) {
		assert(so->s_next != so);
		if ( so->s_op && so->s_op->o_connid == op->o_connid
				&& so->s_op->o_msgid == op->orn_msgid ) {
			ldap_pvt_thread_mutex_lock( &so->s_mutex );
			so->s_flags |= PS_DEAD | OS_REF_ABANDON;
			*pso = so->s_next;
			/* LY: safely mark it as unlinked */
			so->s_next = so;
			/* LY: when called via syncprov.on_bi.bi_op_cancel */
			if (! slap_get_op_abandon(so->s_op))
				slap_set_op_abandon(so->s_op, 1);
			ldap_pvt_thread_mutex_unlock( &so->s_mutex );
			break;
		}
	}
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	if ( so ) {
		/* Is this really a Cancel exop? */
		if ( op->o_tag != LDAP_REQ_ABANDON ) {
			slap_set_op_cancel(so->s_op, SLAP_CANCEL_ACK);
			rs->sr_err = LDAP_CANCELLED;
			send_ldap_result( so->s_op, rs );
			if ( slap_tsan__read_int(&so->s_flags) & PS_IS_DETACHED ) {
				slap_callback *cb;
				cb = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
				cb->sc_cleanup = syncprov_abandon_cleanup;
				cb->sc_next = op->o_callback;
				cb->sc_private = so;
				op->o_callback = cb;
				return SLAP_CB_CONTINUE;
			}
		}
		syncprov_unlink_syncop( so,
			OS_REF_OP_SEARCH | OS_REF_ABANDON, SO_LOCKED_CONN );
	}
	return SLAP_CB_CONTINUE;
}

/* Find which persistent searches are affected by this operation */
static void
syncprov_matchops( Operation *op, opcookie *opc, int saveit )
{
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;

	fbase_cookie fc;
	Entry *e = NULL;
	Attribute *a;
	int rc;
	struct berval newdn;
	int freefdn = 0;
	BackendDB *b0 = op->o_bd, db;

	fc.fdn = &op->o_req_ndn;
	/* compute new DN */
	if ( op->o_tag == LDAP_REQ_MODRDN && !saveit ) {
		struct berval pdn;
		if ( op->orr_nnewSup ) pdn = *op->orr_nnewSup;
		else dnParent( fc.fdn, &pdn );
		build_new_dn( &newdn, &pdn, &op->orr_nnewrdn, op->o_tmpmemctx );
		fc.fdn = &newdn;
		freefdn = 1;
	}
	if ( op->o_tag != LDAP_REQ_ADD ) {
		if ( !SLAP_ISOVERLAY( op->o_bd )) {
			db = *op->o_bd;
			op->o_bd = &db;
		}
		rc = overlay_entry_get_ov( op, fc.fdn, NULL, NULL, 0, &e, on );
		/* If we're sending responses now, make a copy and unlock the DB */
		if ( e && !saveit ) {
			if ( !opc->se )
				opc->se = entry_dup( e );
			overlay_entry_release_ov( op, e, 0, on );
			e = opc->se;
		}
		if ( rc ) {
			op->o_bd = b0;
			return;
		}
	} else {
		e = op->ora_e;
		if ( !saveit ) {
			if ( !opc->se )
				opc->se = entry_dup( e );
			e = opc->se;
		}
	}

	if ( saveit || op->o_tag == LDAP_REQ_ADD ) {
		ber_dupbv_x( &opc->sdn, &e->e_name, op->o_tmpmemctx );
		ber_dupbv_x( &opc->sndn, &e->e_nname, op->o_tmpmemctx );
		opc->sreference = is_entry_referral( e );
		a = attr_find( e->e_attrs, slap_schema.si_ad_entryUUID );
		if ( a )
			ber_dupbv_x( &opc->suuid, &a->a_nvals[0], op->o_tmpmemctx );
	} else if ( op->o_tag == LDAP_REQ_MODRDN && !saveit ) {
		op->o_tmpfree( opc->sndn.bv_val, op->o_tmpmemctx );
		op->o_tmpfree( opc->sdn.bv_val, op->o_tmpmemctx );
		ber_dupbv_x( &opc->sdn, &e->e_name, op->o_tmpmemctx );
		ber_dupbv_x( &opc->sndn, &e->e_nname, op->o_tmpmemctx );
	}

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	syncops *so, *snext;
	for (so = si->si_ops; so; so = snext)
	{
		syncmatches *sm;
		int found = 0;
		snext = so->s_next;

		if (unlikely( is_syncops_abandoned(so)
				|| (slap_tsan__read_int(&so->s_flags) & PS_DEAD) ))
			continue;

		assert(slap_tsan__read_int(&so->s_flags) & OS_REF_OP_SEARCH);

		/* Don't send ops back to the originator */
		if ( op->o_tag != LDAP_REQ_DELETE
				&& opc->osid > 0 && opc->osid == so->s_sid ) {
			Debug( LDAP_DEBUG_SYNC, "syncprov_matchops: %s skipping original sid 0x%03x\n",
				op->o_bd->be_nsuffix->bv_val, opc->osid );
			continue;
		}

		/* Don't send ops back to the messenger */
		if ( opc->rsid > 0 && opc->rsid == so->s_sid ) {
			Debug( LDAP_DEBUG_SYNC, "syncprov_matchops: %s skipping relayed sid 0x%03x\n",
				op->o_bd->be_nsuffix->bv_val, opc->rsid );
			continue;
		}

		/* validate base */
		fc.fss = so;
		fc.fbase = 0;
		fc.fscope = 0;

		/* If the base of the search is missing, signal a refresh */
		rc = syncprov_findbase( op, &fc );
		if ( rc != LDAP_SUCCESS ) {
			ldap_pvt_thread_mutex_lock( &so->s_mutex );
			so->s_flags |= PS_DEAD | PS_LOST_BASE;
kill_locked:
			switch (so->s_flags & (PS_TASK_QUEUED | PS_IS_DETACHED)) {
			default:
				LDAP_BUG();
			case PS_TASK_QUEUED | PS_IS_DETACHED:
				/* LY: playback() will send error and kill psearch */
				ldap_pvt_thread_mutex_unlock( &so->s_mutex );
				continue;
			case 0:
			case PS_IS_DETACHED:
				/* LY: we can send error and kill psearch now,
				 * OS_REF_OP_MATCH will be done by op_cleanup().
				 */
				if (! is_syncops_abandoned( so )) {
					SlapReply rs = {REP_RESULT};
					send_ldap_error( so->s_op_safe, &rs,
						LDAP_SYNC_REFRESH_REQUIRED, "search base has changed" );
				}
				ldap_pvt_thread_mutex_unlock( &so->s_mutex );
				syncprov_unlink_syncop( so, OS_REF_OP_SEARCH, SO_LOCKED_SIOP );
				continue;
			}
		}
		assert(fc.fbase || fc.fscope);

		/* If we're sending results now, look for this op in old matches */
		if ( !saveit ) {
			syncmatches *old;

			/* Did we modify the search base? */
			if ( dn_match( &op->o_req_ndn, &so->s_base )) {
				ldap_pvt_thread_mutex_lock( &so->s_mutex );
				so->s_flags |= PS_WROTE_BASE;
				ldap_pvt_thread_mutex_unlock( &so->s_mutex );
			}

			for ( sm=opc->smatches, old=(syncmatches *)&opc->smatches; sm;
				old=sm, sm=sm->sm_next ) {
				assert(slap_tsan__read_int(&sm->sm_op->s_matchops_inuse) > 0);
				assert(slap_tsan__read_int(&sm->sm_op->s_flags) & OS_REF_OP_MATCH);
				if ( sm->sm_op == so ) {
					found = 1;
					old->sm_next = sm->sm_next;
					op->o_tmpfree( sm, op->o_tmpmemctx );
					break;
				}
			}
		}

		if ( fc.fscope ) {
			ldap_pvt_thread_mutex_lock( &so->s_mutex );
			if ( unlikely(is_syncops_abandoned( so )) ) {
				rc = SLAPD_ABANDON;
				goto kill_locked;
			}
			assert( so->s_flags & OS_REF_OP_SEARCH );
			Operation op2; slap_op_copy(so->s_op_safe, &op2, NULL, NULL);
			Opheader oh = *op->o_hdr;
			oh.oh_conn = so->s_op->o_conn;
			oh.oh_connid = so->s_op->o_connid;
			op2.o_bd = op->o_bd->bd_self;
			op2.o_hdr = &oh;
			op2.o_extra = op->o_extra;

			if ( so->s_flags & PS_FIX_FILTER ) {
				/* Skip the AND/GE clause that we stuck on in front. We
				   would lose deletes/mods that happen during the refresh
				   phase otherwise (ITS#6555) */
				op2.ors_filter = op2.ors_filter->f_and->f_next;
			}
			rc = test_filter( &op2, e, op2.ors_filter );
			ldap_pvt_thread_mutex_unlock( &so->s_mutex );
		}

		Debug( LDAP_DEBUG_TRACE, "syncprov_matchops: sid 0x%03x fscope %d rc %d\n",
			so->s_sid, fc.fscope, rc );

		assert(rc != SLAPD_ABANDON);
		/* check if current o_req_dn is in scope and matches filter */
		if ( fc.fscope && rc == LDAP_COMPARE_TRUE ) {
			if ( saveit ) {
				ldap_pvt_thread_mutex_lock( &so->s_mutex );
				assert(so->s_flags & OS_REF_OP_SEARCH);
				if (++so->s_matchops_inuse == 1) {
					assert(!(so->s_flags & OS_REF_OP_MATCH));
					so->s_flags |= OS_REF_OP_MATCH;
				}
				ldap_pvt_thread_mutex_unlock( &so->s_mutex );

				sm = op->o_tmpalloc( sizeof(syncmatches), op->o_tmpmemctx );
				sm->sm_next = opc->smatches;
				sm->sm_op = so;
				opc->smatches = sm;
			} else {
				/* if found send UPDATE else send ADD */
				if (found)
					assert(op->o_tag == LDAP_REQ_MODIFY
						   || op->o_tag == LDAP_REQ_EXTENDED
						   || op->o_tag == LDAP_REQ_MODDN);
				else
					assert(op->o_tag == LDAP_REQ_ADD);
				syncprov_qresp( opc, so,
					found ? LDAP_SYNC_MODIFY : LDAP_SYNC_ADD );
			}
		} else if ( !saveit && found ) {
			/* send DELETE */
			assert(op->o_tag == LDAP_REQ_DELETE);
			syncprov_qresp( opc, so, LDAP_SYNC_DELETE );
		} else if ( !saveit ) {
			assert((!fc.fscope && rc == LDAP_SUCCESS)
				   || (fc.fscope && rc != LDAP_COMPARE_TRUE && rc != LDAP_SUCCESS));
			assert(op->o_tag == LDAP_REQ_MODIFY
				   || op->o_tag == LDAP_REQ_EXTENDED
				   || op->o_tag == LDAP_REQ_ADD);
			syncprov_qresp( opc, so, LDAP_SYNC_NEW_COOKIE );
		}

		if ( !saveit && found ) {
			/* Decrement s_inuse, was incremented when called
			 * with saveit == TRUE
			 */
			syncprov_unlink_syncop( so, OS_REF_OP_MATCH, SO_LOCKED_SIOP );
			continue;
		}
	}
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	if ( op->o_tag != LDAP_REQ_ADD && e ) {
		if ( !SLAP_ISOVERLAY( op->o_bd )) {
			op->o_bd = &db;
		}
		if ( saveit )
			overlay_entry_release_ov( op, e, 0, on );
		op->o_bd = b0;
	}
	if ( !saveit ) {
		if ( opc->srl.rl_info )
			reslink_detach( &opc->srl );
		else if ( opc->se )
			entry_free( opc->se );
	}
	if ( freefdn ) {
		op->o_tmpfree( fc.fdn->bv_val, op->o_tmpmemctx );
	}
	op->o_bd = b0;
}

static int
syncprov_op_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *cb = op->o_callback;
	opcookie *opc = cb->sc_private;
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;
	syncmatches *sm, *snext;
	modtarget *mt;

	/* LY: don't use si->si_ops_mutex here to avoid
	 * lock-order reversal (deadlock) with checkpoint()
	 * and mdb's internal write-locking. */
	LDAP_ENSURE(__sync_fetch_and_sub(&si->si_active, 1) > 0);

	for (sm = opc->smatches; sm; sm=snext) {
		snext = sm->sm_next;
		syncprov_unlink_syncop(sm->sm_op, OS_REF_OP_MATCH, SO_LOCKED_NONE );
		op->o_tmpfree( sm, op->o_tmpmemctx );
	}

	/* Remove op from lock table */
	mt = opc->smt;
	if ( mt ) {
		modinst *mi = (modinst *)(opc+1), **pmi;
		ldap_pvt_thread_mutex_lock( &mt->mt_mutex );
		for (pmi = &mt->mt_mods; /* *pmi != NULL */; pmi = &(*pmi)->mi_next ) {
			if (*pmi == mi) {
				*pmi = mi->mi_next;
				if ( mt->mt_tail == mi )
					mt->mt_tail = (pmi != &mt->mt_mods) ? (modinst *) pmi : NULL;
				break;
			}
		}
		/* If there are more, promote the next one */
		if ( mt->mt_mods ) {
			ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
		} else {
			void* __maybe_unused removed;
			ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
			ldap_pvt_thread_mutex_lock( &si->si_mods_mutex );
			removed = avl_delete( &si->si_mods, mt, sp_avl_cmp );
			assert(removed == mt);
			ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
			ldap_pvt_thread_mutex_destroy( &mt->mt_mutex );
			ch_free( mt->mt_dn.bv_val );
			ch_free( mt );
		}
	}
	if ( !BER_BVISNULL( &opc->suuid ))
		op->o_tmpfree( opc->suuid.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &opc->sndn ))
		op->o_tmpfree( opc->sndn.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &opc->sdn ))
		op->o_tmpfree( opc->sdn.bv_val, op->o_tmpmemctx );
	if ( opc->srl.rl_info )
		reslink_detach( &opc->srl );
	op->o_callback = cb->sc_next;
	op->o_tmpfree(cb, op->o_tmpmemctx);

	return 0;
}

static void
syncprov_checkpoint( Operation *op, slap_overinst *on )
{
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;
	Modifications mod;
	Operation opm;
	SlapReply rsm = {REP_RESULT};
	slap_callback cb = {0};
	BackendDB be;
	BackendInfo *bi;
	slap_biglock_t *bl = slap_biglock_get(op->o_bd);

	slap_biglock_acquire(bl);
	if (reopenldap_mode_check())
		slap_cookie_verify( &si->si_cookie );

	mod.sml_numvals = si->si_cookie.numcsns;
	mod.sml_values = si->si_cookie.ctxcsn;
	mod.sml_nvalues = NULL;
	mod.sml_desc = slap_schema.si_ad_contextCSN;
	mod.sml_op = LDAP_MOD_REPLACE;
	mod.sml_flags = SLAP_MOD_INTERNAL;
	mod.sml_next = NULL;

	cb.sc_response = slap_null_cb;
	opm = *op;
	opm.o_tag = LDAP_REQ_MODIFY;
	opm.o_callback = &cb;
	opm.orm_modlist = &mod;
	opm.orm_no_opattrs = 1;
	if ( SLAP_GLUE_SUBORDINATE( op->o_bd )) {
		be = *on->on_info->oi_origdb;
		opm.o_bd = &be;
	}
	opm.o_req_dn = si->si_contextdn;
	opm.o_req_ndn = si->si_contextdn;
	bi = opm.o_bd->bd_info;
	opm.o_bd->bd_info = on->on_info->oi_orig;
	opm.o_managedsait = SLAP_CONTROL_NONCRITICAL;
	opm.o_no_schema_check = 1;
	opm.o_dont_replicate = 1;
	opm.o_opid = -1;
	opm.o_bd->bd_info->bi_op_modify( &opm, &rsm );

	if ( rsm.sr_err == LDAP_NO_SUCH_OBJECT &&
		SLAP_SYNC_SUBENTRY( opm.o_bd )) {
		const char	*text;
		char txtbuf[SLAP_TEXT_BUFLEN];
		size_t textlen = sizeof txtbuf;
		Entry *e = slap_create_context_csn_entry( opm.o_bd, NULL );
		rs_reinit( &rsm, REP_RESULT );
		slap_mods2entry( &mod, &e, 0, 1, &text, txtbuf, textlen);
		opm.ora_e = e;
		opm.o_bd->bd_info->bi_op_add( &opm, &rsm );
		if ( e == opm.ora_e )
			be_entry_release_w( &opm, opm.ora_e );
	}
	opm.o_bd->bd_info = bi;

	if ( rsm.sr_err == LDAP_SUCCESS ) {
		si->si_chklast = op->o_time;
		si->si_numops = 0;
	}

	if ( mod.sml_next != NULL ) {
		slap_mods_free( mod.sml_next, 1 );
	}

	slap_biglock_release(bl);
}

static void
syncprov_add_slog( Operation *op )
{
	opcookie *opc = op->o_callback->sc_private;
	slap_overinst *on = opc->son;
	syncprov_info_t	*si = on->on_bi.bi_private;
	sessionlog *sl = si->si_logs;
	slog_entry *se;

	if ( BER_BVISEMPTY( &op->o_csn ) ) {
		/* During the syncrepl refresh phase we can receive operations
		 * without a csn.  We cannot reliably determine the consumers
		 * state with respect to such operations, so we ignore them and
		 * wipe out anything in the log if we see them.
		 */
		ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
		if (!sl->sl_playing) {
			/* can only do this if no one else is reading the log at the moment */
			while ( (se = sl->sl_head) != NULL ) {
				sl->sl_head = se->se_next;
				ch_free( se );
			}
		}
		sl->sl_tail = NULL;
		sl->sl_num = 0;
		ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
		return;
	}

	/* Allocate a record. UUIDs are not NUL-terminated. */
	se = ch_malloc( sizeof( slog_entry ) + opc->suuid.bv_len +
		op->o_csn.bv_len + 1 );
	se->se_next = NULL;
	se->se_tag = op->o_tag;

	se->se_uuid.bv_val = (char *)(&se[1]);
	memcpy( se->se_uuid.bv_val, opc->suuid.bv_val, opc->suuid.bv_len );
	se->se_uuid.bv_len = opc->suuid.bv_len;

	se->se_csn.bv_val = se->se_uuid.bv_val + opc->suuid.bv_len;
	memcpy( se->se_csn.bv_val, op->o_csn.bv_val, op->o_csn.bv_len );
	se->se_csn.bv_val[op->o_csn.bv_len] = '\0';
	se->se_csn.bv_len = op->o_csn.bv_len;
	se->se_sid = slap_csn_get_sid( &se->se_csn );

	ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
	if ( sl->sl_head ) {
		/* Keep the list in csn order. */
		if ( slap_csn_compare_ts( &sl->sl_tail->se_csn, &se->se_csn ) <= 0 ) {
			sl->sl_tail->se_next = se;
			sl->sl_tail = se;
		} else {
			slog_entry **sep;
			for ( sep = &sl->sl_head; *sep; sep = &(*sep)->se_next ) {
				if ( slap_csn_compare_ts( &se->se_csn, &(*sep)->se_csn ) < 0 ) {
					se->se_next = *sep;
					*sep = se;
					break;
				}
			}
		}
	} else {
		sl->sl_head = se;
		sl->sl_tail = se;
		if ( !sl->sl_cookie.ctxcsn ) {
			sl->sl_cookie.numcsns = 1;
			sl->sl_cookie.ctxcsn = ch_malloc( 2*sizeof( struct berval ));
			sl->sl_cookie.sids = ch_malloc( sizeof( int ));
			sl->sl_cookie.sids[0] = se->se_sid;
			ber_dupbv( sl->sl_cookie.ctxcsn, &se->se_csn );
			BER_BVZERO( &sl->sl_cookie.ctxcsn[1] );
		}
	}
	sl->sl_num++;
	if (!sl->sl_playing) {
		while ( sl->sl_num > sl->sl_size ) {
			int i;
			se = sl->sl_head;
			sl->sl_head = se->se_next;
			for ( i=0; i<sl->sl_cookie.numcsns; i++ )
				if ( sl->sl_cookie.sids[i] >= se->se_sid )
					break;
			if  ( i == sl->sl_cookie.numcsns || sl->sl_cookie.sids[i] != se->se_sid ) {
				slap_insert_csn_sids( &sl->sl_cookie, i, se->se_sid, &se->se_csn );
			} else if ( slap_csn_compare_ts( &sl->sl_cookie.ctxcsn[i], &se->se_csn ) < 0 ) {
				ber_bvreplace( &sl->sl_cookie.ctxcsn[i], &se->se_csn );
			}
			ch_free( se );
			sl->sl_num--;
		}
	}
	ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
}

/* Just set a flag if we found the matching entry */
static int
playlog_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		op->o_callback->sc_private = (void *)1;
	}
	return rs->sr_err;
}

/* enter with sl->sl_mutex locked, release before returning */
static int
syncprov_playlog( Operation *op, sessionlog *sl,
	sync_control *srs, BerVarray ctxcsn, int numcsns, int *sids )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	slog_entry *se;
	int i, j, ndel, num, nmods, mmods;
	char cbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	BerVarray uuids;
	struct berval delcsn[2];
	int rc = LDAP_SUCCESS;

	if ( !sl->sl_num ) {
		ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
		return rc;
	}

	num = sl->sl_num;
	i = 0;
	nmods = 0;
	sl->sl_playing++;
	ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );

	uuids = op->o_tmpalloc( (num+1) * sizeof( struct berval ) +
		num * UUID_LEN, op->o_tmpmemctx );
	uuids[0].bv_val = (char *)(uuids + num + 1);

	delcsn[0].bv_len = 0;
	delcsn[0].bv_val = cbuf;
	BER_BVZERO(&delcsn[1]);

	/* Make a copy of the relevant UUIDs. Put the Deletes up front
	 * and everything else at the end. Do this first so we can
	 * unlock the list mutex.
	 */
	Debug( LDAP_DEBUG_SYNC, "srs csn %s\n",
		srs->sr_state.ctxcsn[0].bv_val );
	for ( se=sl->sl_head; se; se=se->se_next ) {
		int k;
		Debug( LDAP_DEBUG_SYNC, "log csn %s\n", se->se_csn.bv_val );
		ndel = 1;
		for ( k=0; k<srs->sr_state.numcsns; k++ ) {
			if ( se->se_sid == srs->sr_state.sids[k] ) {
				ndel = slap_csn_compare_ts( &se->se_csn, &srs->sr_state.ctxcsn[k] );
				break;
			}
		}
		if ( ndel <= 0 ) {
			Debug( LDAP_DEBUG_SYNC, "cmp %d, too old\n", ndel );
			continue;
		}
		ndel = 0;
		for ( k=0; k<numcsns; k++ ) {
			if ( se->se_sid == sids[k] ) {
				ndel = slap_csn_compare_ts( &se->se_csn, &ctxcsn[k] );
				break;
			}
		}
		if ( ndel > 0 ) {
			Debug( LDAP_DEBUG_SYNC, "cmp %d, too new\n", ndel );
			break;
		}
		if ( se->se_tag == LDAP_REQ_DELETE ) {
			j = i;
			i++;
			memcpy( cbuf, se->se_csn.bv_val, se->se_csn.bv_len );
			delcsn[0].bv_len = se->se_csn.bv_len;
			delcsn[0].bv_val[delcsn[0].bv_len] = '\0';
		} else {
			if ( se->se_tag == LDAP_REQ_ADD )
				continue;
			nmods++;
			j = num - nmods;
		}
		uuids[j].bv_val = uuids[0].bv_val + (j * UUID_LEN);
		memcpy(uuids[j].bv_val, se->se_uuid.bv_val, UUID_LEN);
		uuids[j].bv_len = UUID_LEN;
	}
	ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
	sl->sl_playing--;
	ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );

	ndel = i;

	/* Zero out unused slots */
	for ( i=ndel; i < num - nmods; i++ )
		uuids[i].bv_len = 0;

	/* Mods must be validated to see if they belong in this delete set. */

	mmods = nmods;
	/* Strip any duplicates */
	for ( i=0; i<nmods; i++ ) {
		for ( j=0; j<ndel; j++ ) {
			if ( bvmatch( &uuids[j], &uuids[num - 1 - i] )) {
				uuids[num - 1 - i].bv_len = 0;
				mmods --;
				break;
			}
		}
		if ( uuids[num - 1 - i].bv_len == 0 ) continue;
		for ( j=0; j<i; j++ ) {
			if ( bvmatch( &uuids[num - 1 - j], &uuids[num - 1 - i] )) {
				uuids[num - 1 - i].bv_len = 0;
				mmods --;
				break;
			}
		}
	}

	if ( mmods ) {
		Operation fop;
		int rc __maybe_unused;
		Filter mf, af;
		AttributeAssertion eq = ATTRIBUTEASSERTION_INIT;
		slap_callback cb = {0};

		slap_op_copy(op, &fop, NULL, NULL);

		fop.o_sync = 0;
		fop.o_callback = &cb;
		fop.ors_limit = NULL;
		fop.ors_tlimit = SLAP_NO_LIMIT;
		fop.ors_attrs = slap_anlist_all_attributes;
		fop.ors_attrsonly = 0;
		fop.o_managedsait = SLAP_CONTROL_CRITICAL;

		af.f_choice = LDAP_FILTER_AND;
		af.f_next = NULL;
		af.f_and = &mf;
		mf.f_choice = LDAP_FILTER_EQUALITY;
		mf.f_ava = &eq;
		mf.f_av_desc = slap_schema.si_ad_entryUUID;
		mf.f_next = fop.ors_filter;

		fop.ors_filter = &af;

		cb.sc_response = playlog_cb;
		fop.o_bd->bd_info = (BackendInfo *)on->on_info;

		for ( i=ndel; i<num; i++ ) {
		  if ( uuids[i].bv_len != 0 ) {
			SlapReply frs = { REP_RESULT };

			mf.f_av_value = uuids[i];
			cb.sc_private = NULL;
			fop.ors_slimit = 1;
			rc = fop.o_bd->be_search( &fop, &frs );
			/* ignore result */

			/* If entry was not found, add to delete list */
			if ( !cb.sc_private ) {
				uuids[ndel++] = uuids[i];
			}
		  }
		}
		fop.o_bd->bd_info = (BackendInfo *)on;
	}
	if ( ndel ) {
		struct berval cookie = BER_BVNULL;

		if ( delcsn[0].bv_len ) {
			syncprov_compose_cookie( op, &cookie, delcsn, srs->sr_state.rid );
			Debug( LDAP_DEBUG_SYNC, "syncprov_playlog: cookie=%s\n", cookie.bv_val );
		}

		uuids[ndel].bv_val = NULL;
		rc = syncprov_sendinfo( op, LDAP_TAG_SYNC_ID_SET,
			delcsn[0].bv_len ? &cookie : NULL, 0, uuids, 1 );
		if ( delcsn[0].bv_len ) {
			op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
		}
	}
	op->o_tmpfree( uuids, op->o_tmpmemctx );
	return rc;
}

static int
syncprov_new_ctxcsn_and_mayrwunlock( opcookie *opc, syncprov_info_t *si, BerVarray vals, int csn_changed)
{
	if ( slap_cookie_merge_csnset( NULL, &si->si_cookie, vals ) > 0 ) {
		Debug( LDAP_DEBUG_SYNC, "syncprov_op_response: cookie-forward\n" );
		csn_changed = 1;
	} else {
		Debug( LDAP_DEBUG_SYNC, "syncprov_op_response: cookie-kept (mod-context-csn '%s')\n", vals->bv_val );
	}

	if ( csn_changed ) {
		si->si_dirty = 0;
		si->si_numops++;
		ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );

		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		for ( syncops *ss = si->si_ops; ss; ss = ss->s_next ) {
			/* Send the updated csn to all syncrepl consumers,
			 * including the server from which it originated.
			 * The syncrepl consumer and syncprov provider on
			 * the originating server may be configured to store
			 * their csn values in different entries. */
			syncprov_qresp( opc, ss, LDAP_SYNC_NEW_COOKIE );
		}
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
	}
	return csn_changed;
}

static int
syncprov_op_response( Operation *op, SlapReply *rs )
{
	opcookie *opc = op->o_callback->sc_private;
	slap_overinst *on = opc->son;
	syncprov_info_t		*si = on->on_bi.bi_private;
	syncmatches *sm;
	const char* oper = NULL;
	Attribute *a;
	BerValue *entry_uuid = NULL;
	BerValue *entry_csn = NULL;
	BerValue *entry_dn = NULL;
	struct berval maxcsn;
	char cbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	int maxcsn_sid;

	assert( BER_BVISEMPTY( &opc->sctxcsn ) );
	if ( rs->sr_err != LDAP_SUCCESS )
		return SLAP_CB_CONTINUE;

	switch(op->o_tag) {
		default:
			LDAP_BUG();
			oper = "???"; break;
		case LDAP_REQ_ADD:
			entry_dn = &op->ora_e->e_nname;
			a = attr_find( op->ora_e->e_attrs, slap_schema.si_ad_entryUUID );
			if (a)
				entry_uuid = a->a_vals;
			a = attr_find( op->ora_e->e_attrs, slap_schema.si_ad_entryCSN );
			if (a)
				entry_csn = a->a_nvals;
			oper = "ADD"; break;
		case LDAP_REQ_DELETE:
			entry_dn = &op->o_req_ndn;
			oper = "DELETE"; break;
		case LDAP_REQ_MODIFY: {
			Modifications *mod;
			entry_dn = &op->o_req_ndn;
			for ( mod = op->orm_modlist; mod; mod = mod->sml_next ) {
				if ( mod->sml_mod.sm_op != LDAP_MOD_REPLACE
						&& mod->sml_mod.sm_op != LDAP_MOD_ADD )
					continue;
				if ( mod->sml_mod.sm_desc == slap_schema.si_ad_entryCSN )
					entry_csn = mod->sml_mod.sm_nvalues;
				if ( mod->sml_mod.sm_desc == slap_schema.si_ad_entryUUID )
					entry_uuid = mod->sml_mod.sm_values;
			}
			oper = "MODIFY"; break;
		}
		case LDAP_REQ_RENAME:
			oper = "RENAME"; break;
		case LDAP_REQ_EXTENDED:
			oper = "EXTENDED"; break;
		}

		Debug( LDAP_DEBUG_SYNC, "syncprov_op_response: %s entry%s %s, %s, %s\n",
			   oper,
			   op->o_dont_replicate ? "-dont-replicate" : "",
			   entry_dn ? entry_dn->bv_val : NULL,
			   entry_csn ? entry_csn->bv_val : NULL,
			   entry_uuid ? entry_uuid->bv_val : NULL
			);

		ldap_pvt_thread_mutex_lock( &si->si_resp_mutex );
		ldap_pvt_thread_rdwr_wlock( &si->si_csn_rwlock );

		/* Update our context CSN */
		maxcsn.bv_val = cbuf;
		maxcsn.bv_len = sizeof(cbuf);
		maxcsn_sid = slap_get_commit_csn( op, &maxcsn );
		if ( BER_BVISEMPTY( &maxcsn ) && SLAP_GLUE_SUBORDINATE( op->o_bd )) {
			/* syncrepl queues the CSN values in the db where
			 * it is configured , not where the changes are made.
			 * So look for a value in the glue db if we didn't
			 * find any in this db.
			 */
			BackendDB *be = op->o_bd;
			op->o_bd = select_backend( &be->be_nsuffix[0], 1);
			maxcsn.bv_val = cbuf;
			maxcsn.bv_len = sizeof(cbuf);
			maxcsn_sid = slap_get_commit_csn( op, &maxcsn );
			op->o_bd = be;
		}

		int csn_changed = 0;
		if ( !BER_BVISEMPTY( &maxcsn )
				&& slap_cookie_merge_csn( op->o_bd, &si->si_cookie,
						maxcsn_sid, &maxcsn ) > 0 ) {
			Debug( LDAP_DEBUG_SYNC,
			   "syncprov_op_response: cookie-forward, opc->sctxcsn => %s, n-dirty was %d\n",
				maxcsn.bv_val, si->si_dirty);
			csn_changed = 1;
		} else {
			Debug( LDAP_DEBUG_SYNC,
				"syncprov_op_response: cookie-kept (maxcsn '%s')\n", maxcsn.bv_val );
		}

		/* Don't do any processing for consumer contextCSN updates */
		if ( SLAPD_SYNC_IS_SYNCCONN( op->o_connid ) &&
			op->o_tag == LDAP_REQ_MODIFY && op->orm_modlist &&
			op->orm_modlist->sml_op == LDAP_MOD_REPLACE &&
			op->orm_modlist->sml_desc == slap_schema.si_ad_contextCSN ) {
			/* Catch contextCSN updates from syncrepl. We have to look at
			 * all the attribute values, as there may be more than one csn
			 * that changed, and only one can be passed in the csn queue. */
			if (!syncprov_new_ctxcsn_and_mayrwunlock(opc, si, op->orm_modlist->sml_values, csn_changed))
				ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );
			goto leave;
		}
		if ( op->o_dont_replicate ) {
			if ( csn_changed )
				si->si_numops++;
			ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );
			goto leave;
		}

		if ( op->o_tag == LDAP_REQ_ADD &&
				dn_match( &op->o_req_ndn, &si->si_contextdn )) {
			/* If we're adding the context entry, parse all of its contextCSNs */
			Attribute *a = attr_find( op->ora_e->e_attrs, slap_schema.si_ad_contextCSN );
			if (a && syncprov_new_ctxcsn_and_mayrwunlock(opc, si, a->a_vals, csn_changed)) {
				/* LY: update maxcsn from a->a_vals here ? */
				goto skip_unlock;
			}
		} else {
			/* Never checkpoint adding the context entry, it will deadlock */
			if (((si->si_chkops && si->si_numops >= si->si_chkops) ||
					(si->si_chktime && op->o_time - si->si_chklast >= si->si_chktime))) {
				syncprov_checkpoint( op, on );
			}
		}

		si->si_dirty = csn_changed ? 0 : si->si_dirty + 1;
		si->si_numops += csn_changed;
		ldap_pvt_thread_rdwr_wunlock( &si->si_csn_rwlock );

skip_unlock:
		/* only update consumer ctx if this is a newer csn */
		if (csn_changed) {
			opc->sctxcsn = maxcsn;
		}

		/* Handle any persistent searches */
		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		int have_psearches = ( si->si_ops != NULL );
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
		if ( have_psearches ) {
			switch(op->o_tag) {
			case LDAP_REQ_ADD:
			case LDAP_REQ_MODIFY:
			case LDAP_REQ_MODRDN:
			case LDAP_REQ_EXTENDED:
				syncprov_matchops( op, opc, 0 );
				break;
			case LDAP_REQ_DELETE:
				/* for each match in opc->smatches:
				 *   send DELETE msg
				 */
				for ( sm = opc->smatches; sm; sm=sm->sm_next ) {
					syncprov_qresp( opc, sm->sm_op, LDAP_SYNC_DELETE );
				}
				break;
			}
		}

		/* Add any log records */
		if ( si->si_logs )
			syncprov_add_slog( op );

	leave:
		ldap_pvt_thread_mutex_unlock( &si->si_resp_mutex );
	return SLAP_CB_CONTINUE;
}

/* We don't use a subentry to store the context CSN any more.
 * We expose the current context CSN as an operational attribute
 * of the suffix entry.
 */
static int
syncprov_op_compare( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;
	int rc = SLAP_CB_CONTINUE;

	if ( dn_match( &op->o_req_ndn, &si->si_contextdn ) &&
		op->oq_compare.rs_ava->aa_desc == slap_schema.si_ad_contextCSN )
	{
		Entry e = {0};
		Attribute a = {0};

		e.e_name = si->si_contextdn;
		e.e_nname = si->si_contextdn;
		e.e_attrs = &a;

		a.a_desc = slap_schema.si_ad_contextCSN;

		ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );

		if (reopenldap_mode_check())
			slap_cookie_verify( &si->si_cookie );
		a.a_vals = si->si_cookie.ctxcsn;
		a.a_nvals = a.a_vals;
		a.a_numvals = si->si_cookie.numcsns;

		rs->sr_err = access_allowed( op, &e, op->oq_compare.rs_ava->aa_desc,
			&op->oq_compare.rs_ava->aa_value, ACL_COMPARE, NULL );
		if ( ! rs->sr_err ) {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
			goto return_results;
		}

		if ( get_assert( op ) &&
			( test_filter( op, &e, get_assertion( op ) ) != LDAP_COMPARE_TRUE ) )
		{
			rs->sr_err = LDAP_ASSERTION_FAILED;
			goto return_results;
		}

		rs->sr_err = LDAP_COMPARE_FALSE;

		if ( attr_valfind( &a,
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				&op->oq_compare.rs_ava->aa_value, NULL, op->o_tmpmemctx ) == 0 )
		{
			rs->sr_err = LDAP_COMPARE_TRUE;
		}

return_results:;

		ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );

		send_ldap_result( op, rs );

		if( rs->sr_err == LDAP_COMPARE_FALSE || rs->sr_err == LDAP_COMPARE_TRUE ) {
			rs->sr_err = LDAP_SUCCESS;
		}
		rc = rs->sr_err;
	}

	return rc;
}

static int
syncprov_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = on->on_bi.bi_private;
	slap_callback *cb;
	opcookie *opc;
	int have_psearches, cbsize;

	/* LY: aquire si->si_ops_mutex here only to synchronize with op_search() path. */
	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
	have_psearches = ( si->si_ops != NULL );
	LDAP_ENSURE(__sync_fetch_and_add(&si->si_active, 1) >= 0);
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	cbsize = sizeof(slap_callback) + sizeof(opcookie) +
		( have_psearches ? sizeof(modinst) : 0 );

	cb = op->o_tmpcalloc(1, cbsize, op->o_tmpmemctx);
	opc = (opcookie *)(cb+1);
	opc->son = on;
	cb->sc_response = syncprov_op_response;
	cb->sc_cleanup = syncprov_op_cleanup;
	cb->sc_private = opc;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;

	opc->osid = -1;
	opc->rsid = -1;
	if ( ! BER_BVISEMPTY( &op->o_csn ) ) {
		opc->osid = slap_csn_get_sid( &op->o_csn );
	}
	if ( op->o_controls ) {
		struct sync_cookie *cookie = op->o_controls[slap_cids.sc_LDAPsync];
		if ( cookie ) {
			if (reopenldap_mode_check())
				slap_cookie_verify( cookie );
			opc->rsid = cookie->sid;
		}
	}

	if (reopenldap_mode_check()) {
		ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
		slap_cookie_verify( &si->si_cookie );
		ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
	}

	if ( op->o_tag == LDAP_REQ_MODIFY
			&& dn_match( &op->o_req_ndn, &si->si_contextdn ) ) {
		Modifications *mod;
		for ( mod = op->orm_modlist; mod; mod = mod->sml_next ) {
			if ( mod->sml_mod.sm_desc == slap_schema.si_ad_contextCSN ) {
				if (mod->sml_op == LDAP_MOD_REPLACE) {
					ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
					int vector = slap_cookie_compare_csnset( &si->si_cookie, mod->sml_values );
					if ( vector <= 0 && mod->sml_next == NULL ) {
						/* LY: actually here no any changes,
						 * allow to backend skip it if no penging updates. */
						if ( si->si_numops == 0 ) {
							Debug( LDAP_DEBUG_SYNC,
								   "syncprov_op_mod: hollow contextCSN update %s\n",
								   op->o_req_ndn.bv_val );
							op->o_hollow = 1;
						}
					}
					ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
				}
				break;
			}
		}
	}

	if ( op->o_dont_replicate )
		return SLAP_CB_CONTINUE;

	/* If there are active persistent searches, lock this operation.
	 * See seqmod.c for the locking logic on its own.
	 */
	if ( have_psearches ) {
		modtarget *mt, mtdummy;
		modinst *mi;

		mi = (modinst *)(opc+1);
		mi->mi_op = op;

		/* See if we're already modifying this entry... */
		mtdummy.mt_dn = op->o_req_ndn;
retry:
		ldap_pvt_thread_mutex_lock( &si->si_mods_mutex );
		mt = avl_find( si->si_mods, &mtdummy, sp_avl_cmp );
		if ( mt ) {
			ldap_pvt_thread_mutex_lock( &mt->mt_mutex );
			if ( mt->mt_mods == NULL ) {
				/* Cannot reuse this mt, as another thread is about
				 * to release it in syncprov_op_cleanup. Wait for them
				 * to finish; our own insert is required to succeed.
				 */
				ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
				/* LY: Try again, otherwise could be a failure by dup-insertion into the avl-tree.
				 *     This was fixed before ITS#8081 and bug was present before ITS#8063.
				 */
				ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
				ldap_pvt_thread_yield();
				goto retry;
			}
		}
		if ( mt ) {
			mt->mt_tail->mi_next = mi;
			mt->mt_tail = mi;
			ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
			/* wait for this op to get to head of list */
			while ( mt->mt_mods != mi ) {
				modinst *m2;
				/* don't wait on other mods from the same thread */
				for ( m2 = mt->mt_mods; m2; m2 = m2->mi_next ) {
					if ( m2->mi_op->o_threadctx == op->o_threadctx )
						break;
				}
				if ( m2 )
					break;

				ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
				/* FIXME: if dynamic config can delete overlays or
				 * databases we'll have to check for cleanup here.
				 * Currently it's not an issue because there are
				 * no dynamic config deletes...
				 */
				if ( slapd_shutdown )
					return SLAPD_ABANDON;

				if ( ! slap_biglock_pool_pausecheck(op->o_bd) )
					ldap_pvt_thread_yield();
				ldap_pvt_thread_mutex_lock( &mt->mt_mutex );

				/* clean up if the caller is giving up */
				if ( slap_get_op_abandon(op) ) {
					slap_callback **pcb;
					modinst **pmi;
					for (pmi = &mt->mt_mods; /* *pmi != NULL */; pmi = &(*pmi)->mi_next ) {
						if (*pmi == mi) {
							*pmi = mi->mi_next;
							if ( mt->mt_tail == mi )
								mt->mt_tail = (pmi != &mt->mt_mods) ? (modinst *) pmi : NULL;
							break;
						}
					}
					for (pcb = &op->o_callback; /* *pcb != NULL */; pcb = &(*pcb)->sc_next ) {
						if (*pcb == cb) {
							*pcb = cb->sc_next;
							break;
						}
					}
					op->o_tmpfree( cb, op->o_tmpmemctx );
					ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
					return SLAPD_ABANDON;
				}
			}
			ldap_pvt_thread_mutex_unlock( &mt->mt_mutex );
		} else {
			int __maybe_unused avl_err;
			/* Record that we're modifying this entry now */
			mt = ch_malloc( sizeof(modtarget) );
			mt->mt_mods = mi;
			mt->mt_tail = mi;
			ber_dupbv( &mt->mt_dn, &mi->mi_op->o_req_ndn );
			ldap_pvt_thread_mutex_init( &mt->mt_mutex );
			avl_err = avl_insert( &si->si_mods, mt, sp_avl_cmp, avl_dup_error );
			assert(avl_err == 0);
			ldap_pvt_thread_mutex_unlock( &si->si_mods_mutex );
		}
		opc->smt = mt;
	}

	if (( have_psearches || si->si_logs ) && op->o_tag != LDAP_REQ_ADD )
		syncprov_matchops( op, opc, 1 );

	return SLAP_CB_CONTINUE;
}

static int
syncprov_op_extended( Operation *op, SlapReply *rs )
{
	if ( exop_is_write( op ))
		return syncprov_op_mod( op, rs );

	return SLAP_CB_CONTINUE;
}

typedef struct SyncOperationBuffer {
	Operation		sob_op;
	Opheader		sob_hdr;
	OpExtra			sob_oe;
	AttributeName	sob_extra;	/* not always present */
	/* Further data allocated here */
} SyncOperationBuffer;

static void
syncprov_detach_op( Operation *op, syncops *so, slap_overinst *on )
{
	SyncOperationBuffer *sopbuf2;
	Operation *op2;
	int i, alen = 0;
	size_t size;
	char *ptr;
	GroupAssertion *g1, *g2;

	assert(so->s_flags & OS_REF_MASK);
	assert(so->s_op == op);
	/* count the search attrs */
	for (i=0; op->ors_attrs && !BER_BVISNULL( &op->ors_attrs[i].an_name ); i++) {
		alen += op->ors_attrs[i].an_name.bv_len + 1;
	}
	/* Make a new copy of the operation */
	size = offsetof( SyncOperationBuffer, sob_extra ) +
		(i ? ( (i+1) * sizeof(AttributeName) + alen) : 0) +
		op->o_req_dn.bv_len + 1 +
		op->o_req_ndn.bv_len + 1 +
		op->o_ndn.bv_len + 1 +
		so->s_filterstr.bv_len + 1;
	sopbuf2 = ch_calloc( 1, size );
	op2 = &sopbuf2->sob_op;
	op2->o_hdr = &sopbuf2->sob_hdr;
	LDAP_SLIST_FIRST(&op2->o_extra) = &sopbuf2->sob_oe;

	/* Copy the fields we care about explicitly, leave the rest alone */
	*op2->o_hdr = *op->o_hdr;
	op2->o_tag = op->o_tag;
	op2->o_time = op->o_time;
	op2->o_bd = on->on_info->oi_origdb;
	op2->o_request = op->o_request;
	op2->o_managedsait = op->o_managedsait;
	LDAP_SLIST_FIRST(&op2->o_extra)->oe_key = on;
	LDAP_SLIST_NEXT(LDAP_SLIST_FIRST(&op2->o_extra), oe_next) = NULL;

	ptr = (char *) sopbuf2 + offsetof( SyncOperationBuffer, sob_extra );
	if ( i ) {
		op2->ors_attrs = (AttributeName *) ptr;
		ptr = (char *) &op2->ors_attrs[i+1];
		for (i=0; !BER_BVISNULL( &op->ors_attrs[i].an_name ); i++) {
			op2->ors_attrs[i] = op->ors_attrs[i];
			op2->ors_attrs[i].an_name.bv_val = ptr;
			ptr = lutil_strcopy( ptr, op->ors_attrs[i].an_name.bv_val ) + 1;
		}
		BER_BVZERO( &op2->ors_attrs[i].an_name );
	}

	op2->o_authz = op->o_authz;
	op2->o_ndn.bv_val = ptr;
	ptr = lutil_strcopy(ptr, op->o_ndn.bv_val) + 1;
	op2->o_dn = op2->o_ndn;
	op2->o_req_dn.bv_len = op->o_req_dn.bv_len;
	op2->o_req_dn.bv_val = ptr;
	ptr = lutil_strcopy(ptr, op->o_req_dn.bv_val) + 1;
	op2->o_req_ndn.bv_len = op->o_req_ndn.bv_len;
	op2->o_req_ndn.bv_val = ptr;
	ptr = lutil_strcopy(ptr, op->o_req_ndn.bv_val) + 1;
	op2->ors_filterstr.bv_val = ptr;
	strcpy( ptr, so->s_filterstr.bv_val );
	op2->ors_filterstr.bv_len = so->s_filterstr.bv_len;

	/* Skip the AND/GE clause that we stuck on in front */
	if ( so->s_flags & PS_FIX_FILTER ) {
		op2->ors_filter = op->ors_filter->f_and->f_next;
		so->s_flags ^= PS_FIX_FILTER;
	} else {
		op2->ors_filter = op->ors_filter;
	}
	op2->ors_filter = filter_dup( op2->ors_filter, NULL );
	so->s_op_safe = so->s_op = op2;

	/* Copy any cached group ACLs individually */
	op2->o_groups = NULL;
	for ( g1=op->o_groups; g1; g1=g1->ga_next ) {
		g2 = ch_malloc( sizeof(GroupAssertion) + g1->ga_len );
		*g2 = *g1;
		strcpy( g2->ga_ndn, g1->ga_ndn );
		g2->ga_next = op2->o_groups;
		op2->o_groups = g2;
	}
	/* Don't allow any further group caching */
	op2->o_do_not_cache = 1;

	/* Prevent anyone else from trying to send a result for this op */
	so->s_flags |= PS_IS_DETACHED;
	slap_set_op_abandon(op, 1);
	/* LY: Icing on the cake - this is a crutch/workaround
	 * for https://github.com/leo-yuriev/ReOpenLDAP/issues/47 */
	op->o_msgid += ~((~0u) >> 1);

	/* Add op2 to conn so abandon will find us */
	op2->o_conn->c_n_ops_executing++;
	op2->o_conn->c_n_ops_completed--;
	LDAP_STAILQ_INSERT_TAIL( &op2->o_conn->c_ops, op2, o_next );
}

static void
syncprov_refresh_end(syncprov_info_t *si, syncops *so)
{
	assert(slap_tsan__read_int(&si->si_psearches) > 0);
	assert(so->s_flags & PS_IS_REFRESHING);
	assert(si->si_prefresh > 0);
	/* Turn off the refreshing flag */
	so->s_flags -= PS_IS_REFRESHING;
	si->si_prefresh -= 1;
	Debug( LDAP_DEBUG_SYNC,
		"syncprov-search: sid 0x%03x, refresh-end\n",
		so->s_sid );
}

static int
syncprov_search_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *cb = op->o_callback;
	searchstate *ss = cb->sc_private;
	syncops *so = ss->ss_so;

	if ( rs->sr_err != LDAP_SUCCESS && rs->sr_type == REP_RESULT
			&& !slap_get_op_abandon(op) && so != NULL ) {
		ss->ss_so = NULL;
		cb->sc_cleanup = NULL;
		ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
		if ( !slap_get_op_abandon(op) ) {
			Debug( LDAP_DEBUG_SYNC,
				"syncprov-search: sid 0x%03x, refresh-abort, type %d, rc %d\n",
				so->s_sid, rs->sr_type, rs->sr_err );
			syncprov_unlink_syncop( so, OS_REF_OP_SEARCH, SO_LOCKED_CONN );
		} else {
			/* LY: rare race with abandon */
		}
		ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
	}

	return SLAP_CB_CONTINUE;
}

static int
syncprov_search_response( Operation *op, SlapReply *rs )
{
	slap_callback *cb = op->o_callback;
	searchstate *ss = cb->sc_private;
	slap_overinst *on = ss->ss_on;
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;
	sync_control *srs = op->o_controls[slap_cids.sc_LDAPsync];

	if ( rs->sr_type == REP_SEARCH || rs->sr_type == REP_SEARCHREF ) {
		Attribute *entryCSN = NULL;
		/* If we got a referral without a referral object, there's
		 * something missing that we cannot replicate. Just ignore it.
		 * The consumer will abort because we didn't send the expected
		 * control.
		 */
		if ( !rs->sr_entry ) {
			Debug( LDAP_DEBUG_ANY, "bogus referral in context\n" );
			assert( rs->sr_entry != NULL );
			return SLAP_CB_CONTINUE;
		}
		if ( rs->sr_operational_attrs != NULL	 )
			entryCSN = attr_find( rs->sr_operational_attrs, slap_schema.si_ad_entryCSN );
		if ( entryCSN == NULL )
			entryCSN = attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_entryCSN );
		if ( entryCSN ) {
			int i, sid;
			sid = slap_csn_get_sid( &entryCSN->a_nvals[0] );

			/* If not a persistent search */
			if ( !(op->o_sync & SLAP_SYNC_PERSIST) ) {
				/* Make sure entry is less than the snapshot'd contextCSN */
				for ( i=0; i<ss->ss_numcsns; i++ ) {
					if ( sid == ss->ss_sids[i] && slap_csn_compare_ts( &entryCSN->a_nvals[0],
						&ss->ss_ctxcsn[i] ) > 0 ) {
						Debug( LDAP_DEBUG_SYNC,
							"syncprov-response: sid 0x%03x, Entry %s, %s > snapshot %s, skip\n",
							srs->sr_state.sid,
							rs->sr_entry->e_name.bv_val,
							entryCSN->a_nvals[0].bv_val,
							ss->ss_ctxcsn[i].bv_val );
						return LDAP_SUCCESS;
					}
				}
			}

			/* Don't send old entries twice */
			for ( i=0; i<srs->sr_state.numcsns; i++ ) {
				if ( sid == srs->sr_state.sids[i] &&
					slap_csn_compare_ts( &entryCSN->a_nvals[0],
						&srs->sr_state.ctxcsn[i] ) <= 0 ) {
					Debug( LDAP_DEBUG_SYNC,
						"syncprov-response: %03x, Entry %s, %s <= consumer %s, skip\n",
						srs->sr_state.sid,
						rs->sr_entry->e_name.bv_val,
						entryCSN->a_nvals[0].bv_val,
						srs->sr_state.ctxcsn[i].bv_val );
					return LDAP_SUCCESS;
				}
			}
		}
		rs->sr_ctrls = op->o_tmpalloc( sizeof(LDAPControl *)*2,
			op->o_tmpmemctx );
		rs->sr_ctrls[1] = NULL;
		rs->sr_flags |= REP_CTRLS_MUSTBEFREED;
		/* If we're in delta-sync mode, always send a cookie */
		if ( si->si_nopres && si->si_usehint && entryCSN ) {
			rs->sr_err = syncprov_state_ctrl( op, rs, rs->sr_entry,
				LDAP_SYNC_ADD, rs->sr_ctrls, 0, entryCSN->a_nvals, srs->sr_state.rid );
		} else {
			rs->sr_err = syncprov_state_ctrl( op, rs, rs->sr_entry,
				LDAP_SYNC_ADD, rs->sr_ctrls, 0, NULL, 0 );
		}
	} else if ( rs->sr_type == REP_RESULT && rs->sr_err == LDAP_SUCCESS ) {
		struct berval cookie = BER_BVNULL;

		if ((( ss->ss_flags & (SS_CHANGED | SS_PRESENT) )
				&& ss->ss_ctxcsn && !BER_BVISNULL( &ss->ss_ctxcsn[0] )) ) {
			syncprov_compose_cookie( op, &cookie, ss->ss_ctxcsn, srs->sr_state.rid );
			Debug( LDAP_DEBUG_SYNC, "syncprov-response: cookie=%s\n", cookie.bv_val );
		}

		/* Is this a regular refresh?
		 * Note: refresh never gets here if there were no changes
		 */
		if ( !(op->o_sync & SLAP_SYNC_PERSIST) ) {
			rs->sr_ctrls = op->o_tmpalloc( sizeof(LDAPControl *)*2,
				op->o_tmpmemctx );
			rs->sr_ctrls[1] = NULL;
			rs->sr_flags |= REP_CTRLS_MUSTBEFREED;
			rs->sr_err = syncprov_done_ctrl( op, rs, rs->sr_ctrls,
				0, &cookie, ( ss->ss_flags & SS_PRESENT )
					? LDAP_SYNC_REFRESH_PRESENTS : LDAP_SYNC_REFRESH_DELETES );
			if ( !BER_BVISNULL( &cookie ))
				op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
		} else {
			/* It's RefreshAndPersist, transition to Persist phase */
			rs->sr_err = syncprov_sendinfo( op, ( ss->ss_flags & SS_PRESENT ) ?
				LDAP_TAG_SYNC_REFRESH_PRESENT : LDAP_TAG_SYNC_REFRESH_DELETE,
				BER_BVISNULL(&cookie) ? NULL : &cookie,
				1, NULL, 0 );
			if ( !BER_BVISNULL( &cookie ))
				op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );

			if ( rs->sr_err == LDAP_SUCCESS && !srs->sr_jammed ) {
				syncops *so = ss->ss_so;
				ss->ss_so = NULL;

				/* Detach this Op from frontend control */
				ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );

				/* But not if this connection was closed along the way */
				if (unlikely(slap_get_op_abandon(op))) {
	abandon:
					ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
					/* syncprov_abandon_cleanup will free this syncop */
					return SLAPD_ABANDON;
				}

				assert(slap_tsan__read_int(&so->s_flags) & OS_REF_MASK);
				ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
				ldap_pvt_thread_mutex_lock( &so->s_mutex );
				assert(so->s_flags & OS_REF_MASK);

				if (unlikely(is_syncops_abandoned(so))) {
					ldap_pvt_thread_mutex_unlock( &so->s_mutex );
					ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
					goto abandon;
				}

				syncprov_refresh_end( si, so );
				syncprov_detach_op( op, so, on );
				ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

				/* If there are queued responses, fire them off */
				if ( so->s_rl )
					syncprov_playback_enqueue( so );
				ldap_pvt_thread_mutex_unlock( &so->s_mutex );
				ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
				/* LY: psearch was detached, couldn't SLAP_CB_CONTINUE */
				return LDAP_SUCCESS;
			}
		}

		if ( srs->sr_jammed && rs->sr_err == LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_SYNC,
				"syncprov-response: sid 0x%03x,%s%s, force resync\n",
				srs->sr_state.sid, srs->sr_jammed ? " jammed" : "",
				(ss->ss_flags & SS_PRESENT) ? "" : " no-present" );
			send_ldap_error(op, rs,
				LDAP_SYNC_REFRESH_REQUIRED, "unable to provide robust sync (jammed)");
		}
		if ( rs->sr_err != LDAP_SUCCESS ) {
			/* LY: return error if have, otherwise just continue */
			return rs->sr_err;
		}
	}

	return SLAP_CB_CONTINUE;
}

static int
syncprov_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;
	slap_callback *cb;
	int gotstate = 0, changed = 0, do_present = 0;
	syncops *so = NULL;
	searchstate *ss;
	sync_control *srs;
	BerVarray ctxcsn = NULL;
	BerValue pivot_csn = {0};
	int i, *sids = NULL, numcsns;
	int dirty = 0;
	int rc;

	if ( !(op->o_sync & SLAP_SYNC_REFRESH) )
		return SLAP_CB_CONTINUE;

	if ( op->ors_deref & LDAP_DEREF_SEARCHING ) {
		send_ldap_error( op, rs, LDAP_PROTOCOL_ERROR, "illegal value for derefAliases" );
		return rs->sr_err;
	}

	srs = op->o_controls[slap_cids.sc_LDAPsync];
	if ( op->o_sync & SLAP_SYNC_PERSIST ) {
		fbase_cookie fc = {0};
		opcookie opc = {0};
		slap_callback sc = {0};

		so = ch_calloc( 1, sizeof( syncops ) );
		fc.fss = so;
		fc.fbase = 0;
		so->s_eid = NOID;
		so->s_op_safe = so->s_op = op;
		so->s_flags = PS_IS_REFRESHING | PS_FIND_BASE;
		so->s_rid = srs->sr_state.rid;
		so->s_sid = srs->sr_state.sid;
		ldap_pvt_thread_mutex_init( &so->s_mutex );

		/* syncprov_findbase expects to be called as a callback... */
		sc.sc_private = &opc;
		opc.son = on;
		cb = op->o_callback;
		op->o_callback = &sc;
		rs->sr_err = syncprov_findbase( op, &fc );
		op->o_callback = cb;

		if ( rs->sr_err != LDAP_SUCCESS ) {
			ch_free( so );
			rs->sr_text = "wrong sync base";
			send_ldap_result( op, rs );
			return rs->sr_err;
		}
	}

	ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );

	/* If this is a persistent search, set it up right away */
	if ( op->o_sync & SLAP_SYNC_PERSIST ) {
		while ( slap_tsan__read_int(&si->si_active) ) {
			/* Wait for active mods to finish before proceeding, as they
			 * may already have inspected the si_ops list looking for
			 * consumers to replicate the change to.  Using the log
			 * doesn't help, as we may finish playing it before the
			 * active mods gets added to it.
			 */
			ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
			if ( slapd_shutdown ) {
				ch_free( so );
				return SLAPD_ABANDON;
			}
			if ( ! slap_biglock_pool_pausecheck(op->o_bd) )
				ldap_pvt_thread_yield();

			ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		}

		so->s_flags |= OS_REF_PREPARE | OS_REF_OP_SEARCH;
		si->si_prefresh += 1;
		so->s_next = si->si_ops;
		so->s_si = si;
		si->si_ops = so;
		__sync_fetch_and_add(&si->si_psearches, 1);
	}

	/* snapshot the ctxcsn
	 * Note: this must not be done before the psearch setup. (ITS#8365)
	 */
	ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
	if (reopenldap_mode_check())
		slap_cookie_verify( &si->si_cookie );
	numcsns = si->si_cookie.numcsns;
	if ( numcsns ) {
		ber_bvarray_dup_x( &ctxcsn, si->si_cookie.ctxcsn, op->o_tmpmemctx );
		sids = op->o_tmpalloc( numcsns * sizeof(sids[0]), op->o_tmpmemctx );
		for ( i=0; i<numcsns; i++ )
			sids[i] = si->si_cookie.sids[i];
	}
	dirty = si->si_dirty;

	if ( DebugTest( LDAP_DEBUG_SYNC ) ) {
		ldap_debug_print(
			"syncprov-search: %srid=%03d, sid=%03d, hint %d, srs %p, sop %p\n",
			so ? "PERSISTENT, " : "",
			srs->sr_state.rid, srs->sr_state.sid, srs->sr_rhint,
			srs, so );
		slap_cookie_debug_pair(
			"syncprov-search: cookie",
			"provider", &si->si_cookie,
			"consumer", &srs->sr_state, -1 );
	}

	ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
	ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

	/* If we have a cookie, handle the PRESENT lookups */
	if ( srs->sr_state.numcsns ) {
		sessionlog *sl;
		int i, j;

		/* If we don't have any CSN of our own yet, bail out. */
		if ( !numcsns ) {
			if ( op->o_sync & SLAP_SYNC_PERSIST ) {
				/* LY: У получателя есть cookie, а у локального провайдера еще нет.
				 * Такое вполне может быть, если провайдер еще "чистый", но уже
				 * получил запрос от работающего получателя в мульти-мастер
				 * конфигурации, до того как сам был с кем-либо синхронизирован.
				 *
				 * В такой ситуации оптимальнее не прерывать синхронизацию,
				 * а пропустить refresh-стадию (так как локальных данных еще нет)
				 * и перейти к persistent-шагу, т.е. к отправке уведомлений о
				 * последующих изменениях. */
				assert( do_present == 0 && changed == 0 );
				goto shortcut;
			}
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "consumer has state info but provider doesn't!";
			goto bailout;
		}

		if (slap_check_same_server(op->o_bd, srs->sr_state.sid) < 0) {
			rs->sr_err = LDAP_ASSERTION_FAILED;
			rs->sr_text = "consumer has the same ServeID!";
			goto bailout;
		}

		int pivot_sid = -1;
		/* If there are SIDs we don't recognize in the cookie, drop them */
		for (i=0; i<srs->sr_state.numcsns; ) {
			for (j=i; j<numcsns; j++) {
				if ( srs->sr_state.sids[i] <= sids[j] ) {
					break;
				}
			}
			/* not found */
			if ( j == numcsns || srs->sr_state.sids[i] != sids[j] ) {
				char *tmp = srs->sr_state.ctxcsn[i].bv_val;
				Debug( LDAP_DEBUG_SYNC,
					"syncprov_op_search: sid 0x%03x, drop consumer's[%d] %s\n",
					srs->sr_state.sid, i, tmp );
				srs->sr_state.numcsns--;
				for ( j=i; j<srs->sr_state.numcsns; j++ ) {
					srs->sr_state.ctxcsn[j] = srs->sr_state.ctxcsn[j+1];
					srs->sr_state.sids[j] = srs->sr_state.sids[j+1];
				}
				srs->sr_state.ctxcsn[j].bv_val = tmp;
				srs->sr_state.ctxcsn[j].bv_len = 0;
				continue;
			}

			/* LY: Find the smallest consumer CSN */
			if ( BER_BVISEMPTY( &pivot_csn )
					|| slap_csn_compare_ts( &pivot_csn, &srs->sr_state.ctxcsn[i] ) > 0 ) {
				pivot_csn = srs->sr_state.ctxcsn[i];
				pivot_sid = srs->sr_state.sids[i];
			}
			i++;
		}

		/* LY: comparison consumer's and provider's CSNs */
		for ( i=0,j=0; i<srs->sr_state.numcsns; i++ ) {
			while ( srs->sr_state.sids[i] != sids[j] ) j++;
			assert(j < numcsns);
			int cmp = slap_csn_compare_ts( &srs->sr_state.ctxcsn[i], &ctxcsn[j] );
			/* If our state is newer, tell consumer about changes */
			if ( cmp < 0 ) {
				changed = SS_CHANGED;
			} else if ( cmp > 0 && sids[j] == slap_serverID ) {
				/* our state is older, complain to consumer */
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				rs->sr_text = "consumer state is newer than provider!";
				Debug( LDAP_DEBUG_SYNC,
					"syncprov_op_search: consumer sid 0x%03x state %s "
					"is newer than provider 0x%03x state %s\n",
					sids[i], srs->sr_state.ctxcsn[i].bv_val, slap_serverID,
					ctxcsn[j].bv_val);
bailout:
				if ( ctxcsn )
					ber_bvarray_free_x( ctxcsn, op->o_tmpmemctx );
				if ( sids )
					op->o_tmpfree( sids, op->o_tmpmemctx );
				if ( so )
					syncprov_unlink_syncop(so,
						OS_REF_PREPARE | OS_REF_OP_SEARCH, SO_LOCKED_NONE );
				rs->sr_ctrls = NULL;
				send_ldap_result( op, rs );
				return rs->sr_err;
			}
		}

		if ( srs->sr_state.numcsns != numcsns ) {
			/* consumer doesn't have the right set of CSNs */
			changed = SS_CHANGED;
			srs->sr_state.numcsns = 0; /* sr_state.ctxcsn is shared with pivot_csn */
			Debug( LDAP_DEBUG_SYNC,
				"syncprov_op_search: sid 0x%03x, useless consumer-cookie\n",
				srs->sr_state.sid );
			srs->sr_jammed = reopenldap_mode_strict();
			goto shortcut;
		}

		Debug( LDAP_DEBUG_SYNC,
			"syncprov_op_search: sid 0x%03x, provider's state is %s%s, pivot %s\n",
			srs->sr_state.sid, (changed == SS_CHANGED) ? "newer" : "the same",
			dirty ? " and dirty" : "",
			pivot_csn.bv_val );

		/* If nothing has changed, shortcut it */
		if ( !changed && !dirty ) {
			if ( reopenldap_mode_righteous()
					&& SLAP_MULTIMASTER( op->o_bd->bd_self )
					&& !si->si_nopres && !si->si_logs ) {
				Debug( LDAP_DEBUG_SYNC,
					"syncprov_op_search: sid 0x%03x, force present-check\n",
					 srs->sr_state.sid );
				do_present = SS_PRESENT;
				goto shortcut;
			}
no_change:
			if ( !(op->o_sync & SLAP_SYNC_PERSIST) ) {
				LDAPControl	*ctrls[2];
				struct berval cookie = BER_BVNULL;

				/* LY: cookie for quorum's auto-sids */
				if ( reopenldap_mode_righteous() ) {
					syncprov_compose_cookie( op, &cookie, srs->sr_state.ctxcsn, srs->sr_state.rid );
					Debug( LDAP_DEBUG_SYNC, "syncprov_op_search: cookie=%s\n", cookie.bv_val );
				}

				ctrls[0] = NULL;
				ctrls[1] = NULL;
				rs->sr_err = syncprov_done_ctrl( op, rs, ctrls, 0,
					BER_BVISNULL( &cookie ) ? NULL : &cookie,
					LDAP_SYNC_REFRESH_DELETES );
				if ( !BER_BVISNULL( &cookie ))
					op->o_tmpfree( cookie.bv_val, op->o_tmpmemctx );
				rs->sr_ctrls = ctrls;
				send_ldap_result( op, rs );
				rs->sr_ctrls = NULL;
				assert(so == NULL);
				return rs->sr_err;
			}
			goto shortcut;
		}

		if ( !si->si_nopres )
			do_present = SS_PRESENT;

		/* Do we have a sessionlog for this search? */
		sl=si->si_logs;
		if ( sl ) {
			int do_play = 0;
			ldap_pvt_thread_mutex_lock( &sl->sl_mutex );
			/* Are there any log entries, and is the consumer state
			 * present in the session log?
			 */
			if ( sl->sl_num > 0 ) {
				int i;
				for ( i=0; i<sl->sl_cookie.numcsns; i++ ) {
					/* SID not present == new enough */
					if ( pivot_sid < sl->sl_cookie.sids[i] ) {
						do_play = 1;
						break;
					}
					/* SID present */
					if ( pivot_sid == sl->sl_cookie.sids[i] ) {
						/* new enough? */
						if ( slap_csn_compare_ts( &pivot_csn, &sl->sl_cookie.ctxcsn[i] ) >= 0 )
							do_play = 1;
						break;
					}
				}
				/* SID not present == new enough */
				if ( i == sl->sl_cookie.numcsns )
					do_play = 1;
			}
			if ( do_play ) {
				do_present = 0;
				/* mutex is unlocked in playlog */
				rs->sr_err = syncprov_playlog( op, sl, srs, ctxcsn, numcsns, sids );
			} else {
				ldap_pvt_thread_mutex_unlock( &sl->sl_mutex );
			}
		}

		/* Is the CSN still present in the database? */
retry_pivot:
		rc = syncprov_findcsn( op, FIND_CSN, &pivot_csn );
		if ( rc != LDAP_SUCCESS ) {
			if ( rc != LDAP_NO_SUCH_OBJECT ) {
				rs->sr_err = rc;
				rs->sr_text = "unable to provide robust sync (find-csn failed)";
				goto bailout;
			}

			if ( changed == 0 ) {
				/* Try find other csn in ascending order
				 * if consumer's and provider's CSNs are the same. */
				j = -1;
				for (i = 0; i < srs->sr_state.numcsns; ++i) {
					if ( slap_csn_compare_ts( &pivot_csn, &srs->sr_state.ctxcsn[i] ) < 0
							&& (j < 0 || slap_csn_compare_ts( &srs->sr_state.ctxcsn[j], &srs->sr_state.ctxcsn[i]) > 0) )
						j = i;
				}
				if (j >= 0) {
					pivot_csn = srs->sr_state.ctxcsn[j];
					pivot_sid = srs->sr_state.sids[j];
					Debug( LDAP_DEBUG_SYNC,
						"syncprov_op_search: try use %s as pivon-csn\n", pivot_csn.bv_val);
					goto retry_pivot;
				}
			}

			/* No, so a reload is required */
			/* the 2.2 consumer doesn't send this hint */
			if ( si->si_usehint && srs->sr_rhint == 0 ) {
				rs->sr_err = LDAP_SYNC_REFRESH_REQUIRED;
				rs->sr_text = "sync cookie is stale";
				goto bailout;
			}
			if (! reopenldap_mode_righteous()) {
				/* LY: Hm, why we should clean cookie in this case ? */
				srs->sr_state.numcsns = 0; /* sr_state.ctxcsn is shared with pivot_csn */
			}
		} else {
			gotstate = 1;
		}
	} else {
		/* The consumer knows nothing, we know nothing. OK. */
		if (!numcsns)
			goto no_change;
		/* No consumer state, assume something has changed */
		changed = SS_CHANGED;
	}

shortcut:
	/* If changed and doing Present lookup, send Present UUIDs */
	if ( do_present ) {
		rs->sr_err = syncprov_findcsn( op, FIND_PRESENT, &pivot_csn );
		if ( rs->sr_err != LDAP_SUCCESS ) {
			rs->sr_text = "unable to provide robust sync (find-presend failed)";
			goto bailout;
		}
	}

	/* Append CSN range to search filter, save original filter
	 * for persistent search evaluation
	 */
	if ( so ) {
		ldap_pvt_thread_mutex_lock( &so->s_mutex );
		so->s_filterstr = op->ors_filterstr;
	}

	/* If something changed, find the changes */
	if ( gotstate && ( changed || dirty ) ) {
		Filter *fand, *fava;

		fand = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
		fand->f_choice = LDAP_FILTER_AND;
		fand->f_next = NULL;
		fava = op->o_tmpalloc( sizeof(Filter), op->o_tmpmemctx );
		fand->f_and = fava;
		fava->f_choice = LDAP_FILTER_GE;
		fava->f_ava = op->o_tmpalloc( sizeof(AttributeAssertion), op->o_tmpmemctx );
		fava->f_ava->aa_desc = slap_schema.si_ad_entryCSN;
#ifdef LDAP_COMP_MATCH
		fava->f_ava->aa_cf = NULL;
#endif
		/* LY: Cut-off filter's key after the timestamp.
		 * Otherwise is a chance to skip an a entry which has
		 * the same timestamp in entryCSN, but different SID. */
		pivot_csn.bv_val[pivot_csn.bv_len = 30] = '\0';

		ber_dupbv_x( &fava->f_ava->aa_value, &pivot_csn, op->o_tmpmemctx );
		fava->f_next = op->ors_filter;
		op->ors_filter = fand;
		if ( so ) {
			so->s_flags |= PS_FIX_FILTER;
		} else if ( !BER_BVISNULL( &op->ors_filterstr ) ) {
			op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
			BER_BVZERO( &op->ors_filterstr );
		}
		filter2bv_x( op, op->ors_filter, &op->ors_filterstr );
	}

	/* Let our callback add needed info to returned entries */
	cb = op->o_tmpcalloc(1, sizeof(slap_callback)+sizeof(searchstate), op->o_tmpmemctx);
	ss = (searchstate *)(cb+1);
	ss->ss_on = on;
	ss->ss_flags = do_present | changed;
	ss->ss_ctxcsn = ctxcsn;
	ss->ss_numcsns = numcsns;
	ss->ss_sids = sids;
	cb->sc_response = syncprov_search_response;
	cb->sc_private = ss;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;

	if ( so ) {
		if (so->s_flags & OS_REF_OP_SEARCH) {
			/* LY: make a copy to avoid race syncprov_matchops() with over_op_func() */
			slap_op_copy(so->s_op, so->s_op_safe = &so->s_slap_op_copy, NULL, NULL);
			assert(so->s_flags & OS_REF_PREPARE);
			so->s_flags -= OS_REF_PREPARE;
			cb->sc_cleanup = syncprov_search_cleanup;
			ss->ss_so = so;
			ldap_pvt_thread_mutex_unlock( &so->s_mutex );
		} else {
			/* LY: race with async abandon - is this enough ? */
			Debug( LDAP_DEBUG_ANY,
				"syncprov: rare-case race op_search with abandon.\n");
			assert(so->s_next == so);
			op->o_callback = NULL;
			ldap_pvt_thread_mutex_unlock( &so->s_mutex );
			assert((slap_tsan__read_int(&so->s_flags) & PS_IS_DETACHED) == 0);

			syncprov_unlink_syncop( so, OS_REF_PREPARE, SO_LOCKED_NONE );
			return rs->sr_err = SLAPD_ABANDON;
		}
	}

	/* If this is a persistent search and no changes were reported during
	 * the refresh phase, just invoke the response callback to transition
	 * us into persist phase
	 */
	if ( (changed | dirty | do_present) == 0 ) {
		rs->sr_nentries = 0;
		rs->sr_err = LDAP_SUCCESS;
		if (srs->sr_jammed) {
			rs->sr_err = LDAP_SYNC_REFRESH_REQUIRED;
			rs->sr_text = "unable to provide robust sync (no-changes, jammed)";
		}
		send_ldap_result( op, rs );
		return rs->sr_err;
	}
	return SLAP_CB_CONTINUE;
}

#define SS_NONE		0	/* LY: no mock contextCSN */
#define SS_RUNNING	1	/* LY: show mock contextCSN for running (connected) syncrepl(s) */
#define SS_ALL		2	/* LY: show mock contextCSN for any dirty syncrepl(s) */

static int
syncprov_operational(
	Operation *op,
	SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *)op->o_bd->bd_info;
	syncprov_info_t		*si = (syncprov_info_t *)on->on_bi.bi_private;

	/* This prevents generating unnecessarily; frontend will strip
	 * any statically stored copy.
	 */
	if ( op->o_sync != SLAP_CONTROL_NONE )
		return SLAP_CB_CONTINUE;

	if ( rs->sr_entry &&
		dn_match( &rs->sr_entry->e_nname, &si->si_contextdn )) {

		if ( SLAP_OPATTRS( rs->sr_attr_flags ) ||
			ad_inlist( slap_schema.si_ad_contextCSN, rs->sr_attrs )) {
			Attribute *a, **ap = NULL;

			for ( a=rs->sr_entry->e_attrs; a; a=a->a_next ) {
				if ( a->a_desc == slap_schema.si_ad_contextCSN )
					break;
			}

			BerValue status = {0};
			int mock = 0;
			if (! op->o_dont_replicate && si->si_showstatus != SS_NONE ) {
				mock = quorum_query_status(op->o_bd,
						si->si_showstatus == SS_RUNNING, &status, op)
					| slap_tsan__read_int(&si->si_prefresh)
					| slap_tsan__read_int(&si->si_active);

				Debug( LDAP_DEBUG_SYNC, "syncprov_gate: %s\n",
					mock ? "dirty" : "clean" );
			}

			ldap_pvt_thread_rdwr_rlock( &si->si_csn_rwlock );
			if (reopenldap_mode_check())
				slap_cookie_verify( &si->si_cookie );

			if ( si->si_cookie.numcsns || mock ) {
				if ( !a ) {
					for ( ap = &rs->sr_operational_attrs; *ap;
						ap=&(*ap)->a_next );

					a = attr_alloc( slap_schema.si_ad_contextCSN );
					*ap = a;
				}

				if ( !ap ) {
					if ( rs_entry2modifiable( op, rs, on )) {
						a = attr_find( rs->sr_entry->e_attrs,
							slap_schema.si_ad_contextCSN );
					}
					if ( a->a_nvals != a->a_vals ) {
						ber_bvarray_free( a->a_nvals );
					}
					a->a_nvals = NULL;
					ber_bvarray_free( a->a_vals );
					a->a_vals = NULL;
					a->a_numvals = 0;
				}
				if (si->si_cookie.numcsns)
					attr_valadd( a, si->si_cookie.ctxcsn, si->si_cookie.ctxcsn, si->si_cookie.numcsns );
				if (mock)
					attr_valadd( a, &status, &status, 1 );
			}
			ldap_pvt_thread_rdwr_runlock( &si->si_csn_rwlock );
		}
	}
	return SLAP_CB_CONTINUE;
}

enum {
	SP_CHKPT = 1,
	SP_SESSL,
	SP_NOPRES,
	SP_USEHINT,
	SP_SHOWSTATUS
};

static ConfigDriver sp_cf_gen;

static ConfigTable spcfg[] = {
	{ "syncprov-checkpoint", "ops> <minutes", 3, 3, 0, ARG_MAGIC|SP_CHKPT,
		sp_cf_gen, "( OLcfgOvAt:1.1 NAME 'olcSpCheckpoint' "
			"DESC 'ContextCSN checkpoint interval in ops and minutes' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-sessionlog", "ops", 2, 2, 0, ARG_INT|ARG_MAGIC|SP_SESSL,
		sp_cf_gen, "( OLcfgOvAt:1.2 NAME 'olcSpSessionlog' "
			"DESC 'Session log size in ops' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-nopresent", NULL, 2, 2, 0, ARG_ON_OFF|ARG_MAGIC|SP_NOPRES,
		sp_cf_gen, "( OLcfgOvAt:1.3 NAME 'olcSpNoPresent' "
			"DESC 'Omit Present phase processing' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-reloadhint", NULL, 2, 2, 0, ARG_ON_OFF|ARG_MAGIC|SP_USEHINT,
		sp_cf_gen, "( OLcfgOvAt:1.4 NAME 'olcSpReloadHint' "
			"DESC 'Observe Reload Hint in Request control' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "syncprov-showstatus", "mode", 2, 2, 0, ARG_STRING|ARG_MAGIC|SP_SHOWSTATUS,
		sp_cf_gen, "( OLcfgOvAt:1.9 NAME 'olcSpShowStatus' "
			"DESC 'Show a Mock contextCSN Until Synchronization is Perfect' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs spocs[] = {
	{ "( OLcfgOvOc:1.1 "
		"NAME 'olcSyncProvConfig' "
		"DESC 'SyncRepl Provider configuration' "
		"SUP olcOverlayConfig "
		"MAY ( olcSpCheckpoint "
			"$ olcSpSessionlog "
			"$ olcSpNoPresent "
			"$ olcSpReloadHint "
			"$ olcSpShowStatus "
		") )",
			Cft_Overlay, spcfg },
	{ NULL, 0, NULL }
};

static int
sp_cf_gen(ConfigArgs *c)
{
	slap_overinst		*on = (slap_overinst *)c->bi;
	syncprov_info_t		*si = (syncprov_info_t *)on->on_bi.bi_private;
	int rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch ( c->type ) {
		case SP_CHKPT:
			if ( si->si_chkops || si->si_chktime ) {
				struct berval bv;
				/* we assume si_chktime is a multiple of 60
				 * because the parsed value was originally
				 * multiplied by 60 */
				bv.bv_len = snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"%d %d", si->si_chkops, si->si_chktime/60 );
				if ( bv.bv_len >= sizeof( c->cr_msg ) ) {
					rc = 1;
				} else {
					bv.bv_val = c->cr_msg;
					value_add_one( &c->rvalue_vals, &bv );
				}
			} else {
				rc = 1;
			}
			break;
		case SP_SESSL:
			if ( si->si_logs ) {
				c->value_int = si->si_logs->sl_size;
			} else {
				rc = 1;
			}
			break;
		case SP_NOPRES:
			if ( si->si_nopres ) {
				c->value_int = 1;
			} else {
				rc = 1;
			}
			break;
		case SP_USEHINT:
			if ( si->si_usehint ) {
				c->value_int = 1;
			} else {
				rc = 1;
			}
			break;
		case SP_SHOWSTATUS:
			switch ( si->si_showstatus ) {
			default:
				rc = 1;
				break;
			case SS_NONE:
				c->value_string = ch_strdup( "none" );
				break;
			case SS_RUNNING:
				c->value_string = ch_strdup( "running" );
				break;
			case SS_ALL:
				c->value_string = ch_strdup( "all" );
				break;
			}
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch ( c->type ) {
		case SP_CHKPT:
			si->si_chkops = 0;
			si->si_chktime = 0;
			break;
		case SP_SESSL:
			si->si_logs->sl_size = 0;
			break;
		case SP_NOPRES:
			si->si_nopres = 0;
			break;
		case SP_USEHINT:
			si->si_usehint = 0;
			break;
		case SP_SHOWSTATUS:
			si->si_showstatus = SS_NONE;
			break;
		}
		return rc;
	}
	switch ( c->type ) {
	case SP_CHKPT:
		if ( lutil_atoi( &si->si_chkops, c->argv[1] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s unable to parse checkpoint ops # \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		if ( si->si_chkops <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid checkpoint ops # \"%d\"",
				c->argv[0], si->si_chkops );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		if ( lutil_atoi( &si->si_chktime, c->argv[2] ) != 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s unable to parse checkpoint time \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		if ( si->si_chktime <= 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid checkpoint time \"%d\"",
				c->argv[0], si->si_chkops );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		si->si_chktime *= 60;
		break;
	case SP_SESSL:
		if ( c->value_int < 0 ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s size %d is negative",
				c->argv[0], c->value_int );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
		if ( !si->si_logs ) {
			si->si_logs = ch_calloc( 1, sizeof( sessionlog ));
			ldap_pvt_thread_mutex_init( &si->si_logs->sl_mutex );
		}
		si->si_logs->sl_size = c->value_int;
		break;
	case SP_NOPRES:
		si->si_nopres = c->value_int;
		break;
	case SP_USEHINT:
		si->si_usehint = c->value_int;
		break;
	case SP_SHOWSTATUS:
		if ( strcasecmp( c->value_string, "none" ) == 0 )
			si->si_showstatus = SS_NONE;
		else if ( strcasecmp( c->value_string, "running" ) == 0)
			si->si_showstatus = SS_RUNNING;
		else if ( strcasecmp( c->value_string, "all" ) == 0)
			si->si_showstatus = SS_ALL;
		else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid mode \"%s\" (none|running|all)",
				c->argv[0], c->value_string );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg );
			return ARG_BAD_CONF;
		}
	}
	return rc;
}

/* ITS#3456 we cannot run this search on the main thread, must use a
 * child thread in order to insure we have a big enough stack.
 */
static void *
syncprov_db_otask(
	void *ptr
)
{
	syncprov_findcsn( ptr, FIND_MAXCSN, 0 );
	return NULL;
}


/* Read any existing contextCSN from the underlying db.
 * Then search for any entries newer than that. If no value exists,
 * just generate it. Cache whatever result.
 */
static int
syncprov_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst   *on = (slap_overinst *) be->bd_info;
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;

	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	Entry *e = NULL;
	Attribute *a;
	int rc;
	void *thrctx = NULL;

	if ( !SLAP_LASTMOD( be )) {
		Debug( LDAP_DEBUG_ANY,
			"syncprov_db_open: invalid config, lastmod must be enabled\n" );
		return -1;
	}

	if ( slapMode & SLAP_TOOL_MODE ) {
		return 0;
	}

	rc = overlay_register_control( be, LDAP_CONTROL_SYNC );
	if ( rc ) {
		return rc;
	}

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op = &opbuf.ob_op;
	op->o_bd = be;
	op->o_dn = be->be_rootdn;
	op->o_ndn = be->be_rootndn;

	if ( SLAP_SYNC_SUBENTRY( be )) {
		build_new_dn( &si->si_contextdn, be->be_nsuffix,
			(struct berval *)&slap_ldapsync_cn_bv, NULL );
	} else {
		si->si_contextdn = be->be_nsuffix[0];
	}
	rc = overlay_entry_get_ov( op, &si->si_contextdn, NULL,
		slap_schema.si_ad_contextCSN, 0, &e, on );

	if ( e ) {
		a = attr_find( e->e_attrs, slap_schema.si_ad_contextCSN );
		if ( a ) {
			if ( slap_cookie_pull( &si->si_cookie, a->a_nvals, 0 ) < 0 ) {
				rc = LDAP_OTHER;
				goto out;
			}
		}
		overlay_entry_release_ov( op, e, 0, on );
	}

	if ( !SLAP_DBCLEAN( be ) && (si->si_cookie.numcsns || reopenldap_mode_righteous())) {
		ldap_pvt_thread_t tid;

		op->o_tag = LDAP_REQ_SEARCH;
		op->o_req_dn = be->be_suffix[0];
		op->o_req_ndn = be->be_nsuffix[0];
		op->ors_scope = LDAP_SCOPE_SUBTREE;
		ldap_pvt_thread_create( &tid, 0, syncprov_db_otask, op );
		ldap_pvt_thread_join( tid, NULL );
	}

	/* Didn't find a contextCSN, should we generate one? */
	rc = 0;
	if ( !si->si_cookie.numcsns ) {

		if ( slap_serverID || SLAP_SYNC_SHADOW( op->o_bd )) {
			/* If we're also a consumer, then don't generate anything.
			 * Wait for our provider to send it to us, or for a local
			 * modify if we have multimaster.
			 */
			goto out;
		}

		if ( reopenldap_mode_righteous() ) {
			rc = slap_cookie_stubself( &si->si_cookie );
			if (rc < 0)
				goto out;
			Debug( LDAP_DEBUG_SYNC, "syncprov: %s force stub-csn for self-sid %d\n",
				be->be_nsuffix->bv_val, slap_serverID );
			rc = 0;
		} else {
			char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];
			struct berval csn;

			csn.bv_val = csnbuf;
			csn.bv_len = sizeof( csnbuf );
			slap_get_csn( NULL, &csn );
			value_add_one( &si->si_cookie.ctxcsn, &csn );
			si->si_cookie.numcsns = 1;
			si->si_cookie.sids = ch_malloc( sizeof(int) );
			si->si_cookie.sids[0] = slap_serverID;
		}

		/* make sure we do a checkpoint on close */
		si->si_numops++;
	}

	if (reopenldap_mode_check())
		slap_cookie_verify( &si->si_cookie );

	/* Initialize the sessionlog mincsn */
	if ( si->si_logs && si->si_cookie.numcsns )
		slap_cookie_copy( &si->si_logs->sl_cookie, &si->si_cookie );

out:
	op->o_bd->bd_info = (BackendInfo *)on;
	return rc;
}

/* Write the current contextCSN into the underlying db.
 */
static int
syncprov_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst   *on = (slap_overinst *) be->bd_info;
	syncprov_info_t *si = (syncprov_info_t *)on->on_bi.bi_private;

	if ( slapMode & SLAP_TOOL_MODE ) {
		return 0;
	}
	if ( si->si_numops ) {
		Connection conn = {0};
		OperationBuffer opbuf;
		Operation *op;
		void *thrctx;

		thrctx = ldap_pvt_thread_pool_context();
		connection_fake_init2( &conn, &opbuf, thrctx, 0 );
		op = &opbuf.ob_op;
		op->o_bd = be;
		op->o_dn = be->be_rootdn;
		op->o_ndn = be->be_rootndn;
		syncprov_checkpoint( op, on );
	}

#ifdef SLAP_CONFIG_DELETE
	if ( !slapd_shutdown ) {
		int paused = slap_biglock_pool_pause(be);
		ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		while ( si->si_ops != NULL ) {
			syncops *so = si->si_ops;
			SlapReply rs = {REP_RESULT};
			rs.sr_err = LDAP_UNAVAILABLE;
			send_ldap_result( so->s_op_safe, &rs );
			ldap_pvt_thread_mutex_lock( &so->s_mutex );
			si->si_ops = so->s_next;
			so->s_next = so; /* LY: safely mark it as unlinked */
			so->s_flags |= PS_DEAD | OS_REF_CLOSE;
			if ( (so->s_flags & PS_TASK_QUEUED)
					&& ldap_pvt_thread_pool_retract( so->s_pool_cookie ) > 0)
				so->s_flags -= PS_TASK_QUEUED;
			ldap_pvt_thread_mutex_unlock( &so->s_mutex );
			ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );

			syncprov_unlink_syncop( so, OS_REF_OP_SEARCH | OS_REF_CLOSE, SO_LOCKED_NONE );
			ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
		}
		ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
		if (paused == LDAP_SUCCESS)
			slap_biglock_pool_resume(be);
	}
	overlay_unregister_control( be, LDAP_CONTROL_SYNC );
#endif /* SLAP_CONFIG_DELETE */

	return 0;
}

static int
syncprov_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	syncprov_info_t	*si;

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		Debug( LDAP_DEBUG_ANY,
			"syncprov must be instantiated within a database.\n" );
		return 1;
	}

	si = ch_calloc(1, sizeof(syncprov_info_t));
	si->si_showstatus = SS_RUNNING;
	si->si_chklast = ldap_time_steady();
	on->on_bi.bi_private = si;
	ldap_pvt_thread_rdwr_init( &si->si_csn_rwlock );
	ldap_pvt_thread_mutex_init( &si->si_ops_mutex );
	ldap_pvt_thread_mutex_init( &si->si_mods_mutex );
	ldap_pvt_thread_mutex_init( &si->si_resp_mutex );

	csn_anlist[0].an_desc = slap_schema.si_ad_entryCSN;
	csn_anlist[0].an_name = slap_schema.si_ad_entryCSN->ad_cname;
	csn_anlist[1].an_desc = slap_schema.si_ad_entryUUID;
	csn_anlist[1].an_name = slap_schema.si_ad_entryUUID->ad_cname;

	uuid_anlist[0].an_desc = slap_schema.si_ad_entryUUID;
	uuid_anlist[0].an_name = slap_schema.si_ad_entryUUID->ad_cname;

	return 0;
}

static int
syncprov_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	syncprov_info_t	*si = (syncprov_info_t *)on->on_bi.bi_private;

	if ( si ) {
		/* LY: workaround for https://github.com/leo-yuriev/ReOpenLDAP/issues/45 */
		for(;;) {
			int drained, paused = -1;

			if ( slapMode == SLAP_SERVER_MODE && !slap_biglock_pool_pausing(be) )
				paused = slap_biglock_pool_pause(be);
			ldap_pvt_thread_mutex_lock( &si->si_ops_mutex );
			drained = 0;
			if (si->si_ops == NULL
					&& slap_tsan__read_int(&si->si_psearches) == 0
					&& slap_tsan__read_int(&si->si_active) == 0)
				drained = 1;
			ldap_pvt_thread_mutex_unlock( &si->si_ops_mutex );
			if (paused == LDAP_SUCCESS)
				slap_biglock_pool_resume(be);
			if (drained)
				break;
			if ( slapd_shutdown )
				return SLAPD_ABANDON;
			ldap_pvt_thread_yield();
		}

		on->on_bi.bi_private = NULL;
		assert(si->si_ops == NULL && si->si_mods == NULL);

		if ( si->si_logs ) {
			sessionlog *sl = si->si_logs;
			slog_entry *se = sl->sl_head;

			while ( se ) {
				slog_entry *se_next = se->se_next;
				ch_free( se );
				se = se_next;
			}
			slap_cookie_free( &sl->sl_cookie, 0 );
			ldap_pvt_thread_mutex_destroy(&si->si_logs->sl_mutex);
			ch_free( si->si_logs );
		}
		slap_cookie_free( &si->si_cookie, 0 );

		ldap_pvt_thread_mutex_destroy( &si->si_resp_mutex );
		ldap_pvt_thread_mutex_destroy( &si->si_mods_mutex );
		ldap_pvt_thread_mutex_destroy( &si->si_ops_mutex );
		ldap_pvt_thread_rdwr_destroy( &si->si_csn_rwlock );
		ch_free( si );
	}

	return 0;
}

static int syncprov_parseCtrl (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	ber_tag_t tag;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_int_t mode;
	ber_len_t len;
	struct berval cookie = BER_BVNULL;
	sync_control *sr;
	int rhint = 0;

	if ( op->o_sync != SLAP_CONTROL_NONE ) {
		rs->sr_text = "Sync control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( op->o_pagedresults != SLAP_CONTROL_NONE ) {
		rs->sr_text = "Sync control specified with pagedResults control";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Sync control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Sync control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	/* Parse the control value
	 *      syncRequestValue ::= SEQUENCE {
	 *              mode   ENUMERATED {
	 *                      -- 0 unused
	 *                      refreshOnly		(1),
	 *                      -- 2 reserved
	 *                      refreshAndPersist	(3)
	 *              },
	 *              cookie  syncCookie OPTIONAL
	 *      }
	 */

	ber_init2( ber, &ctrl->ldctl_value, 0 );

	if ( (tag = ber_scanf( ber, "{i" /*}*/, &mode )) == LBER_ERROR ) {
		rs->sr_text = "Sync control : mode decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	switch( mode ) {
	case LDAP_SYNC_REFRESH_ONLY:
		mode = SLAP_SYNC_REFRESH;
		break;
	case LDAP_SYNC_REFRESH_AND_PERSIST:
		mode = SLAP_SYNC_REFRESH_AND_PERSIST;
		break;
	default:
		rs->sr_text = "Sync control : unknown update mode";
		return LDAP_PROTOCOL_ERROR;
	}

	tag = ber_peek_tag( ber, &len );

	if ( tag == LDAP_TAG_SYNC_COOKIE ) {
		if (( ber_scanf( ber, /*{*/ "m", &cookie )) == LBER_ERROR ) {
			rs->sr_text = "Sync control : cookie decoding error";
			return LDAP_PROTOCOL_ERROR;
		}
		tag = ber_peek_tag( ber, &len );
	}
	if ( tag == LDAP_TAG_RELOAD_HINT ) {
		if (( ber_scanf( ber, /*{*/ "b", &rhint )) == LBER_ERROR ) {
			rs->sr_text = "Sync control : rhint decoding error";
			return LDAP_PROTOCOL_ERROR;
		}
	}
	if (( ber_scanf( ber, /*{*/ "}")) == LBER_ERROR ) {
			rs->sr_text = "Sync control : decoding error";
			return LDAP_PROTOCOL_ERROR;
	}
	sr = op->o_tmpcalloc( 1, sizeof(struct sync_control), op->o_tmpmemctx );
	sr->sr_rhint = rhint;
	if (!BER_BVISNULL(&cookie)) {
		/* If parse fails, pretend no cookie was sent */
		if ( slap_cookie_parse( &sr->sr_state, &cookie, op->o_tmpmemctx ) ) {
			if (reopenldap_mode_strict()) {
				rs->sr_text = "Sync control : invalid cookie";
				return LDAP_PROTOCOL_ERROR;
			}
		}
	}

	op->o_controls[slap_cids.sc_LDAPsync] = sr;

	op->o_sync = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	op->o_sync |= mode;

	return LDAP_SUCCESS;
}

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

static slap_overinst 		syncprov;

int
syncprov_over_initialize()
{
	int rc;

	rc = register_supported_control( LDAP_CONTROL_SYNC,
		SLAP_CTRL_SEARCH, NULL,
		syncprov_parseCtrl, &slap_cids.sc_LDAPsync );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"syncprov_init: Failed to register control %d\n", rc );
		return rc;
	}

	syncprov.on_bi.bi_type = "syncprov";
	syncprov.on_bi.bi_db_init = syncprov_db_init;
	syncprov.on_bi.bi_db_destroy = syncprov_db_destroy;
	syncprov.on_bi.bi_db_open = syncprov_db_open;
	syncprov.on_bi.bi_db_close = syncprov_db_close;

	syncprov.on_bi.bi_op_abandon = syncprov_op_abandon;
	syncprov.on_bi.bi_op_cancel = syncprov_op_abandon;

	syncprov.on_bi.bi_op_add = syncprov_op_mod;
	syncprov.on_bi.bi_op_compare = syncprov_op_compare;
	syncprov.on_bi.bi_op_delete = syncprov_op_mod;
	syncprov.on_bi.bi_op_modify = syncprov_op_mod;
	syncprov.on_bi.bi_op_modrdn = syncprov_op_mod;
	syncprov.on_bi.bi_op_search = syncprov_op_search;
	syncprov.on_bi.bi_extended = syncprov_op_extended;
	syncprov.on_bi.bi_operational = syncprov_operational;

	syncprov.on_bi.bi_cf_ocs = spocs;

	generic_filter.f_desc = slap_schema.si_ad_objectClass;

	rc = config_register_schema( spcfg, spocs );
	if ( rc ) return rc;

	return overlay_register( &syncprov );
}

#if SLAPD_OVER_SYNCPROV == SLAPD_MOD_DYNAMIC
SLAP_OVERLAY_INIT_MODULE(syncprov)
#endif /* SLAPD_OVER_SYNCPROV == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_SYNCPROV) */
