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

SUBDIRS = progs
RUN = $(builddir)/run
BACKENDS = $(SLAPD_DYNAMIC_BACKENDS) $(SLAPD_STATIC_BACKENDS)
check test: $(BACKENDS)

clean-local:
	-$(RM) -r testrun configpw configpw.conf *gmon *core

its regressions:
	@echo "Testing (available) ITS regressions"
	@$(RUN) its-all

# LY: still not ready
#ldif:
#	@echo "Initiating LDAP tests for LDIF..."
#	@$(RUN) -b $@ all

mdb mdbx:
if BUILD_MDBX
	@echo "Initiating LDAP tests for MDBX..."
	@$(RUN) -b $@ all
else
	@echo "run configure with --enable-mdbx to run MDBX tests"
endif

bdb:
if BUILD_BDB
	@echo "Initiating LDAP tests for BDB..."
	@$(RUN) -b $@ all
else
	@echo "run configure with --enable-bdb to run BDB tests"
endif

hdb:
if BUILD_HDB
	@echo "Initiating LDAP tests for HDB..."
	@$(RUN) -b $@ all
else
	@echo "run configure with --enable-hdb to run HDB tests"
endif

# LY: TODO
ldif dnssrv ldap meta asyncmeta monitor ndb null passwd perl relay shell sock sql wt:
	@echo "Skip testing $@-backend for now (TODO)"
