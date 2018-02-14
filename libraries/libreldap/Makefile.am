## $ReOpenLDAP$
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

pkglib_LTLIBRARIES = libreldap.la
sysconf_DATA = ldap.conf
EXTRA_DIST = ldap.conf

AM_CPPFLAGS = -DRELDAP_LIBRARY -DLDAP_R_COMPILE -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name reldap --global 'libreldap' > $@ || $(RM) $@
endif

libreldap_la_LDFLAGS = -no-undefined -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION) \
	-weak libldap.la -weak liblber.la

libreldap_la_SOURCES = abandon.c add.c addentry.c assertion.c \
	banner.c bprint.c cancel.c charray.c compare.c \
	controls.c cyrus.c dds.c debug.c decode.c delete.c deref.c \
	dnssrv.c encode.c error.c extended.c fetch.c filter.c free.c \
	getattr.c getdn.c getentry.c getvalues.c gssapi.c hipagut.c \
	init.c io.c lber_options.c ldap_options.c ldap_sync.c ldif.c \
	memory.c messages.c modify.c modrdn.c open.c ntlm.c os-ip.c \
	os-local.c pagectrl.c passwd.c posix.c ppolicy.c references.c \
	request.c result.c rq.c sasl.c schema.c lbase64.c \
	search.c sockbuf.c sort.c sortctrl.c stctrl.c stdio.c string.c \
	t61.c threads.c tls2.c tls_g.c tls_m.c tls_o.c tpool.c turn.c \
	txn.c unbind.c url.c utf-8.c utf-8-conv.c util-int.c vlvctrl.c \
	whoami.c crutch.c lber-int.h ldap-int.h ldap-tls.h

# LY: append-hack CFLAGS to avoid lost symver's tags by LTO.
nolto_CFLAGS := $(CFLAGS) $$(echo "$(CFLAGS)" | grep -q ' -flto' && echo '-fno-lto') -save-temps
libreldap_la-crutch.o libreldap_la-crutch.lo crutch.o crutch.lo: CFLAGS = $(nolto_CFLAGS)

if LDAP_DEPRECATED
libreldap_la_SOURCES += deprecated.c
libreldap_la-deprecated.o libreldap_la-deprecated.lo deprecated.o deprecated.lo: CFLAGS += -Wno-deprecated-declarations
endif

libreldap_la_LIBADD = $(SECURITY_LIBS) $(LTHREAD_LIBS)

CHECK_LIBS = ../liblutil/liblutil.la libreldap.la
check_PROGRAMS = check/apitest check/dntest check/dtest check/etest check/ftest \
	check/idtest check/test check/urltest check/avltest check/tavltest

# LY: ptest is broken (TODO: fix or remote it).
# check_PROGRAMS += ptest
# check_ptest_LDADD = $(CHECK_LIBS)

check_apitest_LDADD = $(CHECK_LIBS)
check_avltest_LDADD = $(CHECK_LIBS)
check_dntest_LDADD = $(CHECK_LIBS)
check_dtest_LDADD = $(CHECK_LIBS)
check_etest_LDADD = $(CHECK_LIBS)
check_ftest_LDADD = $(CHECK_LIBS)
check_idtest_LDADD = $(CHECK_LIBS)
check_tavltest_LDADD = $(CHECK_LIBS)
check_test_LDADD = $(CHECK_LIBS)
check_urltest_LDADD = $(CHECK_LIBS)
