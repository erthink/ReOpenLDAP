# Makefile.in for back-monitor
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
back_monitor_la_LDFLAGS = -module
AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/include
EXTRA_DIST = README

if MONITOR_DYNAMIC
pkglib_LTLIBRARIES += back_monitor.la
back_monitor_la_LDFLAGS += -export-dynamic \
	-release $(API_LIB_RELEASE) -version-info $(API_LIB_VERSION)
else
noinst_LTLIBRARIES += back_monitor.la
endif

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name backend_monitor --global 'backend-monitor' > $@ || $(RM) $@
endif

back_monitor_la_SOURCES = backend.c banner.c bind.c cache.c \
	compare.c conn.c database.c entry.c init.c listener.c log.c \
	modify.c operational.c operation.c overlay.c rww.c search.c \
	sent.c thread.c time.c back-monitor.h proto-back-monitor.h
