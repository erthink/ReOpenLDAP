#!/bin/bash

N=${1:-42}
HERE=$(readlink -f $(pwd))

function msg() {
	sed "s/>>>>>\s\+\(.*\)\$/##teamcity[progressMessage 'Round $1 of $N: \1']/g"
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
	if ! make test; then
		killall -9 slapd 2>/dev/null
		exit 1
	fi | $filter $n
	echo "##teamcity[blockClosed name='Round $n of $N']"
	sleep 1
done
