/* posix.c - wrapper around posix and posixish thread implementations.  */
/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "reldap.h"

#if defined( HAVE_PTHREADS )

#include <ac/errno.h>

#ifdef REPLACE_BROKEN_YIELD
#ifndef HAVE_NANOSLEEP
#include <ac/socket.h>
#endif
#include <ac/time.h>
#endif

#include "ldap_pvt_thread.h" /* Get the thread interface */
#include <signal.h>			 /* For pthread_kill() */
#include <unistd.h>
#include <sched.h>

extern int ldap_int_stackguard;

#if HAVE_PTHREADS < 6
#  define LDAP_INT_THREAD_ATTR_DEFAULT		pthread_attr_default
#  define LDAP_INT_THREAD_CONDATTR_DEFAULT	pthread_condattr_default
#  define LDAP_INT_THREAD_MUTEXATTR_DEFAULT	pthread_mutexattr_default
#else
#  define LDAP_INT_THREAD_ATTR_DEFAULT		NULL
#  define LDAP_INT_THREAD_CONDATTR_DEFAULT	NULL
#  define LDAP_INT_THREAD_MUTEXATTR_DEFAULT NULL
#endif

#if HAVE_PTHREADS < 7
#define ERRVAL(val)	((val) < 0 ? errno : 0)
#else
#define ERRVAL(val)	(val)
#endif

static pthread_mutexattr_t mutex_attr_errorcheck;

static pthread_mutexattr_t* ldap_mutex_attr() {
	if (reopenldap_mode_check())
		return &mutex_attr_errorcheck;
	return LDAP_INT_THREAD_MUTEXATTR_DEFAULT;
}

int
ldap_int_thread_initialize( void )
{
	LDAP_ENSURE( pthread_mutexattr_init( &mutex_attr_errorcheck ) == 0 );
	LDAP_ENSURE( pthread_mutexattr_settype( &mutex_attr_errorcheck, PTHREAD_MUTEX_ERRORCHECK ) == 0);
	return 0;
}

int
ldap_int_thread_destroy( void )
{
#ifdef HAVE_PTHREAD_KILL_OTHER_THREADS_NP
	/* LinuxThreads: kill clones */
	pthread_kill_other_threads_np();
#endif
	LDAP_ENSURE( pthread_mutexattr_destroy( &mutex_attr_errorcheck ) == 0);
	return 0;
}

#ifdef LDAP_THREAD_HAVE_SETCONCURRENCY
int
ldap_pvt_thread_set_concurrency(int n)
{
#ifdef HAVE_PTHREAD_SETCONCURRENCY
	return pthread_setconcurrency( n );
#elif defined(HAVE_THR_SETCONCURRENCY)
	return thr_setconcurrency( n );
#else
	return 0;
#endif
}
#endif

#ifdef LDAP_THREAD_HAVE_GETCONCURRENCY
int
ldap_pvt_thread_get_concurrency(void)
{
#ifdef HAVE_PTHREAD_GETCONCURRENCY
	return pthread_getconcurrency();
#elif defined(HAVE_THR_GETCONCURRENCY)
	return thr_getconcurrency();
#else
	return 0;
#endif
}
#endif

/* detachstate appeared in Draft 6, but without manifest constants.
 * in Draft 7 they were called PTHREAD_CREATE_UNDETACHED and ...DETACHED.
 * in Draft 8 on, ...UNDETACHED became ...JOINABLE.
 */
#ifndef PTHREAD_CREATE_JOINABLE
#ifdef PTHREAD_CREATE_UNDETACHED
#define	PTHREAD_CREATE_JOINABLE	PTHREAD_CREATE_UNDETACHED
#else
#define	PTHREAD_CREATE_JOINABLE	0
#endif
#endif

#ifndef PTHREAD_CREATE_DETACHED
#define	PTHREAD_CREATE_DETACHED	1
#endif

int
ldap_pvt_thread_create( ldap_pvt_thread_t * thread,
	int detach,
	void *(*start_routine)( void * ),
	void *arg)
{
	int rtn;
	pthread_attr_t attr;

/* Always create the thread attrs, so we can set stacksize if we need to */
#if HAVE_PTHREADS > 5
	pthread_attr_init(&attr);
#else
	pthread_attr_create(&attr);
#endif

#ifdef LDAP_PVT_THREAD_SET_STACK_SIZE
	/* this should be tunable */
	pthread_attr_setstacksize( &attr, LDAP_PVT_THREAD_STACK_SIZE );
	if ( ldap_int_stackguard )
		pthread_attr_setguardsize( &attr, LDAP_PVT_THREAD_STACK_SIZE );
#endif

#if HAVE_PTHREADS > 5
	detach = detach ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
#if HAVE_PTHREADS == 6
	pthread_attr_setdetachstate(&attr, &detach);
#else
	pthread_attr_setdetachstate(&attr, detach);
#endif
#endif

#if HAVE_PTHREADS < 5
	rtn = pthread_create( thread, attr, start_routine, arg );
#else
	rtn = pthread_create( thread, &attr, start_routine, arg );
#endif

#if HAVE_PTHREADS > 5
	pthread_attr_destroy(&attr);
#else
	pthread_attr_delete(&attr);
	if( detach ) {
		pthread_detach( thread );
	}
#endif

#if HAVE_PTHREADS < 7
	if ( rtn < 0 ) rtn = errno;
#endif
	LDAP_JITTER(33);
	return rtn;
}

void
ldap_pvt_thread_exit( void *retval )
{
	LDAP_JITTER(33);
	pthread_exit( retval );
}

int
ldap_pvt_thread_join( ldap_pvt_thread_t thread, void **thread_return )
{
	LDAP_JITTER(33);
#if HAVE_PTHREADS < 7
	void *dummy;
	if (thread_return==NULL)
	  thread_return=&dummy;
#endif
	return ERRVAL( pthread_join( thread, thread_return ) );
}

int
ldap_pvt_thread_kill( ldap_pvt_thread_t thread, int signo )
{
	LDAP_JITTER(33);
#if defined(HAVE_PTHREAD_KILL) && HAVE_PTHREADS > 4
	/* MacOS 10.1 is detected as v10 but has no pthread_kill() */
	return ERRVAL( pthread_kill( thread, signo ) );
#else
	/* pthread package with DCE */
	if (kill( getpid(), signo )<0)
		return errno;
	return 0;
#endif
}

int
ldap_pvt_thread_yield( void )
{
#ifdef REPLACE_BROKEN_YIELD
#ifdef HAVE_NANOSLEEP
	struct timespec t = { 0, 0 };
	nanosleep(&t, NULL);
#else
	struct timeval tv = {0,0};
	select( 0, NULL, NULL, NULL, &tv );
#endif
	return 0;

#elif defined(HAVE_THR_YIELD)
	thr_yield();
	return 0;

#elif HAVE_PTHREADS == 10
	return sched_yield();

#elif defined(_POSIX_THREAD_IS_GNU_PTH)
	sched_yield();
	return 0;

#elif HAVE_PTHREADS == 6
	pthread_yield(NULL);
	return 0;

#else
	pthread_yield();
	return 0;
#endif
}

int
ldap_pvt_thread_cond_init( ldap_pvt_thread_cond_t *cond )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_cond_init( cond, LDAP_INT_THREAD_CONDATTR_DEFAULT ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_cond_destroy( ldap_pvt_thread_cond_t *cond )
{
	int rc;
	LDAP_JITTER(25);
	rc =  ERRVAL( pthread_cond_destroy( cond ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_cond_signal( ldap_pvt_thread_cond_t *cond )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_cond_signal( cond ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_cond_broadcast( ldap_pvt_thread_cond_t *cond )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_cond_broadcast( cond ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_cond_wait( ldap_pvt_thread_cond_t *cond,
		      ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_cond_wait( cond, mutex ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_cond_timedwait(
		unsigned timeout_ms,
		ldap_pvt_thread_cond_t *cond,
		ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	struct timespec abstime;

	LDAP_ENSURE(0 == clock_gettime(CLOCK_REALTIME_COARSE, &abstime));
	if (timeout_ms) {
		abstime.tv_sec += timeout_ms / 1000u;
		abstime.tv_nsec += (timeout_ms % 1000u) * 1000000ul;
	} else {
		static struct timespec rr_interval;
		if (! rr_interval.tv_nsec) {
			LDAP_ENSURE(0 == sched_rr_get_interval(getpid(), &rr_interval));
			LDAP_ENSURE(rr_interval.tv_sec == 0);
		}
		abstime.tv_nsec += rr_interval.tv_nsec;
	}
	if (abstime.tv_nsec >= 1000000000ul) {
		abstime.tv_sec += 1;
		abstime.tv_nsec -= 1000000000ul;
	}

	LDAP_JITTER(25);
	rc = pthread_cond_timedwait( cond, mutex, &abstime );
	LDAP_ENSURE(rc == 0 || rc == ETIMEDOUT || rc == EINTR);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_mutex_init( ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_mutex_init( mutex, ldap_mutex_attr() ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_mutex_destroy( ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_mutex_destroy( mutex ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_mutex_lock( ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_mutex_lock( mutex ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_mutex_trylock( ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_mutex_trylock( mutex ) );
	LDAP_ENSURE(rc == 0 || rc == EBUSY);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_mutex_unlock( ldap_pvt_thread_mutex_t *mutex )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_mutex_unlock( mutex ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

ldap_pvt_thread_t ldap_pvt_thread_self( void )
{
	return pthread_self();
}

int
ldap_pvt_thread_key_create( ldap_pvt_thread_key_t *key )
{
	int rc;
	LDAP_JITTER(25);
	rc = pthread_key_create( key, NULL );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_key_destroy( ldap_pvt_thread_key_t key )
{
	int rc;
	LDAP_JITTER(25);
	rc = pthread_key_delete( key );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_key_setdata( ldap_pvt_thread_key_t key, void *data )
{
	int rc;
	LDAP_JITTER(25);
	rc = pthread_setspecific( key, data );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_key_getdata( ldap_pvt_thread_key_t key, void **data )
{
	LDAP_JITTER(25);
	*data = pthread_getspecific( key );
	LDAP_JITTER(25);
	return 0;
}

#ifdef LDAP_THREAD_HAVE_RDWR
#ifdef HAVE_PTHREAD_RWLOCK_DESTROY
int
ldap_pvt_thread_rdwr_init( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_init( rw, NULL ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int
ldap_pvt_thread_rdwr_destroy( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_destroy( rw ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int ldap_pvt_thread_rdwr_rlock( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_rdlock( rw ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int ldap_pvt_thread_rdwr_rtrylock( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_tryrdlock( rw ) );
	LDAP_ENSURE(rc == 0 || rc == EBUSY);
	LDAP_JITTER(25);
	return rc;
}

int ldap_pvt_thread_rdwr_runlock( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_unlock( rw ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int ldap_pvt_thread_rdwr_wlock( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_wrlock( rw ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

int ldap_pvt_thread_rdwr_wtrylock( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_trywrlock( rw ) );
	LDAP_ENSURE(rc == 0 || rc == EBUSY);
	LDAP_JITTER(25);
	return rc;
}

int ldap_pvt_thread_rdwr_wunlock( ldap_pvt_thread_rdwr_t *rw )
{
	int rc;
	LDAP_JITTER(25);
	rc = ERRVAL( pthread_rwlock_unlock( rw ) );
	LDAP_ENSURE(rc == 0);
	LDAP_JITTER(25);
	return rc;
}

#endif /* HAVE_PTHREAD_RWLOCK_DESTROY */
#endif /* LDAP_THREAD_HAVE_RDWR */
#endif /* HAVE_PTHREADS */
