#!/bin/bash

N=${1:-42}
HERE=$(readlink -f $(pwd))

function msg() {
	sed "s/>>>>>\s\+\(.*\)\$/##teamcity[progressMessage 'Round $n of $N: \1']/g"
}

if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
	filter=msg
	rm -rf tests/testrun || exit $?
	if [ -d /ramfs ]; then
		echo "tests/testrun -> /ramfs"
		ln -s /ramfs tests/testrun || exit $?
	else
		echo "tests/testrun -> /tmp"
		ln -s /tmp tests/testrun || exit $?
	fi
else
	filter=cat
	(mount | grep ${HERE}/tests/testrun \
		|| (mkdir -p ${HERE}/tests/testrun && sudo mount -t tmpfs RAM ${HERE}/tests/testrun)) \
		|| exit $?
fi

for n in $(seq 1 $N); do
	echo "##teamcity[blockOpened name='Round $n of $N']"
	NOEXIT="${TEAMCITY_PROCESS_FLOW_ID}" make test 2>&1 | tee ci-test-$n.log | $filter
	if [ ${PIPESTATUS[0]} -ne 0 ]; then
		killall -9 slapd 2>/dev/null
		echo "##teamcity[buildProblem description='Test failed']"
		exit 1
	fi
	echo "##teamcity[blockClosed name='Round $n of $N']"
	sleep 1
done

echo "##teamcity[buildStatus text='Tests passed']"
exit 0
