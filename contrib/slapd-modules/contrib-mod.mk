# $ReOpenLDAP$
# Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
# Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
#
# This file is part of ReOpenLDAP.
#
# ReOpenLDAP is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# ReOpenLDAP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CONTRIB_DIR 	:= $(dir $(lastword $(MAKEFILE_LIST)))
LDAP_SRC	?= $(CONTRIB_DIR)/../..
LDAP_BUILD	?= $(LDAP_SRC)
LDAP_INC	:= -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB	:= $(LDAP_BUILD)/libraries/libreldap/libreldap.la
LIBTOOL		:= $(LDAP_BUILD)/libtool
CC		?= gcc
CXX		?= g++
CFLAGS		?= -g -O2 -Wall -Werror
