# Makefile.in for shell-backends
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

bin_PROGRAMS = passwd-shell
AM_CPPFLAGS = -I$(top_srcdir)/include

if BEHIND_CURTIAN
BUILT_SOURCES = banner.c
banner.c: Makefile
	$(MKBANNER) --name passwd_shell --global 'passwd-shell' > $@
endif

passwd_shell_SOURCES = banner.c passwd-shell.c shellutil.c shellutil.h
passwd_shell_LDADD = $(LDAP_LIBLUTIL_LA) $(LDAP_LIBRELDAP_LA)
