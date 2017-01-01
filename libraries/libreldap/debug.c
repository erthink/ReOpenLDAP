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

#include "reldap.h"

#include <stdio.h>

#include <ac/stdarg.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/ctype.h>

#ifdef LDAP_SYSLOG
#include <ac/syslog.h>
#endif

#include "ldap_log.h"
#include "ldap_defaults.h"
#include "lber.h"
#include "ldap_pvt.h"
#include "ldap-int.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <errno.h>

static pthread_mutex_t debug_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static FILE *log_file = NULL;
static int debug_lastc = '\n';

void ldap_debug_lock(void) {
	LDAP_ENSURE(pthread_mutex_lock(&debug_mutex) == 0);
}

int ldap_debug_trylock(void) {
	int rc = pthread_mutex_trylock(&debug_mutex);
	LDAP_ENSURE(rc == 0 || rc == EBUSY);
	return rc;
}

void ldap_debug_unlock(void) {
	LDAP_ENSURE(pthread_mutex_unlock(&debug_mutex) == 0);
}

int ldap_debug_file( FILE *file )
{
	ldap_debug_lock();
	log_file = file;
	ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, file );
	ldap_debug_unlock();

	return 0;
}

void ldap_debug_print( const char *fmt, ... )
{
	va_list vl;

	va_start( vl, fmt );
	ldap_debug_va( fmt, vl);
	va_end( vl );
}

void ldap_debug_va( const char* fmt, va_list vl )
{
	char buffer[4096];
	int len, off = 0;

	ldap_debug_lock();
	if (debug_lastc == '\n') {
		struct timeval now;
		struct tm tm;
		long rc;

		ldap_timeval(&now);
		/* LY: it is important to don't use extra spaces here, to avoid break a test(s). */
		gmtime_r(&now.tv_sec, &tm);
		off += strftime(buffer+off, sizeof(buffer)-off, "%y%m%d-%H:%M:%S", &tm);
		assert(off > 0);
		rc = syscall(SYS_gettid, NULL, NULL, NULL);
		assert(rc > 0);
		off += snprintf(buffer+off, sizeof(buffer)-off, ".%06ld_%05ld ", now.tv_usec, rc);
		assert(off > 0);
	}
	len = vsnprintf( buffer+off, sizeof(buffer)-off, fmt, vl );
	if (len > sizeof(buffer)-off)
		len = sizeof(buffer)-off;
	debug_lastc = buffer[len+off-1];
	buffer[sizeof(buffer)-1] = '\0';
	if( log_file != NULL ) {
		fputs( buffer, log_file );
		fflush( log_file );
	}
	fputs( buffer, stderr );
	ldap_debug_unlock();
}

void ldap_debug_perror( LDAP *ld, LDAP_CONST char *str )
{
	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( str != NULL );

	ldap_debug_lock();

	ldap_debug_print( "%s: %s (%d)\n",
		str ? str : "ldap_debug_perror",
		ldap_err2string( ld->ld_errno ),
		ld->ld_errno );

	if ( ld->ld_matched != NULL && ld->ld_matched[0] != '\0' ) {
		ldap_debug_print( "\tmatched DN: %s\n", ld->ld_matched );
	}

	if ( ld->ld_error != NULL && ld->ld_error[0] != '\0' ) {
		ldap_debug_print( "\tadditional info: %s\n", ld->ld_error );
	}

	if ( ld->ld_referrals != NULL && ld->ld_referrals[0] != NULL) {
		int i;
		ldap_debug_print( "\treferrals:\n" );
		for (i = 0; ld->ld_referrals[i]; i++) {
			ldap_debug_print( "\t\t%s\n", ld->ld_referrals[i] );
		}
	}

	ldap_debug_unlock();
	fflush( stderr );
}
