/* globals.c - various global variables */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2016 The OpenLDAP Foundation.
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

#include "portable.h"

#include <ac/string.h>
#include "lber_pvt.h"

#include "slap.h"


/*
 * Global variables, in general, should be declared in the file
 * primarily responsible for its management.  Configurable globals
 * belong in config.c.  Variables declared here have no other
 * sensible home.
 */

const struct berval slap_empty_bv = BER_BVC("");
const struct berval slap_unknown_bv = BER_BVC("unknown");

/* normalized boolean values */
const struct berval slap_true_bv = BER_BVC("TRUE");
const struct berval slap_false_bv = BER_BVC("FALSE");

static void** memleak_crutch;
static pthread_mutex_t memleak_crutch_lock = PTHREAD_MUTEX_INITIALIZER;

static void memleak_crutch_atexit(void)
{
	int i;
	assert(memleak_crutch != NULL);

	LDAP_ENSURE(pthread_mutex_lock(&memleak_crutch_lock) == 0);
	for(i = 0; memleak_crutch[i] != (void*)0xDEAD; ++i)
		if (memleak_crutch[i])
			free(memleak_crutch[i]);
	free(memleak_crutch);
	LDAP_ENSURE(pthread_mutex_unlock(&memleak_crutch_lock) == 0);
}

void memleak_crutch_push(void *p)
{
	int i;
	assert(p && p != (void*) 0xDEAD);

	LDAP_ENSURE(pthread_mutex_lock(&memleak_crutch_lock) == 0);
	if (memleak_crutch == NULL) {
		memleak_crutch = ch_calloc(42, sizeof(void*));
		memleak_crutch[42 - 1] = (void*) 0xDEAD;

		if (atexit(memleak_crutch_atexit)) {
			perror("atexit(memleak_crutch)");
			abort();
		}
	}

	for(i = 0; memleak_crutch[i] != (void*)0xDEAD; ++i)
		if (! memleak_crutch[i]) {
			memleak_crutch[i] = p;
			LDAP_ENSURE(pthread_mutex_unlock(&memleak_crutch_lock) == 0);
			return;
		}

	memleak_crutch = ch_realloc(memleak_crutch, (i * 2 + 1) * sizeof(void*));
	memset(memleak_crutch + i, 0, i * sizeof(void*));
	memleak_crutch[i] = p;
	memleak_crutch[i*2] = (void*) 0xDEAD;
	LDAP_ENSURE(pthread_mutex_unlock(&memleak_crutch_lock) == 0);
}

void memleak_crutch_pop(void *p)
{
	int i;
	assert(p && p != (void*) 0xDEAD);
	assert(memleak_crutch != NULL);

	LDAP_ENSURE(pthread_mutex_lock(&memleak_crutch_lock) == 0);
	if (memleak_crutch) {
		for(i = 0; memleak_crutch[i] != (void*) 0xDEAD; ++i)
			if (memleak_crutch[i] == p) {
				memleak_crutch[i] = NULL;
				break;
			}
	}
	LDAP_ENSURE(pthread_mutex_unlock(&memleak_crutch_lock) == 0);
}

#ifdef __SANITIZE_THREAD__

ATTRIBUTE_NO_SANITIZE_THREAD
int get_op_abandon(const struct Operation *op)
{
	return op->_o_abandon;
}

ATTRIBUTE_NO_SANITIZE_THREAD
int get_op_cancel(const struct Operation *op)
{
	return op->_o_cancel;
}

ATTRIBUTE_NO_SANITIZE_THREAD
void set_op_abandon(struct Operation *op, int v)
{
	op->_o_abandon = v;
}

ATTRIBUTE_NO_SANITIZE_THREAD
void set_op_cancel(struct Operation *op, int v)
{
	op->_o_cancel = v;
}

ATTRIBUTE_NO_SANITIZE_THREAD
int read_int__tsan_workaround(volatile int *ptr) {
		return *ptr;
}

ATTRIBUTE_NO_SANITIZE_THREAD
char read_char__tsan_workaround(volatile char *ptr) {
		return *ptr;
}

ATTRIBUTE_NO_SANITIZE_THREAD
void* read_ptr__tsan_workaround(void *ptr) {
		return *(void * volatile *)ptr;
}

#endif /* __SANITIZE_THREAD__ */

ATTRIBUTE_NO_SANITIZE_THREAD
void op_copy(const volatile Operation *src, Operation *op, Opheader *hdr, BackendDB *be)
{
	BackendDB* bd;
	BackendInfo *bi;

retry:
	bd = src->o_bd;
	compiler_barrier();
	bi = bd->bd_info;
	compiler_barrier();
	*op = *src;
	compiler_barrier();
	if (be) {
		*be = *bd;
		compiler_barrier();
		op->o_bd = be;
	}
	if (hdr) {
		*hdr = *src->o_hdr;
		compiler_barrier();
		op->o_hdr = hdr;
	}
	if (unlikely(bd != src->o_bd))
		goto retry;
	if (unlikely(bi != bd->bd_info))
		goto retry;
	if (be && unlikely(bi != be->bd_info))
		goto retry;

	op->o_callback = NULL;
}

ldap_pvt_thread_mutex_t* __scoped_lock(ldap_pvt_thread_mutex_t *m) {
	if (likely(m))
		LDAP_ENSURE(pthread_mutex_lock(m) == 0);
	return m;
}

void __scoped_unlock(lock_holder_t *lh) {
	if (lh->lh_mutex) {
		LDAP_ENSURE(pthread_mutex_unlock(lh->lh_mutex) == 0);
		lh->lh_mutex = NULL;
	}
}

ldap_pvt_thread_mutex_t* __op_scoped_lock(const Operation *op) {
	static ldap_pvt_thread_mutex_t forest_of_crutch
			[ 17 /* LY: the prime number, it is important! */ ] = {
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
	};

	size_t n = ((unsigned)((size_t) op)) % 17;
	ldap_pvt_thread_mutex_t *m = &forest_of_crutch[n];
	LDAP_ENSURE(pthread_mutex_lock(m) == 0);
	return m;
}
