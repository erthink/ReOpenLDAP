# LIBREWRITE
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
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##
## Copyright 2000-2001 Pierangelo Masarati <ando@sys-net.it>
##

SRCS = config.c context.c info.c ldapmap.c map.c params.c rule.c \
	session.c subst.c var.c xmap.c

XSRCS = version.c
OBJS = $(SRCS:.c=.o)

LDAP_INCDIR= ../../include
LDAP_LIBDIR= ../../libraries

LIBRARY	= librewrite.a
bin_PROGRAMS= rewrite
XLIBS	= $(LIBRARY) $(LDAP_LIBLUTIL_A) $(LDAP_LIBRELDAP_LA)
XXLIBS	= $(SECURITY_LIBS) $(LUTIL_LIBS)
XXXLIBS	= $(LTHREAD_LIBS)

rewrite$(EXEEXT):	$(XLIBS) rewrite.o parse.o
	$(LTLINK) -o $@ rewrite.o parse.o $(LIBS)
