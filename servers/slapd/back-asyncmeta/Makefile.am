## $ReOpenLDAP$
## Copyright 2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
back_asyncmeta_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include

if ASYNCMETA_DYNAMIC
pkglib_LTLIBRARIES += back_asyncmeta.la
back_asyncmeta_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_asyncmeta.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_asyncmeta --global 'backend-asyncmeta' > $@
endif

back_asyncmeta_la_SOURCES = abandon.c add.c back-asyncmeta.h \
	banner.c bind.c candidates.c compare.c config.c conn.c delete.c \
	dncache.c init.c map.c message_queue.c meta_result.c modify.c \
	modrdn.c proto-asyncmeta.h search.c suffixmassage.c unbind.c
