## $ReOpenLDAP$
## Copyright 2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
