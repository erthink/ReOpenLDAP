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
/* Copyright 1997, 1998, 1999 Computing Research Labs,
 * New Mexico State University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* $Id: utbm.h,v 1.1 1999/09/21 15:45:18 mleisher Exp $ */

#ifndef _h_utbm
#define _h_utbm

#include "reldap.h"

LDAP_BEGIN_DECL

/*************************************************************************
 *
 * Types.
 *
 *************************************************************************/

/*
 * Fundamental character types.
 */
typedef unsigned long ucs4_t;
typedef unsigned short ucs2_t;

/*
 * An opaque type used for the search pattern.
 */
typedef struct _utbm_pattern_t *utbm_pattern_t;

/*************************************************************************
 *
 * Flags.
 *
 *************************************************************************/

#define UTBM_CASEFOLD          0x01
#define UTBM_IGNORE_NONSPACING 0x02
#define UTBM_SPACE_COMPRESS    0x04

/*************************************************************************
 *
 * API.
 *
 *************************************************************************/

LDAP_LUNICODE_F (utbm_pattern_t) utbm_create_pattern LDAP_P((void));

LDAP_LUNICODE_F (void) utbm_free_pattern LDAP_P((utbm_pattern_t pattern));

LDAP_LUNICODE_F (void)
utbm_compile LDAP_P((ucs2_t *pat, unsigned long patlen,
		     unsigned long flags, utbm_pattern_t pattern));

LDAP_LUNICODE_F (int)
utbm_exec LDAP_P((utbm_pattern_t pat, ucs2_t *text,
		  unsigned long textlen, unsigned long *match_start,
		  unsigned long *match_end));

/*************************************************************************
 *
 * Prototypes for the stub functions needed.
 *
 *************************************************************************/

LDAP_LUNICODE_F (int) _utbm_isspace LDAP_P((ucs4_t c, int compress));

LDAP_LUNICODE_F (int) _utbm_iscntrl LDAP_P((ucs4_t c));

LDAP_LUNICODE_F (int) _utbm_nonspacing LDAP_P((ucs4_t c));

LDAP_LUNICODE_F (ucs4_t) _utbm_tolower LDAP_P((ucs4_t c));

LDAP_LUNICODE_F (ucs4_t) _utbm_toupper LDAP_P((ucs4_t c));

LDAP_LUNICODE_F (ucs4_t) _utbm_totitle LDAP_P((ucs4_t c));

LDAP_END_DECL

#endif


#endif /* _h_utbm */
