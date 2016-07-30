# Makefile for LDAP tools
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

bin_PROGRAMS = ldapsearch ldapmodify ldapdelete ldapmodrdn \
	ldappasswd ldapwhoami ldapvc ldapcompare ldapexop \
	ldapurl

AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --global 'ldaptools' > $@ || $(RM) $@
endif

ldapsearch_SOURCES = common.c ldapsearch.c banner.c common.h
ldapsearch_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapmodify_SOURCES = common.c ldapmodify.c banner.c common.h
ldapmodify_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapdelete_SOURCES = common.c ldapdelete.c banner.c common.h
ldapdelete_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapmodrdn_SOURCES = common.c ldapmodrdn.c banner.c common.h
ldapmodrdn_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldappasswd_SOURCES = common.c ldappasswd.c banner.c common.h
ldappasswd_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapwhoami_SOURCES = common.c ldapwhoami.c banner.c common.h
ldapwhoami_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapcompare_SOURCES = common.c ldapcompare.c banner.c common.h
ldapcompare_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapexop_SOURCES = common.c ldapexop.c banner.c common.h
ldapexop_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapurl_SOURCES = ldapurl.c banner.c common.h
ldapurl_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapvc_SOURCES = common.c ldapvc.c banner.c common.h
ldapvc_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

install-exec-hook:
	@if $(AM_V_P); then set -x; else echo "  LN       ldapmodify -> ldapadd"; fi; \
	$(RM) $(DESTDIR)$(bindir)/ldapadd$(EXEEXT) && \
	$(LN_S) $(DESTDIR)$(bindir)/ldapmodify$(EXEEXT) $(DESTDIR)$(bindir)/ldapadd$(EXEEXT)
