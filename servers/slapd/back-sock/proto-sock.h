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

#ifndef _PROTO_SOCK_H
#define _PROTO_SOCK_H

LDAP_BEGIN_DECL

extern BI_init		sock_back_initialize;

extern BI_open		sock_back_open;
extern BI_close		sock_back_close;
extern BI_destroy	sock_back_destroy;

extern BI_db_init	sock_back_db_init;
extern BI_db_destroy	sock_back_db_destroy;

extern BI_op_bind	sock_back_bind;
extern BI_op_unbind	sock_back_unbind;
extern BI_op_search	sock_back_search;
extern BI_op_compare	sock_back_compare;
extern BI_op_modify	sock_back_modify;
extern BI_op_modrdn	sock_back_modrdn;
extern BI_op_add	sock_back_add;
extern BI_op_delete	sock_back_delete;

extern int sock_back_init_cf( BackendInfo *bi );

LDAP_END_DECL

#endif /* _PROTO_SOCK_H */
