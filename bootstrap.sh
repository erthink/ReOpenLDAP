#!/bin/bash

## Copyright 2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

rm -rf tests/testrun/*

if [ -z "$AUTORECONF" ]; then
	if [ -n "$(which autoreconf)" ] && autoreconf --version | grep -q 'autoreconf (GNU Autoconf) 2\.69'; then
		AUTORECONF=$(which autoreconf)
	elif [ -n "$(which autoreconf-2.69)" ]; then
		AUTORECONF=$(which autoreconf-2.69)
	elif [ -n "$(which autoreconf2.69)" ]; then
		AUTORECONF=$(which autoreconf2.69)
	else
		echo "no suitable autoreconf available" >&2; exit 2
	fi
fi

if git clean -x -f -d -e tests/testrun \
		&& $AUTORECONF --force --install --include=build \
		&& patch -p1 -i build/libltdl.patch; then
	echo "done"; exit 0
else
	echo "failed" >&2; exit 1
fi
