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

MAINT_CHARSET = utf8
SUFFIXES = .soelim .man.in .man

mandir_en = $(mandir)
man1dir = $(mandir_en)/man1
man2dir = $(mandir_en)/man2
man3dir = $(mandir_en)/man3
man4dir = $(mandir_en)/man4
man5dir = $(mandir_en)/man5
man6dir = $(mandir_en)/man6
man7dir = $(mandir_en)/man7
man8dir = $(mandir_en)/man8

.soelim.man.in:
	$(MKDIR_P) $(@D) && $(SOELIM_R) -I $(<D) $< | $(FIXUP_DIST) > $@

.man.in.man:
	$(MKDIR_P) $(@D) && $(FIXUP_DIRS) $< > $@

man_links = man3/ldap_first_entry.3.links \
	man3/ldap_schema.3.links man3/ldap_search.3.links \
	man3/ldap_parse_result.3.links man3/ldap_delete.3.links \
	man3/ldap_open.3.links man3/ldap_first_reference.3.links \
	man3/ldap_controls.3.links man3/ldap_result.3.links \
	man3/ldap_abandon.3.links man3/ldap_extended_operation.3.links \
	man3/lber-types.3.links man3/ldap_url.3.links \
	man3/lber-decode.3.links man3/ldap_rename.3.links \
	man3/ldap_sort.3.links man3/ldap_get_values.3.links \
	man3/ldap_get_option.3.links man3/ldap_tls.3.links \
	man3/ldap_first_message.3.links man3/ldap_dup.3.links \
	man3/ldap_memory.3.links man3/ldap_modify.3.links \
	man3/ldap_get_dn.3.links man3/ldap_error.3.links \
	man3/ldap_first_attribute.3.links man3/lber-encode.3.links \
	man3/ldap_add.3.links man3/ldap_modrdn.3.links \
	man3/ldap_compare.3.links man3/ldap_bind.3.links \
	man5/slapd-sock.5.links man5/slapd-bdb.5.links \
	man1/ldapmodify.1.links

EXTRA_DIST = $(man_links)

dist_man1_MANS = man1/ldapmodify.man.in man1/ldapcompare.man.in \
	man1/ldapsearch.man.in man1/ldapdelete.man.in man1/ldapwhoami.man.in \
	man1/ldappasswd.man.in man1/ldapurl.man.in man1/ldapexop.man.in \
	man1/ldapmodrdn.man.in

dist_man3_MANS = man3/ldap_get_option.man.in man3/ldap_dup.man.in man3/ldap_modify.man.in \
	man3/lber-encode.man.in man3/ldap_result.man.in man3/ldap_memory.man.in \
	man3/lber-sockbuf.man.in man3/ldap_parse_reference.man.in \
	man3/ldap_error.man.in man3/ldap_delete.man.in man3/ldap_tls.man.in \
	man3/ldap_controls.man.in man3/ldap_parse_result.man.in \
	man3/ldap_search.man.in man3/ldap_get_values.man.in man3/ldap_schema.man.in \
	man3/ldap_get_dn.man.in man3/ldap_first_message.man.in man3/ldap_open.man.in \
	man3/ldap_sort.man.in man3/ldap_extended_operation.man.in \
	man3/ldap_parse_sort_control.man.in man3/ldap.man.in man3/ldap_modrdn.man.in \
	man3/ldap_first_entry.man.in man3/ldap_add.man.in man3/ldap_sync.man.in \
	man3/ldap_compare.man.in man3/ldap_first_reference.man.in \
	man3/lber-types.man.in man3/lber-decode.man.in \
	man3/ldap_first_attribute.man.in man3/ldap_url.man.in man3/ldap_bind.man.in \
	man3/ldap_abandon.man.in man3/ldap_rename.man.in \
	man3/ldap_parse_vlv_control.man.in man3/lber-memory.man.in

dist_man5_MANS = man5/slapd-config.man.in man5/slapd.backends.man.in \
	man5/slapo-accesslog.man.in man5/slapo-memberof.man.in man5/ldif.man.in \
	man5/slapo-dyngroup.man.in man5/slapo-dds.man.in man5/slapd-perl.man.in \
	man5/slapo-pcache.man.in man5/slapd-monitor.man.in \
	man5/slapo-constraint.man.in man5/slapd-passwd.man.in man5/ldap.conf.man.in \
	man5/slapd-null.man.in man5/slapd-meta.man.in man5/slapd-sql.man.in \
	man5/slapd.overlays.man.in man5/slapd-ldap.man.in man5/slapo-rwm.man.in \
	man5/slapd.plugin.man.in man5/slapo-unique.man.in man5/slapd-ndb.man.in \
	man5/slapo-dynlist.man.in man5/slapd-sock.man.in man5/slapo-refint.man.in \
	man5/slapo-pbind.man.in man5/slapd-mdb.man.in man5/slapo-sssvlv.man.in \
	man5/slapd-shell.man.in man5/slapo-retcode.man.in man5/slapo-collect.man.in \
	man5/slapo-auditlog.man.in man5/slapo-translucent.man.in \
	man5/slapo-valsort.man.in man5/slapd-relay.man.in man5/slapd.conf.man.in \
	man5/slapo-ppolicy.man.in man5/slapo-syncprov.man.in man5/slapd-dnssrv.man.in \
	man5/slapd-ldif.man.in man5/slapo-chain.man.in man5/slapd-bdb.man.in \
	man5/slapd.access.man.in man5/slapd-asyncmeta.man.in man5/slapd-wt.man.in

dist_man8_MANS = man8/slapcat.man.in man8/slappasswd.man.in man8/slapacl.man.in \
	man8/slapschema.man.in man8/slapadd.man.in man8/slapindex.man.in man8/slapdn.man.in \
	man8/slapauth.man.in man8/slaptest.man.in man8/slapd.man.in man8/slapmodify.man.in

man1_MANS = man1/ldapmodify.man man1/ldapcompare.man \
	man1/ldapsearch.man man1/ldapdelete.man man1/ldapwhoami.man \
	man1/ldappasswd.man man1/ldapurl.man man1/ldapexop.man \
	man1/ldapmodrdn.man

man3_MANS = man3/ldap_get_option.man man3/ldap_dup.man man3/ldap_modify.man \
	man3/lber-encode.man man3/ldap_result.man man3/ldap_memory.man \
	man3/lber-sockbuf.man man3/ldap_parse_reference.man \
	man3/ldap_error.man man3/ldap_delete.man man3/ldap_tls.man \
	man3/ldap_controls.man man3/ldap_parse_result.man \
	man3/ldap_search.man man3/ldap_get_values.man man3/ldap_schema.man \
	man3/ldap_get_dn.man man3/ldap_first_message.man man3/ldap_open.man \
	man3/ldap_sort.man man3/ldap_extended_operation.man \
	man3/ldap_parse_sort_control.man man3/ldap.man man3/ldap_modrdn.man \
	man3/ldap_first_entry.man man3/ldap_add.man man3/ldap_sync.man \
	man3/ldap_compare.man man3/ldap_first_reference.man \
	man3/lber-types.man man3/lber-decode.man \
	man3/ldap_first_attribute.man man3/ldap_url.man man3/ldap_bind.man \
	man3/ldap_abandon.man man3/ldap_rename.man \
	man3/ldap_parse_vlv_control.man man3/lber-memory.man

man5_MANS = man5/slapd-config.man man5/slapd.backends.man \
	man5/slapo-accesslog.man man5/slapo-memberof.man man5/ldif.man \
	man5/slapo-dyngroup.man man5/slapo-dds.man man5/slapd-perl.man \
	man5/slapo-pcache.man man5/slapd-monitor.man \
	man5/slapo-constraint.man man5/slapd-passwd.man man5/ldap.conf.man \
	man5/slapd-null.man man5/slapd-meta.man man5/slapd-sql.man \
	man5/slapd.overlays.man man5/slapd-ldap.man man5/slapo-rwm.man \
	man5/slapd.plugin.man man5/slapo-unique.man man5/slapd-ndb.man \
	man5/slapo-dynlist.man man5/slapd-sock.man man5/slapo-refint.man \
	man5/slapo-pbind.man man5/slapd-mdb.man man5/slapo-sssvlv.man \
	man5/slapd-shell.man man5/slapo-retcode.man man5/slapo-collect.man \
	man5/slapo-auditlog.man man5/slapo-translucent.man \
	man5/slapo-valsort.man man5/slapd-relay.man man5/slapd.conf.man \
	man5/slapo-ppolicy.man man5/slapo-syncprov.man man5/slapd-dnssrv.man \
	man5/slapd-ldif.man man5/slapo-chain.man man5/slapd-bdb.man \
	man5/slapd.access.man man5/slapd-asyncmeta.man man5/slapd-wt.man

man8_MANS = man8/slapcat.man man8/slappasswd.man man8/slapacl.man \
	man8/slapschema.man man8/slapadd.man man8/slapindex.man man8/slapdn.man \
	man8/slapauth.man man8/slaptest.man man8/slapd.man man8/slapmodify.man

install-data-hook:
	(cd $(srcdir) && find . -name '*.links' -exec \
		sh -c 'xargs -I "#" -n 1 -r sh -c "cd $(mandir_en)/$$(dirname {}) && ln -s -f -T $$(basename {} .links) #" < $(srcdir)/{}' \
		\;)

uninstall-hook:
	(cd $(srcdir) && find . -name '*.links' -exec \
		sh -c 'xargs -I "#" -n 1 -r sh -c "rm -f $(mandir_en)/$$(dirname {})/#" < $(srcdir)/{}' \
		\;)
	find $(mandir_en) -type d -empty -exec rmdir {} \;
