/* Generic stdlib.h */
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

#ifndef _AC_STDLIB_H
#define _AC_STDLIB_H

#if defined( HAVE_CSRIMALLOC )
#include <stdio.h>
#define MALLOC_TRACE
#include <libmalloc.h>
#endif

#include <stdlib.h>

/* Ignore malloc.h if we have STDC_HEADERS */
#if defined(HAVE_MALLOC_H) && !defined(STDC_HEADERS)
#	include <malloc.h>
#endif

#ifndef EXIT_SUCCESS
#	define EXIT_SUCCESS 0
#	define EXIT_FAILURE 1
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#if defined(LINE_MAX)
#	define AC_LINE_MAX LINE_MAX
#else
#	define AC_LINE_MAX 2048 /* POSIX MIN */
#endif

#endif /* _AC_STDLIB_H */
