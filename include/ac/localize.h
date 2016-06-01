/* localize.h (i18n/l10n) */
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

#ifndef _AC_LOCALIZE_H
#define _AC_LOCALIZE_H

#ifdef LDAP_LOCALIZE

#	include <locale.h>
#	include <libintl.h>

	/* enable i18n/l10n */
#	define gettext_noop(s)		s
#	define _(s)					gettext(s)
#	define N_(s)				gettext_noop(s)
#	define ldap_pvt_setlocale(c,l)		((void) setlocale(c, l))
#	define ldap_pvt_textdomain(d)		((void) textdomain(d))
#	define ldap_pvt_bindtextdomain(p,d)	((void) bindtextdomain(p, d))

#else

	/* disable i18n/l10n */
#	define _(s)					s
#	define N_(s)				s
#	define ldap_pvt_setlocale(c,l)		((void) 0)
#	define ldap_pvt_textdomain(d)		((void) 0)
#	define ldap_pvt_bindtextdomain(p,d)	((void) 0)

#endif

#endif /* _AC_LOCALIZE_H */
