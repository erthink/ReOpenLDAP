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

MAINT_CHARSET = utf8
SUFFIXES = .soelim .man.in .man

mandir_ru = $(mandir)/ru
man1dir = $(mandir_ru)/man1
man2dir = $(mandir_ru)/man2
man3dir = $(mandir_ru)/man3
man4dir = $(mandir_ru)/man4
man5dir = $(mandir_ru)/man5
man6dir = $(mandir_ru)/man6
man7dir = $(mandir_ru)/man7
man8dir = $(mandir_ru)/man8

.soelim.man.in:
	@if $(AM_V_P); then set -x; else echo "  SOELIM   $@"; fi; \
	$(MKDIR_P) $(@D) && $(SOELIM_R) -I $(<D) $< | $(FIXUP_DIST) > $@

.man.in.man:
	@if $(AM_V_P); then set -x; else echo "  FIXUP    $@"; fi; \
	$(MKDIR_P) $(@D) && $(FIXUP_DIRS) $< > $@

man_links = man5/slapd-bdb.5.links man1/ldapmodify.1.links

EXTRA_DIST = $(man_links) \
	man1/ldapmodify.man.in man1/ldapcompare.man.in \
	man1/ldapsearch.man.in man1/ldapdelete.man.in man1/ldapwhoami.man.in \
	man1/ldappasswd.man.in man1/ldapurl.man.in man1/ldapexop.man.in \
	man1/ldapmodrdn.man.in \
	\
	man5/slapd-config.man.in man5/slapo-accesslog.man.in \
	man5/slapo-memberof.man.in man5/ldif.man.in man5/ldap.conf.man.in \
	man5/slapd.plugin.man.in man5/slapd-mdb.man.in man5/slapd.conf.man.in \
	man5/slapo-ppolicy.man.in man5/slapo-syncprov.man.in \
	man5/slapd-bdb.man.in man5/slapd.access.man.in \
	\
	man8/slapcat.man.in man8/slappasswd.man.in \
	man8/slapacl.man.in man8/slapschema.man.in man8/slapadd.man.in \
	man8/slapindex.man.in man8/slapdn.man.in man8/slapauth.man.in \
	man8/slaptest.man.in man8/slapd.man.in

man1_MANS = man1/ldapmodify.man man1/ldapcompare.man \
	man1/ldapsearch.man man1/ldapdelete.man man1/ldapwhoami.man \
	man1/ldappasswd.man man1/ldapurl.man man1/ldapexop.man \
	man1/ldapmodrdn.man

man5_MANS = man5/slapd-config.man man5/slapo-accesslog.man \
	man5/slapo-memberof.man man5/ldif.man man5/ldap.conf.man \
	man5/slapd.plugin.man man5/slapd-mdb.man man5/slapd.conf.man \
	man5/slapo-ppolicy.man man5/slapo-syncprov.man \
	man5/slapd-bdb.man man5/slapd.access.man

man8_MANS = man8/slapcat.man man8/slappasswd.man \
	man8/slapacl.man man8/slapschema.man man8/slapadd.man \
	man8/slapindex.man man8/slapdn.man man8/slapauth.man \
	man8/slaptest.man man8/slapd.man

install-data-hook:
	@if $(AM_V_P); then set -x; else echo "  LN       $(man_links)"; fi; \
	(cd $(srcdir) && echo $(man_links) | tr -s '[:space:]' '\n' | xargs -r -n 1 -I {} \
		sh -c 'xargs -I "#" -n 1 -r sh -c "cd $(DESTDIR)$(mandir_ru)/$$(dirname {}) && ln -s -f -T $$(basename {} .links) #" < $(srcdir)/{}' \
		\;)

uninstall-hook:
	@if $(AM_V_P); then set -x; else echo "  RM       $(man_links)"; fi; \
	(cd $(srcdir) && echo $(man_links) | tr -s '[:space:]' '\n' | xargs -r -n 1 -I {} \
		sh -c 'xargs -I "#" -n 1 -r sh -c "rm -f $(DESTDIR)$(mandir_ru)/$$(dirname {})/#" < $(srcdir)/{}' \
		\;); \
	find $(DESTDIR)$(mandir_ru) -depth -type d -empty -exec rmdir {} \;
