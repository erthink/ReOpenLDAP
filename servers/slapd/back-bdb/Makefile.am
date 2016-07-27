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
back_bdb_la_LDFLAGS = -module
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include \
	-UBDB_HIER $(BDB_INCLUDES)

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
