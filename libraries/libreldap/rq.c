/* $ReOpenLDAP$ */
/* Copyright 1990-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
/* This work was initially developed by Jong Hyuk Choi for inclusion
 * in OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/stdarg.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"
#include "ldap_pvt_thread.h"
#include "ldap_queue.h"
#include "ldap_rq.h"

struct re_s *
ldap_pvt_runqueue_insert_ns(
	struct runqueue_s* rq,
	slap_time_t interval,
	ldap_pvt_thread_start_t *routine,
	void *arg,
	char *tname,
	char *tspec
)
{
	struct re_s* entry;

	entry = (struct re_s *) LDAP_CALLOC( 1, sizeof( struct re_s ));
	if ( entry ) {
		entry->interval = interval;
		entry->next_sched = ldap_now_steady();
		entry->routine = routine;
		entry->arg = arg;
		entry->tname = tname;
		entry->tspec = tspec;
		LDAP_STAILQ_INSERT_HEAD( &rq->task_list, entry, tnext );
	}
	return entry;
}

struct re_s *
ldap_pvt_runqueue_insert(
	struct runqueue_s* rq,
	int interval_seconds,
	ldap_pvt_thread_start_t *routine,
	void *arg,
	char *tname,
	char *tspec
)
{
	return ldap_pvt_runqueue_insert_ns(
				rq, ldap_from_seconds(interval_seconds),
				routine, arg, tname, tspec );
}

struct re_s *
ldap_pvt_runqueue_find(
	struct runqueue_s *rq,
	ldap_pvt_thread_start_t *routine,
	void *arg
)
{
	struct re_s* e;

	LDAP_STAILQ_FOREACH( e, &rq->task_list, tnext ) {
		if ( e->routine == routine && e->arg == arg )
			return e;
	}
	return NULL;
}

void
ldap_pvt_runqueue_remove(
	struct runqueue_s* rq,
	struct re_s* entry
)
{
	struct re_s* e;

	LDAP_STAILQ_FOREACH( e, &rq->task_list, tnext ) {
		if ( e == entry)
			break;
	}

	LDAP_ENSURE ( e == entry );

	LDAP_STAILQ_REMOVE( &rq->task_list, entry, re_s, tnext );

	LDAP_FREE( entry );
}

struct re_s*
ldap_pvt_runqueue_next_sched(
	struct runqueue_s* rq,
	slap_time_t* next_run
)
{
	struct re_s* entry;

	entry = LDAP_STAILQ_FIRST( &rq->task_list );
	if ( entry == NULL || entry->next_sched.ns == 0 ) {
		return NULL;
	} else {
		*next_run = entry->next_sched;
		return entry;
	}
}

void
ldap_pvt_runqueue_runtask(
	struct runqueue_s* rq,
	struct re_s* entry
)
{
	LDAP_STAILQ_INSERT_TAIL( &rq->run_list, entry, rnext );
}

void
ldap_pvt_runqueue_stoptask(
	struct runqueue_s* rq,
	struct re_s* entry
)
{
	LDAP_STAILQ_REMOVE( &rq->run_list, entry, re_s, rnext );
}

int
ldap_pvt_runqueue_isrunning(
	struct runqueue_s* rq,
	struct re_s* entry
)
{
	struct re_s* e;

	LDAP_STAILQ_FOREACH( e, &rq->run_list, rnext ) {
		if ( e == entry ) {
			return 1;
		}
	}
	return 0;
}

void
ldap_pvt_runqueue_resched(
	struct runqueue_s* rq,
	struct re_s* entry,
	int defer
)
{
	struct re_s* prev;
	struct re_s* e;

	LDAP_STAILQ_FOREACH( e, &rq->task_list, tnext ) {
		if ( e == entry )
			break;
	}

	LDAP_ENSURE ( e == entry );

	LDAP_STAILQ_REMOVE( &rq->task_list, entry, re_s, tnext );

	entry->next_sched.ns = 0;
	if ( !defer )
		entry->next_sched.ns = ldap_now_steady_ns() + entry->interval.ns;

	if ( LDAP_STAILQ_EMPTY( &rq->task_list )) {
		LDAP_STAILQ_INSERT_HEAD( &rq->task_list, entry, tnext );
	} else if ( entry->next_sched.ns == 0 ) {
		LDAP_STAILQ_INSERT_TAIL( &rq->task_list, entry, tnext );
	} else {
		prev = NULL;
		LDAP_STAILQ_FOREACH( e, &rq->task_list, tnext ) {
			if ( e->next_sched.ns == 0 ) {
				if ( prev == NULL ) {
					LDAP_STAILQ_INSERT_HEAD( &rq->task_list, entry, tnext );
				} else {
					LDAP_STAILQ_INSERT_AFTER( &rq->task_list, prev, entry, tnext );
				}
				return;
			} else if ( e->next_sched.ns > entry->next_sched.ns ) {
				if ( prev == NULL ) {
					LDAP_STAILQ_INSERT_HEAD( &rq->task_list, entry, tnext );
				} else {
					LDAP_STAILQ_INSERT_AFTER( &rq->task_list, prev, entry, tnext );
				}
				return;
			}
			prev = e;
		}
		LDAP_STAILQ_INSERT_TAIL( &rq->task_list, entry, tnext );
	}
}

int
ldap_pvt_runqueue_persistent_backload(
	struct runqueue_s* rq
)
{
	struct re_s* e;
	int count = 0;

	ldap_pvt_thread_mutex_lock( &rq->rq_mutex );
	if ( !LDAP_STAILQ_EMPTY( &rq->task_list )) {
		LDAP_STAILQ_FOREACH( e, &rq->task_list, tnext ) {
			if ( e->next_sched.ns == 0 )
				count++;
		}
	}
	ldap_pvt_thread_mutex_unlock( &rq->rq_mutex );
	return count;
}
