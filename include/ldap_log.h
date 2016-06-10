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
/* Portions Copyright (c) 1990 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
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

/*
 * The "OLD_DEBUG" means that all logging occurs at LOG_DEBUG
 */

#ifdef OLD_DEBUG
/* original behavior: all logging occurs at the same severity level */
#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
#define LDAP_LEVEL_EMERG	ldap_syslog_level
#define LDAP_LEVEL_ALERT	ldap_syslog_level
#define LDAP_LEVEL_CRIT		ldap_syslog_level
#define LDAP_LEVEL_ERR		ldap_syslog_level
#define LDAP_LEVEL_WARNING	ldap_syslog_level
#define LDAP_LEVEL_NOTICE	ldap_syslog_level
#define LDAP_LEVEL_INFO		ldap_syslog_level
#define LDAP_LEVEL_DEBUG	ldap_syslog_level
#else /* !LDAP_DEBUG || !LDAP_SYSLOG */
#define LDAP_LEVEL_EMERG	(7)
#define LDAP_LEVEL_ALERT	(7)
#define LDAP_LEVEL_CRIT		(7)
#define LDAP_LEVEL_ERR		(7)
#define LDAP_LEVEL_WARNING	(7)
#define LDAP_LEVEL_NOTICE	(7)
#define LDAP_LEVEL_INFO		(7)
#define LDAP_LEVEL_DEBUG	(7)
#endif /* !LDAP_DEBUG || !LDAP_SYSLOG */

#else /* ! OLD_DEBUG */
/* map syslog onto LDAP severity levels */
#ifdef LOG_DEBUG
#define LDAP_LEVEL_EMERG	LOG_EMERG
#define LDAP_LEVEL_ALERT	LOG_ALERT
#define LDAP_LEVEL_CRIT		LOG_CRIT
#define LDAP_LEVEL_ERR		LOG_ERR
#define LDAP_LEVEL_WARNING	LOG_WARNING
#define LDAP_LEVEL_NOTICE	LOG_NOTICE
#define LDAP_LEVEL_INFO		LOG_INFO
#define LDAP_LEVEL_DEBUG	LOG_DEBUG
#else /* ! LOG_DEBUG */
#define LDAP_LEVEL_EMERG	(0)
#define LDAP_LEVEL_ALERT	(1)
#define LDAP_LEVEL_CRIT		(2)
#define LDAP_LEVEL_ERR		(3)
#define LDAP_LEVEL_WARNING	(4)
#define LDAP_LEVEL_NOTICE	(5)
#define LDAP_LEVEL_INFO		(6)
#define LDAP_LEVEL_DEBUG	(7)
#endif /* ! LOG_DEBUG */
#endif /* ! OLD_DEBUG */
#if 0
/* in case we need to reuse the unused bits of severity */
#define	LDAP_LEVEL_MASK(s)	((s) & 0x7)
#else
#define	LDAP_LEVEL_MASK(s)	(s)
#endif

/* (yet) unused */
#define LDAP_LEVEL_ENTRY	(0x08)	/* log function entry points */
#define LDAP_LEVEL_ARGS		(0x10)	/* log function call parameters */
#define LDAP_LEVEL_RESULTS	(0x20)	/* Log function results */
#define LDAP_LEVEL_DETAIL1	(0x40)	/* log level 1 function operational details */
#define LDAP_LEVEL_DETAIL2	(0x80)	/* Log level 2 function operational details */
/* end of (yet) unused */

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
#if 0 /* no longer used (nor supported) */
#define LDAP_DEBUG_CACHE	0x1000
#define LDAP_DEBUG_INDEX	0x2000
#endif
#define LDAP_DEBUG_SYNC		0x4000

#define LDAP_DEBUG_NONE		0x8000
#define LDAP_DEBUG_ANY		(-1)

/* debugging stuff */
#ifdef LDAP_DEBUG
    /*
     * This is a bogus extern declaration for the compiler. No need to ensure
     * a 'proper' dllimport.
     */
#ifndef ldap_debug
LDAP_V(int)	ldap_debug;
#endif /* !ldap_debug */

#endif /* LDAP_DEBUG */

#ifdef LDAP_SYSLOG
LDAP_V(int)	ldap_syslog;
LDAP_V(int)	ldap_syslog_level;

#ifdef HAVE_EBCDIC
#define syslog	_ldap_eb_syslog
LDAP_F(void) _ldap_eb_syslog(int pri, const char *fmt, ...);
#endif /* HAVE_EBCDIC */

#endif /* LDAP_SYSLOG */

/* this doesn't below as part of ldap.h */
#ifdef LDAP_SYSLOG
#	ifdef LDAP_DEBUG
#		define Log( level, severity, ... )	\
			do { \
				if ( ldap_debug & (level) ) \
					ldap_debug_print( __VA_ARGS__ ); \
				if ( ldap_syslog & (level) ) \
					syslog( LDAP_LEVEL_MASK((severity)), __VA_ARGS__ ); \
			} while ( 0 )

#		define LogTest(level) ( ( ldap_debug | ldap_syslog ) & (level) )
#	else /* ! LDAP_DEBUG */
#		define Log( level, severity, ... )	\
			do { \
				if ( ldap_syslog & (level) ) \
					syslog( LDAP_LEVEL_MASK((severity)), __VA_ARGS__ ); \
			} while ( 0 )
#		define LogTest(level) ( ldap_syslog & (level) )
#	endif /* ! LDAP_DEBUG */
#else /* ! LDAP_SYSLOG */
#	ifdef LDAP_DEBUG
#		define Log( level, severity, ... )	\
			do { \
				if ( ldap_debug & (level) ) \
					ldap_debug_print( __VA_ARGS__ ); \
			} while ( 0 )
#		define LogTest(level) ( ldap_debug & (level) )
#	else /* ! LDAP_DEBUG */
		/* TODO: in case LDAP_DEBUG is undefined, make sure logs with appropriate
		 * severity gets thru anyway */
#		define Log( level, severity, fmt, ... ) ((void)0)
#		define LogTest(level) ( 0 )
#	endif /* ! LDAP_DEBUG */
#endif /* ! LDAP_SYSLOG */

#define Debug( level, ... )	\
	Log( (level), ldap_syslog_level, __VA_ARGS__ )

/* Actually now in libreldap/debug.c */
LDAP_LUTIL_F(int) ldap_debug_file LDAP_P(( FILE *file ));

LDAP_LUTIL_F(void) ldap_debug_log LDAP_P((
	int debug, int level,
	const char* fmt, ... )) LDAP_GCCATTR((format(printf, 3, 4)));

LDAP_LUTIL_F(void) ldap_debug_print LDAP_P((
	const char* fmt, ... )) LDAP_GCCATTR((format(printf, 1, 2)));

LDAP_LUTIL_F(void) ldap_debug_va LDAP_P((
	const char* fmt, va_list args ));

LDAP_LUTIL_F(void) ldap_debug_lock(void);
LDAP_LUTIL_F(int) ldap_debug_trylock(void);
LDAP_LUTIL_F(void) ldap_debug_unlock(void);

#ifdef LDAP_DEFINE_LDAP_DEBUG
/* This struct matches the head of ldapoptions in <ldap-int.h> */
struct ldapoptions_prefix {
	short	ldo_valid;
	int		ldo_debug;
};
#define ldap_debug \
	(*(int *) ((char *)&ldap_int_global_options \
		 + offsetof(struct ldapoptions_prefix, ldo_debug)))

struct ldapoptions;
LDAP_V ( struct ldapoptions ) ldap_int_global_options;
#endif /* LDAP_DEFINE_LDAP_DEBUG */

LDAP_END_DECL

#endif /* LDAP_LOG_H */
