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
BUILT_SOURCES = s_banner.c m_banner.c d_banner.c r_banner.c \
	p_banner.c w_banner.c c_banner.c e_banner.c u_banner.c \
	v_banner.c

s_banner.c: Makefile
	$(MKBANNER) --global 'ldapsearch' > $@
m_banner.c: Makefile
	$(MKBANNER) --global 'ldapmodify' > $@
d_banner.c: Makefile
	$(MKBANNER) --global 'ldapdelete' > $@
r_banner.c: Makefile
	$(MKBANNER) --global 'ldapmodrdn' > $@
p_banner.c: Makefile
	$(MKBANNER) --global 'ldappasswd' > $@
w_banner.c: Makefile
	$(MKBANNER) --global 'ldapwhoami' > $@
c_banner.c: Makefile
	$(MKBANNER) --global 'ldapcompare' > $@
e_banner.c: Makefile
	$(MKBANNER) --global 'ldapexop' > $@
u_banner.c: Makefile
	$(MKBANNER) --global 'ldapurl' > $@
v_banner.c: Makefile
	$(MKBANNER) --global 'ldapvc' > $@
endif

ldapsearch_SOURCES = common.c ldapsearch.c s_banner.c common.h
ldapsearch_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapmodify_SOURCES = common.c ldapmodify.c m_banner.c common.h
ldapmodify_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapdelete_SOURCES = common.c ldapdelete.c d_banner.c common.h
ldapdelete_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapmodrdn_SOURCES = common.c ldapmodrdn.c r_banner.c common.h
ldapmodrdn_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldappasswd_SOURCES = common.c ldappasswd.c p_banner.c common.h
ldappasswd_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapwhoami_SOURCES = common.c ldapwhoami.c w_banner.c common.h
ldapwhoami_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapcompare_SOURCES = common.c ldapcompare.c c_banner.c common.h
ldapcompare_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapexop_SOURCES = common.c ldapexop.c e_banner.c common.h
ldapexop_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapurl_SOURCES = ldapurl.c u_banner.c common.h
ldapurl_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

ldapvc_SOURCES = common.c ldapvc.c v_banner.c common.h
ldapvc_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)

install-exec-hook:
	@if $(AM_V_P); then set -x; else echo "  LN       ldapmodify -> ldapadd"; fi; \
	$(RM) $(DESTDIR)$(bindir)/ldapadd$(EXEEXT) && \
	$(LN_S) $(DESTDIR)$(bindir)/ldapmodify$(EXEEXT) $(DESTDIR)$(bindir)/ldapadd$(EXEEXT)
