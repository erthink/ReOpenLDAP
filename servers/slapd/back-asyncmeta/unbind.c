/* unbind.c - unbind handler for back-asyncmeta */
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
 * Copyright 2016 The OpenLDAP Foundation.
 * Portions Copyright 2016 Symas Corporation.
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
