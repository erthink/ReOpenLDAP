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
 * Copyright 1998-2014 The OpenLDAP Foundation.
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
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lutil.h>

/* Return a pair of socket descriptors that are connected to each other.
 * The returned descriptors are suitable for use with select(). The two
 * descriptors may or may not be identical; the function may return
 * the same descriptor number in both slots. It is guaranteed that
 * data written on sds[1] will be readable on sds[0]. The returned
 * descriptors may be datagram oriented, so data should be written
 * in reasonably small pieces and read all at once. On Unix systems
 * this function is best implemented using a single pipe() call.
 */

int lutil_pair( ber_socket_t sds[2] )
{
#ifdef USE_PIPE
	return pipe( sds );
#else
	struct sockaddr_in si;
	int rc;
	ber_socklen_t len = sizeof(si);
	ber_socket_t sd;

	sd = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sd == AC_SOCKET_INVALID ) {
		return sd;
	}

	(void) memset( (void*) &si, '\0', len );
	si.sin_family = AF_INET;
	si.sin_port = 0;
	si.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

	rc = bind( sd, (struct sockaddr *)&si, len );
	if ( rc == AC_SOCKET_ERROR ) {
		tcp_close(sd);
		return rc;
	}

	rc = getsockname( sd, (struct sockaddr *)&si, &len );
	if ( rc == AC_SOCKET_ERROR ) {
		tcp_close(sd);
		return rc;
	}

	rc = connect( sd, (struct sockaddr *)&si, len );
	if ( rc == AC_SOCKET_ERROR ) {
		tcp_close(sd);
		return rc;
	}

	sds[0] = sd;
	sds[1] = dup( sds[0] );
	return 0;
#endif
}
