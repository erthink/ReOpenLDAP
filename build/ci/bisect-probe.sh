#!/bin/bash

function bailout() {
	echo "BISECT-PROBE: $*"
	exit 125
}


function probe() {
	echo "******************************************************************************************"
	#rm -rf @dumps
	if [ -s .git/bisect-testno ]; then
		sed -i -e "s|/test\*|/test$(cat .git/bisect-testno)\*|g" tests/scripts/all || bailout "sed-4-select-test"
		rm -rf tests/data/regressions || bailout "rm-4-select-test"
	fi

	if make test; then
		echo "******************************************************************************************"
		echo "*** OK"
		git checkout .
	else
		echo "******************************************************************************************"
		echo "*** FAIL ($counter of $N)"
		git checkout .
		echo "******************************************************************************************"
		exit 1
	fi
	echo "******************************************************************************************"
}

N=${1:-5}
shift

[ -x ./configure ] || bailout "no-configure"

if [ $# -eq 0 ]; then
	.git/build.sh --no-lto --insrc --no-dynamic || bailout "build failed"
else
	git clean -f -x -d -e tests/testrun || bailout "git-clean failed"
	git submodule foreach --recursive git clean -x -f -d || bailout "git-submoduleclean failed"
	./configure "$@" && make -j4 || bailout "configure/make failed"
fi

last=unknown
counter=0
while [ $counter -lt $N ]; do
	counter=$(($counter + 1))
	echo "*** repeat-probe $counter of $N"
	REOPENLDAP_FORCE_IDKFA=1 probe
done
echo "******************************************************************************************"
sleep 7
exit 0
