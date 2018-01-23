## $ReOpenLDAP$
## Copyright 2017-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

ACLOCAL_AMFLAGS = -I build/m4

DIST_SUBDIRS = include libraries clients servers doc/man build/libltdl contrib
EXTRA_DIST = build/stub build/common.mk

if SLAPD_DYNAMIC
SUBDIRS = build/libltdl
else
SUBDIRS =
endif

SUBDIRS += include libraries clients servers doc/man

if CONTRIB
SUBDIRS += contrib
endif

am__tar = $${TAR-tar} cof - "$$tardir"
if BEHIND_CURTIAN
SUBDIRS += tests
test:
	$(MAKE) -C tests check </dev/null

dist-hook:
	find $(distdir) -type f | xargs -r \
		sed -i 's/\$$ReOpenLDAP\$$/$$ $(RELEASE_STAMP) | $(RELEASE_DATE) | $(shell $(srcdir)/build/BRANDING --title) $$/g' && \
	find $(distdir) -name Makefile.am -exec sh -c 'ln -sfr -T $(distdir)/build/stub {}' \; && \
	ln -sfr -T $(distdir)/build/stub $(distdir)/configure.ac && \
	find $(distdir) -type f | xargs -r \
		touch -r $(distdir)/configure && \
	find $(distdir) -type d -empty -exec rmdir {} \;

endif
