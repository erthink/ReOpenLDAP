## $ReOpenLDAP$
## Copyright 2017-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
back_monitor_la_LDFLAGS = -module
AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/include
EXTRA_DIST = README

if MONITOR_DYNAMIC
pkglib_LTLIBRARIES += back_monitor.la
back_monitor_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_monitor.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_monitor --global 'backend-monitor' > $@ || $(RM) $@
endif

back_monitor_la_SOURCES = backend.c banner.c bind.c cache.c \
	compare.c conn.c database.c entry.c init.c listener.c log.c \
	modify.c operational.c operation.c overlay.c rww.c search.c \
	sent.c thread.c time.c back-monitor.h proto-back-monitor.h
