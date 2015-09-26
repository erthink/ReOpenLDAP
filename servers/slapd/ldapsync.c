/* ldapsync.c -- LDAP Content Sync Routines */
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
#include "../../libraries/liblber/lber-int.h" /* get ber_strndup() */
#include "lutil_ldap.h"

#if LDAP_MEMORY_DEBUG
#	include <lber_hipagut.h>
#	define CHEK_MEM_VALID(p) lber_hug_memchk_ensure(p, 0)
#else
#	define CHEK_MEM_VALID(p) __noop()
#endif /* LDAP_MEMORY_DEBUG */

struct slap_sync_cookie_s slap_sync_cookie =
	LDAP_STAILQ_HEAD_INITIALIZER( slap_sync_cookie );

void
slap_compose_sync_cookie(
	Operation *op,
	struct berval *cookie,
	BerVarray csn,
	int rid,
	int sid )
{
	int len, numcsn = 0;

	if ( csn ) {
		for (; !BER_BVISNULL( &csn[numcsn] ); numcsn++);
	}

	if ( numcsn == 0 || rid == -1 ) {
		char cookiestr[ LDAP_PVT_CSNSTR_BUFSIZE + 20 ];
		if ( rid == -1 ) {
			cookiestr[0] = '\0';
			len = 0;
		} else {
			len = snprintf( cookiestr, sizeof( cookiestr ),
					"rid=%03d", rid );
			if ( sid >= 0 ) {
				len += sprintf( cookiestr+len, ",sid=%03x", sid );
			}
		}
		ber_str2bv_x( cookiestr, len, 1, cookie,
			op ? op->o_tmpmemctx : NULL );
	} else {
		char *ptr;
		int i;

		len = 0;
		for ( i=0; i<numcsn; i++)
			len += csn[i].bv_len + 1;

		len += STRLENOF("rid=123,csn=");
		if ( sid >= 0 )
			len += STRLENOF("sid=xxx,");

		cookie->bv_val = slap_sl_malloc( len, op ? op->o_tmpmemctx : NULL );

		len = sprintf( cookie->bv_val, "rid=%03d,", rid );
		ptr = cookie->bv_val + len;
		if ( sid >= 0 ) {
			ptr += sprintf( ptr, "sid=%03x,", sid );
		}
		ptr = lutil_strcopy( ptr, "csn=" );
		for ( i=0; i<numcsn; i++) {
			ptr = lutil_strncopy( ptr, csn[i].bv_val, csn[i].bv_len );
			*ptr++ = ';';
		}
		ptr--;
		*ptr = '\0';
		cookie->bv_len = ptr - cookie->bv_val;
	}
}

int *
slap_parse_csn_sids( BerVarray csns, int numcsns, void *memctx )
{
	int i, *ret;

	ret = slap_sl_malloc( numcsns * sizeof(int), memctx );
	for ( i=0; i<numcsns; i++ ) {
		ret[i] = slap_csn_get_sid( &csns[i] );
	}
	return ret;
}

static slap_mr_match_func sidsort_cmp;

static const MatchingRule sidsort_mr = {
	{ 0 },
	NULL,
	{ 0 },
	{ 0 },
	0,
	NULL, NULL, NULL, sidsort_cmp
};
static const AttributeType sidsort_at = {
	{ 0 },
	{ 0 },
	NULL, NULL, (MatchingRule *)&sidsort_mr,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, SLAP_AT_SORTED_VAL
};
static const AttributeDescription sidsort_ad = {
	NULL,
	(AttributeType *)&sidsort_at
};

static int
sidsort_cmp(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *b1,
	void *v2 )
{
	struct berval *b2 = v2;
	*matchp = b1->bv_len - b2->bv_len;
	return LDAP_SUCCESS;
}

int slap_check_same_server(BackendDB *bd, int sid) {
	return ( sid == slap_serverID
			&& reopenldap_mode_idclip() && SLAP_MULTIMASTER(bd) ) ? -1 : 0;
}

int
slap_csn_stub_self( BerVarray *ctxcsn, int **sids, int *numcsns )
{
	int i, rc;
	struct berval csn;
	int *new_sids;
	char buf[ LDAP_PVT_CSNSTR_BUFSIZE ];

	for (i = *numcsns; --i >= 0; )
		if (slap_serverID == (*sids)[i])
			return 0;

	new_sids = ber_memrealloc( *sids, (*numcsns + 1) * sizeof(**sids) );
	if (! new_sids)
		return LDAP_NO_MEMORY;

	csn.bv_val = buf;
	csn.bv_len = snprintf( buf, sizeof( buf ),
		"%4d%02d%02d%02d%02d%02d.%06dZ#%06x#%03x#%06x",
		1900, 1, 1, 0, 0, 0, 0, 0, slap_serverID, 0 );

	rc = value_add_one( ctxcsn, &csn );
	if (rc < 0)
		return rc;

	*sids = new_sids;
	(*sids)[*numcsns] = slap_serverID;
	*numcsns += 1;

	rc = slap_sort_csn_sids( *ctxcsn, *sids, *numcsns, NULL );
	if (rc < 0)
		return rc;

	return 1;
}

/* sort CSNs by SID. Use a fake Attribute with our own
 * syntax and matching rule, which sorts the nvals by
 * bv_len order. Stuff our sids into the bv_len.
 */
int
slap_sort_csn_sids( BerVarray csns, int *sids, int numcsns, void *memctx )
{
	Attribute a;
	const char *text;
	int i, rc;

	a.a_desc = (AttributeDescription *)&sidsort_ad;
	a.a_nvals = slap_sl_malloc( numcsns * sizeof(struct berval), memctx );
	for ( i=0; i<numcsns; i++ ) {
		a.a_nvals[i].bv_len = sids[i];
		a.a_nvals[i].bv_val = NULL;
	}
	a.a_vals = csns;
	a.a_numvals = numcsns;
	a.a_flags = 0;
	rc = slap_sort_vals( (Modifications *)&a, &text, &i, memctx );
	for ( i=0; i<numcsns; i++ )
		sids[i] = a.a_nvals[i].bv_len;
	slap_sl_free( a.a_nvals, memctx );
	return rc;
}

void
slap_insert_csn_sids(
	struct sync_cookie *ck,
	int pos,
	int sid,
	struct berval *csn
)
{
	int i;
	ck->numcsns++;
	ck->ctxcsn = ber_memrealloc( ck->ctxcsn,
		(ck->numcsns+1) * sizeof(struct berval));
	BER_BVZERO( &ck->ctxcsn[ck->numcsns] );
	ck->sids = ber_memrealloc( ck->sids, ck->numcsns * sizeof(int));
	for ( i = ck->numcsns-1; i > pos; i-- ) {
		ck->ctxcsn[i] = ck->ctxcsn[i-1];
		ck->sids[i] = ck->sids[i-1];
	}
	ck->sids[i] = sid;
	ber_dupbv( &ck->ctxcsn[i], csn );

	if (reopenldap_mode_idkfa())
		slap_cookie_verify( ck );
}

int
slap_parse_sync_cookie(
	struct sync_cookie *cookie,
	void *memctx
)
{
	char *csn_ptr;
	char *csn_str;
	char *cval;
	char *next, *end;
	AttributeDescription *ad = slap_schema.si_ad_entryCSN;

	if ( cookie == NULL )
		goto bailout;

	cookie->rid = -1;
	cookie->sid = -1;
	cookie->ctxcsn = NULL;
	cookie->sids = NULL;
	cookie->numcsns = 0;

	if ( cookie->octet_str.bv_len <= STRLENOF( "rid=" ) )
		goto bailout;

	end = cookie->octet_str.bv_val + cookie->octet_str.bv_len;

	for ( next=cookie->octet_str.bv_val; next < end; ) {
		if ( !strncmp( next, "rid=", STRLENOF("rid=") )) {
			char *rid_ptr = next;
			cookie->rid = strtol( &rid_ptr[ STRLENOF( "rid=" ) ], &next, 10 );
			if ( next == rid_ptr ||
				next > end ||
				( *next && *next != ',' ) ||
				cookie->rid < 0 ||
				cookie->rid > SLAP_SYNC_RID_MAX )
			{
				goto bailout;
			}
			if ( *next == ',' ) {
				next++;
			}
			if ( !ad ) {
				break;
			}
			continue;
		}
		if ( !strncmp( next, "sid=", STRLENOF("sid=") )) {
			char *sid_ptr = next;
			sid_ptr = next;
			cookie->sid = strtol( &sid_ptr[ STRLENOF( "sid=" ) ], &next, 16 );
			if ( next == sid_ptr ||
				next > end ||
				( *next && *next != ',' ) ||
				cookie->sid < 0 ||
				cookie->sid > SLAP_SYNC_SID_MAX )
			{
				goto bailout;
			}
			if ( *next == ',' ) {
				next++;
			}
			continue;
		}
		if ( !strncmp( next, "csn=", STRLENOF("csn=") )) {
			struct berval stamp;

			next += STRLENOF("csn=");
			while ( next < end ) {
				csn_str = next;
				csn_ptr = strchr( csn_str, '#' );
				if ( !csn_ptr || csn_ptr > end )
					break;
				/* ad will be NULL when called from main. we just
				 * want to parse the rid then. But we still iterate
				 * through the string to find the end.
				 */
				cval = strchr( csn_ptr, ';' );
				if ( !cval )
					cval = strchr(csn_ptr, ',' );
				if ( cval )
					stamp.bv_len = cval - csn_str;
				else
					stamp.bv_len = end - csn_str;
				if ( ad ) {
					struct berval bv;
					stamp.bv_val = csn_str;
					if ( ad->ad_type->sat_syntax->ssyn_validate(
						ad->ad_type->sat_syntax, &stamp ) != LDAP_SUCCESS )
						break;
					if ( ad->ad_type->sat_equality->smr_normalize(
						SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
						ad->ad_type->sat_syntax,
						ad->ad_type->sat_equality,
						&stamp, &bv, memctx ) != LDAP_SUCCESS )
						break;
					ber_bvarray_add_x( &cookie->ctxcsn, &bv, memctx );
					cookie->numcsns++;
				}
				if ( cval ) {
					next = cval + 1;
					if ( *cval != ';' )
						break;
				} else {
					next = end;
					break;
				}
			}
			continue;
		}
		next++;
	}
	if ( cookie->numcsns ) {
		cookie->sids = slap_parse_csn_sids( cookie->ctxcsn, cookie->numcsns,
			memctx );
		if ( cookie->numcsns > 1 )
			slap_sort_csn_sids( cookie->ctxcsn, cookie->sids, cookie->numcsns, memctx );
	}
	return LDAP_SUCCESS;

bailout:
	cookie->rid = -1;
	cookie->sid = -1;
	cookie->ctxcsn = NULL;
	cookie->sids = NULL;
	cookie->numcsns = 0;

	return LDAP_PROTOCOL_ERROR;
}

/* count the numcsns and regenerate the list of SIDs in a recomposed cookie */
void
slap_reparse_sync_cookie(
	struct sync_cookie *cookie,
	void *memctx )
{
	if ( cookie->ctxcsn ) {
		for (; !BER_BVISNULL( &cookie->ctxcsn[cookie->numcsns] ); cookie->numcsns++);
	}
	if ( cookie->numcsns ) {
		cookie->sids = slap_parse_csn_sids( cookie->ctxcsn, cookie->numcsns, NULL );
		if ( cookie->numcsns > 1 )
			slap_sort_csn_sids( cookie->ctxcsn, cookie->sids, cookie->numcsns, memctx );
	}
}

int
slap_init_sync_cookie_ctxcsn(
	struct sync_cookie *cookie
)
{
	char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE + 4 ];
	struct berval octet_str = BER_BVNULL;
	struct berval ctxcsn = BER_BVNULL;

	if ( cookie == NULL )
		return -1;

	octet_str.bv_len = snprintf( csnbuf, LDAP_PVT_CSNSTR_BUFSIZE + 4,
					"csn=%4d%02d%02d%02d%02d%02dZ#%06x#%02x#%06x",
					1900, 1, 1, 0, 0, 0, 0, 0, 0 );
	octet_str.bv_val = csnbuf;
	ch_free( cookie->octet_str.bv_val );
	ber_dupbv( &cookie->octet_str, &octet_str );

	ctxcsn.bv_val = octet_str.bv_val + 4;
	ctxcsn.bv_len = octet_str.bv_len - 4;
	cookie->ctxcsn = NULL;
	value_add_one( &cookie->ctxcsn, &ctxcsn );
	cookie->numcsns = 1;
	cookie->sid = -1;

	return 0;
}

/*----------------------------------------------------------------------------*/

void slap_cookie_verify(const struct sync_cookie *cookie)
{
	int i;

	if ( cookie->numcsns ) {
		LDAP_ENSURE( cookie->ctxcsn != NULL );
		LDAP_ENSURE( cookie->sids != NULL );
		CHEK_MEM_VALID( cookie->ctxcsn );
		CHEK_MEM_VALID( cookie->sids );
	}

	if (cookie->ctxcsn) {
		LDAP_ENSURE( cookie->ctxcsn[cookie->numcsns].bv_len == 0 );
		LDAP_ENSURE( cookie->ctxcsn[cookie->numcsns].bv_val == NULL );
	}

	for ( i = 0; i < cookie->numcsns; i++ ) {
		CHEK_MEM_VALID( cookie->ctxcsn[i].bv_val );
		LDAP_ENSURE( slap_csn_verify_full( cookie->ctxcsn + i ));
		LDAP_ENSURE( cookie->sids[i] == slap_csn_get_sid( cookie->ctxcsn + i ) );
		LDAP_ENSURE( cookie->sids[i] >= 0 && cookie->sids[i] <= SLAP_SYNC_SID_MAX );
		if ( i > 0 )
			LDAP_ENSURE( cookie->sids[i-1] < cookie->sids[i] );
	}
}

void slap_cookie_init( struct sync_cookie *cookie )
{
	cookie->rid = -1;
	cookie->sid = -1;
	cookie->numcsns = 0;
	cookie->sids = NULL;
	cookie->ctxcsn = NULL;
	BER_BVZERO( &cookie->octet_str );
}

void slap_cookie_clean( struct sync_cookie *cookie )
{
	cookie->rid = -1;
	cookie->sid = -1;
	cookie->numcsns = 0;
	if ( cookie->ctxcsn )
		BER_BVZERO( cookie->ctxcsn );
	if ( cookie->octet_str.bv_val ) {
		cookie->octet_str.bv_val[0] = 0;
		cookie->octet_str.bv_len = 0;
	}
}

void slap_cookie_copy(
	struct sync_cookie *dst,
	const struct sync_cookie *src )
{
	if (reopenldap_mode_idkfa())
		slap_cookie_verify( src );

	dst->rid = src->rid;
	dst->sid = src->sid;
	dst->sids = NULL;
	dst->ctxcsn = NULL;
	BER_BVZERO( &dst->octet_str );
	if ( (dst->numcsns = src->numcsns) > 0 ) {
		ber_bvarray_dup_x( &dst->ctxcsn, src->ctxcsn, NULL );
		ber_dupbv( &dst->octet_str, &src->octet_str );
		dst->sids = ber_memalloc( dst->numcsns * sizeof(dst->sids[0]) );
		memcpy( dst->sids, src->sids, dst->numcsns * sizeof(dst->sids[0]) );
	}
}

void slap_cookie_move(
	struct sync_cookie *dst,
	struct sync_cookie *src )
{
	if (reopenldap_mode_idkfa())
		slap_cookie_verify( src );

	dst->rid = src->rid;
	dst->sid = src->sid;
	dst->ctxcsn = src->ctxcsn;
	dst->sids = src->sids;
	dst->numcsns = src->numcsns;
	dst->octet_str = src->octet_str;
	slap_cookie_init( src );
}

void slap_cookie_free(
	struct sync_cookie *cookie,
	int free_cookie )
{
	if ( cookie ) {

		cookie->rid = -1;
		cookie->sid = -1;
		cookie->numcsns = 0;

		if ( cookie->sids ) {
			ber_memfree( cookie->sids );
			cookie->sids = NULL;
		}

		if ( cookie->ctxcsn ) {
			ber_bvarray_free( cookie->ctxcsn );
			cookie->ctxcsn = NULL;
		}

		if ( !BER_BVISNULL( &cookie->octet_str )) {
			ch_free( cookie->octet_str.bv_val );
			BER_BVZERO( &cookie->octet_str );
		}

		if ( free_cookie )
			ber_memfree( cookie );
	}
}

int slap_cookie_merge(
	BackendDB *bd,
	struct sync_cookie *dst,
	const struct sync_cookie* src )
{
	int i, j, lead = -1;

	if ( reopenldap_mode_idkfa() ) {
		slap_cookie_verify( dst );
		slap_cookie_verify( src );
	}

	/* find any CSNs in the SRC that are newer than the DST */
	for ( i = 0; i < src->numcsns; i++ ) {
		for ( j = 0; j < dst->numcsns; j++ ) {
			if ( src->sids[i] < dst->sids[j] )
				break;
			if ( src->sids[i] != dst->sids[j] )
				continue;
			if ( slap_csn_compare_ts( &src->ctxcsn[i], &dst->ctxcsn[j] ) > 0 ) {
				Debug( LDAP_DEBUG_SYNC,
					"cookie_merge: forward %s => %s\n",
					dst->ctxcsn[j].bv_val, src->ctxcsn[i].bv_val );
				ber_bvreplace( &dst->ctxcsn[j], &src->ctxcsn[i] );
				if ( lead < 0 ||
					slap_csn_compare_ts( &dst->ctxcsn[j], &dst->ctxcsn[lead] ) > 0 ) {
					lead = i;
				}
			}
			break;
		}

		/* there was no match for this SID, it's a new CSN */
		if ( j == dst->numcsns || src->sids[i] != dst->sids[j] ) {
			Debug( LDAP_DEBUG_SYNC,
				"cookie_merge: append %s\n", src->ctxcsn[i].bv_val );
			slap_insert_csn_sids( dst, j, src->sids[i], &src->ctxcsn[i] );
			if (bd) {
				quorum_notify_csn( bd, src->sids[i] );
			}
			if ( lead < 0 ||
				slap_csn_compare_ts( &dst->ctxcsn[j], &dst->ctxcsn[lead] ) > 0 ) {
				lead = i;
			}
		}
	}

	if ( reopenldap_mode_idkfa() && lead > -1) {
		slap_cookie_verify( dst );
	}
	return lead;
}

void slap_cookie_fetch(
	struct sync_cookie *dst,
	BerVarray src )
{
	ber_bvarray_free( dst->ctxcsn );
	dst->ctxcsn = src;
	dst->numcsns = slap_csns_length( dst->ctxcsn );
	dst->sids = slap_csns_parse_sids( dst->ctxcsn, dst->sids );

	if ( reopenldap_mode_idkfa() )
		slap_cookie_verify( dst );
}

int slap_cookie_pull(
	struct sync_cookie *dst,
	BerVarray src,
	int consume )
{
	int vector;

	if ( ! consume ) {
		BerVarray clone = NULL;
		int rc = ber_bvarray_dup_x( &clone, src, NULL );
		if ( rc < 0 )
			return rc;
		src = clone;
	}

	if ( src )
		slap_csns_validate_and_sort( src );
	vector = slap_csns_compare( src, dst->ctxcsn );
	if ( vector > 0 ) {
		slap_cookie_fetch( dst, src );
	} else {
		if (unlikely( vector < 0 )) {
			slap_csns_backward_debug( __FUNCTION__, dst->ctxcsn, src );
			assert(vector >= 0);
		}
		ber_bvarray_free( src );
	}
	if (reopenldap_mode_idkfa())
		slap_cookie_verify( dst );
	return vector;
}

int slap_cookie_merge_csn(
	BackendDB *bd,
	struct sync_cookie *dst,
	int sid,
	BerValue *src )
{
	int i;

	if ( !slap_csn_verify_full( src ) )
		return -1;

	if (reopenldap_mode_idkfa())
		slap_cookie_verify( dst );

	if ( sid < 0 ) {
		sid = slap_csn_get_sid ( src );
		if ( sid < 0 )
			return -1;
	}

	for ( i = 0; i < dst->numcsns; ++i ) {
		if ( sid < dst->sids[i] )
			break;
		if ( sid == dst->sids[i] ) {
			int vector = slap_csn_compare_ts( src, &dst->ctxcsn[i] );
			if ( vector > 0 )
				ber_bvreplace( &dst->ctxcsn[i], src );
			return vector;
		}
	}

	/* It's a new SID for us */
	if ( i == dst->numcsns || sid != dst->sids[i] ) {
		slap_insert_csn_sids( dst, i, sid, src );
		if (bd) {
			quorum_notify_csn( bd, sid );
		}
		return 1;
	}

	return 0;
}

int slap_cookie_compare_csn(
	const struct sync_cookie *cookie,
	BerValue *csn )
{
	int sid, i;

	if ( !slap_csn_verify_full( csn ) )
		return -1;

	sid = slap_csn_get_sid ( csn );
	if ( sid < 0 )
		return -1;

	for ( i = 0; i < cookie->numcsns; ++i ) {
		if ( sid < cookie->sids[i] )
			break;
		if ( sid == cookie->sids[i] )
			return slap_csn_compare_ts( csn, &cookie->ctxcsn[i] );
	}

	/* LY: new SID */
	return 1;
}

static int strntoi( char* str, int n, char** end, int base)
{
	int r, d;

	for ( r = 0; n > 0; --n, ++str) {
		if ( *str >= '0' && *str <= '9' )
			d = *str - '0';
		else if ( *str >= 'A' && *str <= 'Z' )
			d = *str - 'A' + 10;
		else if ( *str >= 'a' && *str <= 'z' )
			d = *str - 'a' + 10;
		else
			break;

		if ( d >= base )
			break;

		d += r * base;
		if (d < r)
			break;
		r = d;
	}

	*end = str;
	return r;
}

int slap_cookie_parse(
	struct sync_cookie *dst,
	const BerValue *src )
{
	char *next, *end, *anchor;
	AttributeDescription *ad = slap_schema.si_ad_entryCSN;

	/* LY: This should be replaced by slap_cookie_clean(), but
	 * not earlier that octer_str will be removed from cookie struct. */
	dst->rid = -1;
	dst->sid = -1;
	dst->numcsns = 0;
	if ( dst->ctxcsn )
		BER_BVZERO( dst->ctxcsn );

	if ( !src || src->bv_len <= STRLENOF( "rid=" ) )
		goto bailout;
	if ( src->bv_len != strnlen( src->bv_val, src->bv_len ) )
		goto bailout;

	end = src->bv_val + src->bv_len;
	for ( next = src->bv_val; next < end; ) {
		if ( strncmp( next, "rid=", STRLENOF("rid=")) == 0 ) {
			anchor = next + STRLENOF("rid=");
			dst->rid = strntoi( anchor, end - anchor, &next, 10 );
			if (unlikely( next == anchor ||
					( *next && *next != ',' ) ||
					dst->rid < 0 || dst->rid > SLAP_SYNC_RID_MAX ))
				goto bailout;

			if ( *next == ',' )
				next++;

		} else if ( strncmp( next, "sid=", STRLENOF("sid=")) == 0 ) {
			anchor = next + STRLENOF("sid=");
			dst->sid = strntoi( anchor, end - anchor, &next, 16 );
			if (unlikely( next == anchor ||
					( *next && *next != ',' ) ||
					dst->sid < 0 || dst->sid > SLAP_SYNC_SID_MAX ))
				goto bailout;

			if ( *next == ',' )
				next++;

		} else if ( strncmp( next, "csn=", STRLENOF("csn=")) == 0 ) {
			next += STRLENOF("csn=");
			do {
				BerValue csn;
				char *comma;

				comma = strpbrk( anchor = next, ";," );
				next = comma ? comma + 1 : (comma = end);

				csn.bv_len = comma - anchor;
				csn.bv_val = anchor;

				if ( ad ) {
					BerValue bv;
					if ( LDAP_SUCCESS == ad->ad_type->sat_syntax->ssyn_validate(
							ad->ad_type->sat_syntax, &csn )
						&& LDAP_SUCCESS == ad->ad_type->sat_equality->smr_normalize(
							SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
							ad->ad_type->sat_syntax,
							ad->ad_type->sat_equality,
							&csn, &bv, NULL ) ) {
						csn = bv;
					}
				}

				if ( ! slap_csn_verify_full( &csn ) ) {
					if ( csn.bv_val != anchor )
						ber_memfree( csn.bv_val );
					if ( reopenldap_mode_idclip() )
						goto bailout;
					csn.bv_val = ber_strdup(
						"19000101000000.000000Z#000000#000#000000" );
					csn.bv_len = 40;
				}

				if ( csn.bv_val == anchor )
					csn.bv_val = ber_strndup( anchor, csn.bv_len );
				ber_bvarray_add( &dst->ctxcsn, &csn );
				dst->numcsns++;
			}
			while ( next < end );

		} else if ( *next == ' ' ) {
			next++;
		} else {
			goto bailout;
		}
	}

	if ( dst->rid < 0 )
		goto bailout;

	if ( dst->numcsns == 0 )
		return LDAP_SUCCESS;

	if ( dst->numcsns == slap_csns_validate_and_sort( dst->ctxcsn ) ) {
		dst->sids = slap_csns_parse_sids( dst->ctxcsn, dst->sids );
		if ( reopenldap_mode_idkfa() )
			slap_cookie_verify( dst );
		return LDAP_SUCCESS;
	}

bailout:
	slap_cookie_free( dst, 0 );
	return LDAP_PROTOCOL_ERROR;
}

int slap_cookie_stubself( struct sync_cookie *dst )
{
	int i;
	char buf[ LDAP_PVT_CSNSTR_BUFSIZE ];
	BerValue csn;

	for ( i = 0; i < dst->numcsns; ++i )
		if ( slap_serverID == dst->sids[i] )
			return 0;

	csn.bv_val = buf;
	csn.bv_len = snprintf( buf, sizeof( buf ),
		"%4d%02d%02d%02d%02d%02d.%06dZ#%06x#%03x#%06x",
		1900, 1, 1, 0, 0, 0, 0, 0, slap_serverID, 0 );

	return slap_cookie_merge_csn( NULL, dst, slap_serverID, &csn );
}

void slap_cookie_compose(
	BerValue *dst,
	BerVarray csns,
	int rid,
	int sid,
	void *memctx )
{
	char buf[32], *p;
	BerValue *csn;
	int len;

	buf[len = 0] = 0;
	if ( rid >= 0 )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"rid=%03d", rid );
	if ( sid >= 0 )
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%ssid=%03x", len ? "," : "", sid );

	if ( csns && !BER_BVISNULL( csns ) ) {
		len += snprintf( buf + len, sizeof( buf ) - len,
			"%scsn=", len ? "," : "" ) - 1;
		for ( csn = csns; !BER_BVISNULL( csn ); ++csn )
			len += csn->bv_len + 1;
	}

	dst->bv_len = len;
	dst->bv_val = ber_memrealloc_x( dst->bv_val, len + 1, memctx );

	p = stpcpy( dst->bv_val, buf );
	if ( csns && !BER_BVISNULL( csns ) ) {
		for ( csn = csns; !BER_BVISNULL( csn ); ++csn ) {
			assert( strlen( csn->bv_val ) == csn->bv_len );
			p = stpcpy( p, csn->bv_val );
			*p++ = ';';
		}
		*--p = 0;
	}
	assert( p == dst->bv_val + dst->bv_len );
}

void slap_cookie_debug( const char *prefix, const struct sync_cookie *sc )
{
	int i;

	lutil_debug_print( "%s %p: rid=%d, sid=%d\n", prefix,
		sc, sc->rid, sc->sid );
	for( i = 0; i < sc->numcsns; ++i )
		lutil_debug_print( "%s: %d) %ssid=%d %s\n", prefix, i,
			slap_csn_verify_full( &sc->ctxcsn[i] ) ? "" : "INVALID ",
			sc->sids[i], sc->ctxcsn[i].bv_val );
}

/*----------------------------------------------------------------------------*/

int slap_csn_verify_lite( const BerValue *csn )
{
	if ( unlikely(
			csn->bv_len != 40 ||
			csn->bv_val[14] != '.' ||
			csn->bv_val[21] != 'Z' ||
			csn->bv_val[22] != '#' ||
			csn->bv_val[29] != '#' ||
			csn->bv_val[33] != '#' ||
			csn->bv_val[40] != 0 ))
		return 0;

	return 1;
}

int slap_csn_verify_full( const BerValue *csn )
{
	int i;
	char nowbuf[LDAP_PVT_CSNSTR_BUFSIZE];
	BerValue now;

	if (unlikely( csn->bv_len != 40 ))
		goto bailout;

	for( i = 0; i < 40; ++i ) {
		char c = csn->bv_val[i];
		const char x = "99999999999999.999999Z#FFFFFF#FFF#FFFFFF"[i];
		switch ( x ) {
		case 'F':
			if ( c >= 'a' && c <= 'f' ) continue;
			if ( c >= 'A' && c <= 'F' ) continue;
		case '9':
			if ( c >= '0' && c <= '9' ) continue;
		default:
			if ( c == x ) continue;
			goto bailout;
		}
	}

	if ( reopenldap_mode_idclip() ) {
		if (unlikely( memcmp( csn->bv_val, "19000101000000.000000", 21 ) < 0 ))
			goto bailout;

		now.bv_len = ldap_pvt_csnstr( now.bv_val = nowbuf, sizeof(nowbuf), 0, 0 );
		if ( slap_csn_compare_ts( &now, csn ) < 0 )
			goto bailout;
	}
	return 1;

bailout:
	return 0;
}

int slap_csn_match( const BerValue *a, const BerValue *b )
{
	assert( slap_csn_verify_lite( a ) && slap_csn_verify_lite( b ) );
	return memcmp(a->bv_val, b->bv_val, 40) == 0;
}

int slap_csn_compare_sr( const BerValue *a, const BerValue *b )
{
	assert( slap_csn_verify_lite( a ) && slap_csn_verify_lite( b ) );
	return memcmp( a->bv_val + 30, b->bv_val + 30,
				   4 /* LY: 3 is enough, but +1 allows one 32-bits ops */ );
}

int slap_csn_compare_ts( const BerValue *a, const BerValue *b )
{
	assert( slap_csn_verify_lite( a ) && slap_csn_verify_lite( b ) );
	return memcmp( a->bv_val, b->bv_val, 29 );
}

int slap_csn_get_sid( const BerValue *csn )
{
	char* endptr;
	int sid;

	if ( unlikely( ! slap_csn_verify_lite( csn ) ))
		goto bailout;

	sid = strtol( csn->bv_val + 30, &endptr, 16 );
	if ( unlikely(
			endptr != csn->bv_val + 33 ||
			sid < 0 ||
			sid > SLAP_SYNC_SID_MAX /* LY: paranoia ;) */ ))
		goto bailout;

	return sid;

bailout:
	Debug( LDAP_DEBUG_SYNC, "slap_csn_get_sid: invalid-CSN '%s'\n",
		   csn->bv_val );
	return -1;
}

static int csn_qsort_cmp(const void *a, const void * b)
{
	int cmp = slap_csn_compare_sr( a, b );
	if (unlikely( cmp == 0 ))
		/* LY: for the same SID move recent to the front */
		cmp = slap_csn_compare_ts( b, a );
	return cmp;
}

int slap_csns_validate_and_sort( BerVarray vals )
{
	BerValue *r, *w, *end;

	for( r = w = vals; !BER_BVISNULL( r ); ++r ) {
		/* LY: validate and filter-out invalid:
		   { X1, Y2, bad, Z3, X4 | NULL } => { X1, Y2, Z3, X4 | bad, NULL } */
		if (unlikely( !slap_csn_verify_full( r ) )) {
			ber_memfree( r->bv_val );
			continue;
		}
		*w++ = *r;
	}
	end = w;

	if ( end > vals ) {
		/* LY: sort by SID and timestamp (recent first)
		   { X1, Y2, Z3, X4 | bad, NULL } => { X4, X1, Y2, Z3 | bad, NULL } */
		qsort( vals, end - vals, sizeof(BerValue), csn_qsort_cmp );

		/* LY: filter-out dups of SID.
		   { X4, X1, Y2, Z3 | bad, NULL } => { X4, Y2, Z3 | X1, bad, NULL } */
		for( r = w = vals + 1; r < end; ++r ) {
			if (unlikely( slap_csn_compare_sr( w - 1, r ) == 0 )) {
				ber_memfree( r->bv_val );
				continue;
			}
			*w++ = *r;
		}
		end = w;
	}

	BER_BVZERO( end );
	return end - vals;
}

int slap_csns_match( BerVarray a, BerVarray b )
{
	while ( a && !BER_BVISNULL(a) && b && !BER_BVISNULL(b) ) {
		if ( ! slap_csn_match ( a, b ) )
			return 0;
		++a, ++b;
	}

	return (!a || BER_BVISNULL(a)) && (!b || BER_BVISNULL(b));
}

int slap_csns_length( BerVarray vals )
{
	int res = 0;

	if ( vals )
		while( ! BER_BVISNULL( vals ) )
			++vals, ++res;

	return res;
}

int slap_csns_compare( BerVarray next, BerVarray base )
{
	int cmp, n, res = 0;

	for ( n = 1; next && ! BER_BVISNULL( next ); ++next, ++n ) {
		if ( ! base || BER_BVISNULL( base ) )
			return INT_MAX;

		cmp = slap_csn_compare_sr ( next, base );
		if ( cmp > 0 )
			/* LY: a base's sid is absent in the next. */
			return INT_MIN;

		if ( cmp < 0 ) {
			/* LY: a new sid in the next. */
			res = INT_MAX;
			continue;
		}

		cmp = slap_csn_compare_ts ( next, base++ );
		if (cmp < 0)
			/* LY: base's timestamp is recent. */
			return -n;

		if (cmp && res == 0)
			res = n;
	}

	if ( ! base || BER_BVISNULL( base ) )
		return res;

	return INT_MIN;
}

int* slap_csns_parse_sids( BerVarray csns, int* sids )
{
	int i;

	sids = ber_memrealloc( sids, slap_csns_length( csns ) * sizeof(int) );

	for( i = 0; csns && ! BER_BVISNULL( &csns[i] ); ++i)
		sids[i] = slap_csn_get_sid( &csns[i] );

	return sids;
}

void slap_csns_debug( const char *prefix, const BerVarray csns )
{
	int i;

	lutil_debug_print("%s: CSNs %p\n", prefix, csns);
	for( i = 0; csns && ! BER_BVISNULL( &csns[i] ); ++i )
		lutil_debug_print( "%s: %d) %s%s\n", prefix, i,
			slap_csn_verify_full( &csns[i] ) ? "" : "INVALID ",
			csns[i].bv_val
		);
}

void slap_csns_backward_debug(
	const char *prefix,
	const BerVarray current,
	const BerVarray next )
{
	if ( LogTest( LDAP_DEBUG_SYNC ) ) {
		slap_backtrace_debug();
		lutil_debug_print("%s: %s > %s\n", prefix, "current", "next" );
		slap_csns_debug( "current", current );
		slap_csns_debug( "next", next );
	}
}
