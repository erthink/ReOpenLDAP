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

/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "slap.h"
#include "shell.h"

pid_t
forkandexec(
    char	**args,
    FILE	**rfp,
    FILE	**wfp
)
{
	int	p2c[2] = { -1, -1 }, c2p[2];
	pid_t	pid;

	if ( pipe( p2c ) != 0 || pipe( c2p ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "pipe failed\n" );
		close( p2c[0] );
		close( p2c[1] );
		return( -1 );
	}

	/*
	 * what we're trying to set up looks like this:
	 *	parent *wfp -> p2c[1] | p2c[0] -> stdin child
	 *	parent *rfp <- c2p[0] | c2p[1] <- stdout child
	 */

	fflush( NULL );
# ifdef HAVE_THR
	pid = fork1();
# else
	pid = fork();
# endif
	if ( pid == 0 ) {		/* child */
		/*
		 * child could deadlock here due to resources locked
		 * by our parent
		 *
		 * If so, configure --without-threads.
		 */
		if ( dup2( p2c[0], 0 ) == -1 || dup2( c2p[1], 1 ) == -1 ) {
			Debug( LDAP_DEBUG_ANY, "dup2 failed\n" );
			exit( EXIT_FAILURE );
		}
	}
	close( p2c[0] );
	close( c2p[1] );
	if ( pid <= 0 ) {
		close( p2c[1] );
		close( c2p[0] );
	}
	switch ( pid ) {
	case 0:
		execv( args[0], args );

		Debug( LDAP_DEBUG_ANY, "execv failed\n" );
		exit( EXIT_FAILURE );

	case -1:	/* trouble */
		Debug( LDAP_DEBUG_ANY, "fork failed\n" );
		return( -1 );
	}

	/* parent */
	if ( (*rfp = fdopen( c2p[0], "r" )) == NULL || (*wfp = fdopen( p2c[1],
	    "w" )) == NULL ) {
		Debug( LDAP_DEBUG_ANY, "fdopen failed\n" );
		if ( *rfp ) {
			fclose( *rfp );
			*rfp = NULL;
		} else {
			close( c2p[0] );
		}
		close( p2c[1] );

		return( -1 );
	}

	return( pid );
}
