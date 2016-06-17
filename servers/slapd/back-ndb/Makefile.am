# Makefile.in for back-ndb
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
back_ndb_la_LDFLAGS = -module
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
	$(MKBANNER) --name backend_ndb --global 'backend-ndb' > $@
endif

back_ndb_la_SOURCES = add.cpp back-ndb.h banner.c bind.cpp \
	compare.cpp config.cpp delete.cpp init.cpp modify.cpp \
	modrdn.cpp ndbio.cpp proto-ndb.h search.cpp tools.cpp

AM_CPPFLAGS += $(NDB_INCLUDES)
back_ndb_la_LIBADD = $(NDB_LIBS)
