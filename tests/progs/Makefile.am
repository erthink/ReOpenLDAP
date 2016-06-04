## Makefile.in for test programs
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

bin_PROGRAMS = slapd-tester slapd-search slapd-read slapd-addel slapd-modrdn \
		slapd-modify slapd-bind slapd-mtread ldif-filter

SRCS     = slapd-common.c \
		slapd-tester.c slapd-search.c slapd-read.c slapd-addel.c \
		slapd-modrdn.c slapd-modify.c slapd-bind.c slapd-mtread.c \
		ldif-filter.c

LDAP_INCDIR= ../../include
LDAP_LIBDIR= ../../libraries

XLIBS    = $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_A)
XRLIBS    = $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_A)
XXLIBS	 = $(SECURITY_LIBS) $(LUTIL_LIBS)
RLIBS = $(XRLIBS) $(XXLIBS) $(AC_LIBS) $(XXXLIBS)


OBJS     = slapd-common.o

slapd-tester$(EXEEXT): slapd-tester.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-tester.o $(OBJS) $(LIBS)

slapd-search$(EXEEXT): slapd-search.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-search.o $(OBJS) $(LIBS)

slapd-read$(EXEEXT): slapd-read.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-read.o $(OBJS) $(LIBS)

slapd-addel$(EXEEXT): slapd-addel.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-addel.o $(OBJS) $(LIBS)

slapd-modrdn$(EXEEXT): slapd-modrdn.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-modrdn.o $(OBJS) $(LIBS)

slapd-modify$(EXEEXT): slapd-modify.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-modify.o $(OBJS) $(LIBS)

slapd-bind$(EXEEXT): slapd-bind.o $(OBJS) $(XLIBS)
	$(LTLINK) -o $@ slapd-bind.o $(OBJS) $(LIBS)

ldif-filter$(EXEEXT): ldif-filter.o $(XLIBS)
	$(LTLINK) -o $@ ldif-filter.o $(LIBS)

slapd-mtread$(EXEEXT): slapd-mtread.o $(OBJS) $(XRLIBS)
	$(LTLINK) -o $@ slapd-mtread.o $(OBJS) $(RLIBS)
