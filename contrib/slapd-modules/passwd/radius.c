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

#include "reldap.h"
#include <stdio.h>
#include <lber.h>
#include <lber_pvt.h>	/* BER_BVC definition */
#include "lutil.h"
#include <ldap_pvt_thread.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include <radlib.h>

extern char *global_host;	/* from slapd */
static LUTIL_PASSWD_CHK_FUNC chk_radius;
static const struct berval scheme = BER_BVC("{RADIUS}");
static char *config_filename;
static ldap_pvt_thread_mutex_t libradius_mutex;

static int
chk_radius(
	const struct berval	*sc,
	const struct berval	*passwd,
	const struct berval	*cred,
	const char		**text )
{
	unsigned int		i;
	int			rc = LUTIL_PASSWD_ERR;

	struct rad_handle	*h = NULL;

	for ( i = 0; i < cred->bv_len; i++ ) {
		if ( cred->bv_val[ i ] == '\0' ) {
			return LUTIL_PASSWD_ERR;	/* NUL character in cred */
		}
	}

	if ( cred->bv_val[ i ] != '\0' ) {
		return LUTIL_PASSWD_ERR;	/* cred must behave like a string */
	}

	for ( i = 0; i < passwd->bv_len; i++ ) {
		if ( passwd->bv_val[ i ] == '\0' ) {
			return LUTIL_PASSWD_ERR;	/* NUL character in password */
		}
	}

	if ( passwd->bv_val[ i ] != '\0' ) {
		return LUTIL_PASSWD_ERR;	/* passwd must behave like a string */
	}

	ldap_pvt_thread_mutex_lock( &libradius_mutex );

	h = rad_auth_open();
	if ( h == NULL ) {
		ldap_pvt_thread_mutex_unlock( &libradius_mutex );
		return LUTIL_PASSWD_ERR;
	}

	if ( rad_config( h, config_filename ) != 0 ) {
		goto done;
	}

	if ( rad_create_request( h, RAD_ACCESS_REQUEST ) ) {
		goto done;
	}

	if ( rad_put_string( h, RAD_USER_NAME, passwd->bv_val ) != 0 ) {
		goto done;
	}

	if ( rad_put_string( h, RAD_USER_PASSWORD, cred->bv_val ) != 0 ) {
		goto done;
	}

	if ( rad_put_string( h, RAD_NAS_IDENTIFIER, global_host ) != 0 ) {
		goto done;
	}

	switch ( rad_send_request( h ) ) {
	case RAD_ACCESS_ACCEPT:
		rc = LUTIL_PASSWD_OK;
		break;

	case RAD_ACCESS_REJECT:
		rc = LUTIL_PASSWD_ERR;
		break;

	case RAD_ACCESS_CHALLENGE:
		rc = LUTIL_PASSWD_ERR;
		break;

	case -1:
		/* no valid response is received */
		break;
	}

done:;
	rad_close( h );

	ldap_pvt_thread_mutex_unlock( &libradius_mutex );
	return rc;
}

SLAP_OVERLAY_ENTRY(pw_radius, modterm)
{
	return ldap_pvt_thread_mutex_destroy( &libradius_mutex );
}

SLAP_OVERLAY_ENTRY(pw_radius, modinit) (int argc, char *argv[])
{
	int	i;

	for ( i = 0; i < argc; i++ ) {
		if ( strncasecmp( argv[ i ], "config=", STRLENOF( "config=" ) ) == 0 ) {
			/* FIXME: what if multiple loads of same module?
			 * does it make sense (e.g. override an existing one)? */
			if ( config_filename == NULL ) {
				config_filename = ber_strdup( &argv[ i ][ STRLENOF( "config=" ) ] );
			}

		} else {
			fprintf( stderr, "init_module(radius): unknown arg#%d=\"%s\".\n",
				i, argv[ i ] );
			return 1;
		}
	}

	ldap_pvt_thread_mutex_init( &libradius_mutex );

	return lutil_passwd_add( (struct berval *)&scheme, chk_radius, NULL );
}
