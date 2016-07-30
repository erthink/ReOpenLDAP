# Makefile for -llutil
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

noinst_LTLIBRARIES = liblutil.la

AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name lutil 'liblutil' > $@ || $(RM) $@
endif

liblutil_la_SOURCES = avl.c banner.c base64.c detach.c \
	entropy.c getopt.c getpass.c getpeereid.c hash.c lockf.c md5.c \
	meter.c passfile.c passwd.c sasl.c setproctitle.c sha1.c \
	signal.c sockpair.c tavl.c utils.c uuid.c

liblutil_la_LIBADD = $(LUTIL_LIBS) $(SASL_LIBS) $(SECURITY_LIBS) $(UUID_LIB)
