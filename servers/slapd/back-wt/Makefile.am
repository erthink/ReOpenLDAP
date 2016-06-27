# Makefile.in for back-wt
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
back_wt_la_LDFLAGS = -module $(WT_LIBDIR)
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include $(WT_CPPFLAGS)
AM_CLAGS = $(WT_CFLAGS)

if META_DYNAMIC
pkglib_LTLIBRARIES += back_wt.la
back_wt_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_wt.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_wt --global 'backend-wt' > $@
endif

back_wt_la_SOURCES = add.c attr.c back-wt.h banner.c bind.c \
	compare.c config.c ctx.c delete.c dn2entry.c dn2id.c \
	filterindex.c id2entry.c idl.c idl.h index.c init.c key.c \
	modify.c nextid.c operational.c proto-wt.h search.c tools.c

back_wt_la_LIBADD = $(WT_LIBS)
