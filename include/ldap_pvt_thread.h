/* $ReOpenLDAP$ */
/* Copyright 1992-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#ifndef _LDAP_PVT_THREAD_H
#define _LDAP_PVT_THREAD_H /* libreldap/ldap_thr_debug.h #undefines this */

#include "ldap_cdefs.h"
#include "ldap_int_thread.h"

LDAP_BEGIN_DECL

#ifndef LDAP_PVT_THREAD_H_DONE
typedef ldap_int_thread_t			ldap_pvt_thread_t;
#ifdef LDAP_THREAD_DEBUG_WRAP
typedef ldap_debug_thread_mutex_t	ldap_pvt_thread_mutex_t;
typedef ldap_debug_thread_cond_t	ldap_pvt_thread_cond_t;
typedef ldap_debug_thread_rdwr_t	ldap_pvt_thread_rdwr_t;
#define LDAP_PVT_MUTEX_FIRSTCREATE	LDAP_DEBUG_MUTEX_FIRSTCREATE
#define LDAP_PVT_MUTEX_NULL			LDAP_DEBUG_MUTEX_NULL
#else
typedef ldap_int_thread_mutex_t		ldap_pvt_thread_mutex_t;
typedef ldap_int_thread_cond_t		ldap_pvt_thread_cond_t;
typedef ldap_int_thread_rdwr_t		ldap_pvt_thread_rdwr_t;
#define LDAP_PVT_MUTEX_FIRSTCREATE	LDAP_INT_MUTEX_FIRSTCREATE
#define LDAP_PVT_MUTEX_NULL			LDAP_INT_MUTEX_NULL
#endif
typedef ldap_int_thread_mutex_recursive_t ldap_pvt_thread_mutex_recursive_t;
typedef ldap_int_thread_key_t	ldap_pvt_thread_key_t;
#endif /* !LDAP_PVT_THREAD_H_DONE */

#define ldap_pvt_thread_equal		ldap_int_thread_equal

LDAP_F( int )
ldap_pvt_thread_initialize( void );

LDAP_F( int )
ldap_pvt_thread_destroy( void );

LDAP_F( unsigned int )
ldap_pvt_thread_sleep( unsigned int s );

LDAP_F( int )
ldap_pvt_thread_get_concurrency( void );

LDAP_F( int )
ldap_pvt_thread_set_concurrency( int );

#define LDAP_PVT_THREAD_CREATE_JOINABLE 0
#define LDAP_PVT_THREAD_CREATE_DETACHED 1

#ifndef LDAP_PVT_THREAD_H_DONE
#define	LDAP_PVT_THREAD_SET_STACK_SIZE
/* The size may be explicitly #defined to zero to disable it. */
#if defined( LDAP_PVT_THREAD_STACK_SIZE ) && LDAP_PVT_THREAD_STACK_SIZE == 0
#	undef LDAP_PVT_THREAD_SET_STACK_SIZE
#elif !defined( LDAP_PVT_THREAD_STACK_SIZE )
	/* LARGE stack. Will be twice as large on 64 bit machine. */
#	define LDAP_PVT_THREAD_STACK_SIZE ( 1 * 1024 * 1024 * sizeof(void *) )
#endif
#endif /* !LDAP_PVT_THREAD_H_DONE */

LDAP_F( int )
ldap_pvt_thread_create (
	ldap_pvt_thread_t * thread,
	int	detach,
	void *(*start_routine)( void * ),
	void *arg);

LDAP_F( void )
ldap_pvt_thread_exit( void *retval );

LDAP_F( int )
ldap_pvt_thread_join( ldap_pvt_thread_t thread, void **status );

LDAP_F( int )
ldap_pvt_thread_kill( ldap_pvt_thread_t thread, int signo );

LDAP_F( int )
ldap_pvt_thread_yield( void );

LDAP_F( int )
ldap_pvt_thread_cond_init( ldap_pvt_thread_cond_t *cond );

LDAP_F( int )
ldap_pvt_thread_cond_destroy( ldap_pvt_thread_cond_t *cond );

LDAP_F( int )
ldap_pvt_thread_cond_signal( ldap_pvt_thread_cond_t *cond );

LDAP_F( int )
ldap_pvt_thread_cond_broadcast( ldap_pvt_thread_cond_t *cond );

LDAP_F( int )
ldap_pvt_thread_cond_wait (
	ldap_pvt_thread_cond_t *cond,
	ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_cond_timedwait (
	unsigned timeout_ms,
	ldap_pvt_thread_cond_t *cond,
	ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_init( ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_recursive_init( ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_destroy( ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_recursive_destroy( ldap_pvt_thread_mutex_recursive_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_lock( ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_trylock( ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_unlock( ldap_pvt_thread_mutex_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_recursive_lock( ldap_pvt_thread_mutex_recursive_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_recursive_trylock( ldap_pvt_thread_mutex_recursive_t *mutex );

LDAP_F( int )
ldap_pvt_thread_mutex_recursive_unlock( ldap_pvt_thread_mutex_recursive_t *mutex );

LDAP_F( ldap_pvt_thread_t )
ldap_pvt_thread_self( void );

#ifdef	LDAP_INT_THREAD_ASSERT_MUTEX_OWNER
#define	LDAP_PVT_THREAD_ASSERT_MUTEX_OWNER LDAP_INT_THREAD_ASSERT_MUTEX_OWNER
#else
#define	LDAP_PVT_THREAD_ASSERT_MUTEX_OWNER(mutex) ((void) 0)
#endif

LDAP_F( int )
ldap_pvt_thread_rdwr_init(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_destroy(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_rlock(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_rtrylock(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_runlock(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_wlock(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_wtrylock(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_wunlock(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_key_create(ldap_pvt_thread_key_t *keyp);

LDAP_F( int )
ldap_pvt_thread_key_destroy(ldap_pvt_thread_key_t key);

LDAP_F( int )
ldap_pvt_thread_key_setdata(ldap_pvt_thread_key_t key, void *data);

LDAP_F( int )
ldap_pvt_thread_key_getdata(ldap_pvt_thread_key_t key, void **data);

#ifdef LDAP_DEBUG
LDAP_F( int )
ldap_pvt_thread_rdwr_readers(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_writers(ldap_pvt_thread_rdwr_t *rdwrp);

LDAP_F( int )
ldap_pvt_thread_rdwr_active(ldap_pvt_thread_rdwr_t *rdwrp);
#endif /* LDAP_DEBUG */

#define LDAP_PVT_THREAD_EINVAL EINVAL
#define LDAP_PVT_THREAD_EBUSY EINVAL

#ifndef LDAP_PVT_THREAD_H_DONE
typedef ldap_int_thread_pool_t ldap_pvt_thread_pool_t;

typedef void * (ldap_pvt_thread_start_t)(void *ctx, void *arg);
typedef void (ldap_pvt_thread_pool_keyfree_t)(void *key, void *data);
#endif /* !LDAP_PVT_THREAD_H_DONE */

LDAP_F( int )
ldap_pvt_thread_pool_init (
	ldap_pvt_thread_pool_t *pool_out,
	int max_threads,
	int max_pending );

LDAP_F( int )
ldap_pvt_thread_pool_submit (
	ldap_pvt_thread_pool_t *pool,
	ldap_pvt_thread_start_t *start,
	void *arg );

LDAP_F( int )
ldap_pvt_thread_pool_submit2 (
	ldap_pvt_thread_pool_t *pool,
	ldap_pvt_thread_start_t *start,
	void *arg,
	void **cookie );

LDAP_F( int )
ldap_pvt_thread_pool_retract (
	void *cookie );

LDAP_F( int )
ldap_pvt_thread_pool_maxthreads (
	ldap_pvt_thread_pool_t *pool,
	int max_threads );

LDAP_F( int )
ldap_pvt_thread_pool_queues (
	ldap_pvt_thread_pool_t *pool,
	int numqs );

#ifndef LDAP_PVT_THREAD_H_DONE
typedef enum {
	LDAP_PVT_THREAD_POOL_PARAM_UNKNOWN = -1,
	LDAP_PVT_THREAD_POOL_PARAM_MAX,
	LDAP_PVT_THREAD_POOL_PARAM_MAX_PENDING,
	LDAP_PVT_THREAD_POOL_PARAM_OPEN,
	LDAP_PVT_THREAD_POOL_PARAM_STARTING,
	LDAP_PVT_THREAD_POOL_PARAM_ACTIVE,
	LDAP_PVT_THREAD_POOL_PARAM_PAUSING,
	LDAP_PVT_THREAD_POOL_PARAM_PENDING,
	LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD,
	LDAP_PVT_THREAD_POOL_PARAM_ACTIVE_MAX,
	LDAP_PVT_THREAD_POOL_PARAM_PENDING_MAX,
	LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD_MAX,
	LDAP_PVT_THREAD_POOL_PARAM_STATE
} ldap_pvt_thread_pool_param_t;
#endif /* !LDAP_PVT_THREAD_H_DONE */

LDAP_F( int )
ldap_pvt_thread_pool_query (
	ldap_pvt_thread_pool_t *pool,
	ldap_pvt_thread_pool_param_t param, void *value );

LDAP_F( int )
ldap_pvt_thread_pool_pausing (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( int )
ldap_pvt_thread_pool_backload (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( void )
ldap_pvt_thread_pool_idle (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( void )
ldap_pvt_thread_pool_unidle (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( int )
ldap_pvt_thread_pool_pausecheck (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( int )
ldap_pvt_thread_pool_pause (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( int )
ldap_pvt_thread_pool_resume (
	ldap_pvt_thread_pool_t *pool );

LDAP_F( int )
ldap_pvt_thread_pool_destroy (
	ldap_pvt_thread_pool_t *pool,
	int run_pending );

LDAP_F( int )
ldap_pvt_thread_pool_getkey (
	void *ctx,
	void *key,
	void **data,
	ldap_pvt_thread_pool_keyfree_t **kfree );

LDAP_F( int )
ldap_pvt_thread_pool_setkey (
	void *ctx,
	void *key,
	void *data,
	ldap_pvt_thread_pool_keyfree_t *kfree,
	void **olddatap,
	ldap_pvt_thread_pool_keyfree_t **oldkfreep );

LDAP_F( void )
ldap_pvt_thread_pool_purgekey( void *key );

LDAP_F( void *)
ldap_pvt_thread_pool_context( void );

LDAP_F( void )
ldap_pvt_thread_pool_context_reset( void *key );

LDAP_F( ldap_pvt_thread_t )
ldap_pvt_thread_pool_tid( void *ctx );

LDAP_END_DECL

#define LDAP_PVT_THREAD_H_DONE
#endif /* _LDAP_PVT_THREAD_H */
