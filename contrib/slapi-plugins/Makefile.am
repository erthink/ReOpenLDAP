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

EXTRA_DIST = addrdnvalues/README
pkglib_LTLIBRARIES = slapiplugin-addrdnvalues.la
AM_CPPFLAGS = -I$(top_srcdir)/include
BUILT_SOURCES =

if BEHIND_CURTIAN
BUILT_SOURCES += banner.c
banner.c: Makefile
	$(MKBANNER) --name contrib_slapi_plugins --global 'contrib-slapi-plugins' > $@ || $(RM) $@
endif

slapiplugin_addrdnvalues_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
slapiplugin_addrdnvalues_la_SOURCES = ./addrdnvalues/addrdnvalues.c banner.c
