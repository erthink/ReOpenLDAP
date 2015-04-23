#!/bin/bash

N=${1:-42}
HERE=$(readlink -f $(pwd))

function msg() {
	sed "s/>>>>>\s\+\(.*\)\$/##teamcity[progressMessage 'Round $n of $N, $REOPENLDAP_MODE: \1']/g"
}

function find_free_port {
	local begin=$(cut -f 1 /proc/sys/net/ipv4/ip_local_port_range)
	local end=$(cut -f 2 /proc/sys/net/ipv4/ip_local_port_range)
	local n=${1:-1}

	while true; do
		local l=$([ -x /usr/bin/rand ] && rand -M 65535 -s $(date +%N) || od -vAn -N2 -tu2 < /dev/urandom)
		if [ $l -le 1024 ]; then continue; fi
		if [ $l -ge $begin -a $l -lt $end ]; then continue; fi
		local h=$((l+n-1))
		if [ $h -gt 65535 ]; then continue; fi
		if [ $h -ge $begin -a $h -lt $end ]; then continue; fi
		local r=$l
		for p in $(seq $l $h); do
			if netstat -ant | sed -e '/^tcp/ !d' -e 's/^[^ ]* *[^ ]* *[^ ]* *.*[\.:]\([0-9]*\) .*$/\1/' | grep -q -w $p; then
				r=0;
				break;
			fi
		done
		if [ $r -ne 0 ]; then
			echo $r;
			return 0;
		fi
	done
}

if [ -z ${SLAPD_BASEPORT} ]; then
	export SLAPD_BASEPORT=$(find_free_port 8)
fi
echo "Using TCP-ports ${SLAPD_BASEPORT}-$((SLAPD_BASEPORT + 8))"

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

for TEST_ITER in $(seq 1 $N); do
	echo "##teamcity[blockOpened name='Round $TEST_ITER of $N']"
	for m in 3 2 1 0; do
		unset REOPENLDAP_FORCE_IDDQD REOPENLDAP_FORCE_IDKFA
		case $m in
		0)
			export REOPENLDAP_MODE='free'
			;;
		1)
			export REOPENLDAP_FORCE_IDDQD=1 REOPENLDAP_MODE='DQD'
			;;
		2)
			export REOPENLDAP_FORCE_IDKFA=1 REOPENLDAP_MODE='KFA'
			;;
		3)
			export REOPENLDAP_FORCE_IDDQD=1 REOPENLDAP_FORCE_IDKFA=1 REOPENLDAP_MODE='DQD-KFA'
			;;
		esac
		echo "##teamcity[blockOpened name='$REOPENLDAP_MODE']"

		export TEST_ITER
		echo "##teamcity[testStarted name='lmdb' captureStandardOutput='true']"
		if ! make -C libraries/liblmdb test; then
			echo "##teamcity[testFailed name='lmdb']"
		fi
		echo "##teamcity[testFinished name='lmdb']"

		NOEXIT="${NOEXIT:-${TEAMCITY_PROCESS_FLOW_ID}}" make test 2>&1 | tee ci-test-${TEST_ITER}.${REOPENLDAP_MODE}.log | $filter
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			killall -9 slapd 2>/dev/null
			echo "##teamcity[buildProblem description='Test(s) failed']"
			exit 1
		fi
		echo "##teamcity[blockClosed name='$REOPENLDAP_MODE']"
		sleep 1
	done
	echo "##teamcity[blockClosed name='Round $TEST_ITER of $N']"
done

echo "##teamcity[buildStatus text='Tests passed']"
exit 0
