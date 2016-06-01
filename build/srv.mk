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
##---------------------------------------------------------------------------
#
# Makefile Template for Servers
#

all-common: all-$(BUILD_SRV)
all-no lint-no 5lint-no depend-no install-no:
	@echo "run configure with $(BUILD_OPT) to make $(PROGRAMS)"

clean-common: clean-srv FORCE
veryclean-common: veryclean-srv FORCE

lint-common: lint-$(BUILD_SRV)

5lint-common: 5lint-$(BUILD_SRV)

depend-common: depend-$(BUILD_SRV)

install-common: install-$(BUILD_SRV)

all-local-srv:
all-yes: all-local-srv FORCE

install-local-srv:
install-yes: install-local-srv FORCE

lint-local-srv:
lint-yes: lint-local-srv FORCE
	$(LINT) $(DEFS) $(DEFINES) $(SRCS)

5lint-local-srv:
5lint-yes: 5lint-local-srv FORCE
	$(5LINT) $(DEFS) $(DEFINES) $(SRCS)

clean-local-srv:
clean-srv: 	clean-local-srv FORCE
	$(RM) $(PROGRAMS) $(XPROGRAMS) $(XSRCS) *.o a.out core .libs/*

depend-local-srv:
depend-yes: depend-local-srv FORCE
	$(MKDEP) $(DEFS) $(DEFINES) $(SRCS)

veryclean-local-srv:
veryclean-srv: 	clean-srv veryclean-local-srv

Makefile: $(top_srcdir)/build/srv.mk
