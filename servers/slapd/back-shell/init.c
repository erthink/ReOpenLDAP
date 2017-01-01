/* $ReOpenLDAP$ */
/* Copyright 1990-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"

#include "slapconfig.h"

#include "shell.h"

int
shell_back_initialize(
    BackendInfo	*bi
)
{
	bi->bi_open = 0;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = shell_back_db_init;
	bi->bi_db_config = 0;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = shell_back_db_destroy;

	bi->bi_op_bind = shell_back_bind;
	bi->bi_op_unbind = shell_back_unbind;
	bi->bi_op_search = shell_back_search;
	bi->bi_op_compare = shell_back_compare;
	bi->bi_op_modify = shell_back_modify;
	bi->bi_op_modrdn = shell_back_modrdn;
	bi->bi_op_add = shell_back_add;
	bi->bi_op_delete = shell_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return shell_back_init_cf( bi );
}

int
shell_back_db_init(
	Backend	*be,
	ConfigReply *cr
)
{
	struct shellinfo	*si;

	si = (struct shellinfo *) ch_calloc( 1, sizeof(struct shellinfo) );

	be->be_private = si;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return si == NULL;
}

int
shell_back_db_destroy(
	Backend	*be,
	ConfigReply *cr
)
{
	free( be->be_private );
	return 0;
}

#if SLAPD_SHELL == SLAPD_MOD_DYNAMIC
SLAP_BACKEND_INIT_MODULE( shell )
#endif /* SLAPD_SHELL == SLAPD_MOD_DYNAMIC */
