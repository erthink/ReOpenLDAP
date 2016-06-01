# Makefile.in for tests
# $ReOpenLDAP$
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
## ---
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

RUN=./run
SUBDIRS= progs

BUILD_BDB=@BUILD_BDB@
BUILD_HDB=@BUILD_HDB@
BUILD_MDB=@BUILD_MDB@
BUILD_SQL=@BUILD_SQL@

# test primary backends (default)
test tests:
	@$(MAKE) mdb bdb hdb

# test all backends
alltests: tests
	@$(MAKE) ldif sql

bdb test-bdb:	bdb-$(BUILD_BDB)
bdb-no:
	@echo "run configure with --enable-bdb to run BDB tests"

bdb-yes bdb-mod: FORCE
	@echo "Initiating LDAP tests for BDB..."
	@$(RUN) -b bdb all

hdb test-hdb:	hdb-$(BUILD_HDB)
hdb-no:
	@echo "run configure with --enable-hdb to run HDB tests"

hdb-yes hdb-mod: FORCE
	@echo "Initiating LDAP tests for HDB..."
	@$(RUN) -b hdb all

mdb test-mdb:	mdb-$(BUILD_MDB)
mdb-no:
	@echo "run configure with --enable-mdb to run MDB tests"

mdb-yes mdb-mod: FORCE
	@echo "Initiating LDAP tests for MDB..."
	@$(RUN) -b mdb all

sql test-sql:	sql-$(BUILD_SQL)
sql-no:
	@echo "run configure with --enable-sql to run SQL tests"

sql-yes sql-mod:	FORCE
	@echo "Initiating LDAP tests for SQL..."
	@$(RUN) -b sql sql-all

ldif test-ldif: FORCE
	@echo "Initiating LDAP tests for LDIF..."
	@$(RUN) -b ldif all

regressions:	FORCE
	@echo "Testing (available) ITS regressions"
	@$(RUN) its-all

its: regressions

clean-local:	FORCE
	-$(RM) -r testrun configpw configpw.conf *leak *gmon *core

veryclean-local: FORCE
	@-$(RM) run testdata schema ucdata
