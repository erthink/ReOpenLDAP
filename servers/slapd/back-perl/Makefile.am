# Makefile.in for back-perl
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

pkglib_LTLIBRARIES =
noinst_LTLIBRARIES =
back_perl_la_LDFLAGS = -module $(PERL_RDIR) $(PERL_LDFLAGS)
AM_CPPFLAGS = -DSLAPD_IMPORT -I$(srcdir)/.. -I$(top_srcdir)/include
EXTRA_DIST = README SampleLDAP.pm

if PERL_DYNAMIC
pkglib_LTLIBRARIES += back_perl.la
back_perl_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_perl.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_perl --global 'backend-perl' > $@ || $(RM) $@
endif

back_perl_la_SOURCES = add.c banner.c bind.c close.c compare.c \
	config.c delete.c init.c modify.c modrdn.c perl_back.h \
	proto-perl.h search.c

AM_CPPFLAGS += $(PERL_CPPFLAGS)
back_perl_la_LIBADD = $(PERL_LIBS)
