/* OpenLDAP WiredTiger backend */
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
 * Copyright 2002-2014 The OpenLDAP Foundation.
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
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#ifndef _BACK_WT_H_
#define _BACK_WT_H_

#include <portable.h>

#include <ac/errno.h>
#include <sys/stat.h>

#include "slap.h"
#include "wiredtiger.h"

/* The default search IDL stack cache depth */
#define DEFAULT_SEARCH_STACK_DEPTH  16

struct wt_info {
	WT_CONNECTION *wi_conn;
	char *wi_dbenv_home;
	char *wi_dbenv_config;
	ID	wi_lastid;

	slap_mask_t wi_defaultmask;
	int         wi_nattrs;
	struct wt_attrinfo **wi_attrs;
	void *wi_search_stack;
	int wi_search_stack_depth;

	struct re_s *wi_index_task;

	int wi_flags;
#define WT_IS_OPEN      0x01
#define WT_OPEN_INDEX   0x02
#define WT_DEL_INDEX    0x08
#define WT_RE_OPEN      0x10
#define WT_NEED_UPGRADE 0x20
};

#define WT_TABLE_ID2ENTRY "table:id2entry"
#define WT_TABLE_DN2ID "table:dn2id"

#define WT_INDEX_DN "index:id2entry:dn"
#define WT_INDEX_PID "index:dn2id:pid"
#define WT_INDEX_REVDN "index:dn2id:revdn"

#define ITEMzero(item) (memset((item), 0, sizeof(WT_ITEM)))
#define ITEM2bv(item,bv) ((bv)->bv_val = (item)->data, \
						  (bv)->bv_len = (item)->size)
#define bv2ITEM(bv,item) ((item)->data = (bv)->bv_val, \
						 (item)->size = (bv)->bv_len )

typedef struct {
	WT_SESSION *session;
} wt_ctx;

/* for the cache of attribute information (which are indexed, etc.) */
typedef struct wt_attrinfo {
	AttributeDescription *ai_desc; /* attribute description cn;lang-en */
	slap_mask_t ai_indexmask;   /* how the attr is indexed  */
	slap_mask_t ai_newmask; /* new settings to replace old mask */
	#ifdef LDAP_COMP_MATCH
	ComponentReference* ai_cr; /*component indexing*/
	#endif
} AttrInfo;

/* These flags must not clash with SLAP_INDEX flags or ops in slap.h! */
#define	WT_INDEX_DELETING	0x8000U	/* index is being modified */
#define	WT_INDEX_UPDATE_OP	0x03	/* performing an index update */

#include "proto-wt.h"

#endif /* _BACK_WT_H_ */

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
