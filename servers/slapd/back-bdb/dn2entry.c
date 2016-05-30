/* dn2entry.c - routines to deal with the dn2id / id2entry glue */
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

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "back-bdb.h"

/*
 * dn2entry - look up dn in the cache/indexes and return the corresponding
 * entry. If the requested DN is not found and matched is TRUE, return info
 * for the closest ancestor of the DN. Otherwise e is NULL.
 */

int
bdb_dn2entry(
	Operation *op,
	DB_TXN *tid,
	struct berval *dn,
	EntryInfo **e,
	int matched,
	DB_LOCK *lock )
{
	EntryInfo *ei = NULL;
	int rc, rc2;

	Debug(LDAP_DEBUG_TRACE, "bdb_dn2entry(\"%s\")\n",
		dn->bv_val );

	*e = NULL;

	rc = bdb_cache_find_ndn( op, tid, dn, &ei );
	if ( rc ) {
		if ( matched && rc == DB_NOTFOUND ) {
			/* Set the return value, whether we have its entry
			 * or not.
			 */
			*e = ei;
			if ( ei && ei->bei_id ) {
				rc2 = bdb_cache_find_id( op, tid, ei->bei_id,
					&ei, ID_LOCKED, lock );
				if ( rc2 ) rc = rc2;
			} else if ( ei ) {
				bdb_cache_entryinfo_unlock( ei );
				memset( lock, 0, sizeof( *lock ));
				lock->mode = DB_LOCK_NG;
			}
		} else if ( ei ) {
			bdb_cache_entryinfo_unlock( ei );
		}
	} else {
		rc = bdb_cache_find_id( op, tid, ei->bei_id, &ei, ID_LOCKED,
			lock );
		if ( rc == 0 ) {
			*e = ei;
		} else if ( matched && rc == DB_NOTFOUND ) {
			/* always return EntryInfo */
			if ( ei->bei_parent ) {
				ei = ei->bei_parent;
				rc2 = bdb_cache_find_id( op, tid, ei->bei_id, &ei, 0,
					lock );
				if ( rc2 ) rc = rc2;
			}
			*e = ei;
		}
	}

	return rc;
}
