/* error.c - BDB errcall routine */
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

#include "slap.h"
#include "back-bdb.h"

#if DB_VERSION_FULL < 0x04030000
void bdb_errcall( const char *pfx, char * msg )
#else
void bdb_errcall( const DB_ENV *env, const char *pfx, const char * msg )
#endif
{
	Debug( LDAP_DEBUG_ANY, "bdb(%s): %s\n", pfx, msg );
}

#if DB_VERSION_FULL >= 0x04030000
void bdb_msgcall( const DB_ENV *env, const char *msg )
{
	Debug( LDAP_DEBUG_TRACE, "bdb: %s\n", msg );
}
#endif
