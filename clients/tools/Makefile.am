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

uninstall-hook:
	@if $(AM_V_P); then set -x; else echo "  RM       ldapadd"; fi; \
	$(RM) -f $(DESTDIR)$(bindir)/ldapadd
