/* $ReOpenLDAP$ */
/* Copyright 2000-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "proto-dnssrv.h"

int
dnssrv_back_compare(
	Operation	*op,
	SlapReply	*rs
)
{
#if 0
	assert( get_manageDSAit( op ) );
#endif
	send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		"operation not supported within naming context (dns)" );

	/* not implemented */
	return 1;
}
