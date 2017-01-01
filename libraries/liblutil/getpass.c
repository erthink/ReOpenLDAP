/* $ReOpenLDAP$ */
/* Copyright 1992-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* This work was originally developed by the University of Michigan
 * and distributed as part of U-MICH LDAP.  It was adapted for use in
 * -llutil by Kurt D. Zeilenga and subsequently rewritten by Howard Chu.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/string.h>
#include <ac/termios.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/localize.h>

#ifndef HAVE_GETPASSPHRASE

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_CONIO_H
#include <conio.h>
#endif

#include <lber.h>
#include <ldap.h>

#include "ldap_defaults.h"

#define PBUF	512

#define TTY "/dev/tty"

char *
lutil_getpass( const char *prompt )
{
	static char pbuf[PBUF];
	FILE *fi;
	int c;
	unsigned i;
#if defined(HAVE_TERMIOS_H) || defined(HAVE_SGTTY_H)
	TERMIO_TYPE ttyb;
	TERMFLAG_TYPE flags = 0;
	void (*sig)( int sig ) = NULL;
#endif

	if( prompt == NULL ) prompt = _("Password: ");

#ifdef DEBUG
	if (debug & D_TRACE)
		printf("->getpass(%s)\n", prompt);
#endif

#if defined(HAVE_TERMIOS_H) || defined(HAVE_SGTTY_H)
	if ((fi = fopen(TTY, "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	if (fi != stdin) {
		if (GETATTR(fileno(fi), &ttyb) < 0)
			perror("GETATTR");
		sig = SIGNAL (SIGINT, SIG_IGN);
		flags = GETFLAGS( ttyb );
		SETFLAGS( ttyb, flags & ~ECHO );
		if (SETATTR(fileno(fi), &ttyb) < 0)
			perror("SETATTR");
	}
#else
	fi = stdin;
#endif
	fprintf(stderr, "%s", prompt);
	fflush(stderr);
	i = 0;
	while ( (c = getc(fi)) != EOF && c != '\n' && c != '\r' )
		if ( i < (sizeof(pbuf)-1) )
			pbuf[i++] = c;
#if defined(HAVE_TERMIOS_H) || defined(HAVE_SGTTY_H)
	/* tidy up */
	if (fi != stdin) {
		fprintf(stderr, "\n");
		fflush(stderr);
		SETFLAGS( ttyb, flags );
		if (SETATTR(fileno(fi), &ttyb) < 0)
			perror("SETATTR");
		(void) SIGNAL (SIGINT, sig);
		(void) fclose(fi);
	}
#endif
	if ( c == EOF )
		return( NULL );
	pbuf[i] = '\0';
	return (pbuf);
}

#endif /* !NEED_GETPASSPHRASE */
