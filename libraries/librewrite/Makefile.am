# LIBREWRITE
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

noinst_LTLIBRARIES = librewrite.la

AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name rewrite 'librewrite' > $@ || $(RM) $@
endif

librewrite_la_LIBADD = $(LDAP_LIBRELDAP_LA) $(LDAP_LIBLUTIL_LA)

librewrite_la_SOURCES = banner.c config.c context.c info.c \
	ldapmap.c map.c params.c rule.c session.c subst.c var.c xmap.c \
	rewrite-int.h rewrite-map.h

noinst_PROGRAMS = rewrite

rewrite_SOURCES = rewrite.c parse.c
rewrite_LDADD = librewrite.la
