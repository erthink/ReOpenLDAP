## $ReOpenLDAP$
## Copyright 2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
## All rights reserved.
##
## This file is part of ReOpenLDAP.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
##

pkglib_LTLIBRARIES =
noinst_LTLIBRARIES =
back_relay_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include
EXTRA_DIST = README

if RELAY_DYNAMIC
pkglib_LTLIBRARIES += back_relay.la
back_relay_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_relay.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_relay --global 'backend-relay' > $@ || $(RM) $@
endif

back_relay_la_SOURCES = back-relay.h banner.c init.c op.c proto-back-relay.h
