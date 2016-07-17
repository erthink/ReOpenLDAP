# $ReOpenLDAP$
# Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
# Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
#
# This file is part of ReOpenLDAP.
#
# ReOpenLDAP is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# ReOpenLDAP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# ---
#
# Copyright 2008-2014 The OpenLDAP Foundation.
# Portions Copyright 2008 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

include ../contrib-mod.mk

CFLAGS += -Wno-error=address
DEFS =
INCS = $(LDAP_INC) -Inss-pam-ldapd
LIBS = $(LDAP_LIB)

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/reopenldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
sysconfdir = $(prefix)/etc$(ldap_subdir)
schemadir = $(sysconfdir)/schema

all:	nssov.la

XOBJS = tio.lo

OBJS = alias.lo ether.lo group.lo host.lo netgroup.lo network.lo \
	nssov.lo passwd.lo protocol.lo rpc.lo service.lo shadow.lo pam.lo

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(DEFS) $(INCS) -c $<

tio.lo:	nss-pam-ldapd/tio.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(DEFS) $(INCS) -c $?

$(OBJS):	nssov.h

nssov.la:	$(OBJS) $(XOBJS)
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) -version-info 0:0:0 \
	-rpath $(moduledir) -module -o $@ $(OBJS) $(XOBJS) $(LIBS)

install: nssov.la
	mkdir -p $(DESTDIR)$(moduledir)
	$(LIBTOOL) --mode=install cp nssov.la $(DESTDIR)$(moduledir)
	cp ldapns.schema $(DESTDIR)$(schemadir)

clean:
	rm -f *.*o *.la .libs/*
	rm -rf .libs
