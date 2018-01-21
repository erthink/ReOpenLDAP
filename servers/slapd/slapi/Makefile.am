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

pkglib_LTLIBRARIES = libreslapi.la
libreslapi_la_LDFLAGS = -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION) \
	-weak libslapi.la

AM_CPPFLAGS = -DSLAPI_LIBRARY -DSLAPD_IMPORT \
	-I$(srcdir)/.. -I$(top_srcdir)/include $(LTDLINCL)

libreslapi_la_LIBADD = $(SLAPI_LIBS) $(LIBLTDL)
EXTRA_libreslapi_la_DEPENDENCIES = $(LTDLDEPS)

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name SLAPI --global 'libreslapi' > $@ || $(RM) $@
endif

libreslapi_la_SOURCES = banner.c plugin.c printmsg.c slapi_dn.c \
	slapi_ext.c slapi_ops.c slapi_overlay.c slapi_pblock.c \
	slapi_utils.c proto-slapi.h slapi.h
