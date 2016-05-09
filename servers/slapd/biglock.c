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

void slap_biglock_init ( BackendDB *bd ) {
	slap_biglock_t *bl;
	int rc;

	assert(bd->bd_self == bd);
	assert(bd->bd_biglock == NULL);
	bl = ch_calloc(1, sizeof(slap_biglock_t));
	rc = ldap_pvt_thread_mutex_init(&bl->bl_mutex);
	if (rc) {
		Debug( LDAP_DEBUG_ANY, "mutex_init failed for biglock, rc = %d\n", rc );
		LDAP_BUG();
		exit( EXIT_FAILURE );
	}
	bd->bd_biglock = bl;
	bd->bd_biglock_mode = SLAPD_BIGLOCK_NONE;
}

static void slap_biglock_free( slap_biglock_t* bl ) {
	LDAP_ENSURE(bl->bl_recursion == 0);
	ldap_pvt_thread_mutex_destroy(&bl->bl_mutex);
	ch_free(bl);
}

static ATTRIBUTE_NO_SANITIZE_THREAD_INLINE
ldap_pvt_thread_t get_owner(slap_biglock_t* bl) {
	return bl->_bl_owner;
}

void slap_biglock_destroy( BackendDB *bd ) {
	assert(bd->bd_self == bd);
	slap_biglock_t* bl = bd->bd_biglock;
	if (bl) {
		bd->bd_biglock = NULL;
		bd->bd_biglock_mode = -1;
		if (! bl->bl_recursion) {
			slap_biglock_free(bl);
		} else {
			LDAP_ENSURE(ldap_pvt_thread_self() == get_owner(bl));
			bl->bl_free_on_release = 1;
		}
	}
}

static const ldap_pvt_thread_t thread_null;

slap_biglock_t* slap_biglock_get( BackendDB *bd ) {
	bd = bd->bd_self;

	switch (bd->bd_biglock_mode) {
	default:
		/* LY: zero-value indicates than mode was not initialized,
		 * for instance in overlay's code. */
		LDAP_BUG();
	case SLAPD_BIGLOCK_NONE:
		return NULL;

	case SLAPD_BIGLOCK_LOCAL:
		assert(bd->bd_biglock != NULL);
		return bd->bd_biglock;

#ifdef SLAPD_BIGLOCK_ENGINE
	case SLAPD_BIGLOCK_ENGINE:
		assert(bd->bd_info->bi_biglock != NULL);
		return bd->bd_info->bi_biglock;
#endif
	case SLAPD_BIGLOCK_COMMON:
		assert(frontendDB->bd_biglock != NULL);
		return frontendDB->bd_biglock;
	}
}

int slap_biglock_deep ( BackendDB *bd ) {
	slap_biglock_t* bl = slap_biglock_get( bd );
	return bl ? read_int__tsan_workaround(&bl->bl_recursion) : 0;
}

int slap_biglock_owned ( BackendDB *bd ) {
	slap_biglock_t* bl = slap_biglock_get( bd );
	return bl ? ldap_pvt_thread_self() == get_owner(bl) : 1;
}

void slap_biglock_acquire(slap_biglock_t* bl) {
	if (!bl)
		return;

	if (ldap_pvt_thread_self() == get_owner(bl)) {
		assert(bl->bl_recursion > 0);
		assert(bl->bl_recursion < 42);
		bl->bl_recursion += 1;
		Debug(LDAP_DEBUG_TRACE, "biglock: recursion++ (%d)\n", bl->bl_recursion);
	} else {
		int rc = ldap_pvt_thread_mutex_lock(&bl->bl_mutex);
		assert(rc == 0);
		assert(bl->bl_recursion == 0);
		assert(get_owner(bl) == thread_null);
		Debug(LDAP_DEBUG_TRACE, "biglock: acquire\n");

		bl->_bl_owner = ldap_pvt_thread_self();
		bl->bl_recursion = 1;
	}
}

void slap_biglock_release(slap_biglock_t* bl) {
	if (!bl)
		return;

	assert(ldap_pvt_thread_self() == get_owner(bl));
	assert(bl->bl_recursion > 0);

	if (--bl->bl_recursion == 0) {
		bl->_bl_owner = thread_null;
		Debug(LDAP_DEBUG_TRACE, "biglock: release\n");
		int rc = ldap_pvt_thread_mutex_unlock(&bl->bl_mutex);
		assert(rc == 0);
		if (bl->bl_free_on_release)
			slap_biglock_free(bl);
	} else
		Debug(LDAP_DEBUG_TRACE, "biglock: recursion-- (%d)\n", bl->bl_recursion);
}

int slap_biglock_call_be ( slap_operation_t which,
						   Operation *op,
						   SlapReply *rs ) {
	int rc;
	BI_op_func **func;
	slap_biglock_t *bl = slap_biglock_get(op->o_bd);

	slap_biglock_acquire(bl);

	/* LY: this crutch was copied from backover.c */
	func = &op->o_bd->bd_info->bi_op_bind;
	rc = func[which](op, rs);

	slap_biglock_release(bl);
	return rc;
}

int slap_biglock_pool_pause ( BackendDB *bd ) {
	slap_biglock_t* bl = slap_biglock_get( bd );
	int res;

	if ( bl == NULL || ldap_pvt_thread_self() != get_owner(bl)) {
		res = ldap_pvt_thread_pool_pause( &connection_pool );
	} else {
		int rc, deep = bl->bl_recursion;
		assert(bl->bl_recursion > 0);
		bl->bl_recursion = 0;
		bl->_bl_owner = thread_null;
		Debug(LDAP_DEBUG_TRACE, "biglock: pause-release\n");
		rc = ldap_pvt_thread_mutex_unlock(&bl->bl_mutex);
		assert(rc == 0);

		res = ldap_pvt_thread_pool_pause( &connection_pool );

		rc = ldap_pvt_thread_mutex_lock(&bl->bl_mutex);
		assert(rc == 0);
		assert(bl->bl_recursion == 0);
		assert(bl->_bl_owner == thread_null);
		Debug(LDAP_DEBUG_TRACE, "biglock: pause-acquire\n");
		bl->_bl_owner = ldap_pvt_thread_self();
		bl->bl_recursion = deep;
	}
	return res;
}

void slap_biglock_pool_resume ( BackendDB *bd ) {
	ldap_pvt_thread_pool_resume( &connection_pool );
}

int slap_biglock_pool_pausing ( BackendDB *bd ) {
	return ldap_pvt_thread_pool_pausing( &connection_pool );
}

int slap_biglock_pool_pausecheck ( BackendDB *bd ) {
	int res;
	slap_biglock_t* bl = slap_biglock_get( bd );

	if ( bl == NULL || ldap_pvt_thread_self() != get_owner(bl)) {
		res = ldap_pvt_thread_pool_pausecheck( &connection_pool );
	} else {
		int rc, deep = bl->bl_recursion;
		assert(bl->bl_recursion > 0);
		bl->bl_recursion = 0;
		bl->_bl_owner = thread_null;
		Debug(LDAP_DEBUG_TRACE, "biglock: pausecheck-release\n");
		rc = ldap_pvt_thread_mutex_unlock(&bl->bl_mutex);
		assert(rc == 0);

		res = ldap_pvt_thread_pool_pausecheck( &connection_pool );

		rc = ldap_pvt_thread_mutex_lock(&bl->bl_mutex);
		assert(rc == 0);
		assert(bl->bl_recursion == 0);
		assert(get_owner(bl) == thread_null);
		Debug(LDAP_DEBUG_TRACE, "biglock: pausecheck-acquire\n");
		bl->_bl_owner = ldap_pvt_thread_self();
		bl->bl_recursion = deep;
	}

	return res;
}
