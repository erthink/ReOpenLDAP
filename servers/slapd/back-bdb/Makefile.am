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
back_bdb_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include $(BDB_INCLUDES)

if BDB_DYNAMIC
pkglib_LTLIBRARIES += back_bdb.la
back_bdb_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_bdb.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_bdb --global 'backend-bdb' > $@ || $(RM) $@
endif

back_bdb_la_SOURCES = add.c attr.c back-bdb.h banner.c bind.c \
	cache.c compare.c config.c dbcache.c delete.c dn2entry.c \
	dn2id.c error.c extended.c filterindex.c id2entry.c idl.c idl.h \
	index.c init.c key.c Makefile.am modify.c modrdn.c monitor.c \
	nextid.c operational.c proto-bdb.h referral.c search.c tools.c \
	trans.c

back_bdb_la_LIBADD = $(BDB_LIBS)
back_bdb_la_CFLAGS = -UBDB_HIER
