# $ReOpenLDAP$
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
## ---
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
##---------------------------------------------------------------------------
#
# Top-level Makefile template
#

PACKAGE= @PACKAGE@
VERSION= @VERSION@
RELEASEDATE= @REOPENLDAP_RELEASE_DATE@

@SET_MAKE@
SHELL = /bin/sh

top_builddir = @top_builddir@

srcdir = @srcdir@
top_srcdir = @top_srcdir@
VPATH = @srcdir@
prefix = @prefix@
exec_prefix = @exec_prefix@
ldap_subdir = @ldap_subdir@

bindir = @bindir@
datarootdir = @datarootdir@
datadir = @datadir@$(ldap_subdir)
includedir = @includedir@
infodir = @infodir@
libdir = @libdir@
libexecdir = @libexecdir@
localstatedir = @localstatedir@
mandir = @mandir@
moduledir = @libexecdir@$(ldap_subdir)
sbindir = @sbindir@
sharedstatedir = @sharedstatedir@
sysconfdir = @sysconfdir@$(ldap_subdir)
schemadir = $(sysconfdir)/schema

EXEEXT = @EXEEXT@
OBJEXT = @OBJEXT@

BUILD_LIBS_DYNAMIC = @BUILD_LIBS_DYNAMIC@

SHTOOL = $(top_srcdir)/build/shtool

INSTALL = $(SHTOOL) install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_SCRIPT = $(INSTALL)

STRIP = -s

LINT = lint
5LINT = 5lint

MKDEP = $(top_srcdir)/build/mkdep $(MKDEPFLAG) \
	-d "$(srcdir)" -c "$(MKDEP_CC)" -m "$(MKDEP_CFLAGS)"
MKDEP_CC	= @OL_MKDEP@
MKDEP_CFLAGS = @OL_MKDEP_FLAGS@

MKVERSION = $(top_srcdir)/build/mkversion -v "$(VERSION)"

LIBTOOL = @LIBTOOL@
LIBRELEASE = @REOPENLDAP_LIBRELEASE@
LIBVERSION = @REOPENLDAP_LIBVERSION@
LTVERSION = -release $(LIBRELEASE) -version-info $(LIBVERSION)

# libtool --only flag for modules: depends on linkage of module
# The BUILD_MOD macro is defined in each backend Makefile.in file
LTONLY_yes = --tag=disable-shared
LTONLY_mod = --tag=disable-static
LTONLY_MOD = $(LTONLY_$(BUILD_MOD))

# libtool flags
LTFLAGS     =
LTFLAGS_LIB = $(LTVERSION) -rpath $(libdir)
LTFLAGS_MOD = $(LTVERSION) -rpath $(moduledir)

# LIB_DEFS defined in liblber and libldap Makefile.in files.
# MOD_DEFS defined in backend Makefile.in files.

# NEED_LIBS defined in various Makefile.in files.
# LINK_LIBS referenced in library and module link commands.
LINK_LIBS = $(MOD_LIBS) $(NEED_LIBS)

LTSTATIC = @LTSTATIC@

LTLINK   = $(LIBTOOL) --mode=link \
	$(CC) $(LTSTATIC) $(LT_CFLAGS) $(LDFLAGS) $(LTFLAGS)

LTCOMPILE_LIB = $(LIBTOOL) $(LTONLY_LIB) --mode=compile \
	$(CC) $(LT_CFLAGS) $(LT_CPPFLAGS) $(LIB_DEFS) -c

LTLINK_LIB = $(LIBTOOL) $(LTONLY_LIB) --mode=link \
	$(CC) $(LT_CFLAGS) $(LDFLAGS) $(LTFLAGS_LIB)

LTCOMPILE_MOD = $(LIBTOOL) $(LTONLY_MOD) --mode=compile \
	$(CC) $(LT_CFLAGS) $(LT_CPPFLAGS) $(MOD_DEFS) -c

LTLINK_MOD = $(LIBTOOL) $(LTONLY_MOD) --mode=link \
	$(CC) $(LT_CFLAGS) $(LDFLAGS) $(LTFLAGS_MOD)


LTLINK_CXX   = $(LIBTOOL) --mode=link \
	$(CXX) $(LTSTATIC) $(LT_CXXFLAGS) $(LDFLAGS) $(LTFLAGS)

LTCOMPILE_LIB_CXX = $(LIBTOOL) $(LTONLY_LIB) --mode=compile \
	$(CXX) $(LT_CXXFLAGS) $(LT_CPPFLAGS) $(LIB_DEFS) -c

LTLINK_LIB_CXX = $(LIBTOOL) $(LTONLY_LIB) --mode=link \
	$(CXX) $(LT_CXXFLAGS) $(LDFLAGS) $(LTFLAGS_LIB)

LTCOMPILE_MOD_CXX = $(LIBTOOL) $(LTONLY_MOD) --mode=compile \
	$(CXX) $(LT_CXXFLAGS) $(LT_CPPFLAGS) $(MOD_DEFS) -c

LTLINK_MOD_CXX = $(LIBTOOL) $(LTONLY_MOD) --mode=link \
	$(CXX) $(LT_CXXFLAGS) $(LDFLAGS) $(LTFLAGS_MOD)


LTINSTALL = $(LIBTOOL) --mode=install $(INSTALL)
LTFINISH = $(LIBTOOL) --mode=finish

# Misc UNIX commands used in build environment
AR = @AR@
BASENAME = basename
CAT = cat
CHMOD = chmod
DATE = date
ECHO = $(SHTOOL) echo
HOSTNAME = $(SHTOOL) echo -e "%h%d"
LN = $(SHTOOL) mkln
LN_H = $(LN)
LN_S = $(LN) -s
MAKEINFO = @MAKEINFO@
MKDIR = $(SHTOOL) mkdir -p
MV = $(SHTOOL) move
PWD = pwd
RANLIB = @RANLIB@
RM = rm -f
SED = sed
SUBST = $(SHTOOL) subst

# For manual pages
# MANCOMPRESS=@MANCOMPRESS@
# MANCOMPRESSSUFFIX=@MANCOMPRESSSUFFIX@
MANCOMPRESS=$(CAT)
MANCOMPRESSSUFFIX=

SOELIM=soelim

INCLUDEDIR= $(top_srcdir)/include
LDAP_INCPATH= -I$(LDAP_INCDIR) -I$(INCLUDEDIR)
LDAP_LIBDIR= $(top_builddir)/libraries

LUTIL_LIBS = @LUTIL_LIBS@
LTHREAD_LIBS = @LTHREAD_LIBS@

BDB_LIBS = @BDB_LIBS@
SLAPD_NDB_LIBS = @SLAPD_NDB_LIBS@

LDAP_LIBRELDAP_LA = $(LDAP_LIBDIR)/libreldap/libreldap.la
LDAP_LIBREWRITE_A = $(LDAP_LIBDIR)/librewrite/librewrite.a
LDAP_LIBLUNICODE_A = $(LDAP_LIBDIR)/liblunicode/liblunicode.a
LDAP_LIBLUTIL_A = $(LDAP_LIBDIR)/liblutil/liblutil.a

LDAP_L = $(LDAP_LIBLUTIL_A) $(LDAP_LIBRELDAP_LA)
SLAPD_L = $(LDAP_LIBLUNICODE_A) $(LDAP_LIBREWRITE_A) \
	$(LDAP_LIBLUTIL_A) $(LDAP_LIBRELDAP_LA)

WRAP_LIBS = @WRAP_LIBS@
# AutoConfig generated
AC_CC	= @CC@
AC_CFLAGS = @CFLAGS@
AC_CXX	= @CXX@
AC_CXXFLAGS = @CXXFLAGS@
AC_DEFS = @CPPFLAGS@ # @DEFS@
AC_LDFLAGS = @LDFLAGS@
AC_LIBS = @LIBS@

KRB4_LIBS = @KRB4_LIBS@
KRB5_LIBS = @KRB5_LIBS@
KRB_LIBS = @KRB4_LIBS@ @KRB5_LIBS@
SASL_LIBS = @SASL_LIBS@
TLS_LIBS = @TLS_LIBS@
AUTH_LIBS = @AUTH_LIBS@
SECURITY_LIBS = $(SASL_LIBS) $(KRB_LIBS) $(TLS_LIBS) $(AUTH_LIBS)
ICU_LIBS = @ICU_LIBS@

MODULES_CPPFLAGS = @SLAPD_MODULES_CPPFLAGS@
MODULES_LDFLAGS = @SLAPD_MODULES_LDFLAGS@
MODULES_LIBS = @MODULES_LIBS@
SLAPD_PERL_LDFLAGS = @SLAPD_PERL_LDFLAGS@

SLAPD_SQL_LDFLAGS = @SLAPD_SQL_LDFLAGS@
SLAPD_SQL_INCLUDES = @SLAPD_SQL_INCLUDES@
SLAPD_SQL_LIBS = @SLAPD_SQL_LIBS@

SLAPD_LIBS = @SLAPD_LIBS@ @SLAPD_PERL_LDFLAGS@ @SLAPD_SQL_LDFLAGS@ @SLAPD_SQL_LIBS@ @SLAPD_SLP_LIBS@ @SLAPD_GMP_LIBS@ $(ICU_LIBS)

# Our Defaults
CC = $(AC_CC)
CXX = $(AC_CXX)
DEFS = $(LDAP_INCPATH) $(XINCPATH) $(XDEFS) $(AC_DEFS) $(DEFINES)
CFLAGS = $(AC_CFLAGS) $(DEFS)
CXXFLAGS = $(AC_CXXFLAGS) $(DEFS)
LDFLAGS = $(LDAP_LIBPATH) $(AC_LDFLAGS) $(XLDFLAGS)
LIBS = $(XLIBS) $(XXLIBS) $(AC_LIBS) $(XXXLIBS)

LT_CFLAGS = $(AC_CFLAGS)
LT_CXXFLAGS = $(AC_CXXFLAGS)
LT_CPPFLAGS = $(DEFS)

all:		all-common all-local FORCE
install:	install-common install-local FORCE
clean:		clean-common clean-local FORCE
veryclean:	veryclean-common veryclean-local FORCE
depend:		depend-common depend-local FORCE

# empty common rules
all-common:
install-common:
clean-common:
veryclean-common:	clean-common FORCE
depend-common:
lint-common:
lint5-common:

# empty local rules
all-local:
install-local:
clean-local:
veryclean-local:	clean-local FORCE
depend-local:
lint-local:
lint5-local:

veryclean: FORCE
	$(RM) Makefile
	$(RM) -r .libs

Makefile: Makefile.in $(top_srcdir)/build/top.mk

pathtest:
	$(SHTOOL) --version

# empty rule for forcing rules
FORCE:

##---------------------------------------------------------------------------
