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

noinst_LTLIBRARIES = liblunicode.la
BUILT_SOURCES =

AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES += banner.c uctable.h

banner.c: Makefile
	$(MKBANNER) --name lunicode 'liblunicode' > $@ || $(RM) $@

noinst_PROGRAMS = ucgendat
nodist_ucgendat_SOURCES = ucgendat.c
ucgendat_CPPFLAGS = -DSKIP_LDAP_AC_INCLUDES $(AM_CPPFLAGS)

uctable.h: ucgendat$(EXEEXT) UnicodeData.txt CompositionExclusions.txt
	@if $(AM_V_P); then set -x; else echo "  GEN      $@"; fi; \
	./ucgendat$(EXEEXT) $(srcdir)/UnicodeData.txt -x $(srcdir)/CompositionExclusions.txt \
		|| ($(RM) $@; exit 1)

endif

liblunicode_la_SOURCES = banner.c ucstr.c ucdata.c ure.c \
	urestubs.c ucdata.h ucpgba.h uctable.h ure.h utbm.h
