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

noinst_PROGRAMS = ldif_filter slapd_addel slapd_bind \
	slapd_modify slapd_modrdn slapd_mtread slapd_read \
	slapd_search slapd_tester

AM_CPPFLAGS = -I$(top_srcdir)/include

slap_LIBS = $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_LA) \
	 $(LUTIL_LIBS) $(SECURITY_LIBS)

ldif_filter_SOURCES = ldif-filter.c
ldif_filter_LDADD = $(slap_LIBS)

slapd_addel_SOURCES = slapd-addel.c slapd-common.c slapd-common.h
slapd_addel_LDADD = $(slap_LIBS)

slapd_bind_SOURCES = slapd-bind.c slapd-common.c slapd-common.h
slapd_bind_LDADD = $(slap_LIBS)

slapd_modify_SOURCES = slapd-modify.c slapd-common.c slapd-common.h
slapd_modify_LDADD = $(slap_LIBS)

slapd_modrdn_SOURCES = slapd-modrdn.c slapd-common.c slapd-common.h
slapd_modrdn_LDADD = $(slap_LIBS)

slapd_mtread_SOURCES = slapd-mtread.c slapd-common.c slapd-common.h
slapd_mtread_LDADD = $(slap_LIBS)

slapd_read_SOURCES = slapd-read.c slapd-common.c slapd-common.h
slapd_read_LDADD = $(slap_LIBS)

slapd_search_SOURCES = slapd-search.c slapd-common.c slapd-common.h
slapd_search_LDADD = $(slap_LIBS)

slapd_tester_SOURCES = slapd-tester.c slapd-common.c slapd-common.h
slapd_tester_LDADD = $(slap_LIBS)
