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

#ifndef LDAP_LOG_H
#define LDAP_LOG_H

#include <stdio.h>
#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

/*
 * debug reporting levels.
 *
 * They start with the syslog levels, and
 * go down in importance.  The normal
 * debugging levels begin with LDAP_LEVEL_ENTRY
 *
 */

/* map syslog onto LDAP severity levels */
#ifdef LOG_DEBUG
#	define LDAP_LEVEL_EMERG	LOG_EMERG
#	define LDAP_LEVEL_ALERT	LOG_ALERT
#	define LDAP_LEVEL_CRIT		LOG_CRIT
#	define LDAP_LEVEL_ERR		LOG_ERR
#	define LDAP_LEVEL_WARNING	LOG_WARNING
#	define LDAP_LEVEL_NOTICE	LOG_NOTICE
#	define LDAP_LEVEL_INFO		LOG_INFO
#	define LDAP_LEVEL_DEBUG	LOG_DEBUG
#else /* ! LOG_DEBUG */
#	define LDAP_LEVEL_EMERG	(0)
#	define LDAP_LEVEL_ALERT	(1)
#	define LDAP_LEVEL_CRIT		(2)
#	define LDAP_LEVEL_ERR		(3)
#	define LDAP_LEVEL_WARNING	(4)
#	define LDAP_LEVEL_NOTICE	(5)
#	define LDAP_LEVEL_INFO		(6)
#	define LDAP_LEVEL_DEBUG	(7)
#endif /* ! LOG_DEBUG */

#if 0 /* (yet) unused */
#	define LDAP_LEVEL_ENTRY	(0x08)	/* log function entry points */
#	define LDAP_LEVEL_ARGS		(0x10)	/* log function call parameters */
#	define LDAP_LEVEL_RESULTS	(0x20)	/* Log function results */
#	define LDAP_LEVEL_DETAIL1	(0x40)	/* log level 1 function operational details */
#	define LDAP_LEVEL_DETAIL2	(0x80)	/* Log level 2 function operational details */
	/* in case we need to reuse the unused bits of severity */
#	define	LDAP_LEVEL_MASK(s)	((s) & 0x7)
#else
#	define	LDAP_LEVEL_MASK(s)	(s)
#endif

/* original subsystem selection mechanism */
#define LDAP_DEBUG_TRACE	0x0001
#define LDAP_DEBUG_PACKETS	0x0002
#define LDAP_DEBUG_ARGS		0x0004
#define LDAP_DEBUG_CONNS	0x0008
#define LDAP_DEBUG_BER		0x0010
#define LDAP_DEBUG_FILTER	0x0020
#define LDAP_DEBUG_CONFIG	0x0040
#define LDAP_DEBUG_ACL		0x0080
#define LDAP_DEBUG_STATS	0x0100
#define LDAP_DEBUG_STATS2	0x0200
#define LDAP_DEBUG_SHELL	0x0400
#define LDAP_DEBUG_PARSE	0x0800
#define LDAP_DEBUG_CACHE	0x1000
/* no longer used (nor supported)
#define LDAP_DEBUG_INDEX	0x2000
 */
#define LDAP_DEBUG_SYNC		0x4000
#define LDAP_DEBUG_NONE		0x8000
#define LDAP_DEBUG_ANY		(-1)

#ifndef LDAP_DEBUG
#	define ldap_debug_mask (0)
#	define slap_debug_mask (0)
#elif defined(SLAP_INSIDE)
	/* LY: slapd debug level, controlled only by '-d' command line option */
	LDAP_SLAPD_V (int) slap_debug_mask;
#	define ldap_debug_mask slap_debug_mask
#elif defined(RELDAP_LIBRARY)
	/* LY: libreldap debug level, controlled by ldap_set_option(LDAP_OPT_DEBUG_LEVEL) */
	LDAP_V ( struct ldapoptions ) ldap_int_global_options;
#	define ldap_debug_mask (ldap_int_global_options.ldo_debug)
#else /* ! SLAP_INSIDE && ! RELDAP_LIBRARY */
	/* This struct matches the head of ldapoptions in <ldap-int.h> */
	struct ldapoptions_prefix {
		   short	ldo_valid;
		   int		ldo_debug;
	};
	struct ldapoptions;
	LDAP_V ( struct ldapoptions ) ldap_int_global_options;
#	define ldap_debug_mask \
		(*(int *) ((char *)&ldap_int_global_options \
			+ offsetof(struct ldapoptions_prefix, ldo_debug)))
#endif /* LDAP_DEBUG */

#if defined(LDAP_SYSLOG) && defined(SLAP_INSIDE)
	/* LY: syslog debug level, controlled by 'loglevel' in slapd.conf */
	LDAP_SLAPD_V(int) slap_syslog_mask;
	/* LY: syslog severity for debug messages, controlled by 'syslog-severity' in slapd.conf */
	LDAP_SLAPD_V(int) slap_syslog_severity;
#else
#	define slap_syslog_mask (0)
#	define slap_syslog_severity (0)
#endif /* LDAP_SYSLOG */

#if defined(LDAP_DEBUG) && (defined(LDAP_SYSLOG) && defined(SLAP_INSIDE))
#	define Log( level, severity, ... ) do { \
		if ( ldap_debug_mask & (level) ) \
			ldap_debug_print( __VA_ARGS__ ); \
		if ( slap_syslog_mask & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), __VA_ARGS__ ); \
	} while ( 0 )
#elif ! defined(LDAP_DEBUG) && (defined(LDAP_SYSLOG) && defined(SLAP_INSIDE))
#	define Log( level, severity, ... ) do { \
		if ( slap_syslog_mask & (level) ) \
			syslog( LDAP_LEVEL_MASK((severity)), __VA_ARGS__ ); \
	} while ( 0 )
#else
#	define Log( level, severity, ... ) do { \
			if ( ldap_debug_mask & (level) ) \
				ldap_debug_print( __VA_ARGS__ ); \
		} while ( 0 )
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

#define Debug( level, ... )	\
	Log( (level), slap_syslog_severity, __VA_ARGS__ )

#define DebugTest(level) \
	( ( ldap_debug_mask | slap_syslog_mask ) & (level) )

/* debugging stuff */
LDAP_LUTIL_F(int) ldap_debug_file( FILE *file );

LDAP_LUTIL_F(void) ldap_debug_log (
	int debug, int level,
	const char* fmt, ... ) __attribute__((format(printf, 3, 4)));

LDAP_LUTIL_F(void) ldap_debug_print (
	const char* fmt, ... ) __attribute__((format(printf, 1, 2)));

LDAP_LUTIL_F(void) ldap_debug_va (
	const char* fmt, va_list args );

LDAP_LUTIL_F(void) ldap_debug_lock(void);
LDAP_LUTIL_F(int) ldap_debug_trylock(void);
LDAP_LUTIL_F(void) ldap_debug_unlock(void);
LDAP_LUTIL_F(void) ldap_debug_flush(void);
LDAP_LUTIL_F(void) ldap_debug_set_autoflush(int enable);

LDAP_END_DECL

#endif /* LDAP_LOG_H */
