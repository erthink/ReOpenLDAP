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

ACLOCAL_AMFLAGS = -I build/m4

DIST_SUBDIRS = include libraries clients servers doc/man build/libltdl
SUBDIRS = include libraries clients servers doc/man

EXTRA_DIST = build/stub

if SLAPD_DYNAMIC
SUBDIRS += build/libltdl
endif

am__tar = $${TAR-tar} cof - "$$tardir"
if BEHIND_CURTIAN
SUBDIRS += tests
test:
	$(MAKE) -C tests check

dist-hook:
	find $(distdir) -type f | xargs -r \
		sed -i 's/\$$ReOpenLDAP\$$/$$ $(RELEASE_STAMP) | $(RELEASE_DATE) | $(shell $(srcdir)/build/BRANDING --title) $$/g' && \
	find $(distdir) -name Makefile.am -exec sh -c 'ln -sfr -T $(distdir)/build/stub {}' \; && \
	ln -sfr -T $(distdir)/build/stub $(distdir)/configure.ac && \
	find $(distdir) -type f | xargs -r \
		touch -r $(distdir)/configure && \
	find $(distdir) -type d -empty -exec rmdir {} \;

endif
