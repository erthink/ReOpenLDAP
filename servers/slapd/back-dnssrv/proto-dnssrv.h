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
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */

#ifndef PROTO_DNSSRV_H
#define PROTO_DNSSRV_H

LDAP_BEGIN_DECL

extern BI_init			dnssrv_back_initialize;

extern BI_open			dnssrv_back_open;
extern BI_close			dnssrv_back_close;
extern BI_destroy		dnssrv_back_destroy;

extern BI_db_init		dnssrv_back_db_init;
extern BI_db_destroy		dnssrv_back_db_destroy;
extern BI_db_config		dnssrv_back_db_config;

extern BI_op_bind		dnssrv_back_bind;
extern BI_op_search		dnssrv_back_search;
extern BI_op_compare		dnssrv_back_compare;

extern BI_chk_referrals		dnssrv_back_referrals;

extern AttributeDescription	*ad_dc;
extern AttributeDescription	*ad_associatedDomain;

LDAP_END_DECL

#endif /* PROTO_DNSSRV_H */
