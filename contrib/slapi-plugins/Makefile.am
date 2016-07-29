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

EXTRA_DIST = addrdnvalues/README
pkglib_LTLIBRARIES = slapiplugin-addrdnvalues.la
AM_CPPFLAGS = -I$(top_srcdir)/include
BUILT_SOURCES =

if BEHIND_CURTIAN
BUILT_SOURCES += banner.c
banner.c: Makefile
	$(MKBANNER) --name contrib_slapi_plugins --global 'contrib-slapi-plugins' > $@ || $(RM) $@
endif

slapiplugin_addrdnvalues_la_LDFLAGS = -module -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
slapiplugin_addrdnvalues_la_SOURCES = ./addrdnvalues/addrdnvalues.c banner.c
