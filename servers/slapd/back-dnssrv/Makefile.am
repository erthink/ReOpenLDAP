# Makefile.in for back-dnssrv
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
## Portions Copyright 1998-2003 Kurt D. Zeilenga.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
#
# ACKNOWLEDGEMENTS:
#  The DNSSRV backend was written by Kurt D. Zeilenga.
#

SRCS	= init.c bind.c search.c config.c referral.c
OBJS	= $(SRCS:.c=.lo)

LDAP_INCDIR= ../../../include
LDAP_LIBDIR= ../../../libraries

BUILD_OPT = "--enable-dnssrv"
BUILD_MOD = @BUILD_DNSSRV@

mod_DEFS = -DSLAPD_IMPORT
MOD_DEFS = $(@BUILD_DNSSRV@_DEFS)

shared_LDAP_LIBS = $(LDAP_LIBRELDAP_LA)
NEED_LIBS = $(@BUILD_LIBS_DYNAMIC@_LDAP_LIBS)

LIBBASE = back_dnssrv

XINCPATH = -I.. -I$(srcdir)/..
XDEFS = $(MODULES_CPPFLAGS)

all-local-lib:	../.backend

../.backend: lib$(LIBBASE).a
	@touch $@
