/*
    Copyright (c) 2015 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2015 Peter-Service R&D LLC.

    This file is part of ReOpenLDAP.

    ReOpenLDAP is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ReOpenLDAP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "portable.h"

#include "slap.h"
#include "proto-slap.h"

#include "lutil.h"
#include "config.h"

struct quorum_item {
	int sid;
	int ready;
};

static ldap_pvt_thread_mutex_t quorum_mutex;
static struct quorum_item* quorum_list;
static volatile int quorum_state = -1;

void quorum_init() {
	int rc;

	assert(quorum_state == -1);
	rc = ldap_pvt_thread_mutex_init(&quorum_mutex);
	if (rc) {
		Debug( LDAP_DEBUG_ANY, "mutex_init failed for quorum_mutex, rc = %d\n", rc );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}
	quorum_state = 1;
}

void quorum_destroy() {
	int rc;

	if (quorum_state > -1) {
		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		quorum_state = -1;
		ch_free( quorum_list );
		quorum_list = NULL;

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
		ldap_pvt_thread_mutex_destroy(&quorum_mutex);
	}
}

int quorum_query(BackendDB *bd) {
	return SLAP_MULTIMASTER( bd ) ? quorum_state > 0 : 1;
}

static void quorum_update() {
	unsigned total, ready, quorum;
	struct quorum_item* p;

	assert(quorum_state > -1);
	total = ready = 0;
	if (quorum_list) {
		for (p = quorum_list; p->sid; ++p) {
			++total;
			if (p->ready || p->sid == slap_serverID)
				++ready;
		}
	}

	quorum = (total + 2 - (total & 1)) / 2;
	quorum_state = (ready < quorum) ? 0 : 1;
	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s (total %d, quorum %d, ready %d)\n",
		   quorum_state ? "HAVE" : "LACK", total, quorum, ready );
}

void __quorum_notify(int rid, int sid, int status, const char *from, int line) {
	struct quorum_item* p;
	int rc;

	assert(quorum_state > -1);
	assert(sid < 0 || sid <= SLAP_SYNC_SID_MAX);
	assert(rid < 0 || rid <= SLAP_SYNC_RID_MAX);

	if (status && sid > -1 && quorum_list) {
		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		status = (status > 0) ? 1 : 0;
		for (p = quorum_list; p && p->sid; ++p) {
			if (p->sid == sid) {
				if (p->ready != status) {
					Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s-%d-notify sid %d, %s\n",
						   from, line,
						   sid, status ? "ready" : "loser" );
					p->ready = status;
					quorum_update();
				}
				break;
			}
		}

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
	}
}

static int quorum_add(int sid) {
	int n, rc;
	struct quorum_item* p;

	assert(sid > 0 && sid <= SLAP_SYNC_SID_MAX);
	rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
	assert(rc == 0);

	n = 0;
	for (p = quorum_list; p && p->sid; ++p) {
		if (p->sid == sid)
			goto done;
		++n;
	}

	p = ch_realloc(quorum_list, (n + 2) * sizeof(*p));
	if (! p) {
		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
		return LDAP_NO_MEMORY;
	}

	p[n].sid = sid;
	p[n].ready = 0;
	p[n+1].sid = 0;
	quorum_list = p;
	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %d added to sid-list as #%d\n",
		   sid, n );

done:;
	quorum_update();
	rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
	assert(rc == 0);
	return 0;
}

int quorum_config(ConfigArgs *c) {
	int i, rc;

	assert(quorum_state >= 0);
	if (c->op == SLAP_CONFIG_EMIT) {
		struct quorum_item* p;
		for (p = quorum_list; p && p->sid; ++p) {
			char buf[16];
			struct berval sid;

			sid.bv_val = buf;
			sid.bv_len = snprintf(buf, sizeof(buf), "%d", p->sid );
			if (value_add_one( &c->rvalue_vals, &sid ))
				return 1;
		}
		return 0;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		ch_free( quorum_list );
		quorum_list = NULL;

		Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: sid-list cleared\n" );

		quorum_update();
		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
		return 0;
	}

	for( i=1; i < c->argc; i++ ) {
		int	sid;

		if (( lutil_atoi( &sid, c->argv[i] ) &&
			lutil_atoix( &sid, c->argv[i], 16 )) ||
			sid < 1 || sid > SLAP_SYNC_SID_MAX )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> illegal server ID", c->argv[0] );
			Debug(LDAP_DEBUG_ANY, "%s: %s %s\n",
				c->log, c->cr_msg, c->argv[1] );
			return 1;
		}

		if (quorum_add( sid ))
			return 1;
	}
	return 0;
}
