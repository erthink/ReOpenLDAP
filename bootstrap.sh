#!/bin/bash

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

function failure() {
	echo "Oops, $* failed ;(" >&2
	exit 2
}

function patch_apply() {
	patch -t -l -N -p1 -i $1
}

function patch_probe() {
	patch --dry-run -t -s -l -N -p1 -i $1 &>/dev/null
}

function patch_apply_optional() {
	if patch_probe $1; then patch_apply $1; else true; fi
}

###############################################################################

if [ "$1" = "--dont-cleanup" ]; then
	shift
else
	rm -rf tests/testrun/*
	git clean -x -f -d -e tests/testrun -e releasenotes.txt || failure "cleanup"
fi

if [ -z "$AUTORECONF" ]; then
	if [ -n "$(which autoreconf)" ] && autoreconf --version | grep -q 'autoreconf (GNU Autoconf) 2\.71'; then
		AUTORECONF=$(which autoreconf)
	elif [ -n "$(which autoreconf-2.71)" ]; then
		AUTORECONF=$(which autoreconf-2.69)
	elif [ -n "$(which autoreconf2.71)" ]; then
		AUTORECONF=$(which autoreconf2.69)
	else
		echo "no suitable autoreconf available" >&2; exit 2
	fi
fi

$AUTORECONF --force --install --include=build || failure "autoreconf"

###############################################################################

patch_apply build/libltdl-clang-diagnostic.patch || failure "patch libltdl (clang-diagnostic)"
patch_apply_optional build/libltdl-preopen-OPTIONAL.patch || failure "patch libltdl (preopen type-casting)"
if grep -q 'suppress_opt=yes' build/ltmain.sh; then
	(patch_probe build/ltmain-sh-A-libtool-suppress-opt.patch \
		&& patch_apply build/ltmain-sh-A-libtool-suppress-opt.patch) \
	|| (patch_probe build/ltmain-sh-B-libtool-suppress-opt.patch \
		&& patch_apply build/ltmain-sh-B-libtool-suppress-opt.patch) \
	|| failure "patch libtool (suppess-opt default value)"
fi

###############################################################################

echo "done"
exit 0
