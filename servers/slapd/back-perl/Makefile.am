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
back_perl_la_LDFLAGS = -module $(PERL_RDIR) $(PERL_LDFLAGS)
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include
EXTRA_DIST = README SampleLDAP.pm

if PERL_DYNAMIC
pkglib_LTLIBRARIES += back_perl.la
back_perl_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_perl.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_perl --global 'backend-perl' > $@ || $(RM) $@
endif

back_perl_la_SOURCES = add.c banner.c bind.c close.c compare.c \
	config.c delete.c init.c modify.c modrdn.c perl_back.h \
	proto-perl.h search.c

AM_CPPFLAGS += $(PERL_CPPFLAGS)
back_perl_la_LIBADD = $(PERL_LIBS)
