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

noinst_PROGRAMS = ldif_filter slapd_addel slapd_bind \
	slapd_modify slapd_modrdn slapd_mtread slapd_read \
	slapd_search slapd_tester

AM_CPPFLAGS = -I$(top_srcdir)/include

LIBS = $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_LA) \
	 $(LUTIL_LIBS) $(SECURITY_LIBS)

ldif_filter_SOURCES = ldif-filter.c
ldif_filter_LDADD = $(LIBS)

slapd_addel_SOURCES = slapd-addel.c slapd-common.c slapd-common.h
slapd_addel_LDADD = $(LIBS)

slapd_bind_SOURCES = slapd-bind.c slapd-common.c slapd-common.h
slapd_bind_LDADD = $(LIBS)

slapd_modify_SOURCES = slapd-modify.c slapd-common.c slapd-common.h
slapd_modify_LDADD = $(LIBS)

slapd_modrdn_SOURCES = slapd-modrdn.c slapd-common.c slapd-common.h
slapd_modrdn_LDADD = $(LIBS)

slapd_mtread_SOURCES = slapd-mtread.c slapd-common.c slapd-common.h
slapd_mtread_LDADD = $(LIBS)

slapd_read_SOURCES = slapd-read.c slapd-common.c slapd-common.h
slapd_read_LDADD = $(LIBS)

slapd_search_SOURCES = slapd-search.c slapd-common.c slapd-common.h
slapd_search_LDADD = $(LIBS)

slapd_tester_SOURCES = slapd-tester.c slapd-common.c slapd-common.h
slapd_tester_LDADD = $(LIBS)
