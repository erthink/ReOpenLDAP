dnl
dnl Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
dnl Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
dnl
dnl This file is part of ReOpenLDAP.
dnl
dnl ReOpenLDAP is free software; you can redistribute it and/or modify it under
dnl the terms of the GNU Affero General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl ReOpenLDAP is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU Affero General Public License for more details.
dnl
dnl You should have received a copy of the GNU Affero General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl
dnl ---
dnl
dnl
dnl Copyright 1998-2014 The OpenLDAP Foundation.
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted only as authorized by the OpenLDAP
dnl Public License.
dnl
dnl A copy of this license is available in the file LICENSE in the
dnl top-level directory of the distribution or, alternatively, at
dnl <http://www.OpenLDAP.org/license.html>.
dnl
dnl ----------------------------------------------------------------
dnl Disable config.cache!
dnl define([AC_CACHE_LOAD], )dnl
dnl define([AC_CACHE_SAVE], )dnl
dnl ----------------------------------------------------------------
dnl Disable libtool support for languages we don't use
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_GCJ_CONFIG], [:])dnl
dnl ================================================================
dnl Configure.in for ReOpenLDAP
AC_COPYRIGHT([[Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.

This is part of ReOpenLDAP.

ReOpenLDAP is free software; you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

ReOpenLDAP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

---

Copyright 1998-2014 The OpenLDAP Foundation. All rights reserved.
Restrictions apply, see COPYRIGHT and LICENSE files.]])

AC_PREREQ([2.69])dnl Required Autoconf version
AC_REVISION([$Id: 8c9139400f1cfd3f68bc9b65828515fe85e91ee1 $])
AC_INIT(
	m4_esyscmd_s([build/BRANDING --title]),
	m4_esyscmd_s([build/BRANDING --version]),
	m4_esyscmd_s([build/BRANDING --bugs]),
	m4_esyscmd_s([build/BRANDING --tarname]),
	m4_esyscmd_s([build/BRANDING --url]))
AC_CONFIG_MACRO_DIRS([build/m4])
dnl ----------------------------------------------------------------
dnl OpenLDAP Autoconf Macros
builtin(include, build/openldap.m4)
dnl ================================================================

AC_CONFIG_AUX_DIR(build)

REOPENLDAP_REPO=""
if test -d $ac_aux_dir/../.git; then
	REOPENLDAP_REPO="(from Git clone) "
fi

echo "Configuring ${PACKAGE_TITLE} ${REOPENLDAP_REPO}..."

dnl Determine host platform
dnl		we try not to use this for much
AC_CANONICAL_TARGET([])

# LY: +check-news
AM_INIT_AUTOMAKE([subdir-objects --warnings=no-portability --warnings=error dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([disable])
LT_CONFIG_LTDL_DIR([build/libltdl])
LT_INIT([dlopen])

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(PACKAGE_URL)
AC_SUBST(PACKAGE_BUGREPORT)

VERSION_NUMBER=m4_esyscmd_s([build/BRANDING --number])
AC_SUBST(VERSION_NUMBER)
VERSION_MAJOR=m4_esyscmd_s([build/BRANDING --major])
AC_SUBST(VERSION_MAJOR)
VERSION_MINOR=m4_esyscmd_s([build/BRANDING --minor])
AC_SUBST(VERSION_MINOR)
VERSION_PATCH=m4_esyscmd_s([build/BRANDING --patch])
AC_SUBST(VERSION_PATCH)
RELEASE_DATE=m4_esyscmd_s([build/BRANDING --date])
AC_SUBST(RELEASE_DATE)
RELEASE_STAMP=m4_esyscmd_s([build/BRANDING --stamp])
AC_SUBST(RELEASE_STAMP)
API_LIB_RELEASE=m4_esyscmd_s([build/BRANDING --api])
AC_SUBST(API_LIB_RELEASE)
API_LIB_VERSION=m4_esyscmd_s([build/BRANDING --so])
AC_SUBST(API_LIB_VERSION)

AC_DEFINE_UNQUOTED(REOPENLDAP_PACKAGE,"$PACKAGE",[Name of the Package])
AC_DEFINE_UNQUOTED(REOPENLDAP_VERSION,"$VERSION", [Package version as a string])
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION,$VERSION_NUMBER, [Package version as number])
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION_MAJOR,$VERSION_MAJOR,[Major version])
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION_MINOR,$VERSION_MINOR,[Minor version])
AC_DEFINE_UNQUOTED(LDAP_VENDOR_VERSION_PATCH,$VERSION_PATCH,[Patch level])
AC_DEFINE_UNQUOTED(RELEASE_DATE,"$RELEASE_DATE",[Date of release])
AC_DEFINE_UNQUOTED(RELEASE_STAMP,"$RELEASE_STAMP",[Digest stamp])
AC_DEFINE_UNQUOTED(API_LIB_RELEASE,"$API_LIB_RELEASE",[API version])
AC_DEFINE_UNQUOTED(API_LIB_VERSION,"$API_LIB_VERSION",[so-lib version])

MKBANNER_CMD="\$(top_srcdir)/build/mkbanner --version '$VERSION'"
MKBANNER="\$(MKBANNER_VERBOSE_\$(V))"
MKBANNER_VERBOSE_="\$(MKBANNER_VERBOSE_\$(AM_DEFAULT_VERBOSITY))"
MKBANNER_VERBOSE_0="@echo '  GEN      \$@' && ${MKBANNER_CMD}"
MKBANNER_VERBOSE_1="${MKBANNER_CMD}"
AC_SUBST(MKBANNER)
AC_SUBST(MKBANNER_VERBOSE_)
AC_SUBST(MKBANNER_VERBOSE_0)
AC_SUBST(MKBANNER_VERBOSE_1)

FIXUP_DIST="\$(SED) \
	-e 's|@PACKAGE[@]|\$(PACKAGE)|Ig' \
	-e 's|@VERSION[@]|\$(VERSION)|Ig' \
	-e 's|@PACKAGE_URL[@]|\$(PACKAGE_URL)|Ig' \
	-e 's|@PACKAGE_BUGREPORT[@]|\$(PACKAGE_BUGREPORT)|Ig' \
	-e 's|@VERSION_NUMBER[@]|\$(VERSION_NUMBER)|Ig' \
	-e 's|@VERSION_MAJOR[@]|\$(VERSION_MAJOR)|Ig' \
	-e 's|@VERSION_MINOR[@]|\$(VERSION_MINOR)|Ig' \
	-e 's|@VERSION_PATCH[@]|\$(VERSION_PATCH)|Ig' \
	-e 's|@RELEASE_DATE[@]|\$(RELEASE_DATE)|Ig' \
	-e 's|@RELEASE_STAMP[@]|\$(RELEASE_STAMP)|Ig' \
	-e 's|@API_LIB_RELEASE[@]|\$(API_LIB_RELEASE)|Ig' \
	-e 's|@API_LIB_VERSION[@]|\$(API_LIB_VERSION)|Ig'"
AC_SUBST(FIXUP_DIST)

FIXUP_DIRS="\$(SED) \
	-e 's|@sysconfdir[@]|\$(sysconfdir)|Ig' \
	-e 's|@datadir[@]|\$(datadir)|Ig' \
	-e 's|@bindir[@]|\$(bindir)|Ig' \
	-e 's|@sbindir[@]|\$(sbindir)|Ig' \
	-e 's|@libexecdir[@]|\$(libexecdir)|Ig' \
	-e 's|@moduledir[@]|\$(pkglibdir)|Ig' \
	-e 's|@localstatedir[@]|\$(localstatedir)|Ig' \
	-e 's|@runstatedir[@]|\$(runstatedir)|Ig' \
	-e 's|@localedir[@]|\$(localedir)|Ig' \
	-e 's|[^:/]//|/|g'"
AC_SUBST(FIXUP_DIRS)

AC_LANG([C])
AC_PREFIX_DEFAULT("/opt/reopenldap")

dnl ----------------------------------------------------------------
AC_PROG_CC([gcc clang icc cc])
if test "${ac_cv_prog_cc_stdc}" = no ; then
	AC_MSG_ERROR([ReOpenLDAP requires compiler to support STDC constructs.])
fi

AC_PROG_CC_C99
if test "$ac_cv_prog_cc_c99" = no; then
	AC_MSG_ERROR([ReOpenLDAP requires compiler to support C99 constructs.])
fi

AC_PROG_CXX([g++ clang++ icpc c++])
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O
AC_PROG_SED
AC_PROG_GREP
AC_PROG_MKDIR_P
AC_PROG_EGREP
AM_CONDITIONAL(COMPILER_IS_GCC, test "x$GCC" = "xyes")
PKG_PROG_PKG_CONFIG
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_DEFINE(_GNU_SOURCE, 1, [Enable GNU extensions.])
AC_DEFINE(_ISOC99_SOURCE, 1, [Define to enforce ISO C99 compliance.])
AC_DEFINE(_POSIX_C_SOURCE, 200809L, [Define to enforce POSIX.1-2008 compliance.])
AC_DEFINE(_XOPEN_SOURCE, 700, [Define to enforce X/Open 7 (XSI) compliance.])
AC_DEFINE(_REENTRANT, 1, [Define to enable reentrancy interfaces.])

dnl ----------------------------------------------------------------

AH_TOP([
/* begin of reldap_autoconf.h.pre */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef RELDAP_AUTOCONF_H
#define RELDAP_AUTOCONF_H

/* LY: Please do not ask us for Windows support, just never!
 * But you can make a fork for Windows, or become maintainer for FreeBSD... */
#ifndef __gnu_linux__
#	error "ReOpenLDAP supports only GNU Linux"
#endif

/* define this if needed to get reentrant functions */
#ifndef REENTRANT
#	undef REENTRANT
#endif
#ifndef _REENTRANT
#	undef _REENTRANT
#endif

/* define this if needed to get threadsafe functions */
#ifndef THREADSAFE
#	undef THREADSAFE
#endif
#ifndef _THREADSAFE
#	undef _THREADSAFE
#endif
#ifndef THREAD_SAFE
#	undef THREAD_SAFE
#endif
#ifndef _THREAD_SAFE
#	undef _THREAD_SAFE
#endif

#ifndef _SGI_MP_SOURCE
#	undef _SGI_MP_SOURCE
#endif

/* end of reldap_autoconf.h.pre */
])
AH_BOTTOM([
/* begin of reldap_autoconf.h.post */

/* force LDAP_P to always include prototypes */
#define LDAP_NEEDS_PROTOTYPES 1

#ifdef HAVE_FEATURES_H
#	include <features.h>
#endif

#ifdef HAVE_STDDEF_H
#	include <stddef.h>
#endif

#ifdef HAVE_ANSIDECL_H
#	include <ansidecl.h>
#endif

#ifdef HAVE_STDINT_H
#	include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#	include <inttypes.h>
#endif

#ifdef HAVE_ENDIAN_H
#	include <endian.h>
#endif

#endif /* RELDAP_AUTOCONF_H */
/* end of reldap_autoconf.h.post */])

AC_CONFIG_HEADERS([reldap_autoconf.h])
AC_CONFIG_HEADERS([include/ldap_features.h])
AC_CONFIG_HEADERS([include/lber_types.h])

dnl ================================================================
dnl Start Args
AC_MSG_CHECKING(configure arguments)

dnl ----------------------------------------------------------------
dnl --with-subdir
ldap_subdir="/reopenldap"

AC_ARG_WITH(subdir,
[  --with-subdir=DIR       change default subdirectory used for installs],
[case "$withval" in
	no) ldap_subdir=""
		;;
	yes)
		;;
	/*|\\*)
		ldap_subdir="$withval"
		;;
	*)
		ldap_subdir="/$withval"
		;;
esac
])dnl

AC_SUBST(ldap_subdir)dnl

dnl ----------------------------------------------------------------
dnl General "enable" options
dnl set default to traditional to enable the original debug style
OL_ARG_ENABLE(debug,[  --enable-debug          enable debug logging], yes, [no yes traditional])dnl
OL_ARG_ENABLE(syslog,[  --enable-syslog         enable syslog support], auto)dnl
OL_ARG_ENABLE(contrib,[  --enable-contrib        enable extra plugins and overlays], no, [no yes broken], ol_enable_contrib)dnl
OL_ARG_ENABLE(experimental,[  --enable-experimental   enable experimental and developing features], no, [no yes])dnl
OL_ARG_ENABLE(check,[  --enable-check          enable internal checking and assertions], no, [no yes always default])dnl
OL_ARG_ENABLE(hipagut,[  --enable-hipagut        enable internal memory allocation debugger], no, [no yes always extra])dnl
OL_ARG_ENABLE(proctitle,[  --enable-proctitle      enable proctitle support], yes)dnl
dnl OL_ARG_ENABLE(referrals,[  --enable-referrals      enable LDAPv2+ Referrals (experimental)], no)dnl
ol_enable_referrals=${ol_enable_referrals-no}
OL_ARG_ENABLE(ipv6,[  --enable-ipv6           enable IPv6 support], auto)dnl
OL_ARG_ENABLE(local,[  --enable-local          enable AF_LOCAL (AF_UNIX) socket support], auto)dnl
OL_ARG_ENABLE(deprecated,[  --enable-deprecated     enable deprecated interfaces of libreldap], no, [no yes])dnl

if test -a $ac_aux_dir/../tests/scripts/valgrind.supp; then
	AX_VALGRIND_DFLT([memcheck sgcheck], [on])
	AX_VALGRIND_CHECK
	if test $enable_valgrind = yes; then
#		PKG_CHECK_MODULES(VALGRIND, valgrind, valgrind_found=yes, valgrind_found=no)
#		AC_SUBST(VALGRIND_LIBS)
#		AC_SUBST(VALGRIND_CFLAGS)
		AC_DEFINE(HAVE_VALGRIND,1,[define if you have Valgrind])
		AC_CHECK_HEADER([valgrind/memcheck.h])
		if test "$ac_cv_header_valgrind_memcheck_h" = yes; then
			AC_DEFINE(USE_VALGRIND,1,[define to enable interaction with Valgrind])
		else
			AC_MSG_WARN([valgrind/memcheck.h required for Valgrind complete support.])
		fi
	fi
fi

case $ol_enable_experimental in
    yes)
	AC_DEFINE(LDAP_EXPERIMENTAL,1,[define as 0 to disable experimantal/developing features])
	;;
    no)
	AC_DEFINE(LDAP_EXPERIMENTAL,0,[define as 1 to enable experimantal/developing features])
	;;
    *)
	AC_MSG_FAILURE([invalid --enable-experimental value '$ol_enable_experimental'])
	;;
esac

case $ol_enable_check in
    yes)
	AC_DEFINE_UNQUOTED(LDAP_CHECK,1 /* available */,[define as 2 to enable runtime checking by default])
	;;
    default)
	AC_DEFINE_UNQUOTED(LDAP_CHECK,2 /* enabled */,[define as 3 to always runtime checking])
	;;
    always)
	AC_DEFINE_UNQUOTED(LDAP_CHECK,3 /* always */,[define as 0 to disable runtime checking])
	;;
    no)
	AC_DEFINE_UNQUOTED(LDAP_CHECK,0 /* disabled */,[define as 1 to enable runtime checking])
	;;
    *)
	AC_MSG_FAILURE([invalid --enable-check value '$ol_enable_check'])
	;;
esac

case $ol_enable_hipagut in
    yes)
	AC_DEFINE_UNQUOTED(LDAP_MEMORY_DEBUG,1 /* available */,[define as 2 to enable runtime checking by default])
	;;
    always)
	AC_DEFINE_UNQUOTED(LDAP_MEMORY_DEBUG,2 /* always */,[define as 3 to always runtime checking with stats])
	;;
    extra)
	AC_DEFINE_UNQUOTED(LDAP_MEMORY_DEBUG,3 /* extra */,[define as 0 to disable runtime checking])
	;;
    no)
	AC_DEFINE_UNQUOTED(LDAP_MEMORY_DEBUG,0 /* disabled */,[define as 1 to enable runtime checking])
	;;
    *)
	AC_MSG_FAILURE([invalid --enable-hipagut value '$ol_enable_hipagut'])
	;;
esac

AM_CONDITIONAL([SLAPD_DEBUG], [test "$ol_enable_debug" = yes])
AM_CONDITIONAL([SLAPD_SYSLOG], [test "$ol_enable_syslog" = yes])
AM_CONDITIONAL([LDAP_DEPRECATED], [test "$ol_enable_deprecated" = yes])
AM_CONDITIONAL([LDAP_EXPERIMENTAL], [test "$ol_enable_experimental" = yes])

dnl ----------------------------------------------------------------
dnl General "with" options
OL_ARG_WITH(cyrus_sasl,[  --with-cyrus-sasl	  with Cyrus SASL support],
	auto, [auto yes no] )
OL_ARG_WITH(fetch,[  --with-fetch		  with fetch(3) URL support],
	auto, [auto yes no] )
OL_ARG_WITH(tls,[  --with-tls		  with TLS/SSL support auto|openssl|gnutls|moznss],
	auto, [auto openssl gnutls moznss yes no] )
OL_ARG_WITH(yielding_select,
	[  --with-yielding-select  with implicitly yielding select],
	auto, [auto yes no manual] )
OL_ARG_WITH(mp,
	[  --with-mp               with multiple precision statistics auto|longlong|long|bignum|gmp],
	auto, [auto longlong long bignum gmp yes no])
OL_ARG_WITH(odbc,
	[  --with-odbc             with specific ODBC support iodbc|unixodbc|auto],
	auto, [auto iodbc unixodbc] )


dnl ----------------------------------------------------------------
dnl Server options
dnl ----------------------------------------------------------------

dnl ----------------------------------------------------------------
dnl SLAPD OPTIONS

slapd_features="slapi modules wrappers rlookups dynacl aci rewrite"

AC_ARG_ENABLE(xxslapdoptions,[
SLAPD (Standalone LDAP Daemon) Options:])
OL_ARG_ENABLE(slapd,[  --enable-slapd	  enable building slapd], yes)dnl
OL_ARG_ENABLE(dynacl,[    --enable-dynacl	  enable run-time loadable ACL support (experimental)], no)dnl
OL_ARG_ENABLE(aci,[    --enable-aci	  enable per-object ACIs (experimental)], no, [no yes mod])dnl
OL_ARG_ENABLE(cleartext,[    --enable-cleartext	  enable cleartext passwords], yes)dnl
OL_ARG_ENABLE(crypt,[    --enable-crypt	  enable crypt(3) passwords], no)dnl
OL_ARG_ENABLE(lmpasswd,[    --enable-lmpasswd	  enable LAN Manager passwords], no)dnl
OL_ARG_ENABLE(spasswd,[    --enable-spasswd	  enable (Cyrus) SASL password verification], no)dnl
OL_ARG_ENABLE(modules,[    --enable-modules	  enable dynamic module support], yes)dnl
OL_ARG_ENABLE(rewrite,[    --enable-rewrite	  enable DN rewriting in back-ldap and rwm overlay], auto)dnl
OL_ARG_ENABLE(rlookups,[    --enable-rlookups	  enable reverse lookups of client hostnames], no)dnl
OL_ARG_ENABLE(slapi,[    --enable-slapi        enable SLAPI support (experimental)], no)dnl
OL_ARG_ENABLE(slp,[    --enable-slp          enable SLPv2 support], no)dnl
OL_ARG_ENABLE(wrappers,[    --enable-wrappers	  enable tcp wrapper support], no)dnl

dnl ----------------------------------------------------------------
dnl SLAPD Backend Options
slapd_backends="mdb bdb dnssrv hdb ldap meta monitor ndb null passwd perl \
	relay shell sock sql"

AC_ARG_ENABLE(xxslapbackends,[
SLAPD Backend Options:])

OL_ARG_ENABLE(backends,[    --enable-backends	  enable all available backends],
	--, [no yes mod])dnl
OL_ARG_ENABLE(mdb,[    --enable-mdb	  enable MDBX database backend],
	yes, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(hdb,[    --enable-hdb	  enable Hierarchical Berkeley DB backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(bdb,[    --enable-bdb	  enable Berkeley DB backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(dnssrv,[    --enable-dnssrv	  enable dnssrv backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(ldap,[    --enable-ldap	  enable ldap backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(meta,[    --enable-meta	  enable metadirectory backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(monitor,[    --enable-monitor	  enable monitor backend],
	yes, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(ndb,[    --enable-ndb	  enable MySQL NDB Cluster backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(null,[    --enable-null	  enable null backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(passwd,[    --enable-passwd	  enable passwd backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(perl,[    --enable-perl	  enable perl backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(relay,[    --enable-relay  	  enable relay backend],
	yes, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(shell,[    --enable-shell	  enable shell backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(sock,[    --enable-sock	  enable sock backend],
	no, [no yes mod], ol_enable_backends)dnl
OL_ARG_ENABLE(sql,[    --enable-sql	  enable sql backend],
	no, [no yes mod], ol_enable_backends)dnl

dnl ----------------------------------------------------------------
dnl SLAPD Overlay Options
slapd_overlays="accesslog auditlog collect constraint dds deref \
	dyngroup dynlist memberof ppolicy pcache refint \
	retcode rwm seqmod sssvlv syncprov translucent unique \
	valsort"

AC_ARG_ENABLE(xxslapoverlays,[
SLAPD Overlay Options:])

OL_ARG_ENABLE(overlays,[    --enable-overlays	  enable all available overlays],
	--, [no yes mod])dnl
OL_ARG_ENABLE(accesslog,[    --enable-accesslog	  In-Directory Access Logging overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(auditlog,[    --enable-auditlog	  Audit Logging overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(collect,[    --enable-collect	  Collect overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(constraint,[    --enable-constraint	  Attribute Constraint overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(dds,[    --enable-dds  	  Dynamic Directory Services overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(deref,[    --enable-deref	  Dereference overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(dyngroup,[    --enable-dyngroup	  Dynamic Group overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(dynlist,[    --enable-dynlist	  Dynamic List overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(memberof,[    --enable-memberof	  Reverse Group Membership overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(ppolicy,[    --enable-ppolicy	  Password Policy overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(pcache,[    --enable-pcache	  Proxy Cache overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(refint,[    --enable-refint	  Referential Integrity overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(retcode,[    --enable-retcode	  Return Code testing overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(rwm,[    --enable-rwm       	  Rewrite/Remap overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(seqmod,[    --enable-seqmod	  Sequential Modify overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(sssvlv,[    --enable-sssvlv	  ServerSideSort/VLV overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(syncprov,[    --enable-syncprov	  Syncrepl Provider overlay],
	yes, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(translucent,[    --enable-translucent  Translucent Proxy overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(unique,[    --enable-unique       Attribute Uniqueness overlay],
	no, [no yes mod], ol_enable_overlays)
OL_ARG_ENABLE(valsort,[    --enable-valsort      Value Sorting overlay],
	no, [no yes mod], ol_enable_overlays)

dnl ----------------------------------------------------------------
AC_ARG_ENABLE(xxliboptions,[
Library Generation & Linking Options])
LTDL_INIT([recursive])
AC_ENABLE_STATIC
AC_ENABLE_SHARED

dnl ----------------------------------------------------------------
# validate options

for o in $slapd_features $slapd_backends $slapd_overlays; do
	eval "ol_value=\$ol_enable_$o"
	if test $ol_enable_slapd = no -a $ol_value != no; then
		AC_MSG_ERROR([slapd disabled, therefore --enable-$o=$ol_value unavailable ])
	elif test $ol_enable_modules = no -a $ol_value = mod; then
		AC_MSG_ERROR([dynamic modules disabled, therefore --enable-$o=$ol_value unavailable ])
	fi
done

if test $ol_enable_contrib != no; then
	if test $ol_enable_slapd = no ; then
		AC_MSG_ERROR([slapd disabled, therefore --enable-contrib unavailable ])
	else
		if test $ol_enable_modules = no; then
			AC_MSG_WARN([dynamic modules disabled, therefore contrib-overlays unavailable ])
		fi
		if test $ol_enable_slapi = no; then
			AC_MSG_WARN([SLAPI disabled, therefore slapi contrib-pluginus unavailable ])
		fi
		if test $ol_enable_dynacl = no; then
			AC_MSG_WARN([dynamic ACL disabled, therefore acl contrib-pluginus unavailable ])
		fi
	fi
	PKG_CHECK_MODULES(HEIMDAL, heimdal-krb5 heimdal-kadm-server)
	AC_SUBST(HEIMDAL_LIBS)
	AC_SUBST(HEIMDAL_CFLAGS)
fi

if test $ol_enable_slapd != no ; then
	be=
	for o in $slapd_backends; do
		eval "ol_value=\$ol_enable_$o"
		if test $ol_enable_slapd != no; then
			be=$o
			break
		fi
	done
	if test -z "$be"; then
		AC_MSG_WARN([no backend specified, slapd will be used ldif])
	fi
fi

if test $ol_enable_meta/$ol_enable_ldap = yes/no ; then
	AC_MSG_ERROR([--enable-meta requires --enable-ldap])
fi

if test $ol_enable_lmpasswd = yes ; then
	if test $ol_with_tls = no ; then
		AC_MSG_ERROR([LAN Manager passwords require OpenSSL])
	fi
fi

if test $ol_enable_spasswd = yes ; then
	if test $ol_with_cyrus_sasl = no ; then
		AC_MSG_ERROR([options require --with-cyrus-sasl])
	fi
	ol_with_cyrus_sasl=yes
fi

AC_MSG_RESULT(done)

dnl ----------------------------------------------------------------
dnl Initialize vars
LTHREAD_LIBS=
LUTIL_LIBS=

SLAPD_LIBS=

SLAPD_BACKENDS_LIST="ldif $slapd_backends"
SLAPD_STATIC_BACKENDS=ldif
SLAPD_DYNAMIC_BACKENDS=

SLAPD_OVERLAYS_LIST=$slapd_overlays
SLAPD_STATIC_OVERLAYS=
SLAPD_DYNAMIC_OVERLAYS=

BDB_LIBS=
BDB_INCLUDES=

NDB_LIBS=
NDB_INCLUDES=
NDB_LDFLAGS=
NDB_RDIR=

PERL_LIBS=
PERL_CPPFLAGS=
PERL_LDFLAGS=
PERL_RDIR=

SQL_LIBS=
SQL_INCLUDES=

SASL_LIBS=
TLS_LIBS=
SLAPI_LIBS=
AUTH_LIBS=

#ICU_LIBS=

SLAPD_SLP_LIBS=
SLAPD_GMP_LIBS=
SLAPD_TCPWRAP_LIBS=

PKG_CHECK_MODULES(KRB5, krb5, krb5_found=yes, krb5_found=no)
AC_SUBST(KRB5_LIBS)
AC_SUBST(KRB5_CFLAGS)

dnl ================================================================
dnl Checks for programs

dnl ----------------------------------------------------------------
dnl Perl
ol_link_perl=no
if test $ol_enable_perl != no ; then
	AC_PATH_PROG(PERLBIN, perl, /usr/bin/perl)

	if test "no$PERLBIN" = "no" ; then
		if test $ol_enable_perl = yes ; then
			AC_MSG_ERROR([could not locate perl])
		fi

	else
		PERL_CPPFLAGS="`$PERLBIN -MExtUtils::Embed -e ccopts`"
		PERL_LIBS="`$PERLBIN -MExtUtils::Embed -e ldopts | sed -e 's/^-L /-L/g' -e 's/^-l /-l/g' | tr -s '[[:space:]]' '\n' | grep -e '^-l' -e '^-L' | grep -v '^-lc' | tr '\n' ' '`"
		PERL_LDFLAGS="`$PERLBIN -MExtUtils::Embed -e ldopts | sed -e 's/^-L /-L/g' -e 's/^-l /-l/g' | tr -s '[[:space:]]' '\n' | grep -v -e '^-l' -e '^-L' | tr '\n' ' '`"
		if test -z "$PERL_CPPFLAGS" -o -z "$PERL_LIBS"; then
			AC_MSG_ERROR([could not fetch perl libs or options])
		fi
		PERL_RDIR="`echo $PERL_LDFLAGS | tr '[[:space:]]' '\n' | grep -- '-Wl,-rpath,' | sed 's/-Wl,-rpath,/-R/'`"
		dnl should check perl version
		ol_link_perl=yes
	fi
fi

dnl ----------------------------------------------------------------
dnl Checks for file extensions
AC_EXEEXT
AC_OBJEXT
AC_DEFINE_UNQUOTED(EXEEXT, "${EXEEXT}", [defined to be the EXE extension])

dnl ----------------------------------------------------------------
dnl Check for module support

if test $ol_enable_modules != no -o $ol_enable_slapi != no; then
	# The lt_dladvise_init symbol was added with libtool-2.2
	if test yes != "$with_included_ltdl"; then
		save_CFLAGS=$CFLAGS
		save_LDFLAGS=$LDFLAGS
		CFLAGS="$CFLAGS $LTDLINCL"
		LDFLAGS="$LDFLAGS $LIBLTDL"
		AC_CHECK_LIB([ltdl], [lt_dladvise_init],
			[],
			[AC_MSG_ERROR([installed libltdl is too old])])
		LDFLAGS=$save_LDFLAGS
		CFLAGS=$save_CFLAGS
	fi
else
	for be in $slapd_backends; do
		eval "ol_tmp=\$ol_enable_$be"
		if test $ol_tmp = mod ; then
			AC_MSG_WARN([building static $be backend])
			eval "ol_enable_$be=yes"
		fi
	done
	for ov in $slapd_overlays; do
		eval "ol_tmp=\$ol_enable_$ov"
		if test $ol_tmp = mod ; then
			AC_MSG_WARN([building static $ov overlay])
			eval "ol_enable_$ov=yes"
		fi
	done
fi

dnl ----------------------------------------------------------------
dnl Checks for header files.
OL_HEADER_STDC

if test $ol_cv_header_stdc != yes; then
	AC_MSG_WARN([could not locate Standard C compliant headers])
fi

AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIOCGWINSZ

AC_CHECK_HEADERS(	\
	ansidecl.h	\
	arpa/inet.h	\
	arpa/nameser.h	\
	assert.h	\
	bits/types.h	\
	conio.h		\
	crypt.h		\
	direct.h	\
	errno.h		\
	endian.h	\
	features.h	\
	fcntl.h		\
	filio.h		\
	getopt.h	\
	grp.h		\
	inttypes.h	\
	io.h		\
	libutil.h	\
	limits.h	\
	locale.h	\
	malloc.h	\
	memory.h	\
	psap.h		\
	pwd.h		\
	process.h	\
	sgtty.h		\
	shadow.h	\
	stddef.h	\
	string.h	\
	strings.h	\
	stdint.h	\
	sysexits.h	\
	sys/file.h	\
	sys/filio.h	\
	sys/fstyp.h	\
	sys/errno.h	\
	sys/ioctl.h	\
	sys/param.h	\
	sys/privgrp.h	\
	sys/resource.h	\
	sys/select.h	\
	sys/socket.h	\
	sys/stat.h	\
	sys/syslog.h	\
	sys/time.h	\
	sys/types.h	\
	sys/uio.h	\
	sys/vmount.h	\
	syslog.h	\
	termios.h	\
	unistd.h	\
	utime.h		\
)

AC_CHECK_HEADERS( resolv.h, [], [],
[$ac_includes_default
#include <netinet/in.h>
])

AC_CHECK_HEADERS( netinet/tcp.h, [], [],
[$ac_includes_default
#include <netinet/in.h>
])

AC_CHECK_HEADERS( sys/ucred.h, [], [],
[$ac_includes_default
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
])

dnl ----------------------------------------------------------------
dnl Checks for libraries

AC_CHECK_FUNCS( sigaction sigset )

dnl Find socket()
dnl Likely combinations:
dnl		-lsocket [ -lnsl_s | -lnsl ]
dnl		-linet

AC_CHECK_FUNC(socket, :, [
dnl hopefully we won't include too many libraries
	AC_CHECK_LIB(socket, main)
	AC_CHECK_LIB(net, socket)
	AC_CHECK_LIB(nsl_s, main)
	AC_CHECK_LIB(nsl, main)
	AC_CHECK_LIB(inet, socket)
	AC_CHECK_LIB(gen, main)
])

dnl require select
AC_CHECK_FUNC(select, :, AC_MSG_ERROR([select() required.]))

if test "${ac_cv_header_winsock_h}" != yes; then
	dnl Select arg types
	dnl (if this detection becomes permenent, it and the select() detection
	dnl should be done before the yielding select test)
	AC_FUNC_SELECT_ARGTYPES
fi

dnl check to see if system call automatically restart
dnl AC_SYS_RESTARTABLE_SYSCALLS

dnl ----------------------------------------------------------------
AC_CHECK_FUNCS( poll )
if test $ac_cv_func_poll = yes; then
AC_CHECK_HEADERS( poll.h sys/poll.h )
fi

dnl ----------------------------------------------------------------
AC_CHECK_HEADERS( sys/epoll.h )
if test "${ac_cv_header_sys_epoll_h}" = yes; then
	AC_MSG_CHECKING(for epoll system call)
	AC_RUN_IFELSE([AC_LANG_SOURCE([[int main(int argc, char **argv)
{
	int epfd = epoll_create(256);
	exit (epfd == -1 ? 1 : 0);
}]])],[AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_EPOLL,1, [define if your system supports epoll])],[AC_MSG_RESULT(no)],[AC_MSG_RESULT(no)])
fi

dnl ----------------------------------------------------------------
AC_CHECK_HEADERS( sys/devpoll.h )
dnl "/dev/poll" needs <sys/poll.h> as well...
if test "${ac_cv_header_sys_devpoll_h}" = yes \
		-a "${ac_cv_header_poll_h}" = yes ; \
then
	AC_MSG_CHECKING(for /dev/poll)
	AC_RUN_IFELSE([AC_LANG_SOURCE([[int main(int argc, char **argv)
{
	int devpollfd = open("/dev/poll", /* O_RDWR */ 2);
	exit (devpollfd == -1 ? 1 : 0);
}]])],[AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_DEVPOLL,1, [define if your system supports /dev/poll])],[AC_MSG_RESULT(no)],[AC_MSG_RESULT(no)])
fi

dnl ----------------------------------------------------------------
OL_STRERROR

dnl ----------------------------------------------------------------

AC_CHECK_HEADERS(bfd.h libelf.h gelf.h)
AC_CHECK_LIB(elf, elf_begin, have_libelf=yes)
AC_CHECK_LIB(bfd, bfd_openr, have_libbfd=yes)

if test "$ac_cv_header_libelf_h" = yes -a "$ac_cv_header_gelf_h" = yes -a "$have_libelf" = yes; then
	AC_DEFINE(HAVE_LIBELF,1,[define 1 if you have libelf])
	SLAPD_LIBS="$SLAPD_LIBS -lelf"
	if test "$ac_cv_header_bfd_h" = yes -a "$have_libbfd" = yes; then
		AC_DEFINE(HAVE_LIBBFD,1,[define 1 if you have libbfd])
		SLAPD_LIBS="$SLAPD_LIBS -lbfd"
	fi
else
	AC_MSG_WARN([libelf not found, backtrace feature would be disabled])
fi

dnl ----------------------------------------------------------------
dnl require POSIX regex
AC_CHECK_HEADERS( regex.h, [], [],
[$ac_includes_default
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

if test "$ac_cv_header_regex_h" != yes ; then
	AC_MSG_ERROR([POSIX regex.h required.])
fi
AC_SEARCH_LIBS(regfree, [regex gnuregex],
	:, AC_MSG_ERROR([POSIX regex required.]))

OL_POSIX_REGEX
if test "$ol_cv_c_posix_regex" = no ; then
	AC_MSG_ERROR([broken POSIX regex!])
fi

dnl ----------------------------------------------------------------
dnl UUID Support

have_uuid=no
#PKG_CHECK_MODULES(UUID, uuid, have_uuid=yes, have_uuid=no)
#AC_SUBST(UUID_LIBS)
#AC_SUBST(UUID_CFLAGS)

AC_CHECK_HEADERS(sys/uuid.h)
dnl The HAVE_UUID_TO_STR code path also needs uuid_create
if test $ac_cv_header_sys_uuid_h = yes ; then
	save_LIBS="$LIBS"
	AC_SEARCH_LIBS([uuid_to_str], [uuid], [have_uuid=yes], :)
	AC_SEARCH_LIBS([uuid_create], [uuid], :, [have_uuid=no])
	LIBS="$save_LIBS"

	if test $have_uuid = yes ; then
		AC_DEFINE(HAVE_UUID_TO_STR,1,
			[define if you have uuid_to_str()])

		test "$ac_cv_search_uuid_to_str" = "none required" || \
			SLAPD_LIBS="$SLAPD_LIBS $ac_cv_search_uuid_to_str"
	fi
fi

dnl Look for uuid_generate
dnl The HAVE_UUID_GENERATE code path also needs uuid_unparse_lower
if test $have_uuid = no ; then
	AC_CHECK_HEADERS(uuid/uuid.h)
	if test $ac_cv_header_uuid_uuid_h = yes ; then
		save_LIBS="$LIBS"
		AC_SEARCH_LIBS([uuid_generate], [uuid], [have_uuid=yes], :)
		AC_SEARCH_LIBS([uuid_unparse_lower], [uuid], :, [have_uuid=no])
		LIBS="$save_LIBS"

		if test $have_uuid = yes ; then
			AC_DEFINE(HAVE_UUID_GENERATE,1,
				[define if you have uuid_generate()])

			test "$ac_cv_search_uuid_generate" = "none required" || \
				SLAPD_LIBS="$SLAPD_LIBS $ac_cv_search_uuid_generate"
		fi
	fi
fi

if test $have_uuid = no ; then
	AC_MSG_ERROR([uuid support not found (this typically means the uuid development package is missing)])
fi

dnl ----------------------------------------------------------------
dnl Check for resolver routines
OL_RESOLVER_LINK

ol_link_dnssrv=no
if test "$ol_cv_lib_resolver" != no ; then
	AC_DEFINE(HAVE_RES_QUERY,1,
		[define if you have res_query()])

	if test "$ol_enable_dnssrv" != no ; then
		ol_link_dnssrv=yes
	fi

	if test "$ol_cv_lib_resolver" != yes ; then
		LIBS="$ol_cv_lib_resolver $LIBS"
	fi
fi

if test "$ol_enable_dnssrv" = yes || test "$ol_enable_dnssrv" = mod ; then
	if test "$ol_link_dnssrv" = no ; then
		AC_MSG_ERROR([DNSSRV requires res_query()])
	fi
else
	ol_enable_dnssrv=no
fi

AC_CHECK_FUNCS( hstrerror )

dnl ----------------------------------------------------------------
dnl PF_INET6 support requires getaddrinfo and INET6_ADDRSTRLEN
dnl PF_LOCAL may use getaddrinfo in available
AC_CHECK_FUNCS( getaddrinfo getnameinfo gai_strerror inet_ntop )

ol_link_ipv6=no
if test $ac_cv_func_getaddrinfo = no || test $ac_cv_func_inet_ntop = no ; then
	if test $ol_enable_ipv6 = yes ; then
		AC_MSG_ERROR([IPv6 support requires getaddrinfo() and inet_ntop()])
	fi
elif test $ol_enable_ipv6 != no ; then
	AC_CACHE_CHECK([INET6_ADDRSTRLEN],[ol_cv_inet6_addrstrlen],[
		AC_EGREP_CPP(__has_inet6_addrstrlen__,[
#			include <netinet/in.h>
#			ifdef INET6_ADDRSTRLEN
				__has_inet6_addrstrlen__;
#			endif
		], [ol_cv_inet6_addrstrlen=yes], [ol_cv_inet6_addrstrlen=no])])


	AC_CACHE_CHECK([struct sockaddr_storage],ol_cv_struct_sockaddr_storage,[
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
]], [[
			struct sockaddr_storage ss;
]])],[ol_cv_struct_sockaddr_storage=yes],[ol_cv_struct_sockaddr_storage=no])])

	if test $ol_cv_inet6_addrstrlen = yes &&
	   test $ol_cv_struct_sockaddr_storage = yes ; then
		ol_link_ipv6=yes
	elif test $ol_enable_ipv6 = yes &&
	     test $ol_cv_inet6_addrstrlen = no ; then
		AC_MSG_ERROR([IPv6 support requires INET6_ADDRSTRLEN])
	elif test $ol_enable_ipv6 = yes &&
	     test $ol_cv_struct_sockaddr_storage = no ; then
		AC_MSG_ERROR([IPv6 support requires struct sockaddr_storage])
	fi
fi

if test $ol_enable_local != no ; then
	AC_CHECK_HEADERS( sys/un.h )

	if test $ol_enable_local = auto ; then
		ol_enable_local=$ac_cv_header_sys_un_h
	elif test $ac_cv_header_sys_un_h = no ; then
		AC_MSG_ERROR([AF_LOCAL domain support requires sys/un.h])
	fi
fi

dnl ----------------------------------------------------------------
dnl TLS/SSL

if test $ol_with_tls = yes ; then
	ol_with_tls=auto
fi

ol_link_tls=no
if test $ol_with_tls = openssl || test $ol_with_tls = auto ; then
#	PKG_CHECK_MODULES(CRYPTO, libcrypto libssl, ssl_found=yes, ssl_found=no)
#	AC_SUBST(CRYPTO_LIBS)
#	AC_SUBST(CRYPTO_CFLAGS)

	AC_CHECK_HEADERS(openssl/ssl.h)

	if test $ac_cv_header_openssl_ssl_h = yes ; then
		AC_CHECK_LIB(ssl, SSL_library_init,
			[have_openssl=yes
			need_rsaref=no], [have_openssl=no],
			[-lcrypto])

		if test $have_openssl = no ; then
			AC_CHECK_LIB(ssl, ssl3_accept,
				[have_openssl=yes
				need_rsaref=yes], [have_openssl=no],
				[-lcrypto -lRSAglue -lrsaref])
		fi

		if test $have_openssl = yes ; then
			ol_with_tls=openssl
			ol_link_tls=yes

			AC_DEFINE(HAVE_OPENSSL, 1,
				[define if you have OpenSSL])

			if test $need_rsaref = yes; then
				AC_DEFINE(HAVE_RSAREF, 1,
					[define if OpenSSL needs RSAref])

				TLS_LIBS="-lssl -lcrypto -lRSAglue -lrsaref"
			else
				TLS_LIBS="-lssl -lcrypto"
			fi

			OL_SSL_COMPAT
			if test $ol_cv_ssl_crl_compat = yes ; then
				AC_DEFINE(HAVE_OPENSSL_CRL, 1,
					[define if you have OpenSSL with CRL checking capability])
			fi
		fi
	fi
fi

if test $ol_link_tls = no ; then
	if test $ol_with_tls = gnutls || test $ol_with_tls = auto ; then
		AC_CHECK_HEADERS(gnutls/gnutls.h)

		if test $ac_cv_header_gnutls_gnutls_h = yes ; then
			AC_PREPROC_IFELSE([AC_LANG_SOURCE(
				[[#include <gnutls/gnutls.h>]
[#if GNUTLS_VERSION_NUMBER < 0x020c00]
[#error "GnuTLS 2.12.0 or newer required"]
[#endif]])],
				, [AC_MSG_FAILURE([GnuTLS is too old])])

			AC_CHECK_LIB(gnutls, gnutls_init,
				[have_gnutls=yes], [have_gnutls=no])

			if test $have_gnutls = yes ; then
				ol_with_tls=gnutls
				ol_link_tls=yes

				TLS_LIBS="-lgnutls"

				AC_DEFINE(HAVE_GNUTLS, 1,
					[define if you have GNUtls])
			fi
		fi
	fi
fi

dnl NOTE: caller must specify -I/path/to/nspr4 and -I/path/to/nss3
dnl and -L/path/to/nspr4 libs and -L/path/to/nss3 libs if those libs
dnl are not in the default system location
if test $ol_link_tls = no ; then
	if test $ol_with_tls = moznss || test $ol_with_tls = auto ; then
		have_moznss=no
		AC_CHECK_HEADERS([nssutil.h])
		if test "$ac_cv_header_nssutil_h" = yes ; then
			AC_CHECK_LIB([nss3], [NSS_Initialize],
						 [ have_moznss=yes ], [ have_moznss=no ])
		fi

		if test "$have_moznss" = yes ; then
			ol_with_tls=moznss
			ol_link_tls=yes
			AC_DEFINE(HAVE_MOZNSS, 1,
					  [define if you have MozNSS])
			TLS_LIBS="-lssl3 -lsmime3 -lnss3 -lnssutil3 -lplds4 -lplc4 -lnspr4"
		else
			if test $ol_with_tls = moznss ; then
			AC_MSG_ERROR([MozNSS not found - please specify the location to the NSPR and NSS header files in CPPFLAGS and the location to the NSPR and NSS libraries in LDFLAGS (if not in the system location)])
			fi
		fi
	fi
fi

WITH_TLS=no
if test $ol_link_tls = yes ; then
	AC_DEFINE(HAVE_TLS, 1, [define if you have TLS])
	WITH_TLS=yes
elif test $ol_with_tls = auto ; then
	AC_MSG_WARN([Could not locate TLS/SSL package])
	AC_MSG_WARN([TLS data protection not supported!])
elif test $ol_with_tls != no ; then
	AC_MSG_ERROR([Could not locate TLS/SSL package])
else
	AC_MSG_WARN([TLS data protection not supported!])
fi

dnl ----------------------------------------------------------------
dnl LAN Manger password checking requires DES from OpenSSL
if test $ol_enable_lmpasswd != no; then
	if test $ol_link_tls != yes ; then
		AC_MSG_ERROR([LAN Manager passwords require OpenSSL])
	fi

	AC_DEFINE(SLAPD_LMHASH, 1, [define to support LAN Manager passwords])
fi

dnl ----------------------------------------------------------------
dnl Threads?

AX_PTHREAD(ol_with_threads=posix, ol_with_threads=no)
if test $ol_with_threads != posix ; then
	AC_MSG_ERROR([no POSIX thread support])
else
	BUILD_THREAD=yes
fi

ol_link_threads=no
case $ol_with_threads in auto | yes | posix)

	AC_CHECK_HEADERS(pthread.h)

	if test $ac_cv_header_pthread_h = yes ; then
		OL_POSIX_THREAD_VERSION

		if test $ol_cv_pthread_version != 0 ; then
			AC_DEFINE_UNQUOTED(HAVE_PTHREADS,$ol_cv_pthread_version,
				[define to pthreads API spec revision])
		else
			AC_MSG_ERROR([unknown pthread version])
		fi

		# consider threads found
		ol_with_threads=found

		OL_HEADER_LINUX_THREADS
		OL_HEADER_GNU_PTH_PTHREAD_H

		if test $ol_cv_header_gnu_pth_pthread_h = no ; then
			AC_CHECK_HEADERS(sched.h)
		fi

		dnl Now the hard part, how to link?
		dnl
		dnl currently supported checks:
		dnl
		dnl Check for no flags
		dnl 	pthread_create() in $LIBS
		dnl
		dnl Check special pthread (final) flags
		dnl 	[skipped] pthread_create() with -mt (Solaris) [disabled]
		dnl 	pthread_create() with -kthread (FreeBSD)
		dnl 	pthread_create() with -pthread (FreeBSD/Digital Unix)
		dnl 	pthread_create() with -pthreads (?)
		dnl 	pthread_create() with -mthreads (AIX)
		dnl 	pthread_create() with -thread (?)
		dnl
		dnl Check pthread (final) libraries
		dnl 	pthread_mutex_unlock() in -lpthread -lmach -lexc -lc_r (OSF/1)
		dnl 	pthread_mutex_lock() in -lpthread -lmach -lexc (OSF/1)
		dnl 	[skipped] pthread_mutex_trylock() in -lpthread -lexc (OSF/1)
		dnl 	pthread_join() -Wl,-woff,85 -lpthread (IRIX)
		dnl 	pthread_create() in -lpthread (many)
		dnl 	pthread_create() in -lc_r (FreeBSD)
		dnl
		dnl Check pthread (draft4) flags (depreciated)
		dnl 	pthread_create() with -threads (OSF/1)
		dnl
		dnl Check pthread (draft4) libraries (depreciated)
		dnl 	pthread_mutex_unlock() in -lpthreads -lmach -lexc -lc_r (OSF/1)
		dnl 	pthread_mutex_lock() in -lpthreads -lmach -lexc (OSF/1)
		dnl 	pthread_mutex_trylock() in -lpthreads -lexc (OSF/1)
		dnl 	pthread_create() in -lpthreads (many)
		dnl

		dnl pthread_create in $LIBS
		AC_CACHE_CHECK([for pthread_create in default libraries],
			ol_cv_pthread_create,[
			AC_RUN_IFELSE([OL_PTHREAD_TEST_PROGRAM],
				[ol_cv_pthread_create=yes],
				[ol_cv_pthread_create=no],
				[AC_LINK_IFELSE([AC_LANG_PROGRAM([[OL_PTHREAD_TEST_INCLUDES]], [[OL_PTHREAD_TEST_FUNCTION]])],[ol_cv_pthread_create=yes],[ol_cv_pthread_create=no])])])

		if test $ol_cv_pthread_create != no ; then
			ol_link_threads=posix
			ol_link_pthreads=""
		fi

dnl		OL_PTHREAD_TRY([-mt],		[ol_cv_pthread_mt])
		OL_PTHREAD_TRY([-kthread],	[ol_cv_pthread_kthread])
		OL_PTHREAD_TRY([-pthread],	[ol_cv_pthread_pthread])
		OL_PTHREAD_TRY([-pthreads],	[ol_cv_pthread_pthreads])
		OL_PTHREAD_TRY([-mthreads],	[ol_cv_pthread_mthreads])
		OL_PTHREAD_TRY([-thread],	[ol_cv_pthread_thread])

		OL_PTHREAD_TRY([-lpthread -lmach -lexc -lc_r],
			[ol_cv_pthread_lpthread_lmach_lexc_lc_r])
		OL_PTHREAD_TRY([-lpthread -lmach -lexc],
			[ol_cv_pthread_lpthread_lmach_lexc])
dnl		OL_PTHREAD_TRY([-lpthread -lexc],
dnl			[ol_cv_pthread_lpthread_lexc])

		OL_PTHREAD_TRY([-lpthread -Wl,-woff,85],
			[ol_cv_pthread_lib_lpthread_woff])

		OL_PTHREAD_TRY([-lpthread],	[ol_cv_pthread_lpthread])
		OL_PTHREAD_TRY([-lc_r],		[ol_cv_pthread_lc_r])

		OL_PTHREAD_TRY([-threads],	[ol_cv_pthread_threads])

		OL_PTHREAD_TRY([-lpthreads -lmach -lexc -lc_r],
			[ol_cv_pthread_lpthreads_lmach_lexc_lc_r])
		OL_PTHREAD_TRY([-lpthreads -lmach -lexc],
			[ol_cv_pthread_lpthreads_lmach_lexc])
		OL_PTHREAD_TRY([-lpthreads -lexc],
			[ol_cv_pthread_lpthreads_lexc])

		OL_PTHREAD_TRY([-lpthreads],[ol_cv_pthread_lib_lpthreads])

		if test $ol_link_threads != no ; then
			LTHREAD_LIBS="$LTHREAD_LIBS $ol_link_pthreads"

			dnl save flags
			save_CPPFLAGS="$CPPFLAGS"
			save_LIBS="$LIBS"
			LIBS="$LTHREAD_LIBS $LIBS"

			dnl All POSIX Thread (final) implementations should have
			dnl sched_yield instead of pthread yield.
			dnl check for both, and thr_yield for Solaris
			AC_CHECK_FUNCS(sched_yield pthread_yield thr_yield)

			if test $ac_cv_func_sched_yield = no &&
			   test $ac_cv_func_pthread_yield = no &&
			   test $ac_cv_func_thr_yield = no ; then
				dnl Digital UNIX has sched_yield() in -lrt
				AC_CHECK_LIB(rt, sched_yield,
					[LTHREAD_LIBS="$LTHREAD_LIBS -lrt"
					AC_DEFINE(HAVE_SCHED_YIELD,1,
						[Define if you have the sched_yield function.])
					ac_cv_func_sched_yield=yes],
					[ac_cv_func_sched_yield=no])
			fi
			if test $ac_cv_func_sched_yield = no &&
			   test $ac_cv_func_pthread_yield = no &&
			   test "$ac_cv_func_thr_yield" = no ; then
				AC_MSG_WARN([could not locate sched_yield() or pthread_yield()])
			fi

			dnl Check functions for compatibility
			AC_CHECK_FUNCS(pthread_kill)

			dnl Check for pthread_rwlock_destroy with <pthread.h>
			dnl as pthread_rwlock_t may not be defined.
			AC_CACHE_CHECK([for pthread_rwlock_destroy with <pthread.h>],
				[ol_cv_func_pthread_rwlock_destroy], [
				dnl save the flags
				AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <pthread.h>
pthread_rwlock_t rwlock;
]], [[pthread_rwlock_destroy(&rwlock);]])],[ol_cv_func_pthread_rwlock_destroy=yes],[ol_cv_func_pthread_rwlock_destroy=no])
			])
			if test $ol_cv_func_pthread_rwlock_destroy = yes ; then
				AC_DEFINE(HAVE_PTHREAD_RWLOCK_DESTROY,1,
					[define if you have pthread_rwlock_destroy function])
			fi

			dnl Check for pthread_detach with <pthread.h> inclusion
			dnl as it's symbol may have been mangled.
			AC_CACHE_CHECK([for pthread_detach with <pthread.h>],
				[ol_cv_func_pthread_detach], [
				dnl save the flags
				AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <pthread.h>
#ifndef NULL
#define NULL (void*)0
#endif
]], [[pthread_detach(NULL);]])],[ol_cv_func_pthread_detach=yes],[ol_cv_func_pthread_detach=no])
			])

			if test $ol_cv_func_pthread_detach = no ; then
				AC_MSG_ERROR([could not locate pthread_detach()])
			fi

			AC_DEFINE(HAVE_PTHREAD_DETACH,1,
				[define if you have pthread_detach function])

			dnl Check for setconcurreny functions
			AC_CHECK_FUNCS(	\
				pthread_setconcurrency \
				pthread_getconcurrency \
				thr_setconcurrency \
				thr_getconcurrency \
			)

			OL_SYS_LINUX_THREADS
			OL_LINUX_THREADS

			if test $ol_cv_linux_threads = error; then
				AC_MSG_ERROR([LinuxThreads header/library mismatch]);
			fi

			AC_CACHE_CHECK([if pthread_create() works],
				ol_cv_pthread_create_works,[
			AC_RUN_IFELSE([OL_PTHREAD_TEST_PROGRAM],
				[ol_cv_pthread_create_works=yes],
				[ol_cv_pthread_create_works=no],
				[dnl assume yes
				ol_cv_pthread_create_works=yes])])

			if test $ol_cv_pthread_create_works = no ; then
				AC_MSG_ERROR([pthread_create is not usable, check environment settings])
			fi

			ol_replace_broken_yield=no
dnl			case "$target" in
dnl			*-*-linux*)
dnl				AC_CHECK_FUNCS(nanosleep)
dnl				ol_replace_broken_yield=yes
dnl			;;
dnl			esac

			if test $ol_replace_broken_yield = yes ; then
				AC_DEFINE([REPLACE_BROKEN_YIELD],1,
					[define if sched_yield yields the entire process])
			fi

			dnl Check if select causes an yield
			if test $ol_with_yielding_select = auto ; then
				AC_CACHE_CHECK([if select yields when using pthreads],
					ol_cv_pthread_select_yields,[
				AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#ifndef NULL
#define NULL (void*) 0
#endif

static int fildes[2];

static void *task(p)
	void *p;
{
	int i;
	struct timeval tv;

	fd_set rfds;

	tv.tv_sec=10;
	tv.tv_usec=0;

	FD_ZERO(&rfds);
	FD_SET(fildes[0], &rfds);

	/* we're not interested in any fds */
	i = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);

	if(i < 0) {
		perror("select");
		exit(10);
	}

	exit(0); /* if we exit here, the select blocked the whole process */
}

int main(argc, argv)
	int argc;
	char **argv;
{
	pthread_t t;

	/* create a pipe to select */
	if(pipe(&fildes[0])) {
		perror("select");
		exit(1);
	}

#ifdef HAVE_PTHREAD_SETCONCURRENCY
	(void) pthread_setconcurrency(2);
#else
#ifdef HAVE_THR_SETCONCURRENCY
	/* Set Solaris LWP concurrency to 2 */
	thr_setconcurrency(2);
#endif
#endif

#if HAVE_PTHREADS < 6
	pthread_create(&t, pthread_attr_default, task, NULL);
#else
	pthread_create(&t, NULL, task, NULL);
#endif

	/* make sure task runs first */
#ifdef HAVE_THR_YIELD
	thr_yield();
#elif defined( HAVE_SCHED_YIELD )
	sched_yield();
#elif defined( HAVE_PTHREAD_YIELD )
	pthread_yield();
#endif

	exit(2);
}]])],[ol_cv_pthread_select_yields=no],[ol_cv_pthread_select_yields=yes],[ol_cv_pthread_select_yields=cross])])

				if test $ol_cv_pthread_select_yields = cross ; then
					AC_MSG_ERROR([crossing compiling: use --with-yielding_select=yes|no|manual])
				fi

				if test $ol_cv_pthread_select_yields = yes ; then
					ol_with_yielding_select=yes
				fi
			fi

			dnl restore flags
			CPPFLAGS="$save_CPPFLAGS"
			LIBS="$save_LIBS"
		else
			AC_MSG_ERROR([could not locate usable POSIX Threads])
		fi
	fi

	if test $ol_with_threads = posix ; then
		AC_MSG_ERROR([could not locate POSIX Threads])
	fi
	;;
esac

if test $ol_with_yielding_select = yes ; then
	AC_DEFINE(HAVE_YIELDING_SELECT,1,
		[define if select implicitly yields])
fi

AC_CHECK_FUNCS([clock_gettime])
if test $ac_cv_func_clock_gettime = no; then
	AC_CHECK_LIB(rt, clock_gettime,[ac_cv_func_clock_gettime="yes"])
	if test $ac_cv_func_clock_gettime = yes; then
		LTHREAD_LIBS="$LTHREAD_LIBS -lrt"
		AC_DEFINE(HAVE_CLOCK_GETTIME, 1,
			[Define to 1 if you have the `clock_gettime' function.])
	fi
fi

if test $ol_link_threads != no ; then
	dnl needed to get reentrant/threadsafe versions
	dnl
	AC_DEFINE(REENTRANT,1)
	AC_DEFINE(_REENTRANT,1)
	AC_DEFINE(THREAD_SAFE,1)
	AC_DEFINE(_THREAD_SAFE,1)
	AC_DEFINE(THREADSAFE,1)
	AC_DEFINE(_THREADSAFE,1)
	AC_DEFINE(_SGI_MP_SOURCE,1)

	dnl The errno declaration may dependent upon _REENTRANT.
	dnl If it does, we must link with thread support.
	AC_CACHE_CHECK([for thread specific errno],
		[ol_cv_errno_thread_specific], [
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <errno.h>]], [[errno = 0;]])],[ol_cv_errno_thread_specific=yes],[ol_cv_errno_thread_specific=no])
	])

	dnl The h_errno declaration may dependent upon _REENTRANT.
	dnl If it does, we must link with thread support.
	AC_CACHE_CHECK([for thread specific h_errno],
		[ol_cv_h_errno_thread_specific], [
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]], [[h_errno = 0;]])],[ol_cv_h_errno_thread_specific=yes],[ol_cv_h_errno_thread_specific=no])
	])

	if test $ol_cv_errno_thread_specific != yes ||
	   test $ol_cv_h_errno_thread_specific != yes ; then
		LIBS="$LTHREAD_LIBS $LIBS"
		LTHREAD_LIBS=""
	fi
fi


dnl ----------------------------------------------------------------
dnl Tests for reentrant functions necessary to build -lreldap
AC_CHECK_FUNCS(		\
	ctime_r			\
	gmtime_r localtime_r \
	gethostbyname_r	gethostbyaddr_r \
)

if test "$ac_cv_func_ctime_r" = no ; then
	ol_cv_func_ctime_r_nargs=0
else
	OL_FUNC_CTIME_R_NARGS
dnl	OL_FUNC_CTIME_R_TYPE
fi

if test "$ac_cv_func_gethostbyname_r" = yes ; then
	OL_FUNC_GETHOSTBYNAME_R_NARGS
else
	ol_cv_func_gethostbyname_r_nargs=0
fi

if test "$ac_cv_func_gethostbyaddr_r" = yes ; then
	OL_FUNC_GETHOSTBYADDR_R_NARGS
else
	ol_cv_func_gethostbyaddr_r_nargs=0
fi

dnl ----------------------------------------------------------------
ol_link_bdb=no

if test $ol_enable_bdb/$ol_enable_hdb != no/no; then
	OL_BERKELEY_DB

	if test $ol_cv_berkeley_db = no ; then
		AC_MSG_ERROR(BDB/HDB: BerkeleyDB not available)
	fi

	AC_DEFINE(HAVE_BERKELEY_DB,1,
		[define this if Berkeley DB is available])

	dnl $ol_cv_lib_db should be yes or -ldb
	dnl (it could be no, but that would be an error
	if test $ol_cv_lib_db != yes ; then
		BDB_LIBS="$BDB_LIBS $ol_cv_lib_db"
	fi

	SLAPD_LIBS="$SLAPD_LIBS \$(BDB_LIBS)"

	ol_link_bdb=yes
fi

dnl ----------------------------------------------------------------
if test $ol_enable_wrappers != no ; then
	AC_CHECK_HEADERS(tcpd.h,[
		AC_MSG_CHECKING([for TCP wrappers library])
		save_LIBS="$LIBS"
		LIBS="$LIBS -lwrap"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <tcpd.h>
int allow_severity = 0;
int deny_severity  = 0;

struct request_info *req;
		]], [[
hosts_access(req)
		]])],[AC_MSG_RESULT([-lwrap])
		have_wrappers=yes
		LIBS="$save_LIBS"],[
		dnl try with -lnsl
		LIBS="$LIBS -lnsl"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <tcpd.h>
int allow_severity = 0;
int deny_severity  = 0;

struct request_info *req;
		]], [[
hosts_access(req)
		]])],[AC_MSG_RESULT([-lwrap -lnsl])
		have_wrappers=yes
		LIBS="$save_LIBS -lnsl"],[
		AC_MSG_RESULT(no)
		have_wrappers=no
		LIBS=$save_LIBS])])],[have_wrappers=no])

	if test $have_wrappers = yes ; then
		AC_DEFINE(HAVE_TCPD,1, [define if you have -lwrap])
		SLAPD_TCPWRAP_LIBS="$SLAPD_TCPWRAP_LIBS -lwrap"
	elif test $ol_enable_wrappers = yes ; then
		AC_MSG_ERROR([could not find TCP wrappers, select apppropriate options or disable])
	else
		AC_MSG_WARN([could not find TCP wrappers, support disabled])
	fi
fi

dnl ----------------------------------------------------------------
if test $ol_enable_syslog != no ; then
	AC_CHECK_FUNC(openlog)
	if test $ac_cv_func_openlog = no && test $ol_enable_syslog = yes; then
		AC_MSG_ERROR(could not find syslog, select appropriate options or disable)
	fi
	ol_enable_syslog=$ac_cv_func_openlog
fi

dnl ----------------------------------------------------------------
dnl SQL
ol_link_sql=no
if test $ol_enable_sql != no ; then
	save_LIBS="$LIBS"
	LIBS="$LTHREAD_LIBS $LIBS"

	AC_CHECK_HEADERS(sql.h sqlext.h)

	if test $ol_with_odbc = auto ; then
		ol_with_odbc="unixodbc iodbc"
	fi

	for odbc in $ol_with_odbc ; do
		if test $ol_link_sql = no ; then
			case $odbc in
			iodbc)
				save_CPPFLAGS="$CPPFLAGS"
				iodbc_includes=""
				if test $ac_cv_header_sql_h != yes \
				|| test $ac_cv_header_sqlext_h != yes; then
					iodbc_includes="-I/usr/include/iodbc"
					CPPFLAGS=$iodbc_includes
					AC_CHECK_HEADERS(iodbc/sql.h iodbc/sqlext.h)
					ac_cv_header_sql_h=
				fi
				if test "$ac_cv_header_sql_h" = yes -a "$ac_cv_header_sqlext_h" = yes \
				|| test "$ac_cv_header_iodbc_sql_h" = yes -a "$ac_cv_header_iodbc_sqlext_h" = yes; then
					AC_CHECK_LIB(iodbc, SQLDriverConnect, [have_iodbc=yes], [have_iodbc=no])
					if test $have_iodbc = yes ; then
						ol_link_sql="-liodbc"
						SQL_INCLUDES=$iodbc_includes
					fi
				fi
				CPPFLAGS="$save_CPPFLAGS"
				;;

			unixodbc)
				if test $ac_cv_header_sql_h = yes && test $ac_cv_header_sqlext_h = yes; then
					AC_CHECK_LIB(odbc, SQLDriverConnect, [have_odbc=yes], [have_odbc=no])
					if test $have_odbc = yes ; then
						ol_link_sql="-lodbc"
					fi
				fi
				;;

			*)
				AC_MSG_ERROR([unknown ODBC library])
				;;
			esac
		fi
	done

	LIBS="$save_LIBS"

	if test $ol_link_sql != no ; then
		SQL_LIBS="$ol_link_sql"
	elif test $ol_enable_sql != auto ; then
		AC_MSG_ERROR([could not locate suitable ODBC library])
	fi
fi

dnl ----------------------------------------------------------------
dnl MySQL NDBapi
dnl Note: uses C++, but we don't want to add C++ test overhead to
dnl the rest of the libtool machinery.
ol_link_ndb=no
if test $ol_enable_ndb != no ; then
	mysql_cluster_path=$(find -L /opt /usr/local $(echo $PATH | tr ':' ' ') -maxdepth 2 -name 'mysql-cluster*' -type d 2>/dev/null -printf "%p/bin$PATH_SEPARATOR")
	dnl echo "mysql_cluster_path=$mysql_cluster_path"
	AC_PATH_PROG(MYSQL,mysql_config,no,$mysql_cluster_path)
	if test "$MYSQL" = no ; then
		AC_MSG_ERROR([could not locate mysql_config])
	fi

	SQL_INC=`$MYSQL --include`
	NDB_INCLUDES="$SQL_INC $SQL_INC/storage/ndb $SQL_INC/storage/ndb/ndbapi"

	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$NDB_INCLUDES"
	AC_MSG_CHECKING(for NdbApi.hpp)
	AC_PREPROC_IFELSE(
		[AC_LANG_SOURCE([[#include <NdbApi.hpp>]])],
			AC_MSG_RESULT(yes),
			AC_MSG_ERROR([could not locate NdbApi headers])
	)
	CPPFLAGS="$save_CPPFLAGS"

	SQL_LIB="`$MYSQL --libs_r`"
	NDB_LIBS="$SQL_LIB -lndbclient -lstdc++"

	save_LDFLAGS="$LDFLAGS"
	save_LIBS="$LIBS"
	LDFLAGS="$SQL_LIB"
	AC_CHECK_LIB(ndbclient,ndb_init,[: $SQL_LIB],[
		AC_MSG_ERROR([could not locate ndbclient library])
	],[-lstdc++])
	LIBS="$save_LIBS"
	LDFLAGS="$save_LDFLAGS"

	AC_MSG_CHECKING([ndb library present in ld.so search path])
	NDB_LIBDIR="`$MYSQL --libs_r | sed 's/^-L /-L/g' | tr '[[:space:]]' '\n' | grep ^-L | sed 's/^-L//'`"
	if ldconfig -v 2>/dev/null | grep -q "^$NDB_LIBDIR:"; then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no, -rpath option will be passed to linker)
		NDB_LDFLAGS="$NDB_LDFLAGS -Wl,-rpath,$NDB_LIBDIR"
		NDB_RDIR="$NDB_RDIR -R$NDB_LIBDIR"
	fi
fi

#dnl ----------------------------------------------------------------
#
# LY: libuci was never used ever, including in the original OpenLDAP. (2.4.x, 2.5.x).
#
#dnl International Components for Unicode
#OL_ICU
#if test "$ol_icu" = no ; then
#	AC_MSG_WARN([ICU not available])
#else
#	ICU_LIBS="$ol_icu"
#fi

dnl ----------------------------------------------------------------
dnl
dnl Check for Cyrus SASL
dnl
WITH_SASL=no
ol_link_sasl=no
ol_link_spasswd=no
if test $ol_with_cyrus_sasl != no ; then
#	PKG_CHECK_MODULES(SASL, libsasl2, sasl_found=yes, sasl_found=no)
#	AC_SUBST(SASL_LIBS)
#	AC_SUBST(SASL_CFLAGS)

	AC_CHECK_HEADERS(sasl/sasl.h sasl.h)

	if test $ac_cv_header_sasl_sasl_h = yes ||
	   test $ac_cv_header_sasl_h = yes; then
		AC_CHECK_LIB(sasl2, sasl_client_init,
			[ol_link_sasl="-lsasl2"],
			[AC_CHECK_LIB(sasl, sasl_client_init,
				[ol_link_sasl="-lsasl"])])
	fi

	if test $ol_link_sasl = no ; then
		if test $ol_with_cyrus_sasl != auto ; then
			AC_MSG_ERROR([Could not locate Cyrus SASL])
		else
			AC_MSG_WARN([Could not locate Cyrus SASL])
			AC_MSG_WARN([SASL authentication not supported!])
			if test $ol_link_tls = no ; then
				AC_MSG_WARN([Strong authentication not supported!])
			fi
		fi
	else
		OL_SASL_COMPAT
		if test $ol_cv_sasl_compat = no ; then
			ol_link_sasl=no
			AC_MSG_ERROR([Cyrus SASL library located but is incompatible])
		fi

		AC_DEFINE(HAVE_CYRUS_SASL,1,[define if you have Cyrus SASL])
		SASL_LIBS="$ol_link_sasl"
		if test $ol_enable_spasswd != no ; then
			ol_link_spasswd=yes
		fi

		ac_save_LIBS="$LIBS"
		LIBS="$LIBS $ol_link_sasl"
		AC_CHECK_FUNC(sasl_version, [AC_DEFINE(HAVE_SASL_VERSION,1,
			[define if your SASL library has sasl_version()])])
		LIBS="$ac_save_LIBS"

		WITH_SASL=yes
	fi

else
	AC_MSG_WARN([SASL authentication not supported!])
	if test $ol_link_tls = no ; then
		AC_MSG_WARN([Strong authentication not supported!])
	fi
fi

dnl ----------------------------------------------------------------
dnl Check for entropy sources
if test $cross_compiling != yes && test "$ac_cv_mingw32" != yes ; then
	dev=no
	if test -r /dev/urandom ; then
		dev="/dev/urandom";
	elif test -r /idev/urandom ; then
		dev="/idev/urandom";
	elif test -r /dev/srandom ; then
		dev="/dev/srandom";
	elif test -r /dev/random ; then
		dev="/dev/random";
	elif test -r /idev/random ; then
		dev="/idev/random";
	fi

	if test $dev != no ; then
		AC_DEFINE_UNQUOTED(URANDOM_DEVICE,"$dev",[set to urandom device])
	fi
fi

dnl ----------------------------------------------------------------
dnl
dnl Check for fetch URL support
dnl		should be extended to support other fetch URL APIs
dnl
ol_link_fetch=no
if test $ol_with_fetch != no ; then
	OL_LIB_FETCH

	if test $ol_cv_lib_fetch != no ; then
		LIBS="$LIBS $ol_link_fetch"
		ol_link_fetch=freebsd

	elif test $ol_with_fetch != auto ; then
		AC_MSG_ERROR(no suitable API for --with-fetch=$ol_with_fetch)
	fi
fi

dnl ----------------------------------------------------------------
dnl FreeBSD (and others) have crypt(3) in -lcrypt
if test $ol_enable_crypt != no ; then
	save_LIBS="$LIBS"
	LIBS="$TLS_LIBS $LIBS"

	AC_CHECK_FUNC(crypt, [have_crypt=yes], [
		LIBS="$save_LIBS"
		AC_CHECK_LIB(crypt, crypt, [LUTIL_LIBS="$LUTIL_LIBS -lcrypt"
			have_crypt=yes], [have_crypt=no])])

	LIBS="$save_LIBS"

	if test $have_crypt = yes ; then
		AC_DEFINE(HAVE_CRYPT,1, [define if crypt(3) is available])
	else
		AC_MSG_WARN([could not find crypt])
		if test $ol_enable_crypt = yes ; then
			AC_MSG_ERROR([could not find crypt, select appropriate options or disable])
		fi

		AC_MSG_WARN([disabling crypt support])
		ol_enable_crypt=no
	fi
fi

dnl ----------------------------------------------------------------
dnl FreeBSD (and others) have setproctitle(3) in -lutil
if test $ol_enable_proctitle != no ; then
	AC_CHECK_FUNC(setproctitle,	[have_setproctitle=yes], [
		AC_CHECK_LIB(util, setproctitle,
			[have_setproctitle=yes
			LUTIL_LIBS="$LUTIL_LIBS -lutil"],
			[have_setproctitle=no
			AC_LIBOBJ(setproctitle)
			LIBSRCS="$LIBSRCS setproctitle.c"])])

	if test $have_setproctitle = yes ; then
		AC_DEFINE(HAVE_SETPROCTITLE,1,
			[define if setproctitle(3) is available])
	fi
fi

dnl ----------------------------------------------------------------
if test $ol_enable_slp != no ; then
	AC_CHECK_HEADERS( slp.h )

	if test $ac_cv_header_slp_h = yes ; then
		AC_CHECK_LIB(slp, SLPOpen, [have_slp=yes], [have_slp=no])
		if test $have_slp = yes ; then
			AC_DEFINE(HAVE_SLP, 1, [define if you have -lslp])
			SLAPD_SLP_LIBS=-lslp
		fi

	elif test $ol_enable_slp = yes ; then
		AC_MSG_ERROR([SLP not found])
	fi
fi

dnl ----------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.

AC_CHECK_TYPE(mode_t, int)
AC_CHECK_TYPE(off_t, long)
AC_CHECK_TYPE(pid_t, int)
AC_CHECK_TYPE(ssize_t, [signed int])
AC_CHECK_TYPE(caddr_t,	[char *])
AC_CHECK_TYPE(size_t, unsigned)

AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([ptrdiff_t])


AC_CHECK_TYPE([socklen_t],,, [$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])

dnl socklen_t-like type in accept(), default socklen_t or int:
dnl - The OS might define socklen_t without using it.  POSIX moved from
dnl   int to size_t to socklen_t, hoping to stay at a 32-bit type, and
dnl   HP-UX now has selectors for what to use.
dnl - On Solaris 2.8 the prototype has void *len, but the default is OK.
AC_MSG_CHECKING([the type of arg 3 to accept()])
AC_CACHE_VAL(ol_cv_type_ber_socklen_t, [
	set socklen_t int unsigned "unsigned long" long size_t
	test "$ac_cv_type_socklen_t" = yes || shift
	ol_cv_type_ber_socklen_t=$1 guessing="guessing "
	for lentype in "$@" ; do for addrtype in "struct sockaddr" void ; do
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
extern int accept(int s, $addrtype *ap, $lentype *lp);
], [
accept(0, (struct sockaddr *) 0, ($lentype *) 0);
])], [ol_cv_type_ber_socklen_t=$lentype guessing= ; break 2])
	done ; done])
AC_MSG_RESULT([$guessing$ol_cv_type_ber_socklen_t *])
AC_DEFINE_UNQUOTED(ber_socklen_t, $ol_cv_type_ber_socklen_t,
	[Define to the type of arg 3 for `accept'.])

dnl Modules should use ber_socklen_t, not socklen_t.  Define socklen_t
dnl for the time being anyway, for backwards compatibility.
if test "$ac_cv_type_socklen_t" != yes; then
	AC_DEFINE_UNQUOTED([socklen_t], [$ol_cv_type_ber_socklen_t],
		[Define like ber_socklen_t if <sys/socket.h> does not define.])
fi

AC_CHECK_TYPE([sig_atomic_t],,
	[AC_DEFINE_UNQUOTED([sig_atomic_t], [int],
		[Define to `int' if <signal.h> does not define.])],
	[$ac_includes_default
#include <signal.h>
	])

AC_TYPE_UID_T

AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct passwd.pw_gecos],,,[$ac_includes_default
#include <pwd.h>])
AC_CHECK_MEMBERS([struct passwd.pw_passwd],,,[$ac_includes_default
#include <pwd.h>])

OL_C_UPPER_LOWER
AC_C_CONST
OL_C_VOLATILE

if test $cross_compiling = yes ; then
	AC_MSG_WARN([Crossing compiling... all bets are off!])
	AC_DEFINE(CROSS_COMPILING, 1, [define if cross compiling])
else
	AC_C_BIGENDIAN
fi

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(wchar_t)

if test "$ac_cv_sizeof_int" -lt 4 ; then
	AC_MSG_WARN([ReOpenLDAP requires 'int' to be 32 bits or greater.])

	AC_DEFINE(LBER_INT_T,long,[define to 32-bit or greater integer type])
else
	AC_DEFINE(LBER_INT_T,int,[define to 32-bit or greater integer type])
fi

AC_DEFINE(LBER_LEN_T,long,[define to large integer type])
AC_DEFINE(LBER_SOCKET_T,int,[define to socket descriptor type])
AC_DEFINE(LBER_TAG_T,long,[define to large integer type])

dnl ----------------------------------------------------------------
dnl Check for multiple precision support
if test $ol_with_mp = longlong || test $ol_with_mp = auto ; then
	if test $ac_cv_sizeof_long_long -gt 4 ; then
		ol_with_mp=longlong
		AC_DEFINE(USE_MP_LONG_LONG,1,[define to use 'long long' for MP])
	elif test $ol_with_mp = longlong ; then
		AC_MSG_ERROR([long long unusable for multiple precision])
	fi
fi
if test $ol_with_mp = long || test $ol_with_mp = auto ; then
	if test $ac_cv_sizeof_long -gt 4 ; then
		ol_with_mp=long
		AC_DEFINE(USE_MP_LONG,1,[define to use 'long' for MP])
	elif test $ol_with_mp = long ; then
		AC_MSG_ERROR([long unusable for multiple precision])
	fi
fi
if test $ol_with_mp = bignum || test $ol_with_mp = auto ; then
	AC_CHECK_HEADERS(openssl/bn.h)
	AC_CHECK_HEADERS(openssl/crypto.h)
	if test "$ac_cv_header_openssl_bn_h" = "yes" &&
		test "$ac_cv_header_openssl_crypto_h" = "yes" &&
		test "$ol_with_tls" = "found" ; then
		ol_with_mp=bignum
		AC_DEFINE(USE_MP_BIGNUM,1,[define to use OpenSSL BIGNUM for MP])
	elif test $ol_with_mp = bignum ; then
		AC_MSG_ERROR([bignum not available])
	fi
fi
if test $ol_with_mp = gmp || test $ol_with_mp = auto ; then
	AC_CHECK_HEADERS(gmp.h)
	AC_CHECK_LIB(gmp, __gmpz_add_ui)
	if test $ac_cv_header_gmp_h = yes && test $ac_cv_lib_gmp___gmpz_add_ui = yes ; then
		AC_DEFINE(USE_MP_GMP,1,[define to use GMP for MP])
		ol_with_mp=gmp
	elif test $ol_with_mp = gmp ; then
		AC_MSG_ERROR([gmp not available])
	fi
fi
if test $ol_with_mp = auto ; then
	ol_with_mp=no
fi

dnl ----------------------------------------------------------------
dnl Checks for library functions.

AC_FUNC_STRFTIME

OL_FUNC_INET_ATON

dnl Check for NT specific routines
AC_CHECK_FUNC(_spawnlp, AC_DEFINE(HAVE_SPAWNLP,1,[if you have spawnlp()]))

AC_CHECK_FUNC(_snprintf, [ac_cv_func_snprintf=yes
	AC_DEFINE(snprintf, _snprintf, [define to snprintf routine])
])

AC_CHECK_FUNCS(vsnprintf _vsnprintf)

if test $ac_cv_func_vsnprintf = no -a $ac_cv_func__vsnprintf = yes ; then
	ac_cv_func_vsnprintf=yes
	AC_DEFINE(vsnprintf, _vsnprintf, [define to vsnprintf routine])
fi

AC_FUNC_VPRINTF

if test $ac_cv_func_vprintf = yes ; then
	dnl check for vsnprintf
	AC_CHECK_FUNCS(snprintf vsnprintf)
fi

AC_CHECK_FUNCS(		\
	bcopy		\
	closesocket	\
	chroot		\
	endgrent	\
	endpwent	\
	fcntl		\
	flock		\
	fstat		\
	getdtablesize	\
	geteuid		\
	getgrgid	\
	gethostname	\
	getpassphrase	\
	getpwuid	\
	getpwnam	\
	getspnam	\
	gettimeofday	\
	initgroups	\
	inet_ntoa_b	\
	ioctl		\
	lockf		\
	memcpy		\
	memmove		\
	mkstemp		\
	mktemp		\
	pipe		\
	read		\
	recv		\
	recvfrom	\
	setpwfile	\
	setgid		\
	setegid		\
	setsid		\
	setuid		\
	seteuid		\
	signal		\
	strpbrk		\
	strrchr		\
	strsep		\
	strstr		\
	strtol		\
	strtoul		\
	strtoq		\
	strtouq		\
	strtoll		\
	strtoull	\
	strspn		\
	sysconf		\
	waitpid		\
	wait4		\
	write		\
	send		\
	sendmsg		\
	sendto		\
)

dnl We actually may need to replace more than this.
AC_REPLACE_FUNCS(getopt getpeereid)

if test "$ac_cv_func_getopt" != yes; then
	LIBSRCS="$LIBSRCS getopt.c"
fi

if test "$ac_cv_func_getpeereid" != yes; then
	AC_CHECK_FUNCS( getpeerucred )
	if test "$ac_cv_func_getpeerucred" != yes ; then
		AC_CHECK_MEMBERS([struct msghdr.msg_accrightslen],,,
			[$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])
		if test "$ac_cv_member_struct_msghdr_msg_accrightslen" != yes; then
			AC_CHECK_MEMBERS([struct msghdr.msg_control],,,
				[$ac_includes_default
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])
		fi
		AC_CHECK_MEMBERS([struct stat.st_fstype, struct stat.st_vfstype])
		if test "$ac_cv_member_struct_stat_st_fstype" = yes; then
			AC_COMPILE_IFELSE([AC_LANG_SOURCE([struct stat st; char *ptr=st.st_fstype;])],
				AC_DEFINE([HAVE_STRUCT_STAT_ST_FSTYPE_CHAR],1,[define to 1 if st_fstype is char *]),
				AC_DEFINE([HAVE_STRUCT_STAT_ST_FSTYPE_INT],1,[define to 1 if st_fstype is int]))
		fi
	fi
	LIBSRCS="$LIBSRCS getpeereid.c"
fi

if test "$ac_cv_func_snprintf" != yes ||
   test "$ac_cv_func_vsnprintf" != yes; then
	if test "$ac_cv_func_snprintf" != yes; then
		AC_DEFINE(snprintf, ber_pvt_snprintf, [define to snprintf routine])
	fi
	if test "$ac_cv_func_vsnprintf" != yes; then
		AC_DEFINE(vsnprintf, ber_pvt_vsnprintf, [define to snprintf routine])
	fi
fi

dnl ----------------------------------------------------------------
dnl Sort out defines

if test "$ol_enable_debug" != no ; then
	if test "$ol_enable_debug" = traditional; then
		AC_DEFINE(OLD_DEBUG,1,
			[define to use the original debug style])
	fi
	AC_DEFINE(LDAP_DEBUG,1,
		[define this to add debugging code])
fi
if test "$ol_enable_syslog" = yes ; then
	AC_DEFINE(LDAP_SYSLOG,1,
		[define this to add syslog code])
fi
if test "$ol_enable_proctitle" != no ; then
	AC_DEFINE(LDAP_PROCTITLE,1,
		[define this for LDAP process title support])
fi
if test "$ol_enable_referrals" != no ; then
	AC_DEFINE(LDAP_API_FEATURE_X_OPENLDAP_V2_REFERRALS,LDAP_VENDOR_VERSION,
		[define to LDAP VENDOR VERSION])
fi
if test "$ol_enable_local" != no; then
	AC_DEFINE(LDAP_PF_LOCAL,1,[define to support PF_LOCAL])
fi
if test "$ol_link_ipv6" != no; then
	AC_DEFINE(LDAP_PF_INET6,1,[define to support PF_INET6])
fi
if test "$ol_enable_cleartext" != no ; then
	AC_DEFINE(SLAPD_CLEARTEXT,1,[define to support cleartext passwords])
fi
if test "$ol_enable_crypt" != no ; then
	AC_DEFINE(SLAPD_CRYPT,1,[define to support crypt(3) passwords])
fi
if test "$ol_link_spasswd" != no ; then
	AC_DEFINE(SLAPD_SPASSWD,1,[define to support SASL passwords])
fi
if test "$ol_enable_rlookups" != no ; then
	AC_DEFINE(SLAPD_RLOOKUPS,1,[define to support reverse lookups])
fi
if test "$ol_enable_aci" != no ; then
	if test $ol_enable_dynacl = no ; then
		ol_enable_dynacl=yes
		AC_MSG_WARN([ACIs need dynacl])
	fi
	if test "$ol_enable_aci" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		dnl remove this after moving servers/slapd/aci.c in contrib/slapd-modules/acl
		AC_MSG_ERROR([ACI build as dynamic module not supported (yet)])
	else
		MFLAG=SLAPD_MOD_STATIC
	fi
	WITH_ACI_ENABLED=$ol_enable_aci
	AC_DEFINE_UNQUOTED(SLAPD_ACI_ENABLED,$MFLAG,[define to support per-object ACIs])
else
	WITH_ACI_ENABLED=no
fi
if test "$ol_enable_dynacl" != no ; then
	AC_DEFINE(SLAP_DYNACL,1,[define to support run-time loadable ACL])
fi

AC_DEFINE(SLAPD_MOD_STATIC,1,[statically linked module])
AC_DEFINE(SLAPD_MOD_DYNAMIC,2,[dynamically linked module])

dnl back-monitor goes first (well, after back-config)
if test "$ol_enable_monitor" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_monitor" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS monitor"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS monitor"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_MONITOR,$MFLAG,[Monitor backend])
fi

if test "$ol_enable_mdb" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_mdb" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS mdb"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS mdb"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_MDBX,$MFLAG,[MDBX backend])
fi

if test "$ol_enable_bdb" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_bdb" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS bdb"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS bdb"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_BDB,$MFLAG,[define to support BDB backend])
fi

if test "$ol_enable_dnssrv" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_dnssrv" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS dnssrv"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS dnssrv"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_DNSSRV,$MFLAG,[define to support DNS SRV backend])
fi

if test "$ol_enable_hdb" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_hdb" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS hdb"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS hdb"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_HDB,$MFLAG,[define to support HDB backend])
fi

if test "$ol_enable_ldap" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_ldap" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS ldap"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS ldap"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_LDAP,$MFLAG,[define to support LDAP backend])
fi

if test "$ol_enable_meta" != no ; then
	ol_enable_slapd=yes
	ol_enable_rewrite=yes
	if test "$ol_enable_meta" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS meta"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS meta"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_META,$MFLAG,[define to support LDAP Metadirectory backend])
fi

if test "$ol_enable_ndb" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_ndb" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS ndb"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS ndb"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_NDB,$MFLAG,[define to support NDB backend])
fi

if test "$ol_enable_null" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_null" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS null"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS null"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_NULL,$MFLAG,[define to support NULL backend])
fi

if test "$ol_enable_passwd" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_passwd" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS passwd"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS passwd"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_PASSWD,$MFLAG,[define to support PASSWD backend])
fi

if test "$ol_link_perl" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_perl" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS perl"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS perl"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_PERL,$MFLAG,[define to support PERL backend])
fi

if test "$ol_enable_relay" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_relay" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS relay"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS relay"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_RELAY,$MFLAG,[define to support relay backend])
fi

if test "$ol_enable_shell" != no ; then
	if test "$ol_link_threads" != no ; then
		AC_MSG_WARN([Use of back-shell is NOT recommended])
	fi
	ol_enable_slapd=yes
	if test "$ol_enable_shell" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS shell"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS shell"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_SHELL,$MFLAG,[define to support SHELL backend])
fi

if test "$ol_enable_sock" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_sock" = mod ; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS sock"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS sock"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_SOCK,$MFLAG,[define to support SOCK backend])
fi

if test "$ol_link_sql" != no ; then
	ol_enable_slapd=yes
	if test "$ol_enable_sql" = mod; then
		SLAPD_DYNAMIC_BACKENDS="$SLAPD_DYNAMIC_BACKENDS sql"
		MFLAG=SLAPD_MOD_DYNAMIC
	else
		SLAPD_STATIC_BACKENDS="$SLAPD_STATIC_BACKENDS sql"
		MFLAG=SLAPD_MOD_STATIC
	fi
	AC_DEFINE_UNQUOTED(SLAPD_SQL,$MFLAG,[define to support SQL backend])
fi

if test "$ol_enable_accesslog" != no ; then
	if test "$ol_enable_accesslog" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS accesslog"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS accesslog"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_ACCESSLOG,$MFLAG,[define for In-Directory Access Logging overlay])
fi

if test "$ol_enable_auditlog" != no ; then
	if test "$ol_enable_auditlog" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS auditlog"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS auditlog"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_AUDITLOG,$MFLAG,[define for Audit Logging overlay])
fi

if test "$ol_enable_collect" != no ; then
	if test "$ol_enable_collect" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS collect"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS collect"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_COLLECT,$MFLAG,[define for Collect overlay])
fi

if test "$ol_enable_constraint" != no ; then
	if test "$ol_enable_constraint" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS constraint"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS constraint"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_CONSTRAINT,$MFLAG,[define for Attribute Constraint overlay])
fi

if test "$ol_enable_dds" != no ; then
	if test "$ol_enable_dds" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS dds"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS dds"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DDS,$MFLAG,[define for Dynamic Directory Services overlay])
fi

if test "$ol_enable_deref" != no ; then
	if test "$ol_enable_deref" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS deref"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS deref"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DEREF,$MFLAG,[define for Dynamic Directory Services overlay])
fi

if test "$ol_enable_dyngroup" != no ; then
	if test "$ol_enable_dyngroup" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS dyngroup"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS dyngroup"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DYNGROUP,$MFLAG,[define for Dynamic Group overlay])
fi

if test "$ol_enable_dynlist" != no ; then
	if test "$ol_enable_dynlist" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS dynlist"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS dynlist"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_DYNLIST,$MFLAG,[define for Dynamic List overlay])
fi

if test "$ol_enable_memberof" != no ; then
	if test "$ol_enable_memberof" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS memberof"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS memberof"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_MEMBEROF,$MFLAG,[define for Reverse Group Membership overlay])
fi

if test "$ol_enable_ppolicy" != no ; then
	if test "$ol_enable_ppolicy" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS ppolicy"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS ppolicy"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_PPOLICY,$MFLAG,[define for Password Policy overlay])
fi

if test "$ol_enable_pcache" != no ; then
	if test "$ol_enable_pcache" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS pcache"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS pcache"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_PCACHE,$MFLAG,[define for Proxy Cache overlay])
fi

if test "$ol_enable_refint" != no ; then
	if test "$ol_enable_refint" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS refint"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS refint"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_REFINT,$MFLAG,[define for Referential Integrity overlay])
fi

if test "$ol_enable_retcode" != no ; then
	if test "$ol_enable_retcode" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS retcode"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS retcode"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_RETCODE,$MFLAG,[define for Referential Integrity overlay])
fi

if test "$ol_enable_rwm" != no ; then
	ol_enable_rewrite=yes
	if test "$ol_enable_rwm" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS rwm"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS rwm"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_RWM,$MFLAG,[define for Rewrite/Remap overlay])
fi

if test "$ol_enable_seqmod" != no ; then
	if test "$ol_enable_seqmod" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS seqmod"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS seqmod"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_SEQMOD,$MFLAG,[define for Sequential Modify overlay])
fi

if test "$ol_enable_sssvlv" != no ; then
	if test "$ol_enable_sssvlv" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS sssvlv"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS sssvlv"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_SSSVLV,$MFLAG,[define for ServerSideSort/VLV overlay])
fi

if test "$ol_enable_syncprov" != no ; then
	if test "$ol_enable_syncprov" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS syncprov"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS syncprov"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_SYNCPROV,$MFLAG,[define for Syncrepl Provider overlay])
fi

if test "$ol_enable_translucent" != no ; then
	if test "$ol_enable_translucent" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS translucent"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS translucent"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_TRANSLUCENT,$MFLAG,[define for Translucent Proxy overlay])
fi

if test "$ol_enable_unique" != no ; then
	if test "$ol_enable_unique" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS unique"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS unique"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_UNIQUE,$MFLAG,[define for Attribute Uniqueness overlay])
fi

if test "$ol_enable_valsort" != no ; then
	if test "$ol_enable_valsort" = mod ; then
		MFLAG=SLAPD_MOD_DYNAMIC
		SLAPD_DYNAMIC_OVERLAYS="$SLAPD_DYNAMIC_OVERLAYS valsort"
	else
		MFLAG=SLAPD_MOD_STATIC
		SLAPD_STATIC_OVERLAYS="$SLAPD_STATIC_OVERLAYS valsort"
	fi
	AC_DEFINE_UNQUOTED(SLAPD_OVER_VALSORT,$MFLAG,[define for Value Sorting overlay])
fi

if test "$ol_enable_rewrite" != no ; then
	AC_DEFINE(ENABLE_REWRITE,1,[define to enable rewriting in back-ldap and back-meta])
fi

if test "$ol_enable_slapi" != no ; then
	AC_DEFINE(LDAP_SLAPI,1, [define this to add SLAPI code])
fi

AC_SUBST(LDAP_LIBSLAPI_LA)

dnl ----------------------------------------------------------------

#AC_SUBST(LIBSRCS)
AC_SUBST(WITH_SASL)
AC_SUBST(WITH_TLS)
AC_SUBST(WITH_ACI_ENABLED)

if test $enable_shared = yes; then
	AC_DEFINE(RELDAP_LIBS_SHARED,1,[--enable-shared=yes])
fi
#RELDAP_LIBS_SHARED=$enable_shared
#AC_SUBST(RELDAP_LIBS_SHARED)

AM_CONDITIONAL([BEHIND_CURTIAN], [test -x $ac_aux_dir/mkbanner -a -x $ac_aux_dir/BRANDING])
if test -x $ac_aux_dir/mkbanner -a -x $ac_aux_dir/BRANDING; then
	AC_DEFINE_UNQUOTED(SLAPD_BEHIND_CURTIAN,1,[Maintaiter mode])
	SLAPD_MAINTAINER_DIR="$(pwd)"
	AC_DEFINE_UNQUOTED(SLAPD_MAINTAINER_DIR,"$SLAPD_MAINTAINER_DIR",[Maintaiter directory])

	AC_CHECK_PROGS(SOELIM, roffpp gsoelim soelim)
	if test -z "$SOELIM"; then
		AC_MSG_ERROR([could not locate roffpp/gsoelim/soelim])
	fi
	if ! $SOELIM -I. </dev/null >/dev/null 2>/dev/null; then
		AC_MSG_ERROR([$SOELIM does not support -I option])
	fi
	if $SOELIM -r </dev/null >/dev/null 2>/dev/null; then
		SOELIM_R="$SOELIM -r"
	else
		SOELIM_R=$SOELIM
	fi
	AC_SUBST(SOELIM_R)

else
	SLAPD_MAINTAINER_DIR=""
fi
AC_SUBST(SLAPD_MAINTAINER_DIR)

if test "$ol_enable_modules" != no ; then
	AC_DEFINE(SLAPD_DYNAMIC_MODULES,1,[define to support dynamic modules])
fi
AM_CONDITIONAL([BUILD_SLAPD], [test $ol_enable_slapd != no])
AM_CONDITIONAL([SLAPD_DYNAMIC], [test $ol_enable_modules != no -o $ol_enable_slapi != no])
if test $ol_enable_modules != no -o $ol_enable_slapi != no; then
	AC_DEFINE(SLAPD_DYNAMIC,1,[define to export symbols from slapd])
fi

AM_CONDITIONAL([BUILD_REWRITE], [test $ol_enable_rewrite != no])

AM_CONDITIONAL([BUILD_LDIF], [true])
AM_CONDITIONAL([LDIF_DYNAMIC], [false])

AM_CONDITIONAL([BUILD_SLAPI], [test $ol_enable_slapi != no])
AM_CONDITIONAL([SLAPD_DYNACL], [test $ol_enable_dynacl != no])

#dnl backends
	AM_CONDITIONAL([BUILD_MONITOR], [test $ol_enable_monitor != no])
	AM_CONDITIONAL([MONITOR_DYNAMIC], [test $ol_enable_monitor = mod])

	AM_CONDITIONAL([BUILD_MDBX], [test $ol_enable_mdb != no])
	AM_CONDITIONAL([MDBX_DYNAMIC], [test $ol_enable_mdb = mod])

	AM_CONDITIONAL([BUILD_BDB], [test $ol_enable_bdb != no])
	AM_CONDITIONAL([BDB_DYNAMIC], [test $ol_enable_bdb = mod])

	AM_CONDITIONAL([BUILD_DNSSRV], [test $ol_enable_dnssrv != no])
	AM_CONDITIONAL([DNSSRV_DYNAMIC], [test $ol_enable_dnssrv = mod])

	AM_CONDITIONAL([BUILD_HDB], [test $ol_enable_hdb != no])
	AM_CONDITIONAL([HDB_DYNAMIC], [test $ol_enable_hdb = mod])

	AM_CONDITIONAL([BUILD_LDAP], [test $ol_enable_ldap != no])
	AM_CONDITIONAL([LDAP_DYNAMIC], [test $ol_enable_ldap = mod])

	AM_CONDITIONAL([BUILD_META], [test $ol_enable_meta != no])
	AM_CONDITIONAL([META_DYNAMIC], [test $ol_enable_meta = mod])

	AM_CONDITIONAL([BUILD_NDB], [test $ol_enable_ndb != no])
	AM_CONDITIONAL([NDB_DYNAMIC], [test $ol_enable_ndb = mod])

	AM_CONDITIONAL([BUILD_NULL], [test $ol_enable_null != no])
	AM_CONDITIONAL([NULL_DYNAMIC], [test $ol_enable_null = mod])

	AM_CONDITIONAL([BUILD_PASSWD], [test $ol_enable_passwd != no])
	AM_CONDITIONAL([PASSWD_DYNAMIC], [test $ol_enable_passwd = mod])

	AM_CONDITIONAL([BUILD_PERL], [test $ol_enable_perl != no])
	AM_CONDITIONAL([PERL_DYNAMIC], [test $ol_enable_perl = mod])

	AM_CONDITIONAL([BUILD_RELAY], [test $ol_enable_relay != no])
	AM_CONDITIONAL([RELAY_DYNAMIC], [test $ol_enable_relay = mod])

	AM_CONDITIONAL([BUILD_SHELL], [test $ol_enable_shell != no])
	AM_CONDITIONAL([SHELL_DYNAMIC], [test $ol_enable_shell = mod])

	AM_CONDITIONAL([BUILD_SOCK], [test $ol_enable_sock != no])
	AM_CONDITIONAL([SOCK_DYNAMIC], [test $ol_enable_sock = mod])

	AM_CONDITIONAL([BUILD_SQL], [test $ol_enable_sql != no])
	AM_CONDITIONAL([SQL_DYNAMIC], [test $ol_enable_sql = mod])

#dnl overlays
	AM_CONDITIONAL([BUILD_ACCESSLOG], [test $ol_enable_accesslog != no])
	AM_CONDITIONAL([ACCESSLOG_DYNAMIC], [test $ol_enable_accesslog = mod])

	AM_CONDITIONAL([BUILD_AUDITLOG], [test $ol_enable_auditlog != no])
	AM_CONDITIONAL([AUDITLOG_DYNAMIC], [test $ol_enable_auditlog = mod])

	AM_CONDITIONAL([BUILD_COLLECT], [test $ol_enable_collect != no])
	AM_CONDITIONAL([COLLECT_DYNAMIC], [test $ol_enable_collect = mod])

	AM_CONDITIONAL([BUILD_CONSTRAINT], [test $ol_enable_constraint != no])
	AM_CONDITIONAL([CONSTRAINT_DYNAMIC], [test $ol_enable_constraint = mod])

	AM_CONDITIONAL([BUILD_DDS], [test $ol_enable_dds != no])
	AM_CONDITIONAL([DDS_DYNAMIC], [test $ol_enable_dds = mod])

	AM_CONDITIONAL([BUILD_DEREF], [test $ol_enable_deref != no])
	AM_CONDITIONAL([DEREF_DYNAMIC], [test $ol_enable_deref = mod])

	AM_CONDITIONAL([BUILD_DYNGROUP], [test $ol_enable_dyngroup != no])
	AM_CONDITIONAL([DYNGROUP_DYNAMIC], [test $ol_enable_dyngroup = mod])

	AM_CONDITIONAL([BUILD_DYNLIST], [test $ol_enable_dynlist != no])
	AM_CONDITIONAL([DYNLIST_DYNAMIC], [test $ol_enable_dynlist = mod])

	AM_CONDITIONAL([BUILD_MEMBEROF], [test $ol_enable_memberof != no])
	AM_CONDITIONAL([MEMBEROF_DYNAMIC], [test $ol_enable_memberof = mod])

	AM_CONDITIONAL([BUILD_PPOLICY], [test $ol_enable_ppolicy != no])
	AM_CONDITIONAL([PPOLICY_DYNAMIC], [test $ol_enable_ppolicy = mod])

	AM_CONDITIONAL([BUILD_PCACHE], [test $ol_enable_pcache != no])
	AM_CONDITIONAL([PCACHE_DYNAMIC], [test $ol_enable_pcache = mod])

	AM_CONDITIONAL([BUILD_REFINT], [test $ol_enable_refint != no])
	AM_CONDITIONAL([REFINT_DYNAMIC], [test $ol_enable_refint = mod])

	AM_CONDITIONAL([BUILD_RETCODE], [test $ol_enable_retcode != no])
	AM_CONDITIONAL([RETCODE_DYNAMIC], [test $ol_enable_retcode = mod])

	AM_CONDITIONAL([BUILD_RWM], [test $ol_enable_rwm != no])
	AM_CONDITIONAL([RWM_DYNAMIC], [test $ol_enable_rwm = mod])

	AM_CONDITIONAL([BUILD_SEQMOD], [test $ol_enable_seqmod != no])
	AM_CONDITIONAL([SEQMOD_DYNAMIC], [test $ol_enable_seqmod = mod])

	AM_CONDITIONAL([BUILD_SSSVLV], [test $ol_enable_sssvlv != no])
	AM_CONDITIONAL([SSSVLV_DYNAMIC], [test $ol_enable_sssvlv = mod])

	AM_CONDITIONAL([BUILD_SYNCPROV], [test $ol_enable_syncprov != no])
	AM_CONDITIONAL([SYNCPROV_DYNAMIC], [test $ol_enable_syncprov = mod])

	AM_CONDITIONAL([BUILD_TRANSLUCENT], [test $ol_enable_translucent != no])
	AM_CONDITIONAL([TRANSLUCENT_DYNAMIC], [test $ol_enable_translucent = mod])

	AM_CONDITIONAL([BUILD_UNIQUE], [test $ol_enable_unique != no])
	AM_CONDITIONAL([UNIQUE_DYNAMIC], [test $ol_enable_unique = mod])

	AM_CONDITIONAL([BUILD_VALSORT], [test $ol_enable_valsort != no])
	AM_CONDITIONAL([VALSORT_DYNAMIC], [test $ol_enable_valsort = mod])

AM_CONDITIONAL([CONTRIB], [test "ol_enable_contrib" != no])
AM_CONDITIONAL([CONTRIB_BROKEN], [test "ol_enable_contrib" = broken])

AC_SUBST(SLAPD_LIBS)
AC_SUBST(LTHREAD_LIBS)
AC_SUBST(LUTIL_LIBS)

AC_SUBST(SLAPD_STATIC_BACKENDS)
AC_SUBST(SLAPD_DYNAMIC_BACKENDS)
AC_SUBST(SLAPD_STATIC_OVERLAYS)
AC_SUBST(SLAPD_DYNAMIC_OVERLAYS)
AC_SUBST(SLAPD_BACKENDS_LIST)
AC_SUBST(SLAPD_OVERLAYS_LIST)

AC_SUBST(BDB_LIBS)
AC_SUBST(BDB_INCLUDES)

AC_SUBST(NDB_LIBS)
AC_SUBST(NDB_INCLUDES)
AC_SUBST(NDB_LDFLAGS)
AC_SUBST(NDB_RDIR)

AC_SUBST(PERL_CPPFLAGS)
AC_SUBST(PERL_LDFLAGS)
AC_SUBST(PERL_LIBS)
AC_SUBST(PERL_RDIR)

AC_SUBST(SQL_LIBS)
AC_SUBST(SQL_INCLUDES)

AC_SUBST(SASL_LIBS)
AC_SUBST(TLS_LIBS)
AC_SUBST(SLAPI_LIBS)
AC_SUBST(AUTH_LIBS)
#AC_SUBST(ICU_LIBS)

AC_SUBST(SLAPD_SLP_LIBS)
AC_SUBST(SLAPD_GMP_LIBS)

LDAP_LIBRELDAP_LA="\${top_builddir}/libraries/libreldap/libreldap.la"
LDAP_LIBREWRITE_LA="\${top_builddir}/libraries/librewrite/librewrite.la"
LDAP_LIBLUNICODE_LA="\${top_builddir}/libraries/liblunicode/liblunicode.la"
LDAP_LIBLUTIL_LA="\${top_builddir}/libraries/liblutil/liblutil.la"
AC_SUBST(LDAP_LIBRELDAP_LA)
AC_SUBST(LDAP_LIBREWRITE_LA)
AC_SUBST(LDAP_LIBLUNICODE_LA)
AC_SUBST(LDAP_LIBLUTIL_LA)

SECURITY_LIBS="${SASL_LIBS} ${TLS_LIBS} ${AUTH_LIBS}"
AC_SUBST(SECURITY_LIBS)

SLAPD_LIBS="$SLAPD_LIBS $SLAPD_SLP_LIBS $SLAPD_GMP_LIBS $SLAPD_TCPWRAP_LIBS"
AC_SUBST(SLAPD_LIBS)

dnl ----------------------------------------------------------------
dnl final help output
AC_ARG_WITH(xxinstall,[
See INSTALL file for further details.])

dnl ----------------------------------------------------------------
dnl final output
dnl

if test $AM_DEFAULT_VERBOSITY -eq 0; then
	AM_MAKEFLAGS="${AM_MAKEFLAGS} --no-print-directory"
fi
AC_SUBST(AM_MAKEFLAGS)

AC_CONFIG_FILES(
	Makefile
	build/libltdl/Makefile
	include/Makefile
	libraries/Makefile
		libraries/liblunicode/Makefile
		libraries/liblutil/Makefile
		libraries/libreldap/Makefile
		libraries/librewrite/Makefile
	clients/Makefile
		clients/tools/Makefile
	servers/Makefile
		servers/slapd/Makefile
		servers/slapd/slapi/Makefile
		servers/slapd/overlays/Makefile
		servers/slapd/back-ldif/Makefile
		servers/slapd/back-monitor/Makefile
		servers/slapd/back-mdb/Makefile

		servers/slapd/back-null/Makefile
		servers/slapd/back-ldap/Makefile
		servers/slapd/back-meta/Makefile
		servers/slapd/back-relay/Makefile
		servers/slapd/back-sock/Makefile

		servers/slapd/back-bdb/Makefile servers/slapd/back-hdb/Makefile
		servers/slapd/back-dnssrv/Makefile
		servers/slapd/back-ndb/Makefile
		servers/slapd/back-passwd/Makefile
		servers/slapd/back-perl/Makefile
		servers/slapd/back-shell/Makefile servers/slapd/shell-backends/Makefile
		servers/slapd/back-sql/Makefile

	doc/man/Makefile
	doc/man/ru/Makefile
	doc/man/en/Makefile

	contrib/Makefile
	contrib/slapi-plugins/Makefile
	contrib/slapd-tools/Makefile
	contrib/slapd-modules/Makefile
)

if test -x $ac_aux_dir/BRANDING; then
	AC_CONFIG_FILES(tests/Makefile tests/progs/Makefile tests/run)
	AC_CONFIG_COMMANDS([default],[[chmod +x tests/run]])
fi

AC_OUTPUT

AC_MSG_CHECKING(['libtool' for support LTO (Link Time Optimization)])
if sed -i -e 's/\$pic_flag_for_symtable /&`echo -- "\$LTCFLAGS" | grep -q -w -- "-flto" \&\& echo -n "-fno-lto "`/' libtool; then
	AC_MSG_RESULT([done patching])
else
	AC_MSG_RESULT([something went wrong, not supported])
fi
