# Makefile.in for SLAPI
# $ReOpenLDAP$
## Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
## Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
##
## This file is part of ReOpenLDAP.
##
## ReOpenLDAP is free software; you can redistribute it and/or modify it under
## the terms of the GNU Affero General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## ReOpenLDAP is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Affero General Public License for more details.
##
## You should have received a copy of the GNU Affero General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##
## ---
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## Portions Copyright IBM Corp. 1997,2002,2003
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

LIBRARY = libreslapi.la

LIB_DEFS = -DSLAPI_LIBRARY

SRCS=  plugin.c slapi_pblock.c slapi_utils.c printmsg.c slapi_ops.c slapi_dn.c slapi_ext.c slapi_overlay.c
OBJS= $(SRCS:.c=.lo)

XSRCS= version.c

LDAP_INCDIR= ../../../include -I.. -I.
LDAP_LIBDIR= ../../../libraries

XLIBS = $(LIBRARY)
XXLIBS =

XINCPATH = -I$(srcdir)/.. -I$(srcdir)
XDEFS = $(MODULES_CPPFLAGS)

BUILD_MOD = @BUILD_SLAPI@

install-exec-local: FORCE
	if test "$(BUILD_MOD)" = "yes"; then \
		$(MKDIR) $(DESTDIR)$(libdir); \
		$(LTINSTALL) $(INSTALLFLAGS) -m 644 $(LIBRARY) $(DESTDIR)$(libdir); \
	fi
