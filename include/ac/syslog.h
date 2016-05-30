/* Generic syslog.h */
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

#ifndef _AC_SYSLOG_H_
#define _AC_SYSLOG_H_

#if defined( HAVE_SYSLOG_H )
#include <syslog.h>
#elif defined ( HAVE_SYS_SYSLOG_H )
#include <sys/syslog.h>
#endif

#if defined( LOG_NDELAY ) && defined( LOG_NOWAIT )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NDELAY | LOG_NOWAIT )
#elif defined( LOG_NDELAY )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NDELAY )
#elif defined( LOG_NOWAIT )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NOWAIT )
#elif defined( LOG_PID )
#	define OPENLOG_OPTIONS ( LOG_PID )
#else
#   define OPENLOG_OPTIONS ( 0 )
#endif

#endif /* _AC_SYSLOG_H_ */
