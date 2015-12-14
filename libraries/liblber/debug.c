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

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

static pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *log_file = NULL;
static int debug_lastc = '\n';

int lutil_debug_file( FILE *file )
{
	LDAP_ENSURE(pthread_mutex_lock(&debug_mutex) == 0);
	log_file = file;
	ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, file );
	LDAP_ENSURE(pthread_mutex_unlock(&debug_mutex) == 0);

	return 0;
}

void lutil_debug_print( const char *fmt, ... )
{
	va_list vl;

	va_start( vl, fmt );
	lutil_debug_va( fmt, vl);
	va_end( vl );
}

void lutil_debug( int debug, int level, const char *fmt, ... )
{
	if ( level & debug ) {
		va_list vl;

		va_start( vl, fmt );
		lutil_debug_va( fmt, vl);
		va_end( vl );
	}
}

void lutil_debug_va( const char* fmt, va_list vl )
{
	char buffer[4096];
	int len, off = 0;

	LDAP_ENSURE(pthread_mutex_lock(&debug_mutex) == 0);

#ifdef HAVE_WINSOCK
	if( log_file == NULL ) {
		log_file = fopen( LDAP_RUNDIR LDAP_DIRSEP "openldap.log", "w" );

		if ( log_file == NULL ) {
			log_file = fopen( "openldap.log", "w" );
			if ( log_file == NULL ) return;
		}

		ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, log_file );
	}
#endif

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
	LDAP_ENSURE(pthread_mutex_unlock(&debug_mutex) == 0);
}

#if defined(HAVE_EBCDIC) && defined(LDAP_SYSLOG)
#undef syslog
void eb_syslog( int pri, const char *fmt, ... )
{
	char buffer[4096];
	va_list vl;

	va_start( vl, fmt );
	vsnprintf( buffer, sizeof(buffer), fmt, vl );
	buffer[sizeof(buffer)-1] = '\0';

	/* The syslog function appears to only work with pure EBCDIC */
	__atoe(buffer);
#pragma convlit(suspend)
	syslog( pri, "%s", buffer );
#pragma convlit(resume)
	va_end( vl );
}
#endif
