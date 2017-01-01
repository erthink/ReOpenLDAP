## $ReOpenLDAP$
## Copyright 2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

bin_PROGRAMS = passwd-shell
AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name passwd_shell --global 'passwd-shell' > $@ || $(RM) $@
endif

passwd_shell_SOURCES = banner.c passwd-shell.c shellutil.c shellutil.h
passwd_shell_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)
