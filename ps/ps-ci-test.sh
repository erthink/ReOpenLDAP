#!/bin/bash

N=${1:-42}
HERE=$(readlink -f $(pwd))

function msg() {
	sed "s/>>>>>\s\+\(.*\)\$/##teamcity[progressMessage 'Round $n of $N, $mode: \1']/g"
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
	for m in 3 2 1 0; do
		unset REOPENLDAP_FORCE_IDDQD REOPENLDAP_FORCE_IDKFA
		if (($m & 1)); then export REOPENLDAP_FORCE_IDDQD=' iddqd'; fi
		if (($m & 2)); then export REOPENLDAP_FORCE_IDKFA=' idkfa'; fi
		mode=$(if (($m)); then echo "$REOPENLDAP_FORCE_IDDQD$REOPENLDAP_FORCE_IDKFA"; else echo "free"; fi)
		echo "##teamcity[blockOpened name='$mode']"

		echo "##teamcity[testStarted name='lmdb' captureStandardOutput='true']"
		if ! make -C libraries/liblmdb test; then
			echo "##teamcity[testFailed name='lmdb']"
		fi
		echo "##teamcity[testFinished name='lmdb']"

		NOEXIT="${TEAMCITY_PROCESS_FLOW_ID}" make test 2>&1 | tee ci-test-$n.log | $filter
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			killall -9 slapd 2>/dev/null
			echo "##teamcity[buildProblem description='Test failed']"
			exit 1
		fi
		echo "##teamcity[blockClosed name='$mode']"
		sleep 1
	done
	echo "##teamcity[blockClosed name='Round $n of $N']"
done

echo "##teamcity[buildStatus text='Tests passed']"
exit 0
