## $ReOpenLDAP$
## Copyright 2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

noinst_LTLIBRARIES = librewrite.la

AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name rewrite 'librewrite' > $@ || $(RM) $@
endif

librewrite_la_LIBADD = $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_LA)

librewrite_la_SOURCES = banner.c config.c context.c info.c \
	ldapmap.c map.c params.c rule.c session.c subst.c var.c xmap.c \
	rewrite-int.h rewrite-map.h

noinst_PROGRAMS = rewrite

rewrite_SOURCES = rewrite.c parse.c
rewrite_LDADD = librewrite.la
