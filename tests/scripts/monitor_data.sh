#!/bin/bash
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

MONITORDB="$1"
SRCDIR="$2"
DSTDIR="$3"

echo "MONITORDB $MONITORDB"
echo "SRCDIR $SRCDIR"
echo "DSTDIR $DSTDIR"
echo "pwd `pwd`"

# copy test data
cp "$SRCDIR"/do_* "$DSTDIR"
if test $MONITORDB != no ; then

	# add back-monitor testing data
	cat >> "$DSTDIR/do_search.0" << EOF
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
EOF

	cat >> "$DSTDIR/do_read.0" << EOF
cn=Backend 1,cn=Backends,cn=Monitor
cn=Entries,cn=Statistics,cn=Monitor
cn=Database 1,cn=Databases,cn=Monitor
EOF

fi
