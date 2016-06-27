/* OpenLDAP WiredTiger backend */
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
 * Copyright 2002-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS:
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"

/*
 * dn2entry - look up dn in the db and return the corresponding entry.
 * No longer return closest ancestor, see wt_dn2pentry().
 */
int wt_dn2entry( BackendDB *be,
				 wt_ctx *wc,
				 struct berval *ndn,
				 Entry **ep ){
	uint64_t id;
	WT_CURSOR *cursor = NULL;
	WT_ITEM item;
	EntryHeader eh;
	int rc;
	int eoff;
	Entry *e = NULL;
	WT_SESSION *session = wc->session;

	if( ndn->bv_len == 0 ){
		/* parent of root dn */
		return WT_NOTFOUND;
	}

	rc = session->open_cursor(session,
							  WT_INDEX_DN"(id, entry)",
							  NULL, NULL, &cursor);
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2entry)
			   ": open_cursor failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	cursor->set_key(cursor, ndn->bv_val);
	rc = cursor->search(cursor);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		goto done;
	default:
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2entry)
			   ": search failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}
	cursor->get_value(cursor, &id, &item);
	rc = wt_entry_header( &item,  &eh );

	eoff = eh.data - (char *)item.data;
	eh.bv.bv_len = eh.nvals * sizeof( struct berval ) + item.size;
	eh.bv.bv_val = ch_malloc( eh.bv.bv_len );
	memset(eh.bv.bv_val, 0xff, eh.bv.bv_len);
	eh.data = eh.bv.bv_val + eh.nvals * sizeof( struct berval );
	memcpy(eh.data, item.data, item.size);
	eh.data += eoff;
	rc = entry_decode( &eh, &e );
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2entry)
			   ": entry decode error: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}

	e->e_id = id;
	*ep = e;

done:
	if(cursor){
		cursor->close(cursor);
	}
	return rc;
}

/* dn2pentry - return parent entry */
int wt_dn2pentry( BackendDB *be,
				  wt_ctx *wc,
				  struct berval *ndn,
				  Entry **ep ){
	Entry *e = NULL;
	struct berval pdn;
	int rc;

	if (be_issuffix( be, ndn )) {
		*ep = NULL;
		return WT_NOTFOUND;
	}

	dnParent( ndn, &pdn );
	rc = wt_dn2entry(be, wc, &pdn, &e);
	*ep = e;
	return rc;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
