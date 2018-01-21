/* $ReOpenLDAP$ */
/* Copyright 2014-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#include "reldap.h"

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
#if SLAPD_BIGLOCK_TRACELATENCY > 0
static uint64_t biglock_max_latency_ns = 1e9 / 1000; /* LY: 1 ms */
#endif /* SLAPD_BIGLOCK_TRACELATENCY */

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
	return bl ? slap_tsan__read_int(&bl->bl_recursion) : 0;
}

int slap_biglock_owned ( BackendDB *bd ) {
	slap_biglock_t* bl = slap_biglock_get( bd );
	return bl ? ldap_pvt_thread_self() == get_owner(bl) : 1;
}

void __noinline
slap_biglock_acquire(slap_biglock_t* bl) {
	if (!bl)
		return;

	if (ldap_pvt_thread_self() == get_owner(bl)) {
		assert(bl->bl_recursion > 0);
		assert(bl->bl_recursion < 42);
		bl->bl_recursion += 1;
		Debug(LDAP_DEBUG_TRACE, "biglock: recursion++ %p (%d)\n", bl, bl->bl_recursion);
	} else {
		ldap_pvt_thread_mutex_lock(&bl->bl_mutex);
		assert(bl->bl_recursion == 0);
		assert(get_owner(bl) == thread_null);
		Debug(LDAP_DEBUG_TRACE, "biglock: acquire %p\n", bl);
		bl->_bl_owner = ldap_pvt_thread_self();
		bl->bl_recursion = 1;

#if SLAPD_BIGLOCK_TRACELATENCY > 0
		bl->bl_timestamp_ns = ldap_now_steady_ns();
#if SLAPD_BIGLOCK_TRACELATENCY == 1
		bl->bl_backtrace[0] = __builtin_extract_return_addr(__builtin_return_address(0));
#else
		bl->bl_backtrace_deep = backtrace(bl->bl_backtrace, SLAPD_BIGLOCK_TRACELATENCY);
#endif
#endif /* SLAPD_BIGLOCK_TRACELATENCY */
	}
}

void __noinline
slap_biglock_release(slap_biglock_t* bl) {
	if (!bl)
		return;

	assert(ldap_pvt_thread_self() == get_owner(bl));
	assert(bl->bl_recursion > 0);

	if (--bl->bl_recursion == 0) {
		bl->_bl_owner = thread_null;
		Debug(LDAP_DEBUG_TRACE, "biglock: release %p\n", bl);

#if SLAPD_BIGLOCK_TRACELATENCY > 0
		uint64_t latency_ns = ldap_now_steady_ns() - bl->bl_timestamp_ns;
		if (biglock_max_latency_ns < latency_ns) {
			biglock_max_latency_ns = latency_ns;

			ldap_debug_print(
				"*** Biglock new latency achievement: %'.6f seconds\n",
				latency_ns * 1e-9);

			slap_backtrace_log(bl->bl_backtrace,
#if SLAPD_BIGLOCK_TRACELATENCY == 1
				1, "Biglock acquirer"
#else
				SLAPD_BIGLOCK_TRACELATENCY, "Biglock acquirer backtrace"
#endif
			);

#if SLAPD_BIGLOCK_TRACELATENCY == 1
			bl->bl_backtrace[0] = __builtin_extract_return_addr(__builtin_return_address(0));
#else
			bl->bl_backtrace_deep = backtrace(bl->bl_backtrace, SLAPD_BIGLOCK_TRACELATENCY);
#endif
			slap_backtrace_log(bl->bl_backtrace,
#if SLAPD_BIGLOCK_TRACELATENCY == 1
				1, "Biglock releaser"
#else
				SLAPD_BIGLOCK_TRACELATENCY, "Biglock releaser backtrace"
#endif
			);
		}
#endif /* SLAPD_BIGLOCK_TRACELATENCY */

		ldap_pvt_thread_mutex_unlock(&bl->bl_mutex);
		if (bl->bl_free_on_release)
			slap_biglock_free(bl);
	} else
		Debug(LDAP_DEBUG_TRACE, "biglock: recursion-- %p (%d)\n", bl, bl->bl_recursion);
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
		int MAY_UNUSED rc, deep = bl->bl_recursion;
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
		int MAY_UNUSED rc, deep = bl->bl_recursion;
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
