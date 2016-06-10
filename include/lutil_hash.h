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
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _LUTIL_HASH_H_
#define _LUTIL_HASH_H_

#include <lber_types.h>

LDAP_BEGIN_DECL

#define LUTIL_HASH_BYTES 4

#ifdef HAVE_LONG_LONG

typedef union lutil_HASHContext {
	ber_uint_t hash;
	unsigned long long hash64;
} lutil_HASH_CTX;

#else /* !HAVE_LONG_LONG */

typedef struct lutil_HASHContext {
	ber_uint_t hash;
} lutil_HASH_CTX;

#endif /* HAVE_LONG_LONG */

LDAP_LUTIL_F( void )
lutil_HASHInit LDAP_P((
	lutil_HASH_CTX *context));

LDAP_LUTIL_F( void )
lutil_HASHUpdate LDAP_P((
	lutil_HASH_CTX *context,
	unsigned char const *buf,
	ber_len_t len));

LDAP_LUTIL_F( void )
lutil_HASHFinal LDAP_P((
	unsigned char digest[LUTIL_HASH_BYTES],
	lutil_HASH_CTX *context));

#ifdef HAVE_LONG_LONG

#define LUTIL_HASH64_BYTES	8

LDAP_LUTIL_F( void )
lutil_HASH64Init LDAP_P((
	lutil_HASH_CTX *context));

LDAP_LUTIL_F( void )
lutil_HASH64Update LDAP_P((
	lutil_HASH_CTX *context,
	unsigned char const *buf,
	ber_len_t len));

LDAP_LUTIL_F( void )
lutil_HASH64Final LDAP_P((
	unsigned char digest[LUTIL_HASH64_BYTES],
	lutil_HASH_CTX *context));

#endif /* HAVE_LONG_LONG */

LDAP_END_DECL

#endif /* _LUTIL_HASH_H_ */
