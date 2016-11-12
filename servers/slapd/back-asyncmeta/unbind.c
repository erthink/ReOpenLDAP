/* $ReOpenLDAP$ */
/* Copyright 2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was developed by Symas Corporation
 * based on back-meta module for inclusion in OpenLDAP Software.
 * This work was sponsored by Ericsson. */

#include "reldap.h"

#include <stdio.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

int
asyncmeta_back_conn_destroy(
	Backend		*be,
	Connection	*conn )
{
	a_metainfo_t	*mi = ( a_metainfo_t * )be->be_private;
	int		i;

	Debug( LDAP_DEBUG_TRACE,
		"=>asyncmeta_back_conn_destroy: fetching conn=%ld DN=\"%s\"\n",
		conn->c_connid,
		BER_BVISNULL( &conn->c_ndn ) ? "" : conn->c_ndn.bv_val );
	/*
	 * Cleanup rewrite session
	 */
	for ( i = 0; i < mi->mi_ntargets; ++i ) {
		rewrite_session_delete( mi->mi_targets[ i ]->mt_rwmap.rwm_rw, conn );
	}

	return 0;
}
