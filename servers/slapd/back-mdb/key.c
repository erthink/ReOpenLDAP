/* index.c - routines for dealing with attribute indexes */
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
 * Copyright 2000-2014 The OpenLDAP Foundation.
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

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-mdb.h"
#include "idl.h"

/* read a key */
int
mdb_key_read(
	Backend	*be,
	MDB_txn *txn,
	MDB_dbi dbi,
	struct berval *k,
	ID *ids,
	MDB_cursor **saved_cursor,
	int get_flag
)
{
	int rc;
	MDB_val key;
#ifndef MISALIGNED_OK
	int kbuf[2];
#endif

	Debug( LDAP_DEBUG_TRACE, "=> key_read\n" );

#ifndef MISALIGNED_OK
	if (k->bv_len & ALIGNER) {
		key.mv_size = sizeof(kbuf);
		key.mv_data = kbuf;
		kbuf[1] = 0;
		memcpy(kbuf, k->bv_val, k->bv_len);
	} else
#endif
	{
		key.mv_size = k->bv_len;
		key.mv_data = k->bv_val;
	}

	rc = mdb_idl_fetch_key( be, txn, dbi, &key, ids, saved_cursor, get_flag );

	if( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "<= mdb_index_read: failed (%d)\n",
			rc );
	} else {
		Debug( LDAP_DEBUG_TRACE, "<= mdb_index_read %ld candidates\n",
			(long) MDB_IDL_N(ids) );
	}

	return rc;
}
