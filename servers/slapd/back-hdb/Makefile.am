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
back_hdb_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include $(BDB_INCLUDES)

if HDB_DYNAMIC
pkglib_LTLIBRARIES += back_hdb.la
back_hdb_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_hdb.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_hdb --global 'backend-hdb' > $@ || $(RM) $@
endif

back_hdb_la_SOURCES = ../back-bdb/add.c ../back-bdb/attr.c \
	../back-bdb/back-bdb.h banner.c ../back-bdb/bind.c \
	../back-bdb/cache.c ../back-bdb/compare.c ../back-bdb/config.c \
	../back-bdb/dbcache.c ../back-bdb/delete.c \
	../back-bdb/dn2entry.c ../back-bdb/dn2id.c ../back-bdb/error.c \
	../back-bdb/extended.c ../back-bdb/filterindex.c \
	../back-bdb/id2entry.c ../back-bdb/idl.c ../back-bdb/idl.h \
	../back-bdb/index.c ../back-bdb/init.c ../back-bdb/key.c \
	../back-bdb/modify.c ../back-bdb/modrdn.c ../back-bdb/monitor.c \
	../back-bdb/nextid.c ../back-bdb/operational.c \
	../back-bdb/proto-bdb.h ../back-bdb/referral.c \
	../back-bdb/search.c ../back-bdb/tools.c ../back-bdb/trans.c

back_hdb_la_LIBADD = $(BDB_LIBS)
back_hdb_la_CFLAGS = -DBDB_HIER
