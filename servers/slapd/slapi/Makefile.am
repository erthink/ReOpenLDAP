# Makefile.in for SLAPI
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

pkglib_LTLIBRARIES = libreslapi.la
libreslapi_la_LDFLAGS = -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION) \
	-weak libslapi.la

AM_CPPFLAGS = -DSLAPI_LIBRARY -DSLAPD_IMPORT \
	-I$(srcdir)/.. -I$(top_srcdir)/include $(LTDLINCL)

libreslapi_la_LIBADD = $(SLAPI_LIBS) $(LIBLTDL)
libreslapi_la_DEPENDENCIES = $(LTDLDEPS)

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name SLAPI --global 'libreslapi' > $@
endif

libreslapi_la_SOURCES = banner.c plugin.c printmsg.c slapi_dn.c \
	slapi_ext.c slapi_ops.c slapi_overlay.c slapi_pblock.c \
	slapi_utils.c proto-slapi.h slapi.h
