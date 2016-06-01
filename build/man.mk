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
# Makefile Template for Manual Pages
#

MANDIR=$(mandir)/man$(MANSECT)
TMP_SUFFIX=tmp

all-common:
	PAGES=`cd $(srcdir); echo *.$(MANSECT)`; \
	for page in $$PAGES; do \
		$(SED) -e "s%LDVERSION%$(VERSION)%" \
			-e 's%ETCDIR%$(sysconfdir)%g' \
			-e 's%LOCALSTATEDIR%$(localstatedir)%' \
			-e 's%SYSCONFDIR%$(sysconfdir)%' \
			-e 's%DATADIR%$(datadir)%' \
			-e 's%SBINDIR%$(sbindir)%' \
			-e 's%BINDIR%$(bindir)%' \
			-e 's%LIBDIR%$(libdir)%' \
			-e 's%LIBEXECDIR%$(libexecdir)%' \
			-e 's%MODULEDIR%$(moduledir)%' \
			-e 's%RELEASEDATE%$(RELEASEDATE)%' \
				$(srcdir)/$$page \
			| (cd $(srcdir); $(SOELIM) -) > $$page.$(TMP_SUFFIX); \
	done

install-common:
	-$(MKDIR) $(DESTDIR)$(MANDIR)
	PAGES=`cd $(srcdir); echo *.$(MANSECT)`; \
	for page in $$PAGES; do \
		echo "installing $$page in $(DESTDIR)$(MANDIR)"; \
		$(RM) $(DESTDIR)$(MANDIR)/$$page; \
		$(INSTALL) $(INSTALLFLAGS) -m 644 $$page.$(TMP_SUFFIX) $(DESTDIR)$(MANDIR)/$$page; \
		if test -f "$(srcdir)/$$page.links" ; then \
			for link in `$(CAT) $(srcdir)/$$page.links`; do \
				echo "installing $$link in $(DESTDIR)$(MANDIR) as link to $$page"; \
				$(RM) $(DESTDIR)$(MANDIR)/$$link ; \
				$(LN_S) $(DESTDIR)$(MANDIR)/$$page $(DESTDIR)$(MANDIR)/$$link; \
			done; \
		fi; \
	done

clean-common:   FORCE
	$(RM) *.tmp all-common

Makefile: $(top_srcdir)/build/man.mk
