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
