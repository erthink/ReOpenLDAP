/* trans.c - bdb backend transaction routines */
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

#include "back-bdb.h"
#include "lber_pvt.h"
#include "lutil.h"


/* Congestion avoidance code
 * for Deadlock Rollback
 */

void
bdb_trans_backoff( int num_retries )
{
	int i;
	int delay = 0;
	int pow_retries = 1;
	unsigned long key = 0;
	unsigned long max_key = -1;
	struct timeval timeout;

	lutil_entropy( (unsigned char *) &key, sizeof( unsigned long ));

	for ( i = 0; i < num_retries; i++ ) {
		if ( i >= 5 ) break;
		pow_retries *= 4;
	}

	delay = 16384 * (key * (double) pow_retries / (double) max_key);
	delay = delay ? delay : 1;

	Debug( LDAP_DEBUG_TRACE,  "delay = %d, num_retries = %d\n", delay, num_retries );

	timeout.tv_sec = delay / 1000000;
	timeout.tv_usec = delay % 1000000;
	select( 0, NULL, NULL, NULL, &timeout );
}
