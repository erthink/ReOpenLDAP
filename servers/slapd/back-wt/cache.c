/* $ReOpenLDAP$ */
/* Copyright 2002-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#include "reldap.h"

#include <stdio.h>
#include "back-wt.h"
#include "slapconfig.h"
#include "idl.h"

int wt_idlcache_get(wt_ctx *wc, struct berval *ndn, int scope, ID *ids)
{
	int rc;
	WT_ITEM item;
	WT_SESSION *session = wc->cache_session;
	WT_CURSOR *cursor = wc->idlcache;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_get(\"%s\", %d)\n",
		   ndn->bv_val, scope );

	if(!cursor){
		rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
								  NULL, &cursor);
		if(rc){
			Debug( LDAP_DEBUG_ANY,
				   "wt_idlcache_get: open_cursor failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			return rc;
		}
		wc->idlcache = cursor;
	}
	cursor->set_key(cursor, ndn->bv_val, (int8_t)scope);
	rc = cursor->search(cursor);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		Debug(LDAP_DEBUG_TRACE, "<= wt_idlcache_get: miss\n");
		return rc;
	default:
		Debug( LDAP_DEBUG_ANY, "<= wt_idlcache_get: search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return 0;
	}
	rc = cursor->get_value(cursor, &item);
	if (rc) {
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_get: get_value failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}
	memcpy(ids, item.data, item.size);

	Debug(LDAP_DEBUG_TRACE,
		  "<= wt_idlcache_get: hit id=%ld first=%ld last=%ld\n",
		  (long)ids[0],
		  (long)WT_IDL_FIRST(ids),
		  (long)WT_IDL_LAST(ids));
	return rc;
}

int wt_idlcache_set(wt_ctx *wc, struct berval *ndn, int scope, ID *ids)
{
	int rc;
	WT_ITEM item;
	WT_SESSION *session = wc->cache_session;
	WT_CURSOR *cursor = wc->idlcache;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_set(\"%s\", %d)\n",
		   ndn->bv_val, scope );

	item.size = WT_IDL_SIZEOF(ids);
	item.data = ids;

	if(!cursor){
		rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
								  NULL, &cursor);
		if(rc){
			Debug( LDAP_DEBUG_ANY,
				   "wt_idlcache_set: open_cursor failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			return rc;
		}
		wc->idlcache = cursor;
	}
	cursor->set_key(cursor, ndn->bv_val, (int8_t)scope);
	cursor->set_value(cursor, &item);
	rc = cursor->insert(cursor);
	if(rc){
		Debug( LDAP_DEBUG_ANY,
			   "wt_idlcache_set: insert failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return rc;
	}

	Debug(LDAP_DEBUG_TRACE,
		  "<= wt_idlcache_set: set idl size=%ld\n",
		  (long)ids[0]);
	return rc;
}

int wt_idlcache_clear(Operation *op, wt_ctx *wc, struct berval *ndn)
{
	BackendDB *be = op->o_bd;
	int rc;
	struct berval pdn = *ndn;
	WT_SESSION *session = wc->cache_session;
	WT_CURSOR *cursor = wc->idlcache;
	int level = 0;

	Debug( LDAP_DEBUG_TRACE,
		   "=> wt_idlcache_clear(\"%s\")\n",
		   ndn->bv_val );

	if (be_issuffix( be, ndn )) {
		return 0;
	}

	if(!cursor){
		rc = session->open_cursor(session, WT_TABLE_IDLCACHE, NULL,
								  NULL, &cursor);
		if(rc){
			Debug( LDAP_DEBUG_ANY,
				   "wt_idlcache_clear: open_cursor failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
			return rc;
		}
		wc->idlcache = cursor;
	}

	do{
		dnParent( &pdn, &pdn );
		level++;
		if (level == 1) {
			/* clear only parent level cache */
			cursor->set_key(cursor, pdn.bv_val, (int8_t)LDAP_SCOPE_ONE);
			cursor->remove(cursor);
		}
		cursor->set_key(cursor, pdn.bv_val, (int8_t)LDAP_SCOPE_SUB);
		cursor->remove(cursor);
		cursor->set_key(cursor, pdn.bv_val, (int8_t)LDAP_SCOPE_CHILDREN);
		cursor->remove(cursor);
		level++;
	}while(!be_issuffix( be, &pdn ));

	return 0;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
