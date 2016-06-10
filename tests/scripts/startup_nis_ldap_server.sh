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

if [ $# -eq 0 ]; then
	SRCDIR="."
else
	SRCDIR=$1; shift
fi
if [ $# -eq 1 ]; then
	BDB2=$1; shift
fi

. $SRCDIR/scripts/defines.sh $SRCDIR $BDB2

# Sample NIS database in LDIF format
NIS_LDIF=$SRCDIR/data/nis_sample.ldif

# Sample configuration file for your LDAP server
if test "$BACKEND" = "bdb2" ; then
	NIS_CONF=$DATADIR/slapd-bdb2-nis-master.conf
else
	NIS_CONF=$DATADIR/slapd-nis-master.conf
fi

echo "Cleaning up in $DBDIR..."

rm -f $DBDIR/[!C]*

echo "Running slapadd to build slapd database..."
$SLAPADD -f $NIS_CONF -l $NIS_LDIF
RC=$?
if [ $RC != 0 ]; then
	echo "slapadd failed!"
	exit $RC
fi

echo "Starting slapd on TCP/IP port $PORT..."
$SLAPD -f $NIS_CONF -p $PORT $TIMING > $MASTERLOG 2>&1 &
PID=$!

echo ">>>>> LDAP server with NIS schema is up! PID=$PID"


exit 0
