/* $ReOpenLDAP$ */
/*
    Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2015,2016 Peter-Service R&D LLC.

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

struct present {
	void *key;
	short rid, sid, status;
};

struct requirment {
#	define QR_FLAG_DEMAND 1
#	define QR_FLAG_RID    2
#	define QR_FLAG_AUTO   4

#	define QR_VOTED_SID  0
#	define QR_VOTED_RID  QR_FLAG_RID

#	define QR_DEMAND_RID (QR_VOTED_RID | QR_FLAG_AUTO)
#	define QR_DEMAND_SID (QR_VOTED_SID | QR_FLAG_DEMAND)

#	define QR_IS_RID(type) ((type) & QR_FLAG_RID)
#	define QR_IS_SID(type) (!QR_IS_RID(type))

#	define QR_IS_DEMAND(type) ((type) & QR_FLAG_DEMAND)
#	define QR_IS_AUTO(type) ((type) & QR_FLAG_AUTO)
	short type, id;
};

struct slap_quorum {
	slap_quorum_t* qr_next;
	struct present* qr_present;
	struct requirment* qr_requirements;
#	define qr_cluster qr_bd->be_nsuffix->bv_val
	BackendDB *qr_bd;
#	define QR_AUTO_RIDS	1
#	define QR_AUTO_SIDS	2
#	define QR_ALL_LINKS	4
	int flags;

#define QR_SYNCREPL_MAX 15
	int qr_syncrepl_limit, qt_running;
	int qt_status[QS_MAX], qt_left;
	uint32_t salt;
	char cns_status_buf[ LDAP_PVT_CSNSTR_BUFSIZE ];
};

static ldap_pvt_thread_mutex_t quorum_mutex;
#define QR_POISON ((void*) (intptr_t) -1)
slap_quorum_t *quorum_list = QR_POISON;

void quorum_global_init() {
	int rc;

	assert(quorum_list == QR_POISON);
	rc = ldap_pvt_thread_mutex_init(&quorum_mutex);
	if (rc) {
		Debug( LDAP_DEBUG_ANY, "mutex_init failed for quorum_mutex, rc = %d\n", rc );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}
	quorum_list = NULL;
}

static void lock() {
	ldap_pvt_thread_mutex_lock(&quorum_mutex);
}

static void unlock() {
	ldap_pvt_thread_mutex_unlock(&quorum_mutex);
}

void quorum_global_destroy() {
	if (quorum_list != QR_POISON) {
		lock();
		while (quorum_list) {
			slap_quorum_t *next = quorum_list->qr_next;
			ch_free(quorum_list->qr_present);
			ch_free(quorum_list->qr_requirements);
			ch_free(quorum_list);
			quorum_list = next;
		}
		quorum_list = QR_POISON;
		unlock();
		ldap_pvt_thread_mutex_destroy(&quorum_mutex);
	}
}

static ATTRIBUTE_NO_SANITIZE_THREAD_INLINE
void set_cache(BackendDB *bd, int value) {
	assert(bd->bd_self == bd);
	bd->bd_quorum_cache = value;
}

static ATTRIBUTE_NO_SANITIZE_THREAD_INLINE
int get_cache(BackendDB *bd) {
	assert(bd->bd_self == bd);
	return bd->bd_quorum_cache;
}

static void quorum_be_init(BackendDB *bd) {
	slap_quorum_t *q;

	assert(quorum_list != QR_POISON);
	assert(bd->bd_self == bd);
	q = ch_calloc(1, sizeof(slap_quorum_t));

	q->qr_bd = bd;
	q->qr_next = quorum_list;
	bd->bd_quorum = q;
	set_cache(bd, 1);
	q->qt_left = -1;
	quorum_list = q;
}

void quorum_be_destroy(BackendDB *bd) {
	assert(quorum_list != QR_POISON);
	assert(bd->bd_self == bd);

	if (bd->bd_quorum) {
		slap_quorum_t *q, **pq;

		lock();
		q = bd->bd_quorum;
		if (q) {
			set_cache(bd, 1);
			bd->bd_quorum = NULL;

			for(pq = &quorum_list; *pq; pq = &((*pq)->qr_next))
				if (*pq == q) {
					*pq = q->qr_next;
					break;
				}
		}
		unlock();

		if (q) {
			ch_free(q->qr_present);
			ch_free(q->qr_requirements);
			ch_free(q);
		}
	}
}

static int quorum(unsigned total, unsigned wanna, unsigned ready) {
	if (wanna > total)
		wanna = total;
	return (wanna == 0 || ready < (wanna + 2 - (wanna & 1)) / 2) ? 0 : 1;
}

static int lazy_update(slap_quorum_t *q) {
	unsigned total_links, ready_links;
	unsigned ready_rids, ready_sids, wanna_rids, wanna_sids;
	struct present* p;
	struct requirment* r;
	int state = 1;

	ready_rids = ready_sids = 0;
	wanna_rids = wanna_sids = 0;
	total_links = ready_links = 0;
	if (q->qr_present) {
		for(p = q->qr_present; p->key != NULL; ++p) {
			++total_links;
			ready_links += (p->status >= QS_READY);
		}
	}

	if ((q->flags & QR_ALL_LINKS) && ready_links < total_links) {
		Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s FORCE-LACK"
				" (all-links, not ready)\n",
			   q->qr_cluster );
		state = -1;
	}

	if (q->qr_requirements) {
		for(r = q->qr_requirements; r->type > -1; ++r) {
			if (QR_IS_SID(r->type)) {
				if (r->id == slap_serverID)
					continue;
				++wanna_sids;
				for(p = q->qr_present; p && p->key != NULL; ++p) {
					if (p->sid == r->id) {
						ready_sids += (p->status >= QS_READY);
						break;
					}
				}
			} else {
				++wanna_rids;
				for(p = q->qr_present; p && p->key != NULL; ++p) {
					if (p->rid == r->id) {
						ready_rids += (p->status >= QS_READY);
						break;
					}
				}
			}
			if (QR_IS_DEMAND(r->type) && (!p || p->status < QS_READY)) {
				Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s FORCE-LACK"
						" (required %s%s %d not ready)\n",
					   q->qr_cluster,
					   QR_IS_AUTO(r->type) ? "auto-" : "",
					   QR_IS_SID(r->type) ? "sid" : "rid",
					   r->id );
				state = -1;
			}
		}

		if (state > 0 && (wanna_sids || wanna_rids))
			state = (quorum(total_links, wanna_sids, ready_sids)
					|| quorum(total_links, wanna_rids, ready_rids)) ? 1 : -1;
	}

	if ((q->flags & QR_AUTO_SIDS) && state > 0 && total_links
	&& ! quorum (total_links, total_links, wanna_sids)) {
		Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s FORCE-LACK"
				" (auto-sids, not enough for voting)\n",
			   q->qr_cluster );
		state = -1;
	}

	Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s %s (wanna/ready: "
							"link %d/%d, rid %d/%d, sid %d/%d)\n",
		   q->qr_cluster, (state > 0) ? "HAVE" : "LACK",
		   total_links, ready_links, wanna_rids, ready_rids, wanna_sids, ready_sids );
	return state;
}

static void quorum_invalidate(BackendDB *bd) {
	set_cache(bd, 0);
}

void quorum_notify_self_sid() {
	slap_quorum_t *q;
	assert(quorum_list != QR_POISON);

	lock();
	for(q = quorum_list; q; q = q->qr_next)
		quorum_invalidate(q->qr_bd);
	unlock();
}

int quorum_query(BackendDB *bd) {
	bd = bd->bd_self;
	int snap = get_cache(bd);
	if (unlikely(snap == 0)) {
		assert(quorum_list != QR_POISON);

		lock();
		snap = get_cache(bd);
		if (snap == 0) {
			snap = bd->bd_quorum ? lazy_update(bd->bd_quorum) : 1;
			assert(snap != 0);
			set_cache(bd, snap);
		}
		unlock();
	}
	return snap > 0;
}

static int lpq(slap_quorum_t *q, int status)
{
	int r = q->qt_status[status];
	return r > 15 ? 15 : r;
}

static void kick(slap_quorum_t *q)
{
	struct lutil_tm tm;

	ldap_pvt_gettime( &tm );
	snprintf( q->cns_status_buf, sizeof(q->cns_status_buf),
		"%4d%02d%02d%02d%02d%02d.%06dZ#%06x#%03x#%x%x%x.%x%x",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		tm.tm_min, tm.tm_sec, tm.tm_usec,
		q->salt & 0xFFFFFFu, slap_serverID,
		lpq(q, QS_DEAD), lpq(q, QS_DIRTY), lpq(q, QS_REFRESH),
		lpq(q, QS_PROCESS), lpq(q, QS_READY));

	int now = q->qt_status[QS_DEAD] + q->qt_status[QS_DIRTY]
			+ q->qt_status[QS_REFRESH] + q->qt_status[QS_PROCESS];
	if (q->qt_left != now) {
		q->qt_left = now;
		Debug( LDAP_DEBUG_SYNC, "syncrepl_gate: %s %s\n",
			q->qr_cluster, now ? "dirty" : "clean" );
	}
}

int quorum_query_status(BackendDB *bd, int running_only, BerValue *status, Operation *op)
{
	lock();
	bd = bd->bd_self;
	if (! bd->bd_quorum)
		quorum_be_init(bd);

	slap_quorum_t *q = bd->bd_quorum;
	if (! q->cns_status_buf[0])
		kick(q);

	status->bv_val = (op && op->o_tmpmemctx)
		? ber_strdup_x( q->cns_status_buf, op->o_tmpmemctx )
		: q->cns_status_buf;
	status->bv_len = strlen(status->bv_val);

	int left = q->qt_left;
	if (running_only)
		left -= q->qt_status[QS_DEAD];

	unlock();
	return left;
}

static int require_append(slap_quorum_t *q, int type, int id) {
	int n;
	struct requirment* r;

	assert(id > -1);
	if (QR_IS_SID(type))
		assert(id <= SLAP_SYNC_SID_MAX);
	else if (QR_IS_RID(type))
		assert(id <= SLAP_SYNC_RID_MAX);
	else
		assert(0);

	for(n = 0, r = q->qr_requirements; r && r->type > -1; ++r, ++n ) {
		if (r->id == id && QR_IS_SID(r->type) == QR_IS_SID(type)) {
			if (QR_IS_AUTO(r->type) && ! QR_IS_AUTO(type)) {
				r->type = type;
				goto ok;
			}
			return 0;
		}
	}

	q->qr_requirements = ch_realloc(q->qr_requirements,
								(n + 2) * sizeof(struct requirment));
	r = q->qr_requirements + n;
	r->id = id;
	r->type = type;
	r[1].type = -1;

ok:
	Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: added %s%s-%s %d to %s requirements as #%d\n",
		   QR_IS_AUTO(type) ? "auto-" : "", QR_IS_DEMAND(type) ? "demand" : "vote",
		   QR_IS_SID(type) ? "sid" : "rid", id, q->qr_cluster, n );

	return 1;
}

static int require_remove(slap_quorum_t *q, int type, int type_mask, int id) {
	int n;
	struct requirment* r;

	assert(id > -1);
	if (QR_IS_SID(type))
		assert(id <= SLAP_SYNC_SID_MAX);
	else if (QR_IS_RID(type))
		assert(id <= SLAP_SYNC_RID_MAX);
	else
		assert(0);

	for(n = 0, r = q->qr_requirements; r && r->type > -1; ++r, ++n ) {
		if (r->id == id && (r->type & type_mask) == type) {
			Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s%s-%s %d removed from %s requirements as #%d\n",
				   QR_IS_AUTO(r->type) ? "auto-" : "", QR_IS_DEMAND(type) ? "demand" : "vote",
				   QR_IS_SID(r->type) ? "sid" : "rid", id, q->qr_cluster, n );

			for (;;) {
				r[0] = r[1];
				if (r->type < 0)
					break;
				++n; ++r;
			}

			if (! n) {
				ch_free(q->qr_requirements);
				q->qr_requirements = NULL;
			}

			return 1;
		}
	}

	return 0;
}

static int require_auto_rids(slap_quorum_t *q) {
	int changed = 0;

	if (! (q->flags & QR_AUTO_RIDS)) {
		q->flags |= QR_AUTO_RIDS;
		if (q->qr_present) {
			struct present* p;
			for (p = q->qr_present; p->key != NULL; ++p)
				changed |= require_append(q, QR_VOTED_RID | QR_FLAG_AUTO, p->rid);
		}
	}
	return changed;
}

static int require_auto_sids(slap_quorum_t *q) {
	int changed = 0;

	if (! (q->flags & QR_AUTO_SIDS)) {
		q->flags |= QR_AUTO_SIDS;
		if (q->qr_present) {
			struct present* p;
			for (p = q->qr_present; p->key != NULL; ++p)
				if (p->sid > -1)
					changed |= require_append(q, QR_VOTED_SID | QR_FLAG_AUTO, p->sid);
		}
	}
	return changed;
}

static int notify_sid(slap_quorum_t *q, void* key, int sid) {
	struct present* p;
	int n = 0;

	if ( q->qr_present) {
		for(p = q->qr_present; p->key != NULL; ++p, ++n) {
			if (p->key == key) {
				if (p->sid != sid) {
					Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: notify-sid %s, rid %d/%p, sid %d->%d\n",
						   q->qr_cluster, p->rid, key, p->sid, sid );
					if (q->flags & QR_AUTO_SIDS) {
						if (p->sid > -1)
							require_remove(q, QR_VOTED_SID | QR_FLAG_AUTO,
										   ~QR_FLAG_DEMAND, p->sid);
						if (sid > -1)
							require_append(q, QR_VOTED_SID | QR_FLAG_AUTO, sid);
					}
					p->sid = sid;
					return 1;
				}
				return 0;
			}
		}
	}
	/* LY: this may occur when 'syncrepl' removed by config_syncrepl()
	 * while do_syncrepl() is running. */
	return 0;
}

static const char* status2str(int status)
{
	const char* str[QS_MAX] = {
		"dirty", "dead", "refresh", "ready", "process"
	};

	assert(status >= QS_DIRTY && status <= QS_PROCESS);
	return str[status];
}

static int notify_status(slap_quorum_t *q, void* key, int status) {
	int invalidate = 0;

	if ( q->qr_present) {
		struct present* p;
		int n = 0;
		for(p = q->qr_present; p->key != NULL; ++p, ++n) {
			if (p->key == key) {
				if (p->status != status) {
					Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: notify-status %s, rid %d/%p, sid %d, %s->%s\n",
						   q->qr_cluster, p->rid, key, p->sid,
						   status2str(p->status),
						   status2str(status) );
					assert(q->qt_status[p->status] > 0);
					invalidate = (p->status < QS_READY) != (status < QS_READY);
					q->qt_status[p->status] -= 1;
					q->qt_status[p->status = status] += 1;
					kick(q);
				}
				break;
			}
		}
	}
	return invalidate;
}

uint32_t mesh(uint32_t x)
{
	x ^= x << 13;
	x *= 3931654393u;
	x ^= x >> 13;
	x *= 3058479007u;
	x ^= x >> 16;
	return x;
}

void quorum_add_rid(BackendDB *bd, void* key, int rid) {
	int n;
	struct present* p;
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != QR_POISON);
	assert(key != NULL);
	bd = bd->bd_self;

	lock();
	if (! bd->bd_quorum)
		quorum_be_init(bd);

	n = 0;
	if (bd->bd_quorum->qr_present) {
		for(n = 0, p = bd->bd_quorum->qr_present; p->key != NULL; ++p, ++n ) {
			if (p->key == key)
				goto bailout;
		}
	}

	Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: add link-rid %d/%p to %s\n",
		   rid, key, bd->bd_quorum->qr_cluster );

	bd->bd_quorum->qr_present = ch_realloc(bd->bd_quorum->qr_present,
								(n + 2) * sizeof(struct present));
	p = bd->bd_quorum->qr_present + n;
	p->key = key;
	p->rid = rid;
	p->sid = -1;
	p->status = QS_DIRTY;
	p[1].key = NULL;

	bd->bd_quorum->qt_status[p->status] += 1;
	bd->bd_quorum->salt += mesh(rid);
	kick(bd->bd_quorum);

	if ((bd->bd_quorum->flags & QR_AUTO_RIDS)
	&& require_append(bd->bd_quorum, QR_VOTED_RID | QR_FLAG_AUTO, rid))
		quorum_invalidate(bd);

bailout:
	unlock();
}

void quorum_remove_rid(BackendDB *bd, void* key) {
	int n;
	struct present* p;
	assert(quorum_list != QR_POISON);
	assert(key != NULL);
	bd = bd->bd_self;

	lock();
	if (bd->bd_quorum) {

		if (bd->bd_quorum->qr_present) {
			for(n = 0, p = bd->bd_quorum->qr_present; p->key != NULL; ++p, ++n) {
				if (p->key == key) {
					Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: remove link-rid %d/%p from %s\n",
						   p->rid, key, bd->bd_quorum->qr_cluster );
					if (p->sid > -1 && (bd->bd_quorum->flags & QR_AUTO_SIDS))
						require_remove(bd->bd_quorum,
									   QR_VOTED_SID | QR_FLAG_AUTO, ~QR_FLAG_DEMAND, p->sid);
					if (bd->bd_quorum->flags & QR_AUTO_RIDS)
						require_remove(bd->bd_quorum,
									   QR_VOTED_RID | QR_FLAG_AUTO, ~QR_FLAG_DEMAND, p->rid);

					assert(bd->bd_quorum->qt_status[p->status] > 0);
					bd->bd_quorum->qt_status[p->status] -= 1;
					bd->bd_quorum->salt -= mesh(p->rid);
					kick(bd->bd_quorum);

					for (;;) {
						p[0] = p[1];
						if (p->key == NULL)
							break;
						++n; ++p;
					}

					if (! n) {
						ch_free(bd->bd_quorum->qr_present);
						bd->bd_quorum->qr_present = NULL;
					}

					quorum_invalidate(bd);
					break;
				}
			}
		}
	}
	unlock();
}

void quorum_notify_sid(BackendDB *bd, void* key, int sid) {
	assert(key != NULL);
	assert(sid < 0 || sid <= SLAP_SYNC_SID_MAX);
	assert(quorum_list != QR_POISON);
	bd = bd->bd_self;

	if (bd->bd_quorum) {
		lock();
		if (bd->bd_quorum && notify_sid(bd->bd_quorum, key, sid))
			quorum_invalidate(bd);
		unlock();
	}
}

void quorum_notify_status(BackendDB *bd, void* key, int status) {
	assert(key != NULL);
	assert(status >= QS_DIRTY && status <= QS_PROCESS);
	assert(quorum_list != QR_POISON);
	bd = bd->bd_self;

	if (bd->bd_quorum) {
		lock();
		if (bd->bd_quorum && notify_status(bd->bd_quorum, key, status))
			quorum_invalidate(bd);
		unlock();
	}
}

void quorum_notify_csn(BackendDB *bd, int csnsid) {
	assert(quorum_list != QR_POISON);
	assert(csnsid > -1 && csnsid <= SLAP_SYNC_SID_MAX);
	bd = bd->bd_self;

	if (bd->bd_quorum && (bd->bd_quorum->flags & QR_AUTO_SIDS))  {
		lock();
		if (require_append(bd->bd_quorum, QR_VOTED_SID | QR_FLAG_AUTO, csnsid))
			quorum_invalidate(bd);
		unlock();
	}
}

static int unparse(BerVarray *vals, slap_quorum_t *q, int type, const char* verb) {
	struct requirment* r;
	int i;

	for(i = 0, r = q->qr_requirements; r->type > -1; ++r) {
		if (r->type == type) {
			if (i == 0 && value_add_one_str(vals, verb))
				return 1;
			if (value_add_one_int(vals, r->id))
				return 1;
			++i;
		}
	}
	return 0;
}

int config_quorum(ConfigArgs *c) {
	int i, type;
	slap_quorum_t *q = c->be->bd_quorum;
	assert(c->be->bd_self == c->be);

	assert(quorum_list != QR_POISON);
	if (c->op == SLAP_CONFIG_EMIT) {
		BerVarray vals = NULL;
		if (! q)
			return 1;
		if ((q->flags & QR_ALL_LINKS) != 0
		&& value_add_one_str(&vals, "all-links"))
			return 1;
		if ((q->flags & QR_AUTO_SIDS) != 0
		&& value_add_one_str(&vals, "auto-sids"))
			return 1;
		if ((q->flags & QR_AUTO_RIDS) != 0
		&& value_add_one_str(&vals, "auto-rids"))
			return 1;
		if (q->qr_requirements
		&& (unparse(&vals, q, QR_DEMAND_RID, "require-rids")
		 || unparse(&vals, q, QR_DEMAND_SID, "require-sids")
		 || unparse(&vals, q, QR_VOTED_RID, "vote-rids")
		 || unparse(&vals, q, QR_VOTED_SID, "vote-sids")))
			return 1;
		if (q->qr_syncrepl_limit > 0
		&& (value_add_one_str(&vals, "limit-concurrent-refresh")
		 || value_add_one_int(&vals, q->qr_syncrepl_limit)))
			return 1;

		if (! vals)
			return 1;
		if (value_join_str(vals, " ", &c->value_bv) < 0) {
			ber_bvarray_free(vals);
			return -1;
		}
		ber_bvarray_free(vals);
		return 0;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		lock();
		if (q) {
			q->flags = 0;
			if(q->qr_requirements) {
				ch_free(q->qr_requirements);
				q->qr_requirements = NULL;
				set_cache(c->be, 1);
				Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s requirements-list cleared\n",
					   q->qr_cluster );
			}
			if (q->qr_syncrepl_limit) {
				Debug( LDAP_DEBUG_SYNC, "syncrepl_quorum: %s limit-concurrent-refresh cleared\n",
					   q->qr_cluster );
				q->qr_syncrepl_limit = 0;
			}
		}
		unlock();
		return 0;
	}

	lock();
	if (! q) {
		quorum_be_init(c->be);
		q = c->be->bd_quorum;
	}

	type = -1;
	for( i = 1; i < c->argc; i++ ) {
		int	id;
		if (strcasecmp(c->argv[i], "all-links") == 0) {
			type = -1;
			if (! (q->flags & QR_ALL_LINKS)) {
				q->flags |= QR_ALL_LINKS;
				quorum_invalidate(c->be);
			}
		} else if (strcasecmp(c->argv[i], "auto-sids") == 0) {
			type = -1;
			if (require_auto_sids(q))
				quorum_invalidate(c->be);
		} else if (strcasecmp(c->argv[i], "auto-rids") == 0) {
			type = -1;
			if (require_auto_rids(q))
				quorum_invalidate(c->be);
		} else if (strcasecmp(c->argv[i], "vote-sids") == 0)
			type = QR_VOTED_SID;
		else if (strcasecmp(c->argv[i], "vote-rids") == 0)
			type = QR_VOTED_RID;
		else if (strcasecmp(c->argv[i], "require-sids") == 0)
			type = QR_DEMAND_SID;
		else if (strcasecmp(c->argv[i], "require-rids") == 0)
			type = QR_DEMAND_RID;
		else if (strcasecmp(c->argv[i], "limit-concurrent-refresh") == 0) {
			if (q->qr_syncrepl_limit) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"<%s> limit-concurrent-refresh was already defined for %s as %d",
					c->argv[i], q->qr_cluster,
					q->qr_syncrepl_limit );
				unlock();
				Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
				return 1;
			}
#define QR_FAKETYPE_max_concurrent_refresh -2
			type = QR_FAKETYPE_max_concurrent_refresh;
			q->qr_syncrepl_limit = 1;
		} else if (type == QR_FAKETYPE_max_concurrent_refresh) {
			int n;
			if ( lutil_atoi( &n, c->argv[i]) || n < 1 || n > QR_SYNCREPL_MAX ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"<%s> illegal limit-concurrent-refresh for %s (maximum is %d)",
					c->argv[i], q->qr_cluster,
					QR_SYNCREPL_MAX );
				unlock();
				Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
				return 1;
			}
			q->qr_syncrepl_limit = n;
			type = -1;
		} else if (type < 0) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> mode-key must precede an ID for %s quorum-requirements",
				c->argv[i], q->qr_cluster );
			unlock();
			Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
			return 1;
		} else if (( lutil_atoi( &id, c->argv[i] ) &&
				   lutil_atoix( &id, c->argv[i], 16 ))
			|| id < 0
			|| id > (QR_IS_SID(type) ? SLAP_SYNC_SID_MAX : SLAP_SYNC_RID_MAX) )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> illegal %s-ID or mode-key for %s quorum-requirements",
				c->argv[i], QR_IS_SID(type) ? "Server" : "Repl",
				q->qr_cluster );
			unlock();
			Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
			return 1;
		} else {
			if (! require_append( q, type, id )) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"<%s> %s-ID already present in %s quorum-requirements",
					c->argv[i], QR_IS_SID(type) ? "Server" : "Repl",
					q->qr_cluster );
				unlock();
				Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
				return 1;
			}
			quorum_invalidate(c->be);
		}
	}

	unlock();
	return 0;
}

//-----------------------------------------------------------------------------

int quorum_syncrepl_maxrefresh(BackendDB *bd)
{
	slap_quorum_t *q = bd->bd_quorum;

	if (! q || ! q->qr_syncrepl_limit)
		return INT_MAX;

	return q->qr_syncrepl_limit;
}

int quorum_syncrepl_gate(BackendDB *bd, void *instance_key, int in)
{
	slap_quorum_t *q;
	int rc;
	bd = bd->bd_self;

	assert(instance_key != NULL);
	if (! instance_key)
		return LDAP_ASSERTION_FAILED;

	lock();
	q = bd->bd_quorum;
	if (! q) {
		quorum_be_init(bd);
		q = bd->bd_quorum;
	}

	rc = LDAP_SUCCESS;
	if (in) {
		if (q->qr_syncrepl_limit && q->qt_running >= q->qr_syncrepl_limit)
			rc = LDAP_BUSY;
		else
			q->qt_running += 1;
	} else {
		assert(q->qt_running > 0);
		q->qt_running--;
	}

	if (rc == LDAP_SUCCESS)
		kick(q);

	Debug( LDAP_DEBUG_SYNC, "quorum_syncrepl_gate: %s, "
							"limit %d, running %d, %p %s, rc %d\n",
		q->qr_cluster, q->qr_syncrepl_limit, q->qt_running,
		instance_key, in ? "in" : "out", rc );
	assert(q->qt_running >= 0 && q->qt_running <= QR_SYNCREPL_MAX);

	unlock();
	return rc;
}
