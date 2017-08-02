/* $ReOpenLDAP$ */
/* Copyright 1990-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* globals.c - various global variables */

#include "reldap.h"

#include <ac/string.h>
#include <ac/stdarg.h>
#include "lber_pvt.h"

#include "slap.h"
#include "ldap_log.h"
#include "ldif.h"

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
int slap_get_op_abandon(const struct Operation *op)
{
	return op->_o_abandon;
}

ATTRIBUTE_NO_SANITIZE_THREAD
int slap_get_op_cancel(const struct Operation *op)
{
	return op->_o_cancel;
}

ATTRIBUTE_NO_SANITIZE_THREAD
void slap_set_op_abandon(struct Operation *op, int v)
{
	op->_o_abandon = v;
}

ATTRIBUTE_NO_SANITIZE_THREAD
void slap_set_op_cancel(struct Operation *op, int v)
{
	op->_o_cancel = v;
}

ATTRIBUTE_NO_SANITIZE_THREAD
int slap_tsan__read_int(volatile int *ptr) {
		return *ptr;
}

ATTRIBUTE_NO_SANITIZE_THREAD
char slap_tsan__read_char(volatile char *ptr) {
		return *ptr;
}

ATTRIBUTE_NO_SANITIZE_THREAD
void* slap_tsan__read_ptr(void *ptr) {
		return *(void * volatile *)ptr;
}

#endif /* __SANITIZE_THREAD__ */

ATTRIBUTE_NO_SANITIZE_THREAD
void slap_op_copy(const volatile Operation *src, Operation *op, Opheader *hdr, BackendDB *be)
{
	BackendDB* bd;
	slap_mask_t	flags;
	BackendInfo *bi;

	/* LY: solve races with over_op_func() */
retry:
	bd = src->o_bd;
	flags = bd->be_flags;
	bi = bd->bd_info;
	compiler_barrier();
	*op = *src;
	if (hdr) {
		*hdr = *src->o_hdr;
		op->o_hdr = hdr;
	}
	if (be) {
		*be = *bd;
		op->o_bd = be;
	}
	compiler_barrier();
	if (unlikely(bd != src->o_bd))
		goto retry;
	if (unlikely(bi != bd->bd_info || flags != bd->be_flags))
		goto retry;
	if (unlikely(bi != op->o_bd->bd_info || flags != op->o_bd->be_flags))
		goto retry;

	op->o_callback = NULL;
}

/* LY: override weaks from libreldap */
void ber_error_print( LDAP_CONST char *str )
{
	Debug(LDAP_DEBUG_BER, "%s", str);
}

LDAP_SLAPD_F(void) __ldap_assert_fail(
		const char* assertion,
		const char* file,
		unsigned line,
		const char* function)
{
	slap_backtrace_debug();
	__assert_fail(assertion, file, line, function);
}

void slap_set_debug_level(int mask)
{
#ifdef LDAP_DEBUG
	ldap_debug_lock();
	slap_debug_mask = mask;
	ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &mask);
	ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &mask);
	ldif_debug = mask;
	ldap_debug_unlock();
#endif /* LDAP_DEBUG */
}
