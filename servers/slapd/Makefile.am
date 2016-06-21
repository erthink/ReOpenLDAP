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

DIST_SUBDIRS = overlays slapi back-bdb back-dnssrv back-hdb \
	back-ldap back-ldif back-mdb back-meta back-monitor back-ndb \
	back-null back-passwd back-perl back-relay back-shell back-sock \
	back-sql shell-backends

SUBDIRS	= overlays

backends_BUILTIN = config
backends_DYNAMIC =
overlays_BUILTIN =
overlays_DYNAMIC =

AM_CPPFLAGS = -I${top_srcdir}/include
libexec_PROGRAMS = slapd
slapd_LDADD = overlays/liboverlays_builtin.la
slapd_LDFLAGS =

EXTRA_DIST = slapd.ldif.in slapd.conf.in
CLEANFILES = slapd.ldif slapd.conf
sysconf_DATA = slapd.ldif slapd.conf

if BUILD_LDIF
SUBDIRS	+= back-ldif
if LDIF_DYNAMIC
backends_DYNAMIC += ldif
else
backends_BUILTIN += ldif
slapd_LDADD += back-ldif/back_ldif.la
endif
endif

if BUILD_MONITOR
SUBDIRS	+= back-monitor
if MONITOR_DYNAMIC
backends_DYNAMIC += monitor
else
backends_BUILTIN += monitor
slapd_LDADD += back-monitor/back_monitor.la
endif
endif

if BUILD_MDBX
SUBDIRS	+= back-mdb
if MDBX_DYNAMIC
backends_DYNAMIC += mdb
else
backends_BUILTIN += mdb
slapd_LDADD += back-mdb/back_mdb.la
endif
endif

if BUILD_NULL
SUBDIRS	+= back-null
if NULL_DYNAMIC
backends_DYNAMIC += null
else
backends_BUILTIN += null
slapd_LDADD += back-null/back_null.la
endif
endif

if BUILD_LDAP
SUBDIRS	+= back-ldap
if LDAP_DYNAMIC
backends_DYNAMIC += ldap
else
backends_BUILTIN += ldap
slapd_LDADD += back-ldap/back_ldap.la
endif
endif

if BUILD_META
SUBDIRS	+= back-meta
if META_DYNAMIC
backends_DYNAMIC += meta
else
backends_BUILTIN += meta
slapd_LDADD += back-meta/back_meta.la
endif
endif

if BUILD_RELAY
SUBDIRS	+= back-relay
if RELAY_DYNAMIC
backends_DYNAMIC += relay
else
backends_BUILTIN += relay
slapd_LDADD += back-relay/back_relay.la
endif
endif

if BUILD_SOCK
SUBDIRS	+= back-sock
if SOCK_DYNAMIC
backends_DYNAMIC += sock
else
backends_BUILTIN += sock
slapd_LDADD += back-sock/back_sock.la
endif
endif

if BUILD_BDB
SUBDIRS	+= back-bdb
if BDB_DYNAMIC
backends_DYNAMIC += bdb
else
backends_BUILTIN += bdb
slapd_LDADD += back-bdb/back_bdb.la
endif
endif

if BUILD_HDB
SUBDIRS	+= back-hdb
if HDB_DYNAMIC
backends_DYNAMIC += hdb
else
backends_BUILTIN += hdb
slapd_LDADD += back-hdb/back_hdb.la
endif
endif

if BUILD_DNSSRV
SUBDIRS	+= back-dnssrv
if DNSSRV_DYNAMIC
backends_DYNAMIC += dnssrv
else
backends_BUILTIN += dnssrv
slapd_LDADD += back-dnssrv/back_dnssrv.la
endif
endif

if BUILD_NDB
SUBDIRS	+= back-ndb
if NDB_DYNAMIC
backends_DYNAMIC += ndb
else
backends_BUILTIN += ndb
slapd_LDADD += back-ndb/back_ndb.la -lstdc++
# LY: workaround for buggy libtool
slapd_LDFLAGS += $(NDB_RDIR)
endif
endif

if BUILD_PASSWD
SUBDIRS	+= back-passwd
if PASSWD_DYNAMIC
backends_DYNAMIC += passwd
else
backends_BUILTIN += passwd
slapd_LDADD += back-passwd/back_passwd.la
endif
endif

if BUILD_PERL
SUBDIRS	+= back-perl
if PERL_DYNAMIC
backends_DYNAMIC += perl
else
backends_BUILTIN += perl
slapd_LDADD += back-perl/back_perl.la
# LY: workaround for buggy libtool
slapd_LDFLAGS += $(PERL_RDIR)
endif
endif

if BUILD_SHELL
SUBDIRS	+= back-shell shell-backends
if SHELL_DYNAMIC
backends_DYNAMIC += shell
else
backends_BUILTIN += shell
slapd_LDADD += back-shell/back_shell.la
endif
endif

if BUILD_SQL
SUBDIRS	+= back-sql
if SQL_DYNAMIC
backends_DYNAMIC += sql
else
backends_BUILTIN += sql
slapd_LDADD += back-sql/back_sql.la
endif
endif

#########################################

if BUILD_SLAPI
SUBDIRS	+= slapi
slapd_LDADD += slapi/libreslapi.la
endif

if SLAPD_DYNAMIC
AM_CPPFLAGS += $(LTDLINCL)
slapd_LDFLAGS += -export-dynamic -dlopen self
slapd_LDADD += $(LIBLTDL)
slapd_DEPENDENCIES = $(LTDLDEPS)
endif

#########################################

if BUILD_ACCESSLOG
if ! ACCESSLOG_DYNAMIC
overlays_BUILTIN += accesslog
endif
endif

if BUILD_AUDITLOG
if ! AUDITLOG_DYNAMIC
overlays_BUILTIN += auditlog
endif
endif

if BUILD_COLLECT
if ! COLLECT_DYNAMIC
overlays_BUILTIN += collect
endif
endif

if BUILD_CONSTRAINT
if ! CONSTRAINT_DYNAMIC
overlays_BUILTIN += constraint
endif
endif

if BUILD_DDS
if ! DDS_DYNAMIC
overlays_BUILTIN += dds
endif
endif

if BUILD_DEREF
if ! DEREF_DYNAMIC
overlays_BUILTIN += deref
endif
endif

if BUILD_DYNGROUP
if ! DYNGROUP_DYNAMIC
overlays_BUILTIN += dyngroup
endif
endif

if BUILD_DYNLIST
if ! DYNLIST_DYNAMIC
overlays_BUILTIN += dynlist
endif
endif

if BUILD_MEMBEROF
if ! MEMBEROF_DYNAMIC
overlays_BUILTIN += memberof
endif
endif

if BUILD_PPOLICY
if ! PPOLICY_DYNAMIC
overlays_BUILTIN += ppolicy
endif
endif

if BUILD_PCACHE
if ! PCACHE_DYNAMIC
overlays_BUILTIN += pcache
endif
endif

if BUILD_REFINT
if ! REFINT_DYNAMIC
overlays_BUILTIN += refint
endif
endif

if BUILD_RETCODE
if ! RETCODE_DYNAMIC
overlays_BUILTIN += retcode
endif
endif

if BUILD_RWM
if ! RWM_DYNAMIC
overlays_BUILTIN += rwm
endif
endif

if BUILD_SEQMOD
if ! SEQMOD_DYNAMIC
overlays_BUILTIN += seqmod
endif
endif

if BUILD_SSSVLV
if ! SSSVLV_DYNAMIC
overlays_BUILTIN += sssvlv
endif
endif

if BUILD_SYNCPROV
if ! SYNCPROV_DYNAMIC
overlays_BUILTIN += syncprov
endif
endif

if BUILD_TRANSLUCENT
if ! TRANSLUCENT_DYNAMIC
overlays_BUILTIN += translucent
endif
endif

if BUILD_UNIQUE
if ! UNIQUE_DYNAMIC
overlays_BUILTIN += unique
endif
endif

if BUILD_VALSORT
if ! VALSORT_DYNAMIC
overlays_BUILTIN += valsort
endif
endif

#########################################
BUILT_SOURCES = builtin.c
CLEANFILES += builtin.c
nodist_slapd_SOURCES = builtin.c

if BEHIND_CURTIAN
BUILT_SOURCES += banner.c
banner.c: Makefile
	$(MKBANNER) --name SLAPD --global 'slapd' > $@
endif

builtin.c: Makefile
	@echo "Make $@"; \
	echo "/* This file is automatically generated by configure; please do not edit. */" > $@ \
	&& echo '#include "reldap.h"' >> $@ \
	&& echo '#include "slap.h"' >> $@ \
		|| exit 1
	echo "/*-----------------------------------------*/" >> $@ && \
	for BE in $(backends_BUILTIN); do \
		echo "extern BI_init $${BE}_back_initialize;" >> $@ \
		|| exit 1; \
	done; \
	echo 'BackendInfo slap_binfo[] = {' >>$@ || exit 1;
	for BE in $(backends_BUILTIN); do \
		echo "	backend $BE"; \
		echo "	{ \"$${BE}\", $${BE}_back_initialize }," >> $@ \
		|| exit 1; \
	done
	echo '	{ NULL, NULL }' >> $@ \
	&& echo '};' >> $@ \
		|| exit 1;
	echo "/*-----------------------------------------*/" >> $@ && \
	for OV in $(overlays_BUILTIN); do \
		echo "extern OV_init $${OV}_over_initialize;" >> $@ \
		|| exit 1; \
	done; \
	echo 'OverlayInit slap_oinfo[] = {' >>$@ || exit 1;
	for OV in $(overlays_BUILTIN); do \
		echo "	overlay $OV"; \
		echo "	{ \"$${OV}\", $${OV}_over_initialize }," >> $@ \
		|| exit 1; \
	done; \
	echo '	{ NULL, NULL }' >> $@ \
	&& echo '};' >> $@ \
		|| exit 1;

# LY: TODO += BUILD_REWRITE ?
slapd_LDADD += $(LDAP_LIBREWRITE_LA)
slapd_LDADD += $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_LA) $(LDAP_LIBLUNICODE_LA)
slapd_LDADD += $(SLAPD_LIBS)

slapd_SOURCES = abandon.c aci.c acl.c aclparse.c ad.c add.c \
	alock.c at.c attr.c ava.c backend.c backglue.c backover.c \
	backtrace.c banner.c bconfig.c biglock.c bind.c cancel.c \
	ch_malloc.c compare.c component.c config.c connection.c \
	controls.c cr.c ctxcsn.c daemon.c delete.c dn.c entry.c \
	extended.c filter.c filterentry.c frontend.c globals.c index.c \
	init.c ldapsync.c limits.c lock.c main.c matchedValues.c \
	modify.c modrdn.c mods.c module.c mra.c mr.c oc.c oidm.c \
	operational.c operation.c passwd.c phonetic.c quorum.c \
	referral.c result.c root_dse.c rurwl.c saslauthz.c sasl.c \
	schema.c schema_check.c schema_init.c schemaparse.c \
	schema_prep.c search.c sets.c slapacl.c slapadd.c slapauth.c \
	slapcat.c slapcommon.c slapdn.c slapindex.c slappasswd.c \
	slapschema.c slaptest.c sl_malloc.c starttls.c str2filter.c \
	syncrepl.c syntax.c txn.c unbind.c user.c value.c alock.h \
	component.h slapconfig.h proto-slap.h sets.h slapcommon.h \
	slap.h

schema_files = schema/collective.ldif schema/collective.schema \
	schema/corba.ldif schema/corba.schema schema/core.ldif \
	schema/core.schema schema/cosine.ldif schema/cosine.schema \
	schema/duaconf.ldif schema/duaconf.schema schema/dyngroup.ldif \
	schema/dyngroup.schema schema/inetorgperson.ldif \
	schema/inetorgperson.schema schema/java.ldif schema/java.schema \
	schema/misc.ldif schema/misc.schema schema/nis.ldif \
	schema/nis.schema schema/openldap.ldif schema/openldap.schema \
	schema/pmi.ldif schema/pmi.schema schema/ppolicy.ldif \
	schema/ppolicy.schema schema/README

slapd.ldif: $(srcdir)/slapd.ldif.in Makefile
	@echo "Making $@" && $(RM) $@ && \
	echo "# Generated from $< on `date --rfc-3339=seconds --utc`" > $@ && \
	$(FIXUP_DIST) $< | $(FIXUP_DIRS) >> $@

slapd.conf: $(srcdir)/slapd.conf.in Makefile
	@echo "Making $@" && $(RM) $@ && \
	echo "# Generated from $< on `date --rfc-3339=seconds --utc`" > $@ && \
	$(FIXUP_DIST) $< | $(FIXUP_DIRS) >> $@

EXTRA_DIST += $(schema_files)

install-data-hook:
	$(MKDIR_P) $(sysconfdir)/schema && \
		(cd $(srcdir) && $(INSTALL) -t $(sysconfdir)/schema -m 0444 $(schema_files))

uninstall-hook:
	(cd $(sysconfdir) && rm -f $(schema_files)) && rmdir $(sysconfdir)/schema
