# Makefile.in for back-bdb
## $ReOpenLDAP$
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

pkglib_LTLIBRARIES =
noinst_LTLIBRARIES =
back_hdb_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include

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
	$(MKBANNER) --name backend_hdb --global 'backend-hdb' > $@
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

back_hdb_la_CFLAGS = -DBDB_HIER=1 @BDB_INCLUDES@
back_hdb_la_LIBADD = @BDB_LIBS@
