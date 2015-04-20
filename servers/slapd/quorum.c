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
	short sid, rid, ready;
};

struct requirment {
#	define QR_FLAG_DEMAND 1
#	define QR_FLAG_RID    2
#	define QR_FLAG_AUTO   4

#	define QR_QUORUM_SID 0
#	define QR_DEMAND_SID (QR_QUORUM_SID | QR_FLAG_DEMAND)
#	define QR_QUORUM_RID QR_FLAG_RID
#	define QR_DEMAND_RID (QR_QUORUM_RID | QR_FLAG_AUTO)

#	define QR_IS_RID(type) ((type) & QR_FLAG_RID)
#	define QR_IS_SID(type) (!QR_IS_RID(type))

#	define QR_IS_DEMAND(type) ((type) & QR_FLAG_DEMAND)
#	define QR_IS_QUORUM(type) (!QR_IS_DEMAND(type))

#	define QR_IS_AUTO(type) ((type) & QR_FLAG_AUTO)

	short type, id;
};

struct slap_quorum {
	slap_quorum_t* qr_next;
	struct present* qr_present;
	struct requirment* qr_requirements;
	BackendDB *qr_bd;
#	define qr_cluster qr_bd->be_rootndn.bv_val
	int flags;
#	define QR_AUTO_RIDS	1
};

static ldap_pvt_thread_mutex_t quorum_mutex;
#define POISON ((void*) (intptr_t) -1)
slap_quorum_t *quorum_list = POISON;

void quorum_global_init() {
	int rc;

	assert(quorum_list == POISON);
	rc = ldap_pvt_thread_mutex_init(&quorum_mutex);
	if (rc) {
		Debug( LDAP_DEBUG_ANY, "mutex_init failed for quorum_mutex, rc = %d\n", rc );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}
	quorum_list = NULL;
}

void quorum_global_destroy() {
	int rc;

	if (quorum_list != POISON) {
		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		ch_free( quorum_list );
		quorum_list = POISON;

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
		ldap_pvt_thread_mutex_destroy(&quorum_mutex);
	}
}

static void quorum_be_init(BackendDB *bd) {
	slap_quorum_t *q;
	assert(quorum_list != POISON);

	q = ch_calloc(1, sizeof(slap_quorum_t));
	q->qr_bd = bd;
	q->qr_next = quorum_list;
	bd->bd_quorum = q;
	bd->bd_quorum_cache = 1;
	quorum_list = q;
}

void quorum_be_destroy(BackendDB *bd) {
	assert(quorum_list != POISON);

	if (bd->bd_quorum) {
		int rc;
		slap_quorum_t *q, **pq;

		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		q = bd->bd_quorum;
		if (q) {
			bd->bd_quorum_cache = 1;
			bd->bd_quorum = NULL;

			for(pq = &quorum_list; *pq; pq = &((*pq)->qr_next))
				if (*pq == q) {
					*pq = q->qr_next;
					break;
				}

			rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
			assert(rc == 0);

			ch_free(q->qr_present);
			ch_free(q->qr_requirements);
			ch_free(q);
		}
	}
}

static int quorum(unsigned total, unsigned wanna, unsigned ready) {
	if (wanna > total)
		wanna = total;
	return (ready < (wanna + 2 - (wanna & 1)) / 2) ? 0 : 1;
}

static int lazy_update(slap_quorum_t *q) {
	unsigned links, ready_rids, ready_sids, wanna_rids, wanna_sids;
	struct present* p;
	struct requirment* r;
	int state;

	ready_rids = ready_sids = 0;
	wanna_rids = wanna_sids = 0;
	links = 0;
	if (q->qr_present)
		for(p = q->qr_present; p->ready > -1; ++p)
			++links;

	state = 1;
	if (q->qr_requirements) {
		for(r = q->qr_requirements; r->type > -1; ++r) {
			if (QR_IS_SID(r->type)) {
				if (r->id == slap_serverID)
					continue;
				for(p = q->qr_present; p->ready > -1; ++p) {
					if (p->sid == r->id) {
						wanna_sids += 1;
						ready_sids += p->ready;
						break;
					}
				}
			} else {
				for(p = q->qr_present; p->ready > -1; ++p) {
					if (p->rid == r->id) {
						wanna_rids += 1;
						ready_rids += p->ready;
						break;
					}
				}
			}
			if (QR_IS_DEMAND(r->type) && p->ready < 1) {
				Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s LACK"
						" (required %s%s %d not ready)\n",
					   q->qr_cluster,
					   QR_IS_AUTO(r->type) ? "auto-" : "",
					   QR_IS_SID(r->type) ? "sid" : "rid",
					   r->id );
				return -1;
			}
		}

		state = (quorum(links, wanna_sids, ready_sids)
				|| quorum(links, wanna_rids, ready_rids)) ? 1 : -1;
	}

	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s %s (links %d, "
							"rids/sids: wanna %d/%d, ready %d/%d)\n",
		   q->qr_cluster, (state > 0) ? "HAVE" : "LACK",
		   links, wanna_rids, wanna_sids, ready_rids, ready_sids );
	return state;
}

static void quorum_invalidate(BackendDB *bd) {
	bd->bd_quorum_cache = 0;
}

void quorum_notify_self_sid() {
	int rc;
	slap_quorum_t *q;
	assert(quorum_list != POISON);

	rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
	assert(rc == 0);

	for(q = quorum_list; q; q = q->qr_next)
		quorum_invalidate(q->qr_bd);

	rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
	assert(rc == 0);
}

int quorum_query(BackendDB *bd) {
	int snap = bd->bd_quorum_cache;
	if (unlikely(snap == 0)) {
		int rc;
		assert(quorum_list != POISON);

		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		snap = bd->bd_quorum_cache;
		if (snap == 0) {
			snap = bd->bd_quorum ? lazy_update(bd->bd_quorum) : 1;
			assert(snap != 0);
			bd->bd_quorum_cache = snap;
		}

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
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
			return 1;
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
		   QR_IS_AUTO(type) ? "auto-" : "", QR_IS_DEMAND(type) ? "demand" : "quorum",
		   QR_IS_SID(type) ? "sid" : "rid", id, q->qr_cluster, n );

	return 0;
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
				   QR_IS_AUTO(r->type) ? "auto-" : "", QR_IS_DEMAND(type) ? "demand" : "quorum",
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
				if (require_append(q, QR_QUORUM_RID | QR_FLAG_AUTO, p->rid) == 0)
					changed = 1;
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
					p->sid = sid;
					Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: notify-sid %s, rid %d, sid %d\n",
						   q->qr_cluster, p->rid, p->sid );
					return 1;
				}
				return 0;
			}
		}
	}
	LDAP_BUG();
}

static int notify_ready(slap_quorum_t *q, int rid, int ready) {
	struct present* p;
	int n = 0;

	if ( q->qr_present) {
		for(p = q->qr_present; p->ready > -1; ++p, ++n) {
			if (p->rid == rid) {
				if (p->ready != ready) {
					p->ready = ready;
					Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: notify-status %s, rid %d, sid %d, %s\n",
						   q->qr_cluster, p->rid, p->sid, p->ready ? "ready" : "loser" );
					return 1;
				}
				return 0;
			}
		}
	}
	LDAP_BUG();
}

void quorum_add_rid(BackendDB *bd, int rid) {
	int n, rc;
	struct present* p;
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != POISON);

	rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
	assert(rc == 0);

	if (! bd->bd_quorum)
		quorum_be_init(bd);

	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: add rid %d to %s\n",
		   rid, bd->bd_quorum->qr_cluster );

	n = 0;
	if (bd->bd_quorum->qr_present) {
		for(n = 0, p = bd->bd_quorum->qr_present; p->ready > -1; ++p, ++n ) {
			if (p->rid == rid) {
				assert(0);
				return;
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
	&& require_append(bd->bd_quorum, QR_QUORUM_RID | QR_FLAG_AUTO, rid) == 0)
		quorum_invalidate(bd);

	rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
	assert(rc == 0);
}

void quorum_remove_rid(BackendDB *bd, int rid) {
	int n, rc;
	struct present* p;
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != POISON);

	rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
	assert(rc == 0);

	Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: remove rid %d from %s\n",
		   rid, bd->bd_quorum->qr_cluster );

	assert(bd->bd_quorum != NULL);
	assert(bd->bd_quorum->qr_present != NULL);
	for(n = 0, p = bd->bd_quorum->qr_present; p->ready > -1; ++p, ++n) {
		if (p->rid == rid) {
			if (bd->bd_quorum->flags & QR_AUTO_RIDS)
				require_remove(bd->bd_quorum,
							   QR_FLAG_RID | QR_FLAG_AUTO, ~QR_FLAG_DEMAND, rid);

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

	rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
	assert(rc == 0);
}

void quorum_notify_sid(BackendDB *bd, int rid, int sid) {
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(sid < 0 || sid <= SLAP_SYNC_SID_MAX);
	assert(quorum_list != POISON);

	if (bd->bd_quorum) {
		int rc;

		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		if (bd->bd_quorum && notify_sid(bd->bd_quorum, rid, sid))
			quorum_invalidate(bd);

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
	}
}

void quorum_notify_status(BackendDB *bd, int rid, int ready) {
	assert(rid > -1 || rid <= SLAP_SYNC_RID_MAX);
	assert(quorum_list != POISON);

	if (bd->bd_quorum) {
		int rc;

		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		if (bd->bd_quorum && notify_ready(bd->bd_quorum, rid, ready > 0))
			quorum_invalidate(bd);

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
	}
}

static int unparse(BerVarray *vals, slap_quorum_t *q, int type, const char* verb) {
	struct requirment* r;
	int i;

	for(i = 0, r = q->qr_requirements; r->type > -1; ++r) {
		if (r->type == type) {
			char buf[16];
			struct berval v;

			if (i == 0 && value_add_one_str(vals, verb))
				return 1;

			v.bv_val = buf;
			v.bv_len = snprintf(buf, sizeof(buf), "%d", r->id);
			if (value_add_one(vals, &v))
				return 1;

			++i;
		}
	}
	return 0;
}

int quorum_config(ConfigArgs *c) {
	int i, rc, type;

	assert(quorum_list != POISON);
	if (c->op == SLAP_CONFIG_EMIT) {
		if (c->be->bd_quorum) {
			if ((c->be->bd_quorum->flags & QR_AUTO_RIDS) != 0
			&& value_add_one_str(&c->rvalue_vals, "auto-rids"))
				return 1;
			if (c->be->bd_quorum->qr_requirements
			&& (unparse(&c->rvalue_vals, c->be->bd_quorum, QR_DEMAND_RID, "require-rids")
			 || unparse(&c->rvalue_vals, c->be->bd_quorum, QR_DEMAND_SID, "require-sids")
			 || unparse(&c->rvalue_vals, c->be->bd_quorum, QR_QUORUM_RID, "vote-rids")
			 || unparse(&c->rvalue_vals, c->be->bd_quorum, QR_QUORUM_SID, "vote-sids")))
				return 1;
		}
		return 0;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
		assert(rc == 0);

		if (c->be->bd_quorum && c->be->bd_quorum->qr_requirements) {
			ch_free(c->be->bd_quorum->qr_requirements);
			c->be->bd_quorum->qr_requirements = NULL;
			c->be->bd_quorum_cache = 1;
			Debug( LDAP_DEBUG_SYNC, "syncrep_quorum: %s requirements-list cleared\n",
				   c->be->bd_quorum->qr_cluster );
		}

		rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
		assert(rc == 0);
		return 0;
	}

	rc = ldap_pvt_thread_mutex_lock(&quorum_mutex);
	assert(rc == 0);

	if (! c->be->bd_quorum)
		quorum_be_init(c->be);

	type = -1;
	for( i = 1; i < c->argc; i++ ) {
		int	id;
		if (strcasecmp(c->argv[i], "auto-rids") == 0) {
			type = -1;
			if (require_auto_rids(c->be->bd_quorum))
				quorum_invalidate(c->be);
		} else if (strcasecmp(c->argv[i], "vote-sids") == 0)
			type = QR_QUORUM_SID;
		else if (strcasecmp(c->argv[i], "vote-rids") == 0)
			type = QR_QUORUM_RID;
		else if (strcasecmp(c->argv[i], "require-sids") == 0)
			type = QR_DEMAND_SID;
		else if (strcasecmp(c->argv[i], "require-rids") == 0)
			type = QR_DEMAND_RID;
		else if (type < 0) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> mode-key must precede an ID for %s quorum-requirements",
				c->argv[i], c->be->bd_quorum->qr_cluster );
			Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
			return 1;
		} else if (( lutil_atoi( &id, c->argv[i] ) &&
				   lutil_atoix( &id, c->argv[i], 16 ))
			|| id < 0
			|| id > (QR_IS_SID(type) ? SLAP_SYNC_SID_MAX : SLAP_SYNC_RID_MAX) )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"<%s> illegal %s-ID or mode-key for %s quorum-requirements",
				c->argv[i], QR_IS_SID(type) ? "server" : "repl",
				c->be->bd_quorum->qr_cluster );
			Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
			return 1;
		} else {
			if (require_append( c->be->bd_quorum, type, id )) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"<%s> %s-ID already present in %s quorum-requirements",
					c->argv[i], QR_IS_SID(type) ? "server" : "repl",
					c->be->bd_quorum->qr_cluster );
				Debug(LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
				return 1;
			}
			quorum_invalidate(c->be);
		}
	}

	rc = ldap_pvt_thread_mutex_unlock(&quorum_mutex);
	assert(rc == 0);
	return 0;
}
