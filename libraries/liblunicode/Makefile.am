# Makefile.in for LDAP -llunicode
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

noinst_LTLIBRARIES = liblunicode.la
BUILT_SOURCES =

AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES += banner.c uctable.h

banner.c: Makefile
	$(MKBANNER) 'liblunicode' > $@

noinst_PROGRAMS = ucgendat
nodist_ucgendat_SOURCES = ucgendat.c
ucgendat_CPPFLAGS = -DSKIP_LDAP_AC_INCLUDES $(AM_CPPFLAGS)

uctable.h: ucgendat$(EXEEXT) UnicodeData.txt CompositionExclusions.txt
	@if $(AM_V_P); then set -x; else echo "  GEN      $@"; fi; \
	./ucgendat$(EXEEXT) $(srcdir)/UnicodeData.txt -x $(srcdir)/CompositionExclusions.txt \
		|| ($(RM) $@; exit 1)

endif

liblunicode_la_SOURCES = banner.c ucstr.c ucdata.c ure.c \
	urestubs.c ucdata.h ucpgba.h uctable.h ure.h utbm.h
