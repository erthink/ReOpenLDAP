/* $ReOpenLDAP$ */
/* Copyright 2015-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* ldapsync.c -- LDAP Content Sync Routines */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "../../libraries/libreldap/lber-int.h" /* get ber_strndup() */
#include "lutil_ldap.h"

#if LDAP_MEMORY_DEBUG
#	include <lber_hipagut.h>
#	define CHECK_MEM_VALID(p) lber_hug_memchk_ensure(p, 0)
#else
#	define CHECK_MEM_VALID(p) __noop()
#endif /* LDAP_MEMORY_DEBUG */

static int* slap_csns_parse_sids(BerVarray csns, int* sids, void *memctx);

pthread_mutex_t slap_sync_cookie_mutex = PTHREAD_MUTEX_INITIALIZER;
struct slap_sync_cookie_s slap_sync_cookie =
	LDAP_STAILQ_HEAD_INITIALIZER( slap_sync_cookie );

int slap_check_same_server(BackendDB *bd, int sid) {
	return ( sid == slap_serverID
			&& reopenldap_mode_strict() && SLAP_MULTIMASTER(bd) ) ? -1 : 0;
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
	ck->ctxcsn = ch_realloc( ck->ctxcsn,
		(ck->numcsns+1) * sizeof(struct berval));
	BER_BVZERO( &ck->ctxcsn[ck->numcsns] );
	ck->sids = ch_realloc( ck->sids, ck->numcsns * sizeof(int));
	for ( i = ck->numcsns-1; i > pos; i-- ) {
		ck->ctxcsn[i] = ck->ctxcsn[i-1];
		ck->sids[i] = ck->sids[i-1];
	}
	ck->sids[i] = sid;
	ber_dupbv( &ck->ctxcsn[i], csn );

	if (reopenldap_mode_check())
		slap_cookie_verify( ck );
}

/*----------------------------------------------------------------------------*/

int slap_cookie_is_sid_here(const struct sync_cookie *cookie, int sid)
{
	int i;

	for (i = 0; i < cookie->numcsns; ++i) {
		if (sid == cookie->sids[i])
			return 1;
		if (sid < cookie->sids[i])
			break;
	}

	return 0;
}

void slap_cookie_verify(const struct sync_cookie *cookie)
{
	int i;

	if ( cookie->numcsns ) {
		LDAP_ENSURE( cookie->ctxcsn != NULL );
		LDAP_ENSURE( cookie->sids != NULL );
		CHECK_MEM_VALID( cookie->ctxcsn );
		CHECK_MEM_VALID( cookie->sids );
	}

	if (cookie->ctxcsn) {
		LDAP_ENSURE( cookie->ctxcsn[cookie->numcsns].bv_len == 0 );
		LDAP_ENSURE( cookie->ctxcsn[cookie->numcsns].bv_val == NULL );
	}

	for ( i = 0; i < cookie->numcsns; i++ ) {
		CHECK_MEM_VALID( cookie->ctxcsn[i].bv_val );
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
}

void slap_cookie_clean_csns( struct sync_cookie *cookie, void *memctx )
{
	cookie->numcsns = 0;
	if ( cookie->ctxcsn ) {
		ber_bvarray_free_x( cookie->ctxcsn, memctx );
		cookie->ctxcsn = NULL;
		if ( memctx ) {
			ber_memfree_x( cookie->sids, memctx );
			cookie->sids = NULL;
		}
	}
}

void slap_cookie_clean_all( struct sync_cookie *cookie )
{
	cookie->rid = -1;
	cookie->sid = -1;
	slap_cookie_clean_csns( cookie, NULL );
}

void slap_cookie_copy(
	struct sync_cookie *dst,
	const struct sync_cookie *src )
{
	if (reopenldap_mode_check())
		slap_cookie_verify( src );

	dst->rid = src->rid;
	dst->sid = src->sid;
	dst->sids = NULL;
	dst->ctxcsn = NULL;
	if ( (dst->numcsns = src->numcsns) > 0 ) {
		ber_bvarray_dup_x( &dst->ctxcsn, src->ctxcsn, NULL );
		dst->sids = ber_memalloc_x( dst->numcsns * sizeof(dst->sids[0]), NULL );
		memcpy( dst->sids, src->sids, dst->numcsns * sizeof(dst->sids[0]) );
	}
}

void slap_cookie_move(
	struct sync_cookie *dst,
	struct sync_cookie *src )
{
	if (reopenldap_mode_check())
		slap_cookie_verify( src );

	dst->rid = src->rid;
	dst->sid = src->sid;
	dst->ctxcsn = src->ctxcsn;
	dst->sids = src->sids;
	dst->numcsns = src->numcsns;
	slap_cookie_init( src );
}

static void
slap_cookie_free_x(
	struct sync_cookie *cookie,
	int free_cookie, void *memctx )
{
	if ( cookie ) {

		cookie->rid = -1;
		cookie->sid = -1;
		cookie->numcsns = 0;

		if ( cookie->sids ) {
			ch_free( cookie->sids );
			cookie->sids = NULL;
		}

		if ( cookie->ctxcsn ) {
			ber_bvarray_free_x( cookie->ctxcsn, memctx );
			cookie->ctxcsn = NULL;
		}

		if ( free_cookie )
			ch_free( cookie );
	}
}

void slap_cookie_free( struct sync_cookie *cookie, int free_cookie )
{
	slap_cookie_free_x( cookie, free_cookie, NULL );
}

int slap_cookie_merge(
	BackendDB *bd,
	struct sync_cookie *dst,
	const struct sync_cookie* src )
{
	int i, j, lead = -1;

	if ( reopenldap_mode_check() ) {
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
					lead = j;
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
				lead = j;
			}
		}
	}

	if ( reopenldap_mode_check() && lead > -1) {
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
	dst->sids = slap_csns_parse_sids( dst->ctxcsn, dst->sids, NULL );

	if ( reopenldap_mode_check() )
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
		}
		ber_bvarray_free( src );
	}
	if (reopenldap_mode_check())
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

	if (reopenldap_mode_check())
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
	const BerValue *src,
	void *memctx )
{
	char *next, *end, *anchor;
	AttributeDescription *ad = slap_schema.si_ad_entryCSN;

	slap_cookie_clean_all( dst );

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
							&csn, &bv, memctx ) ) {
						csn = bv;
					}
				}

				if ( ! slap_csn_verify_full( &csn ) ) {
					if ( csn.bv_val != anchor )
						ber_memfree_x( csn.bv_val, memctx );
					if ( reopenldap_mode_strict() )
						goto bailout;
					csn.bv_val = ber_strdup_x(
						"19000101000000.000000Z#000000#000#000000",
						memctx );
					csn.bv_len = 40;
				}

				if ( csn.bv_val == anchor )
					csn.bv_val = ber_strndup_x( anchor, csn.bv_len, memctx  );
				ber_bvarray_add_x( &dst->ctxcsn, &csn, memctx );
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
		dst->sids = slap_csns_parse_sids( dst->ctxcsn, dst->sids, memctx );
		if ( reopenldap_mode_check() )
			slap_cookie_verify( dst );
		return LDAP_SUCCESS;
	}

bailout:
	slap_cookie_free_x( dst, 0, memctx );
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

	ldap_debug_print( "%s %p: rid=%d, sid=%d\n", prefix,
		sc, sc->rid, sc->sid );
	for( i = 0; i < sc->numcsns; ++i )
		ldap_debug_print( "%s: %d) %ssid=%d %s\n", prefix, i,
			slap_csn_verify_full( &sc->ctxcsn[i] ) ? "" : "INVALID ",
			sc->sids[i], sc->ctxcsn[i].bv_val );
}

void slap_cookie_debug_pair( const char *prefix,
	 const char* x_name, const struct sync_cookie *x_cookie,
	 const char* y_name, const struct sync_cookie *y_cookie, int y_marker)
{
	ldap_debug_print( "%s: %s[rid=%d, sid=%d]#%d | %s[rid=%d, sid=%d]#%d\n", prefix,
		x_name, x_cookie->rid, x_cookie->sid, x_cookie->numcsns,
		y_name, y_cookie->rid, y_cookie->sid, y_cookie->numcsns );

	int x, y;
	for(x = y = 0; ; ) {
		int sid;
		if (x < x_cookie->numcsns && y < y_cookie->numcsns)
			sid = (x_cookie->sids[x] < y_cookie->sids[y]) ? x_cookie->sids[x] : y_cookie->sids[y];
		else if (x < x_cookie->numcsns)
			sid = x_cookie->sids[x];
		else if (y < y_cookie->numcsns)
			sid = y_cookie->sids[y];
		else
			break;

		BerValue *x_csn = (x < x_cookie->numcsns && sid == x_cookie->sids[x])
				? &x_cookie->ctxcsn[x] : NULL;
		BerValue *y_csn = (y < y_cookie->numcsns && sid == y_cookie->sids[y])
				? &y_cookie->ctxcsn[y] : NULL;

		int sign = 0;
		if (x_csn && y_csn)
			sign = slap_csn_compare_ts(x_csn, y_csn);
		else if (x_csn || y_csn)
			sign = x_csn ? 1 : -1;

		ldap_debug_print( "%s: %s %s %40s %c %s %s %s\n", prefix,
			(x_csn && sid == slap_serverID) ? "~>" : "  ",
			x_name, x_csn ? x_csn->bv_val : "lack",
			sign ? ((sign > 0) ? '>' : '<')  : '=',
			y_name, y_csn ? y_csn->bv_val : "lack", (y_csn && y_marker == y) ? "<~" : "" );

		if (x_csn) x++;
		if (y_csn) y++;
	}
}

int slap_cookie_merge_csnset(
	BackendDB *bd,
	struct sync_cookie *dst,
	BerVarray src )
{
	int rc;

	if ( reopenldap_mode_check() )
		slap_cookie_verify( dst );

	for( rc = 0; src && ! BER_BVISNULL( src ); ++src ) {
		int vector = slap_cookie_merge_csn( bd, dst, -1, src );
		if ( vector > 0)
			rc = vector;
		else if ( rc == 0 )
			rc = vector;
	}

	return rc;
}

int slap_cookie_compare_csnset(
	struct sync_cookie *base,
	BerVarray next )
{
	int vector;

	if ( reopenldap_mode_check() )
		slap_cookie_verify( base );

	for( vector = 0; next && ! BER_BVISNULL( next ); ++next ) {
		int i, sid = slap_csn_get_sid( next );
		if ( sid < 0 ) {
			vector = -1;
			continue;
		}
		for( i = 0; i < base->numcsns; ++i ) {
			if ( sid < base->sids[i] )
				/* LY: next has at least one new SID. */
				return 1;

			if ( sid == base->sids[i] ) {
				vector = slap_csn_compare_ts( next, &base->ctxcsn[i] );
				if ( vector > 0 )
					/* LY: next has at least one recent timestamp. */
					return vector;
				break;
			}
		}
	}

	return vector;
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

void slap_csn_shift(BerValue *csn, int delta_points )
{
	assert( slap_csn_verify_full( csn ) );

	if ( delta_points ) {
		char *end;
		int v, carry;

		v = strntoi( csn->bv_val + 23, csn->bv_len - 23, &end, 16 );
		LDAP_ENSURE( end == csn->bv_val + 23 + 6 && *end == '#' );

		carry = 0;
		v += delta_points;

		while( v < 0 ) {
			carry -= 1;
			v += 0x1000000;
		}
		while( v >= 0x1000000 ) {
			carry += 1;
			v -= 0x1000000;
		}

		LDAP_ENSURE( snprintf( csn->bv_val + 23, csn->bv_len - 23, "%06x", v ) == 6 );
		csn->bv_val[23 + 6] = '#';

		if ( carry ) {
			v = strntoi( csn->bv_val + 15, csn->bv_len - 15, &end, 10 );
			LDAP_ENSURE( end == csn->bv_val + 15 + 6 && *end == 'Z' );

			v += carry /* + delta_us */;
			carry = 0;

			while( v < 0 ) {
				carry += 1;
				v += 1000000;
			}
			while( v >= 1000000 ) {
				carry += 1;
				v -= 1000000;
			}

			LDAP_ENSURE( snprintf( csn->bv_val + 15, csn->bv_len - 15, "%06d", v ) == 6 );
			csn->bv_val[15 + 6] = 'Z';

			if ( carry ) {
				char *digit = csn->bv_val + 13;
				assert( labs(carry) == 1 );

				while( digit >= csn->bv_val && carry ) {
					*digit += carry;
					carry = 0;

					if ( *digit < '0' ) {
						*digit = '9';
						carry = -1;
					}
					if ( *digit > '9' ) {
						*digit = '0';
						carry = 1;
					}

					--digit;
				}
			}
		}
		assert( slap_csn_verify_full( csn ) );
	}
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

	if ( reopenldap_mode_strict() ) {
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
	int cmp = memcmp( a->bv_val, b->bv_val, 29 );
	if ( cmp == 0 )
		cmp = memcmp( a->bv_val + 34, b->bv_val + 34, 6 );
	return cmp;
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
			ch_free( r->bv_val );
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
				ch_free( r->bv_val );
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
			/* LY: eof of base, but next still continue, a new sid in the next. */
			return INT_MAX /* next > base */;

		cmp = slap_csn_compare_sr( next, base );
		if ( cmp > 0 )
			/* LY: a base's sid is absent in the next. */
			return INT_MIN /* next < base */;

		if ( cmp < 0 ) {
			/* LY: a new sid in the next. */
			res = INT_MAX /* next > base */;
			continue;
		}

		cmp = slap_csn_compare_ts( next, base++ );
		if (cmp < 0)
			/* LY: base's timestamp is recent in #n position. */
			return -n /* next[n] < base[n] */;

		if (cmp && res == 0)
			/* LY: next's timestamp is recent in #n position. */
			res = n /* next[n] > base[n] */;
	}

	if ( ! base || BER_BVISNULL( base ) )
		return res;

	return INT_MIN;
}

static int* slap_csns_parse_sids(BerVarray csns, int* sids, void *memctx)
{
	int i = slap_csns_length( csns );

	sids = ber_memrealloc_x( sids, i * sizeof(sids[0]), memctx );
	while(--i >= 0)
		sids[i] = slap_csn_get_sid( &csns[i] );

	return sids;
}

void slap_csns_debug( const char *prefix, const BerVarray csns )
{
	int i;

	ldap_debug_print("%s: CSNs %p\n", prefix, csns);
	for( i = 0; csns && ! BER_BVISNULL( &csns[i] ); ++i )
		ldap_debug_print( "%s: %d) %s%s\n", prefix, i,
			slap_csn_verify_full( &csns[i] ) ? "" : "INVALID ",
			csns[i].bv_val
		);
}

void slap_csns_backward_debug(
	const char *prefix,
	const BerVarray current,
	const BerVarray next )
{
	if ( DebugTest( LDAP_DEBUG_SYNC ) ) {
		ldap_debug_print("%s: %s > %s\n", prefix, "current", "next" );
		slap_csns_debug( "current", current );
		slap_csns_debug( "next", next );
		slap_backtrace_debug();
	}
}

void slap_cookie_backward_debug(const char *prefix,
	const struct sync_cookie *current,
	const struct sync_cookie *next )
{
	if ( DebugTest( LDAP_DEBUG_SYNC ) ) {
		ldap_debug_print("%s (COOKIE BACKWARD): %s > %s\n", prefix, "current", "next" );
		slap_cookie_debug_pair(prefix, "current", current, "next", next, -1);
		slap_backtrace_debug();
	}
}
