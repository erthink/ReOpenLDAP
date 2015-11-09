#!/bin/bash

function probe() {
	echo "****************************************************************************************** "
	rm -rf @dumps
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

N=${1:-42}
shift

if [ $# -eq 0 ]; then
	.git/ci-build.sh || exit 125
else
	git clean -f -x -d -e ./ps -e tests/testrun || exit 125
	git submodule foreach --recursive git clean -x -f -d || exit 125
	[ -f ./configure ] && ./configure "$@" && make depend && make -j4 || exit 125
fi

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
