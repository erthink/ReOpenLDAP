/* unbind.c - ldap backend unbind function */
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
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "back-ldap.h"

int
ldap_back_conn_destroy(
		Backend		*be,
		Connection	*conn
)
{
	ldapinfo_t	*li = (ldapinfo_t *) be->be_private;
	ldapconn_t	*lc = NULL, lc_curr;

	Debug( LDAP_DEBUG_TRACE,
		"=>ldap_back_conn_destroy: fetching conn %ld\n",
		conn->c_connid );

	lc_curr.lc_conn = conn;

	ldap_pvt_thread_mutex_lock( &li->li_conninfo.lai_mutex );
#if LDAP_BACK_PRINT_CONNTREE > 0
	ldap_back_print_conntree( li, ">>> ldap_back_conn_destroy" );
#endif /* LDAP_BACK_PRINT_CONNTREE */
	while ( ( lc = avl_delete( &li->li_conninfo.lai_tree, (caddr_t)&lc_curr, ldap_back_conn_cmp ) ) != NULL )
	{
		assert( !LDAP_BACK_PCONN_ISPRIV( lc ) );
		Debug( LDAP_DEBUG_TRACE,
			"=>ldap_back_conn_destroy: destroying conn %lu "
			"refcnt=%d flags=0x%08x\n",
			lc->lc_conn->c_connid, lc->lc_refcnt, lc->lc_lcflags );

		if ( lc->lc_refcnt > 0 ) {
			/* someone else might be accessing the connection;
			 * mark for deletion */
			LDAP_BACK_CONN_CACHED_CLEAR( lc );
			LDAP_BACK_CONN_TAINTED_SET( lc );

		} else {
			ldap_back_conn_free( lc );
		}
	}
#if LDAP_BACK_PRINT_CONNTREE > 0
	ldap_back_print_conntree( li, "<<< ldap_back_conn_destroy" );
#endif /* LDAP_BACK_PRINT_CONNTREE */
	ldap_pvt_thread_mutex_unlock( &li->li_conninfo.lai_mutex );

	return 0;
}
