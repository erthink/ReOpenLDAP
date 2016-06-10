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

#ifndef LDAP_RQ_H
#define LDAP_RQ_H 1

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

typedef struct re_s {
	slap_time_t next_sched;
	slap_time_t interval;
	LDAP_STAILQ_ENTRY(re_s) tnext; /* it includes running */
	LDAP_STAILQ_ENTRY(re_s) rnext;
	ldap_pvt_thread_start_t *routine;
	void *arg;
	char *tname;
	char *tspec;
} re_t;

typedef struct runqueue_s {
	LDAP_STAILQ_HEAD(l, re_s) task_list;
	LDAP_STAILQ_HEAD(rl, re_s) run_list;
	ldap_pvt_thread_mutex_t	rq_mutex;
} runqueue_t;

LDAP_F( struct re_s* )
ldap_pvt_runqueue_insert_ns(
	struct runqueue_s* rq,
	slap_time_t interval,
	ldap_pvt_thread_start_t* routine,
	void *arg,
	char *tname,
	char *tspec
);

LDAP_F( struct re_s* )
ldap_pvt_runqueue_insert(struct runqueue_s* rq,
	int interval_seconds,
	ldap_pvt_thread_start_t* routine,
	void *arg,
	char *tname,
	char *tspec
);

LDAP_F( struct re_s* )
ldap_pvt_runqueue_find(
	struct runqueue_s* rq,
	ldap_pvt_thread_start_t* routine,
	void *arg
);

LDAP_F( void )
ldap_pvt_runqueue_remove(
	struct runqueue_s* rq,
	struct re_s* entry
);

LDAP_F( struct re_s* )
ldap_pvt_runqueue_next_sched(
	struct runqueue_s* rq,
	slap_time_t* next_run
);

LDAP_F( void )
ldap_pvt_runqueue_runtask(
	struct runqueue_s* rq,
	struct re_s* entry
);

LDAP_F( void )
ldap_pvt_runqueue_stoptask(
	struct runqueue_s* rq,
	struct re_s* entry
);

LDAP_F( int )
ldap_pvt_runqueue_isrunning(
	struct runqueue_s* rq,
	struct re_s* entry
);

LDAP_F( void )
ldap_pvt_runqueue_resched(
	struct runqueue_s* rq,
	struct re_s* entry,
	int defer
);

LDAP_F( int )
ldap_pvt_runqueue_persistent_backload(
	struct runqueue_s* rq
);

LDAP_END_DECL

#endif
