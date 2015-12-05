/* globals.c - various global variables */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2015 The OpenLDAP Foundation.
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

#endif /* __SANITIZE_THREAD__ */
