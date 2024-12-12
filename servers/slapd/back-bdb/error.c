/* $ReOpenLDAP$ */
/* Copyright 2000-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "back-bdb.h"

#if DB_VERSION_FULL < 0x04030000
void bdb_errcall(const char *pfx, char *msg)
#else
void bdb_errcall(const DB_ENV *env, const char *pfx, const char *msg)
#endif
{
  Debug(LDAP_DEBUG_ANY, "bdb(%s): %s\n", pfx, msg);
}

#if DB_VERSION_FULL >= 0x04030000
void bdb_msgcall(const DB_ENV *env, const char *msg) { Debug(LDAP_DEBUG_TRACE, "bdb: %s\n", msg); }
#endif
