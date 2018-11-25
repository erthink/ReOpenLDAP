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
back_ndb_la_LDFLAGS = -module $(NDB_RDIR) $(NDB_LDFLAGS)
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include
EXTRA_DIST = attrsets.conf

if NDB_DYNAMIC
pkglib_LTLIBRARIES += back_ndb.la
back_ndb_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_ndb.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_ndb --global 'backend-ndb' > $@ || $(RM) $@
endif

back_ndb_la_SOURCES = add.cpp back-ndb.h banner.c bind.cpp \
	compare.cpp config.cpp delete.cpp init.cpp modify.cpp \
	modrdn.cpp ndbio.cpp proto-ndb.h search.cpp tools.cpp

AM_CPPFLAGS += $(NDB_INCLUDES)
back_ndb_la_LIBADD = $(NDB_LIBS)
