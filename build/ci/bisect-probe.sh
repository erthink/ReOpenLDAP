#!/bin/bash

function probe() {
	echo "****************************************************************************************** "
	rm -rf @dumps
	patch -p1 < .git/test-select.patch || exit 125
	if make check; then
		echo "*** OK"
		git checkout .
	else
		echo "*** FAIL"
		git checkout .
		exit 1
	fi
	echo "****************************************************************************************** "
}

N=${1:-1}
shift

if [ $# -eq 0 ]; then
	.git/ci-build.sh --do-not-clean --no-lto || exit 125
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
	REOPENLDAP_FORCE_IDKFA=1 probe
done
echo "****************************************************************************************** "
sleep 7
exit 0
