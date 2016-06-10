/* Generic socket.h */
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

#ifndef _AC_SOCKET_H_
#define _AC_SOCKET_H_

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif

#include <netdb.h>

#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif

#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_PCNFS
#include <tklib.h>
#endif /* HAVE_PCNFS */

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK	(0x7f000001UL)
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif

#undef	sock_errno
#undef	sock_errstr
#define sock_errno()	errno
#define sock_errstr(e)	STRERROR(e)
#define sock_errset(e)	((void) (errno = (e)))

#define tcp_read( s, buf, len)	read( s, buf, len )
#define tcp_write( s, buf, len)	write( s, buf, len )

#ifdef SHUT_RDWR
#	define tcp_close( s )	(shutdown( s, SHUT_RDWR ), close( s ))
#else
#	define tcp_close( s )	close( s )
#endif

#ifdef HAVE_PIPE
/*
 * Only use pipe() on systems where file and socket descriptors
 * are interchangable
 */
#	define USE_PIPE HAVE_PIPE
#endif

#ifndef ioctl_t
#	define ioctl_t				int
#endif

#ifndef AC_SOCKET_INVALID
#	define AC_SOCKET_INVALID	(-1)
#endif
#ifndef AC_SOCKET_ERROR
#	define AC_SOCKET_ERROR		(-1)
#endif

#if !defined( HAVE_INET_ATON ) && !defined( inet_aton )
#	define inet_aton ldap_pvt_inet_aton
struct in_addr;
LDAP_F (int) ldap_pvt_inet_aton LDAP_P(( const char *, struct in_addr * ));
#endif

#define AC_HTONL( l ) htonl( l )
#define AC_NTOHL( l ) ntohl( l )

/* htons()/ntohs() may be broken much like htonl()/ntohl() */
#define AC_HTONS( s ) htons( s )
#define AC_NTOHS( s ) ntohs( s )

#ifdef LDAP_PF_LOCAL
#  if !defined( AF_LOCAL ) && defined( AF_UNIX )
#    define AF_LOCAL	AF_UNIX
#  endif
#  if !defined( PF_LOCAL ) && defined( PF_UNIX )
#    define PF_LOCAL	PF_UNIX
#  endif
#endif

#ifndef INET_ADDRSTRLEN
#	define INET_ADDRSTRLEN 16
#endif
#ifndef INET6_ADDRSTRLEN
#	define INET6_ADDRSTRLEN 46
#endif

#if defined( HAVE_GETADDRINFO ) || defined( HAVE_GETNAMEINFO )
#	ifdef HAVE_GAI_STRERROR
#		define AC_GAI_STRERROR(x)	(gai_strerror((x)))
#	else
#		define AC_GAI_STRERROR(x)	(ldap_pvt_gai_strerror((x)))
		LDAP_F (char *) ldap_pvt_gai_strerror( int );
#	endif
#endif

#if defined(LDAP_PF_LOCAL) && \
	!defined(HAVE_GETPEEREID) && \
	!defined(HAVE_GETPEERUCRED) && \
	!defined(SO_PEERCRED) && !defined(LOCAL_PEERCRED) && \
	defined(HAVE_SENDMSG) && (defined(HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTSLEN) || \
	defined(HAVE_STRUCT_MSGHDR_MSG_CONTROL))
#	define LDAP_PF_LOCAL_SENDMSG 1
#endif

#ifdef HAVE_GETPEEREID
#define	LUTIL_GETPEEREID( s, uid, gid, bv )	getpeereid( s, uid, gid )
#elif defined(LDAP_PF_LOCAL_SENDMSG)
struct berval;
LDAP_LUTIL_F( int ) lutil_getpeereid( int s, uid_t *, gid_t *, struct berval *bv );
#define	LUTIL_GETPEEREID( s, uid, gid, bv )	lutil_getpeereid( s, uid, gid, bv )
#else
LDAP_LUTIL_F( int ) lutil_getpeereid( int s, uid_t *, gid_t * );
#define	LUTIL_GETPEEREID( s, uid, gid, bv )	lutil_getpeereid( s, uid, gid )
#endif

/* DNS RFC defines max host name as 255. New systems seem to use 1024 */
#ifndef NI_MAXHOST
#define	NI_MAXHOST	256
#endif

#ifdef HAVE_POLL
# ifndef INFTIM
#  define INFTIM (-1)
# endif
#undef POLL_OTHER
#define POLL_OTHER   (POLLERR|POLLHUP)
#undef POLL_READ
#define POLL_READ    (POLLIN|POLLPRI|POLL_OTHER)
#undef POLL_WRITE
#define POLL_WRITE   (POLLOUT|POLL_OTHER)
#endif

#endif /* _AC_SOCKET_H_ */
