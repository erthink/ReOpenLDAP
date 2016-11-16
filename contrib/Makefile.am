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

DIST_SUBDIRS = slapd-modules slapd-tools slapi-plugins
SUBDIRS = slapd-tools

EXTRA_DIST = ConfigOIDs README

if SLAPD_DYNAMIC
SUBDIRS += slapd-modules
endif

if BUILD_SLAPI
SUBDIRS += slapi-plugins
endif
