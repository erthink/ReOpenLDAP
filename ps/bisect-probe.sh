#!/bin/bash

function build() {
	git clean -f -x -d -e tests/testrun || exit $?
	[ -f ./configure ] && ./configure && make depend && make -j4 || exit 125
}

function probe() {
	echo "****************************************************************************************** "
	patch -p1 < .git/test-select.patch
	if make test; then
		echo "*** OK"
		git checkout .
	else
		echo "*** FAIL"
		git checkout .
		exit 1
	fi
	echo "****************************************************************************************** "
}

build

last=unknown
counter=0
while [ $counter -lt 100 ]; do
	counter=$(($counter + 1))
	echo "*** repeat-probe $counter of 3"
	probe
done
echo "****************************************************************************************** "
sleep 10
exit 0
