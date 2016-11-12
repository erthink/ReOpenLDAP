/* $ReOpenLDAP$ */
/* Copyright 2001-2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include <lutil.h>
#include "proto-slap.h"
#include "back-monitor.h"

static int
monitor_subsys_time_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e );

int
monitor_subsys_time_init(
	BackendDB		*be,
	monitor_subsys_t	*ms )
{
	monitor_info_t	*mi;

	Entry		*e, **ep, *e_time = 0;
	monitor_entry_t	*mp;
	struct berval	bv, value;

	assert( be != NULL );

	ms->mss_update = monitor_subsys_time_update;

	mi = ( monitor_info_t * )be->be_private;

	if ( monitor_cache_get( mi,
			&ms->mss_ndn, &e_time ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to get entry \"%s\"\n",
			ms->mss_ndn.bv_val );
		return -1;
	}

	mp = ( monitor_entry_t * )e_time->e_private;
	mp->mp_children = NULL;
	ep = &mp->mp_children;
	int rc = -1;

	BER_BVSTR( &bv, "cn=Start" );
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
		mi->mi_oc_monitoredObject, NULL, NULL );
	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to create entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val );
		goto bailout;
	}
	attr_merge_normalize_one( e, mi->mi_ad_monitorTimestamp,
		&mi->mi_startTime, NULL );

	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		goto bailout;
	}
	e->e_private = ( void * )mp;
	mp->mp_info = ms;
	mp->mp_flags = ms->mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to add entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val );
		goto bailout;
	}

	*ep = e;
	ep = &mp->mp_next;

	/*
	 * Current
	 */
	BER_BVSTR( &bv, "cn=Current" );
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
		mi->mi_oc_monitoredObject, NULL, NULL );
	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to create entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val );
		goto bailout;
	}
	attr_merge_normalize_one( e, mi->mi_ad_monitorTimestamp,
		&mi->mi_startTime, NULL );

	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		goto bailout;
	}
	e->e_private = ( void * )mp;
	mp->mp_info = ms;
	mp->mp_flags = ms->mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to add entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val );
		goto bailout;
	}

	*ep = e;
	ep = &mp->mp_next;

	/*
	 * Uptime
	 */
	BER_BVSTR( &bv, "cn=Uptime" );
	e = monitor_entry_stub( &ms->mss_dn, &ms->mss_ndn, &bv,
		mi->mi_oc_monitoredObject, NULL, NULL );
	if ( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to create entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val );
		goto bailout;
	}
	BER_BVSTR( &value, "0" );
	attr_merge_normalize_one( e, mi->mi_ad_monitoredInfo,
		&value, NULL );

	mp = monitor_entrypriv_create();
	if ( mp == NULL ) {
		goto bailout;
	}
	e->e_private = ( void * )mp;
	mp->mp_info = ms;
	mp->mp_flags = ms->mss_flags \
		| MONITOR_F_SUB | MONITOR_F_PERSISTENT;

	if ( monitor_cache_add( mi, e ) ) {
		Debug( LDAP_DEBUG_ANY,
			"monitor_subsys_time_init: "
			"unable to add entry \"%s,%s\"\n",
			bv.bv_val, ms->mss_ndn.bv_val );
		goto bailout;
	}

	*ep = e;
	ep = &mp->mp_next;
	rc = 0;

bailout:
	monitor_cache_release( mi, e_time );
	return rc;
}

static int
monitor_subsys_time_update(
	Operation		*op,
	SlapReply		*rs,
	Entry                   *e )
{
	monitor_info_t		*mi = ( monitor_info_t * )op->o_bd->be_private;
	static struct berval	bv_current = BER_BVC( "cn=current" ),
				bv_uptime = BER_BVC( "cn=uptime" );
	struct berval		rdn;

	assert( mi != NULL );
	assert( e != NULL );

	dnRdn( &e->e_nname, &rdn );

	if ( dn_match( &rdn, &bv_current ) ) {
		struct tm	tm;
		char		tmbuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
		Attribute	*a;
		ber_len_t	len;

		ldap_pvt_gmtime( ldap_now(), &tm );
		lutil_gentime( tmbuf, sizeof( tmbuf ), &tm );

		len = strlen( tmbuf );

		a = attr_find( e->e_attrs, mi->mi_ad_monitorTimestamp );
		if ( a == NULL ) {
			return rs->sr_err = LDAP_OTHER;
		}

		assert( len == a->a_vals[ 0 ].bv_len );
		memcpy( a->a_vals[ 0 ].bv_val, tmbuf, len );

		/* FIXME: touch modifyTimestamp? */

	} else if ( dn_match( &rdn, &bv_uptime ) ) {
		Attribute	*a;
		double		diff;
		char		buf[ BACKMONITOR_BUFSIZE ];
		struct berval	bv;

		a = attr_find( e->e_attrs, mi->mi_ad_monitoredInfo );
		if ( a == NULL ) {
			return rs->sr_err = LDAP_OTHER;
		}

		diff = (ldap_now().ns - starttime.ns) * 1e-9;
		bv.bv_len = snprintf( buf, sizeof( buf ), "%lu",
			(unsigned long) diff );
		bv.bv_val = buf;

		ber_bvreplace( &a->a_vals[ 0 ], &bv );
		if ( a->a_nvals != a->a_vals ) {
			ber_bvreplace( &a->a_nvals[ 0 ], &bv );
		}

		/* FIXME: touch modifyTimestamp? */
	}

	return SLAP_CB_CONTINUE;
}
