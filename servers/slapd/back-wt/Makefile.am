## $ReOpenLDAP$
## Copyright 2016-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
