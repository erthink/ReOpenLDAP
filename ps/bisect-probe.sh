#!/bin/bash

function probe() {
	echo "****************************************************************************************** "
	patch -p1 < .git/test-select.patch || exit 125
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

N=${1:-3}
shift

git clean -f -x -d -e tests/testrun || exit 125
[ -f ./configure ] && ./configure "$@" && make depend && make -j4 || exit 125

last=unknown
counter=0
while [ $counter -lt $N ]; do
	counter=$(($counter + 1))
	echo "*** repeat-probe $counter of $N"
	probe
done
echo "****************************************************************************************** "
sleep 10
exit 0
