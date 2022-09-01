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
noinst_LTLIBRARIES = libmdbx.la
bin_PROGRAMS = mdbx_stat mdbx_copy mdbx_dump mdbx_load mdbx_chk mdbx_drop
back_mdb_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include

if MDBX_DYNAMIC
pkglib_LTLIBRARIES += back_mdb.la
back_mdb_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_mdb.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_mdbx --global 'backend-mdbx' > $@ || $(RM) $@
endif

libmdbx_la_CFLAGS = -DSLAPD_LMDB_LEGACY=1
if SLAPD_DEBUG
if SLAPD_DEBUG_EXTRA
libmdbx_la_CFLAGS += -UNDEBUG -DMDBX_DEBUG=2 $(AM_CFLAGS)
else
libmdbx_la_CFLAGS += -UNDEBUG -DMDBX_DEBUG=1 $(AM_CFLAGS)
endif
else
libmdbx_la_CFLAGS += -DNDEBUG=1 -DMDBX_DEBUG=0 $(AM_CFLAGS)
endif
libmdbx_la_LIBADD = $(LTHREAD_LIBS)
libmdbx_srcdir = ../../../libraries/libmdbx
libmdbx_la_SOURCES = ../../../libraries/libmdbx/mdbx.c

EXTRA_DIST = ../../../libraries/libmdbx/LICENSE \
	../../../libraries/libmdbx/README.md \
	../../../libraries/libmdbx/mdbx.h \
	../../../libraries/libmdbx/AUTHORS \
	../../../libraries/libmdbx/.clang-format

dist_man_MANS = \
	../../../libraries/libmdbx/man1/mdbx_chk.1 \
	../../../libraries/libmdbx/man1/mdbx_copy.1 \
	../../../libraries/libmdbx/man1/mdbx_drop.1 \
	../../../libraries/libmdbx/man1/mdbx_dump.1 \
	../../../libraries/libmdbx/man1/mdbx_load.1 \
	../../../libraries/libmdbx/man1/mdbx_stat.1

back_mdb_la_SOURCES = add.c attr.c banner.c bind.c compare.c \
	config.c delete.c dn2entry.c dn2id.c extended.c filterindex.c \
	id2entry.c idl.c index.c init.c key.c modify.c modrdn.c \
	monitor.c nextid.c operational.c search.c tools.c \
	back-mdb.h idl.h proto-mdb.h

back_mdb_la_CFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/libraries/libmdbx $(AM_CFLAGS)
back_mdb_la_LIBADD = libmdbx.la

mdbx_chk_SOURCES = ../../../libraries/libmdbx/mdbx_chk.c
mdbx_copy_SOURCES = ../../../libraries/libmdbx/mdbx_copy.c
mdbx_dump_SOURCES = ../../../libraries/libmdbx/mdbx_dump.c
mdbx_load_SOURCES = ../../../libraries/libmdbx/mdbx_load.c
mdbx_stat_SOURCES = ../../../libraries/libmdbx/mdbx_stat.c
mdbx_drop_SOURCES = ../../../libraries/libmdbx/mdbx_drop.c

mdbx_chk_LDADD = libmdbx.la
mdbx_copy_LDADD = libmdbx.la
mdbx_dump_LDADD = libmdbx.la
mdbx_load_LDADD = libmdbx.la
mdbx_stat_LDADD = libmdbx.la
mdbx_drop_LDADD = libmdbx.la
