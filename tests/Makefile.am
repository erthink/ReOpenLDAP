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

SUBDIRS = progs
RUN = $(builddir)/run
BACKENDS = $(SLAPD_DYNAMIC_BACKENDS) $(SLAPD_STATIC_BACKENDS)
check test check-TESTS: $(BACKENDS)

@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = scripts/valgrind.supp
EXTRA_DIST = scripts/valgrind.supp

clean-local:
	-$(RM) -r testrun configpw configpw.conf *gmon *core

# LY: still not ready
#ldif:
#	@echo "Initiating LDAP tests for LDIF..."
#	@$(RUN) -b $@ all || exit $$?
#	@echo "Testing (available) ITS regressions for LDIF..."
#	@$(RUN) -b $@ its-all || exit $$?

mdb mdbx:
if BUILD_MDBX
	@echo "Initiating LDAP tests for MDBX-backend..."
	@$(RUN) -b $@ all || exit $$?
	@echo "Testing (available) ITS regressions for MDBX-backend..."
	@$(RUN) -b $@ its-all || exit $$?
else
	@echo "run configure with --enable-mdbx to run MDBX-backend tests"
endif

bdb:
if BUILD_BDB
	@echo "Initiating LDAP tests for obsolete BDB-backend..."
	@$(RUN) -b $@ all || exit $$?
	@echo "Testing (available) ITS regressions for obsolete BDB-backend..."
	@$(RUN) -b $@ its-all || exit $$?
else
	@echo "run configure with --enable-bdb to run obsolete BDB-backend tests"
endif

hdb:
if BUILD_HDB
	@echo "Initiating LDAP tests for obsolete HDB-backend..."
	@$(RUN) -b $@ all || exit $$?
	@echo "Testing (available) ITS regressions for obsolete HDB-backend..."
	@$(RUN) -b $@ its-all || exit $$?
else
	@echo "run configure with --enable-hdb to run obsolete HDB-backend tests"
endif

# LY: TODO
ldif dnssrv ldap meta monitor ndb null passwd perl relay shell sock sql:
	@echo "Skip testing $@-backend for now (TODO)"
