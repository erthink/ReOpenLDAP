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
# Copyright 2008-2014 The OpenLDAP Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.

LDAP_SRC = ../../..
LDAP_BUILD = $(LDAP_SRC)
LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/libreldap/libreldap.la

LIBTOOL = $(LDAP_BUILD)/libtool
CC ?= gcc
CFLAGS ?= -g -O2 -Wall -Werror
DEFS = -DSLAPD_OVER_NOPS=SLAPD_MOD_DYNAMIC
INCS = $(LDAP_INC)
LIBS = $(LDAP_LIB)

PROGRAMS = nops.la
LTVER = 0:0:0

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/reopenldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(DEFS) $(INCS) -c $<

all: $(PROGRAMS)

nops.la: nops.lo
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) -version-info $(LTVER) \
	-rpath $(moduledir) -module -o $@ $? $(LIBS)

clean:
	rm -rf *.o *.lo *.la .libs

install: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(moduledir)
	for p in $(PROGRAMS) ; do \
		$(LIBTOOL) --mode=install cp $$p $(DESTDIR)$(moduledir) ; \
	done
