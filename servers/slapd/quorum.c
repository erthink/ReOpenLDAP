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

struct present {
	short rid, sid, ready;
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

#define QR_SYNCREPL_MAX 8
	int qr_syncrepl_limit;
	void *qr_syncrepl_current[QR_SYNCREPL_MAX];
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
	int rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
	assert(rc == 0);
}

static void unlock() {
	int rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
	assert(rc == 0);
}

void quorum_global_destroy() {
	if (quorum_list != QR_POISON) {
		lock();
		while (quorum_list) {
			slap_quorum_t *next = quorum_list;
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

static void quorum_be_init(BackendDB *bd) {
	slap_quorum_t *q;

	assert(quorum_list != QR_POISON);
	q = ch_calloc(1, sizeof(slap_quorum_t));

	q->qr_bd = bd;
	q->qr_next = quorum_list;
	bd->bd_quorum = q;
	bd->bd_quorum_cache = 1;
	quorum_list = q;
}

void quorum_be_destroy(BackendDB *bd) {
	assert(quorum_list != QR_POISON);

	if (bd->bd_quorum) {
		slap_quorum_t *q, **pq;

		lock();
		q = bd->bd_quorum;
		if (q) {
			bd->bd_quorum_cache = 1;
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
		for(p = q->qr_present; p->ready > -1; ++p) {
			++total_links;
			ready_links += p->ready;
		}
	}

	if ((q->flags & QR_ALL_LINKS) && ready_links < total_links) {
		Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s FORCE-LACK"
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
				for(p = q->qr_present; p && p->ready > -1; ++p) {
					if (p->sid == r->id) {
						ready_sids += p->ready;
						break;
					}
				}
			} else {
				++wanna_rids;
				for(p = q->qr_present; p && p->ready > -1; ++p) {
					if (p->rid == r->id) {
						ready_rids += p->ready;
						break;
					}
				}
			}
			if (QR_IS_DEMAND(r->type) && (!p || p->ready < 1)) {
				Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s FORCE-LACK"
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
		Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s FORCE-LACK"
				" (auto-sids, not enough for voting)\n",
			   q->qr_cluster );
		state = -1;
	}

	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s %s (wanna/ready: "
							"link %d/%d, rid %d/%d, sid %d/%d)\n",
		   q->qr_cluster, (state > 0) ? "HAVE" : "LACK",
		   total_links, ready_links, wanna_rids, ready_rids, wanna_sids, ready_sids );
	return state;
}

static void quorum_invalidate(BackendDB *bd) {
	bd->bd_quorum_cache = 0;
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
	int snap = bd->bd_quorum_cache;
	if (unlikely(snap == 0)) {
		assert(quorum_list != QR_POISON);

		lock();
		snap = bd->bd_quorum_cache;
		if (snap == 0) {
			snap = bd->bd_quorum ? lazy_update(bd->bd_quorum) : 1;
			assert(snap != 0);
			bd->bd_quorum_cache = snap;
		}
		unlock();
	}
	return snap > 0;
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
	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: added %s%s-%s %d to %s requirements as #%d\n",
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
			Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s%s-%s %d removed from %s requirements as #%d\n",
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
			for (p = q->qr_present; p->ready > -1; ++p)
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
			for (p = q->qr_present; p->ready > -1; ++p)
				if (p->sid > -1)
					changed |= require_append(q, QR_VOTED_SID | QR_FLAG_AUTO, p->sid);
		}
	}
	return changed;
}

static int notify_sid(slap_quorum_t *q, int rid, int sid) {
	struct present* p;
	int n = 0;

	if ( q->qr_present) {
		for(p = q->qr_present; p->ready > -1; ++p, ++n) {
			if (p->rid == rid) {
				if (p->sid != sid) {
					Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: notify-sid %s, rid %d, sid %d->%d\n",
						   q->qr_cluster, p->rid, p->sid, sid );
					if (q->flags & QR_AUTO_SIDS) {
						/* if (p->sid > -1)
							require_remove(q, QR_VOTED_SID | QR_FLAG_AUTO,
										   ~QR_FLAG_DEMAND, p->sid); */
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
	/* LY: this may occur when 'syncrepl' removed by syncrepl_config()
	 * while do_syncrepl() is running. */
	return 0;
}

static int notify_ready(slap_quorum_t *q, int rid, int ready) {
	struct present* p;
	int n = 0;

	if ( q->qr_present) {
		for(p = q->qr_present; p->ready > -1; ++p, ++n) {
			if (p->rid == rid) {
				if (p->ready != ready) {
					Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: notify-status %s, rid %d, sid %d, %s->%s\n",
						   q->qr_cluster, p->rid, p->sid,
						   p->ready ? "ready" : "loser",
						   ready ? "ready" : "loser" );
					p->ready = ready;
					return 1;
				}
				return 0;
			}
		}
	}
	/* LY: this may occur when 'syncrepl' removed by syncrepl_config()
	 * while do_syncrepl() is running. */
	return 0;
}

void quorum_add_rid(BackendDB *bd, int rid) {
	int n;
	struct present* p;
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != QR_POISON);

	lock();
	if (! bd->bd_quorum)
		quorum_be_init(bd);

	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: add link-rid %d to %s\n",
		   rid, bd->bd_quorum->qr_cluster );

	n = 0;
	if (bd->bd_quorum->qr_present) {
		for(n = 0, p = bd->bd_quorum->qr_present; p->ready > -1; ++p, ++n ) {
			if (p->rid == rid) {
				assert(0);
				goto bailout;
			}
		}
	}

	bd->bd_quorum->qr_present = ch_realloc(bd->bd_quorum->qr_present,
								(n + 2) * sizeof(struct present));
	p = bd->bd_quorum->qr_present + n;
	p->rid = rid;
	p->sid = -1;
	p->ready = 0;
	p[1].ready = -1;

	if ((bd->bd_quorum->flags & QR_AUTO_RIDS)
	&& require_append(bd->bd_quorum, QR_VOTED_RID | QR_FLAG_AUTO, rid))
		quorum_invalidate(bd);

bailout:
	unlock();
}

void quorum_remove_rid(BackendDB *bd, int rid) {
	int n;
	struct present* p;
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != QR_POISON);

	lock();

	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: remove link-rid %d from %s\n",
		   rid, bd->bd_quorum->qr_cluster );

	assert(bd->bd_quorum != NULL);
	assert(bd->bd_quorum->qr_present != NULL);
	for(n = 0, p = bd->bd_quorum->qr_present; p->ready > -1; ++p, ++n) {
		if (p->rid == rid) {
			/* if (p->sid > -1 && (bd->bd_quorum->flags & QR_AUTO_SIDS))
				require_remove(bd->bd_quorum,
							   QR_VOTED_SID | QR_FLAG_AUTO, ~QR_FLAG_DEMAND, p->sid); */
			if (bd->bd_quorum->flags & QR_AUTO_RIDS)
				require_remove(bd->bd_quorum,
							   QR_VOTED_RID | QR_FLAG_AUTO, ~QR_FLAG_DEMAND, rid);

			for (;;) {
				p[0] = p[1];
				if (p->ready < 0)
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

	unlock();
}

void quorum_notify_sid(BackendDB *bd, int rid, int sid) {
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(sid < 0 || sid <= SLAP_SYNC_SID_MAX);
	assert(quorum_list != QR_POISON);

	if (bd->bd_quorum) {
		lock();
		if (bd->bd_quorum && notify_sid(bd->bd_quorum, rid, sid))
			quorum_invalidate(bd);
		unlock();
	}
}

void quorum_notify_status(BackendDB *bd, int rid, int ready) {
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != QR_POISON);

	if (bd->bd_quorum) {
		lock();
		if (bd->bd_quorum && notify_ready(bd->bd_quorum, rid, ready > 0))
			quorum_invalidate(bd);
		unlock();
	}
}

void quorum_notify_csn(BackendDB *bd, int csnsid) {
	assert(quorum_list != QR_POISON);
	assert(csnsid > -1 && csnsid <= SLAP_SYNC_RID_MAX);

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

int quorum_config(ConfigArgs *c) {
	int i, type;
	slap_quorum_t *q = c->be->bd_quorum;

	assert(quorum_list != QR_POISON);
	if (c->op == SLAP_CONFIG_EMIT) {
		if (q) {
			if ((q->flags & QR_ALL_LINKS) != 0
			&& value_add_one_str(&c->rvalue_vals, "all-links"))
				return 1;
			if ((q->flags & QR_AUTO_SIDS) != 0
			&& value_add_one_str(&c->rvalue_vals, "auto-sids"))
				return 1;
			if ((q->flags & QR_AUTO_RIDS) != 0
			&& value_add_one_str(&c->rvalue_vals, "auto-rids"))
				return 1;
			if (q->qr_requirements
			&& (unparse(&c->rvalue_vals, q, QR_DEMAND_RID, "require-rids")
			 || unparse(&c->rvalue_vals, q, QR_DEMAND_SID, "require-sids")
			 || unparse(&c->rvalue_vals, q, QR_VOTED_RID, "vote-rids")
			 || unparse(&c->rvalue_vals, q, QR_VOTED_SID, "vote-sids")))
				return 1;
			if (q->qr_syncrepl_limit > 0
			&& (value_add_one_str(&c->rvalue_vals, "limit-concurrent-refresh")
			 || value_add_one_int(&c->rvalue_vals, q->qr_syncrepl_limit)))
				return 1;
		}
		return 0;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		lock();
		if (q) {
			q->flags = 0;
			if(q->qr_requirements) {
				ch_free(q->qr_requirements);
				q->qr_requirements = NULL;
				c->be->bd_quorum_cache = 1;
				Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s requirements-list cleared\n",
					   q->qr_cluster );
			}
			if (q->qr_syncrepl_limit) {
				Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s limit-concurrent-refresh cleared\n",
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

int quorum_syncrepl_gate(BackendDB *bd, void *instance_key, int in)
{
	slap_quorum_t *q;
	int i, slot, rc, turn;

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
	turn = 0;
	slot = -1;
	assert(q->qr_syncrepl_limit <= QR_SYNCREPL_MAX);

	/* LY: Firstly find and release slot if any,
	 *     even when limit-concurrent-refresh disabled.
	 *     This is necessary for consistency in case
	 *     it has been disabled at runtime. */
	for(i = 0; i < QR_SYNCREPL_MAX; ++i) {
		if (q->qr_syncrepl_current[i] == instance_key) {
			q->qr_syncrepl_current[i] = NULL;
			slot = i;
			turn = 1;
			if (in && q->qr_syncrepl_limit)
				/* LY: If was already acquired and limit-concurrent-refresh enabled,
				 *     then indicate that syncrepl should yield. */
				rc = LDAP_SYNC_REFRESH_REQUIRED;
		}
	}

	if (in && rc == LDAP_SUCCESS && q->qr_syncrepl_limit) {
		turn = 1;
		rc = LDAP_BUSY;
		for(i = 0; i < q->qr_syncrepl_limit; ++i) {
			if (q->qr_syncrepl_current[i] == NULL) {
				q->qr_syncrepl_current[i] = instance_key;
				slot = i;
				rc = LDAP_SUCCESS;
				break;
			}
		}
	}

	if (turn) {
		Debug( LDAP_DEBUG_SYNC, "quorum_syncrepl_gate: %s, "
								"limit %d, %p %s, slot %d, rc %d\n",
			q->qr_cluster, q->qr_syncrepl_limit,
			instance_key, in ? "in" : "out", slot, rc );
	}

	unlock();
	return rc;
}
