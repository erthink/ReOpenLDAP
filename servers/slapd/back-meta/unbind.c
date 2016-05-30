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
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

int
meta_back_conn_destroy(
	Backend		*be,
	Connection	*conn )
{
	metainfo_t	*mi = ( metainfo_t * )be->be_private;
	metaconn_t	*mc,
			mc_curr = {{ 0 }};
	int		i;


	Debug( LDAP_DEBUG_TRACE,
		"=>meta_back_conn_destroy: fetching conn=%ld DN=\"%s\"\n",
		conn->c_connid,
		BER_BVISNULL( &conn->c_ndn ) ? "" : conn->c_ndn.bv_val );

	mc_curr.mc_conn = conn;

	ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );
#if META_BACK_PRINT_CONNTREE > 0
	meta_back_print_conntree( mi, ">>> meta_back_conn_destroy" );
#endif /* META_BACK_PRINT_CONNTREE */
	while ( ( mc = avl_delete( &mi->mi_conninfo.lai_tree, ( caddr_t )&mc_curr, meta_back_conn_cmp ) ) != NULL )
	{
		assert( !LDAP_BACK_PCONN_ISPRIV( mc ) );
		Debug( LDAP_DEBUG_TRACE,
			"=>meta_back_conn_destroy: destroying conn %lu "
			"refcnt=%d flags=0x%08x\n",
			mc->mc_conn->c_connid, mc->mc_refcnt, mc->msc_mscflags );

		if ( mc->mc_refcnt > 0 ) {
			/* someone else might be accessing the connection;
			 * mark for deletion */
			LDAP_BACK_CONN_CACHED_CLEAR( mc );
			LDAP_BACK_CONN_TAINTED_SET( mc );

		} else {
			meta_back_conn_free( mc );
		}
	}
#if META_BACK_PRINT_CONNTREE > 0
	meta_back_print_conntree( mi, "<<< meta_back_conn_destroy" );
#endif /* META_BACK_PRINT_CONNTREE */
	ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );

	/*
	 * Cleanup rewrite session
	 */
	for ( i = 0; i < mi->mi_ntargets; ++i ) {
		rewrite_session_delete( mi->mi_targets[ i ]->mt_rwmap.rwm_rw, conn );
	}

	return 0;
}

