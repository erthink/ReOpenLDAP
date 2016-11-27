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

EXTRA_DIST = README
pkglib_LTLIBRARIES =
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(top_srcdir)/servers/slapd -I$(top_srcdir)/include
BUILT_SOURCES =

if BEHIND_CURTIAN
BUILT_SOURCES += banner.c
banner.c: Makefile
	$(MKBANNER) --name contrib_modules --global 'contrib-modules' > $@ || $(RM) $@
endif

man_links =
man5_MANS =

EXTRA_DIST += acl/README.gssacl acl/README.posixgroup
if SLAPD_DYNACL
pkglib_LTLIBRARIES += contrib-posixgroup.la
contrib_posixgroup_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_posixgroup_la_SOURCES = acl/posixgroup.c banner.c
if KRB5_AVAIL
pkglib_LTLIBRARIES += contrib-gssacl.la
contrib_gssacl_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_gssacl_la_SOURCES = acl/gssacl.c banner.c
contrib_gssacl_la_CFLAGS = $(KRB5_CFLAGS)
contrib_gssacl_la_LIBADD = $(KRB5_LIBS)
endif
endif

EXTRA_DIST += addpartial/README adremap/slapo-adremap.man.in
pkglib_LTLIBRARIES += contrib-addpartial.la
contrib_addpartial_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_addpartial_la_SOURCES = addpartial/addpartial-overlay.c banner.c

pkglib_LTLIBRARIES += contrib-adremap.la
contrib_adremap_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_adremap_la_SOURCES = adremap/adremap.c banner.c
man5_MANS += adremap/slapo-adremap.man

EXTRA_DIST += allop/README allop/slapo-allop.man.in
pkglib_LTLIBRARIES += contrib-allop.la
contrib_allop_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_allop_la_SOURCES = allop/allop.c banner.c
man5_MANS += allop/slapo-allop.man

EXTRA_DIST += allowed/README
pkglib_LTLIBRARIES += contrib-allowed.la
contrib_allowed_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_allowed_la_SOURCES = allowed/allowed.c banner.c

pkglib_LTLIBRARIES += contrib-authzid.la
contrib_authzid_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_authzid_la_SOURCES = authzid/authzid.c banner.c

EXTRA_DIST += autogroup/README
pkglib_LTLIBRARIES += contrib-autogroup.la
contrib_autogroup_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_autogroup_la_SOURCES = autogroup/autogroup.c banner.c

# Where to look for the CrackLib dictionaries
CRACKLIB_DICTPATH=@CRACKLIB_DICTPATH@
CHECKPASSWD_CONFIGFILE=@sysconfdir@/check_password.conf
EXTRA_DIST += check_password/README check_password/check_password.conf
pkglib_LTLIBRARIES += contrib-check_password.la
contrib_check_password_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_check_password_la_SOURCES = check_password/check_password.c banner.c
contrib_check_password_la_CFLAGS = -Wno-deprecated-declarations $(CRACK_CFLAGS) \
    -DCRACKLIB_DICTPATH="\"$(CRACKLIB_DICTPATH)\"" \
    -DCHECKPASSWD_CONFIGFILE="\"$(CHECKPASSWD_CONFIGFILE)\"" \
    -DCHECKPASSWD_DEBUG=1
contrib_check_password_la_LIBADD = $(CRACKLIB_LIBS)

EXTRA_DIST += cloak/slapo-cloak.man.in
pkglib_LTLIBRARIES += contrib-cloak.la
contrib_cloak_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_cloak_la_SOURCES = cloak/cloak.c banner.c
man5_MANS += cloak/slapo-cloak.man

if CONTRIB_BROKEN
# LY: Requires an obsolete version of libsnacc and
# some functions which were removed from slapd few years ago.
if LDAP_EXPERIMENTAL
# requires LDAP_COMP_MATCH which is EXPERIMENTAL (needs configure --enable-experimental)
EXTRA_DIST += comp_match/README \
	comp_match/authorityKeyIdentifier.asn comp_match/certificate.asn1
pkglib_LTLIBRARIES += contrib-compmatch.la
contrib_compmatch_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_compmatch_la_SOURCES = banner.c comp_match/asn.h comp_match/asn_to_syn_mr.c \
	comp_match/authorityKeyIdentifier.c comp_match/authorityKeyIdentifier.h \
	comp_match/certificate.c comp_match/certificate.h comp_match/componentlib.c \
	comp_match/componentlib.h comp_match/crl.c comp_match/crl.h comp_match/init.c
contrib_compmatch_la_CFLAGS = -DUSE_GEN_BUF
contrib_compmatch_la_LIBADD = $(TLS_LIBS) -lsnacc
endif
endif

pkglib_LTLIBRARIES += contrib-denyop.la
contrib_denyop_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_denyop_la_SOURCES = denyop/denyop.c banner.c

EXTRA_DIST += dsaschema/README
pkglib_LTLIBRARIES += contrib-dsaschema.la
contrib_dsaschema_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_dsaschema_la_SOURCES = dsaschema/dsaschema.c banner.c

pkglib_LTLIBRARIES += contrib-dupent.la
contrib_dupent_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_dupent_la_SOURCES = dupent/dupent.c banner.c

EXTRA_DIST += kinit/README
pkglib_LTLIBRARIES += contrib-kinit.la
contrib_kinit_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_kinit_la_SOURCES = kinit/kinit.c banner.c
contrib_kinit_la_LIBADD = $(KRB5_LIBS)

EXTRA_DIST += lastbind/slapo-lastbind.man.in
pkglib_LTLIBRARIES += contrib-lastbind.la
contrib_lastbind_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_lastbind_la_SOURCES = lastbind/lastbind.c banner.c
man5_MANS += lastbind/slapo-lastbind.man

EXTRA_DIST += lastmod/slapo-lastmod.man.in
pkglib_LTLIBRARIES += contrib-lastmod.la
contrib_lastmod_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_lastmod_la_SOURCES = lastmod/lastmod.c banner.c
man5_MANS += lastmod/slapo-lastmod.man

EXTRA_DIST += mr_passthru/README.md
pkglib_LTLIBRARIES += contrib-mr_passthru.la
contrib_mr_passthru_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_mr_passthru_la_SOURCES = mr_passthru/mr_passthru.c banner.c

pkglib_LTLIBRARIES += contrib-noopsrch.la
contrib_noopsrch_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_noopsrch_la_SOURCES = noopsrch/noopsrch.c banner.c

EXTRA_DIST += nops/slapo-nops.man.in
pkglib_LTLIBRARIES += contrib-nops.la
contrib_nops_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_nops_la_SOURCES = nops/nops.c banner.c
man5_MANS += nops/slapo-nops.man

EXTRA_DIST += nssov/README nssov/ldapns.schema \
	nssov/nss-pam-ldapd/README nssov/slapo-nssov.man.in
pkglib_LTLIBRARIES += contrib-nssov.la
contrib_nssov_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_nssov_la_SOURCES = banner.c nssov/alias.c nssov/ether.c nssov/group.c \
	nssov/host.c nssov/pam.c nssov/passwd.c nssov/protocol.c nssov/rpc.c \
	nssov/service.c nssov/shadow.c nssov/netgroup.c nssov/network.c \
	nssov/nssov.c nssov/nssov.h nssov/nss-pam-ldapd/attrs.h \
	nssov/nss-pam-ldapd/nslcd.h nssov/nss-pam-ldapd/nslcd-prot.h \
	nssov/nss-pam-ldapd/tio.c nssov/nss-pam-ldapd/tio.h
contrib_nssov_la_CFLAGS = -Wno-error=address -I$(srcdir)/nssov/nss-pam-ldapd
man5_MANS += nssov/slapo-nssov.man

EXTRA_DIST += passwd/README \
	passwd/apr1-atol.pl passwd/apr1-ltoa.pl
pkglib_LTLIBRARIES += contrib-pw-apr1.la
contrib_pw_apr1_la_SOURCES = banner.c passwd/apr1.c
contrib_pw_apr1_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)

if HEIMDAL_AVAIL
pkglib_LTLIBRARIES += contrib-pw-kerberos.la
contrib_pw_kerberos_la_SOURCES = banner.c passwd/kerberos.c
contrib_pw_kerberos_la_CFLAGS = -Wno-deprecated-declarations $(HEIMDAL_CFLAGS)
contrib_pw_kerberos_la_LIBADD = $(HEIMDAL_LIBS)
contrib_pw_kerberos_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
endif

pkglib_LTLIBRARIES += contrib-pw-netscape.la
contrib_pw_netscape_la_SOURCES = banner.c passwd/netscape.c
contrib_pw_netscape_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)

if CONTRIB_BROKEN
# LY: Requires an obsolete RADIUS-client library
pkglib_LTLIBRARIES += contrib-pw-radius.la
contrib_pw_radius_la_SOURCES = banner.c passwd/radius.c
contrib_pw_radius_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_pw_radius_la_LIBADD = -lradius
endif

EXTRA_DIST += passwd/pbkdf2/README
# TODO: support MozNSS
if TLS_FALLBACK_AVAIL
pkglib_LTLIBRARIES += contrib-pw-pbkdf2.la
contrib_pw_pbkdf2_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_pw_pbkdf2_la_SOURCES = passwd/pbkdf2/pw-pbkdf2.c banner.c
contrib_pw_pbkdf2_la_CFLAGS = $(TLS_FALLBACK_CFLAGS)
contrib_pw_pbkdf2_la_LIBADD = $(TLS_FALLBACK_LIBS)
endif

EXTRA_DIST += passwd/sha2/README
pkglib_LTLIBRARIES += contrib-pw-sha2.la
contrib_pw_sha2_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_pw_sha2_la_SOURCES = banner.c passwd/sha2/sha2.h \
	passwd/sha2/sha2.c passwd/sha2/slapd-sha2.c

EXTRA_DIST += passwd/totp/README
# TODO: support MozNSS
if TLS_FALLBACK_AVAIL
pkglib_LTLIBRARIES += contrib-pw-totp.la
contrib_pw_totp_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_pw_totp_la_SOURCES = banner.c passwd/totp/slapd-totp.c
contrib_pw_totp_la_CFLAGS = $(TLS_FALLBACK_CFLAGS)
contrib_pw_totp_la_LIBADD = $(TLS_FALLBACK_LIBS)
endif

EXTRA_DIST += proxyOld/README
pkglib_LTLIBRARIES += contrib-proxyOld.la
contrib_proxyOld_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_proxyOld_la_SOURCES = proxyOld/proxyOld.c banner.c

EXTRA_DIST += samba4/README
pkglib_LTLIBRARIES += contrib-pguid.la contrib-rdnval.la contrib-vernum.la
contrib_pguid_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_pguid_la_SOURCES = samba4/pguid.c banner.c
contrib_rdnval_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_rdnval_la_SOURCES = samba4/rdnval.c banner.c
contrib_vernum_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_vernum_la_SOURCES = samba4/vernum.c banner.c

EXTRA_DIST += smbk5pwd/README smbk5pwd/slapo-smbk5pwd.man.in
# TODO: support MozNSS
pkglib_LTLIBRARIES += contrib-smbk5pwd.la
contrib_smbk5pwd_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_smbk5pwd_la_SOURCES = banner.c smbk5pwd/smbk5pwd.c
# Omit DO_KRB5, DO_SAMBA or DO_SHADOW if you don't want to support it.
contrib_smbk5pwd_la_CFLAGS = -DDO_SHADOW -Wno-deprecated-declarations
contrib_smbk5pwd_la_LIBADD =
if TLS_FALLBACK_AVAIL
contrib_smbk5pwd_la_CFLAGS += $(TLS_FALLBACK_CFLAGS) -DDO_SAMBA
contrib_smbk5pwd_la_LIBADD += $(TLS_FALLBACK_LIBS)
endif
if HEIMDAL_AVAIL
contrib_smbk5pwd_la_CFLAGS += -DDO_KRB5 $(HEIMDAL_CFLAGS)
contrib_smbk5pwd_la_LIBADD += $(HEIMDAL_LIBS)
endif
man5_MANS += smbk5pwd/slapo-smbk5pwd.man

pkglib_LTLIBRARIES += contrib-trace.la
contrib_trace_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_trace_la_SOURCES = trace/trace.c banner.c

EXTRA_DIST += usn/README
pkglib_LTLIBRARIES += contrib-usn.la
contrib_usn_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_usn_la_SOURCES = usn/usn.c banner.c

pkglib_LTLIBRARIES += contrib-vc.la
contrib_vc_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
contrib_vc_la_SOURCES = vc/vc.c banner.c

MAINT_CHARSET = utf8
SUFFIXES = .soelim .man.in .man
EXTRA_DIST += $(man_links)

.soelim.man.in:
	@if $(AM_V_P); then set -x; else echo "  SOELIM   $@"; fi; \
	$(MKDIR_P) $(@D) && $(SOELIM_R) -I $(<D) $< | $(FIXUP_DIST) > $@

.man.in.man:
	@if $(AM_V_P); then set -x; else echo "  FIXUP    $@"; fi; \
	$(MKDIR_P) $(@D) && $(FIXUP_DIRS) $< > $@

install-data-hook:
	@if $(AM_V_P); then set -x; else echo "  LN       $(man_links)"; fi; \
	(cd $(srcdir) && echo $(man_links) | tr -s '[:space:]' '\n' | xargs -r -n 1 -I {} \
		sh -c 'xargs -I "#" -n 1 -r sh -c "cd $(DESTDIR)$(mandir)/$$(dirname {}) && ln -s -f -T $$(basename {} .links) #" < $(srcdir)/{}' \
		\;)
	@if $(AM_V_P); then set -x; else echo "  INSTALL  check_password.conf"; fi; \
	$(MKDIR_P) $(DESTDIR)$(sysconfdir) && $(INSTALL) -t $(DESTDIR)$(sysconfdir) -m 0444 $(srcdir)/check_password/check_password.conf

uninstall-hook:
	@if $(AM_V_P); then set -x; else echo "  RM       $(man_links)"; fi; \
	(cd $(srcdir) && echo $(man_links) | tr -s '[:space:]' '\n' | xargs -r -n 1 -I {} \
		sh -c 'xargs -I "#" -n 1 -r sh -c "rm -f $(DESTDIR)$(mandir)/$$(dirname {})/#" < $(srcdir)/{}' \
		\;); \
	find $(DESTDIR)$(mandir) -depth -type d -empty -exec rmdir {} \;
	@if $(AM_V_P); then set -x; else echo "  RM       check_password.conf"; fi; \
	$(RM) -f $(DESTDIR)$(sysconfdir)/check_password.conf
