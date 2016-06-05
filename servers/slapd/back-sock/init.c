/* init.c - initialize sock backend */
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
 * Copyright 2007-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Brian Candler for inclusion
 * in OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"
#include "back-sock.h"

int
sock_back_initialize(
    BackendInfo	*bi
)
{
	bi->bi_open = 0;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = sock_back_db_init;
	bi->bi_db_config = 0;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = sock_back_db_destroy;

	bi->bi_op_bind = sock_back_bind;
	bi->bi_op_unbind = sock_back_unbind;
	bi->bi_op_search = sock_back_search;
	bi->bi_op_compare = sock_back_compare;
	bi->bi_op_modify = sock_back_modify;
	bi->bi_op_modrdn = sock_back_modrdn;
	bi->bi_op_add = sock_back_add;
	bi->bi_op_delete = sock_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return sock_back_init_cf( bi );
}

int
sock_back_db_init(
    Backend	*be,
	struct config_reply_s *cr
)
{
	struct sockinfo	*si;

	si = (struct sockinfo *) ch_calloc( 1, sizeof(struct sockinfo) );

	be->be_private = si;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return si == NULL;
}

int
sock_back_db_destroy(
    Backend	*be,
	struct config_reply_s *cr
)
{
	free( be->be_private );
	return 0;
}

#if SLAPD_SOCK == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( sock )

#endif /* SLAPD_SOCK == SLAPD_MOD_DYNAMIC */
