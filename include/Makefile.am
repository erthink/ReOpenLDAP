# include Makefile.in for ReOpenLDAP
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

all-local: ldap_config.h FORCE

install-data-local: FORCE
	-$(MKDIR) $(DESTDIR)$(includedir)
	for header in $(srcdir)/lber.h lber_types.h \
		$(srcdir)/ldap.h $(srcdir)/ldap_cdefs.h \
		$(srcdir)/ldap_schema.h $(srcdir)/ldap_utf8.h \
		$(srcdir)/slapi-plugin.h ldap_features.h \
		$(srcdir)/ldif.h ; \
	do \
		$(INSTALL) $(INSTALLFLAGS) -m 644 $$header $(DESTDIR)$(includedir); \
	done

clean-local: FORCE
	$(RM) ldap_config.h

veryclean-local: clean-local FORCE
	$(RM) portable.h lber_types.h ldap_features.h

depend-local: ldap_config.h FORCE

LDAP_CONFIG=$(srcdir)/ldap_config.hin

ldap_config.h: $(LDAP_CONFIG) Makefile
	@$(RM) $@
	@echo "Making $@"
	@echo "/* Generated from $(LDAP_CONFIG) on `date` */" > $@;  \
	sysconfdir=$(sysconfdir); \
	datadir=$(datadir); \
	bindir=$(bindir); \
	sbindir=$(sbindir); \
	libexecdir=$(libexecdir); \
	moduledir=$(moduledir); \
	localstatedir=$(localstatedir); \
	localedir=$(localedir); \
	$(SED) \
			-e "s;%SYSCONFDIR%;$$sysconfdir;" \
			-e "s;%DATADIR%;$$datadir;" \
			-e "s;%BINDIR%;$$bindir;" \
			-e "s;%SBINDIR%;$$sbindir;" \
			-e "s;%LIBEXECDIR%;$$libexecdir;" \
			-e "s;%MODULEDIR%;$$moduledir;" \
			-e "s;%RUNDIR%;$$localstatedir;" \
			-e "s;%LOCALEDIR%;$$localedir;" \
			 $(LDAP_CONFIG) >> $@; \
	$(CHMOD) 444 $@

all-common:			all-local
install-common:		all-common install-local
clean-common:		clean-local
veryclean-common:	veryclean-local
depend-common:		depend-local
