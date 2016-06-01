/* sock.h - socket backend header file */
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

#ifndef SLAPD_SOCK_H
#define SLAPD_SOCK_H

#include "proto-sock.h"

LDAP_BEGIN_DECL

struct sockinfo {
	const char	*si_sockpath;
	slap_mask_t	si_extensions;
	slap_mask_t	si_ops;		/* overlay: operations to act on */
	slap_mask_t	si_resps;	/* overlay: responses to forward */
};

#define	SOCK_EXT_BINDDN	1
#define	SOCK_EXT_PEERNAME	2
#define	SOCK_EXT_SSF		4
#define	SOCK_EXT_CONNID		8

extern FILE *opensock LDAP_P((
	const char *sockpath));

extern void sock_print_suffixes LDAP_P((
	FILE *fp,
	BackendDB *bd));

extern void sock_print_conn LDAP_P((
	FILE *fp,
	Connection *conn,
	struct sockinfo *si));

extern int sock_read_and_send_results LDAP_P((
	Operation *op,
	SlapReply *rs,
	FILE *fp));

LDAP_END_DECL

#endif
