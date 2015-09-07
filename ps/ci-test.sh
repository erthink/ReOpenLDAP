#!/bin/bash

N=${1:-42}
HERE=$(readlink -f $(pwd))
TMP=
ulimit -c unlimited
NPORTS=8

cleanup() {
	wait
	[ -n "${TMP}" ] && rm -rf ${TMP}
}

failure() {
	echo "Oops, $* failed ;(" >&2
	cleanup
	exit 1
}

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
		# LY: skip default ports to avoid conflicts with local 'make test'
		if [ $l -ge 9010 -a $h -lt 9017 ]; then continue; fi
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

function verify_ports {
	RANDOM=$$
	if [ -n "$(which nc)" ]; then
		for port in "$@"; do
			token="${RANDOM}-${RANDOM}-${RANDOM}-${RANDOM}-$$"
			(echo $token | timeout 1s nc -l 127.0.0.1 $port) &
			feed=$(sleep 0.1 && timeout 1s nc 127.0.0.1 $port)
			if [ "$feed" != "$token" ]; then
				echo "TCP-post $port is busy, try find again" >&2
				return 1
			fi
		done
	else
		echo "nc (netcat) not available, skip verification"
	fi
	echo "Using TCP-ports $@" >&2
	return 0
}

trap cleanup TERM INT QUIT HUP

if [ -n "${TEST_TEMP_DIR}" ]; then
	rm -rf tests/testrun || failure "rm tests/testrun"
	([ -d $TEST_TEMP_DIR ] || mkdir -p ${TEST_TEMP_DIR}) && ln -s ${TEST_TEMP_DIR} tests/testrun || failure "mkdir-link tests/testrun"
elif [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
	rm -rf tests/testrun || failure "rm tests/testrun"
	leaf="reopenldap-ci-test"
	if [ -d /ramfs ]; then
		TMP="/ramfs/${leaf}"
	elif [ -d /run/user/${EUID} ]; then
		TMP="/run/user/${EUID}/${leaf}"
	elif [ -d /dev/shm ]; then
		TMP="/dev/shm/${EUID}/${leaf}"
	else
		TMP="/tmp/${leaf}"
	fi

	echo "tests/testrun -> ${TMP}"
	if [ -d ${TMP} ]; then
		rm -rf ${TMP}/*
	else
		[ ! -e ${TMP} ] || rm -rf ${TMP} || failure "clean ${TMP}"
		mkdir ${TMP} || failure "mkdir ${TMP}"
	fi
	ln -s ${TMP} tests/testrun || failure "link tests/testrun"
else
	(mount | grep ${HERE}/tests/testrun \
		|| (mkdir -p ${HERE}/tests/testrun && sudo mount -t tmpfs RAM ${HERE}/tests/testrun)) \
		|| failure "mount tests/testrun"
fi

if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
	filter=msg
else
	filter=cat
fi

for n in $(seq 1 $N); do
	echo "##teamcity[blockOpened name='Round $n of $N']"
	for m in 0 1 2 3; do
		unset REOPENLDAP_FORCE_IDDQD REOPENLDAP_FORCE_IDKFA
		case $m in
		0)
			export REOPENLDAP_FORCE_IDDQD=1 REOPENLDAP_FORCE_IDKFA=1 REOPENLDAP_MODE='DQD-KFA'
			;;
		1)
			export REOPENLDAP_FORCE_IDKFA=1 REOPENLDAP_MODE='KFA'
			;;
		2)
			export REOPENLDAP_FORCE_IDDQD=1 REOPENLDAP_MODE='DQD'
			;;
		3)
			export REOPENLDAP_MODE='free'
			;;
		esac
		echo "##teamcity[blockOpened name='$REOPENLDAP_MODE']"

		export TEST_ITER=$(($n*4 - 4 + $m)) TEST_NOOK="@ci-test-${n}.${REOPENLDAP_MODE}"
		(rm -rf ${TEST_NOOK} && mkdir -p ${TEST_NOOK}) || echo "failed: mkdir -p '${TEST_NOOK}'" >&2

		while [ -z ${SLAPD_BASEPORT} ] || ! verify_ports $(seq ${SLAPD_BASEPORT} $((SLAPD_BASEPORT + NPORTS - 1))); do
			export SLAPD_BASEPORT=$(find_free_port ${NPORTS})
		done

		NOEXIT="${NOEXIT:-${TEAMCITY_PROCESS_FLOW_ID}}" ionice -c 3 make test 2>&1 | tee ${TEST_NOOK}/all.log | $filter
		RC=${PIPESTATUS[0]}
		grep ' failed for ' ${TEST_NOOK}/all.log >&2
		sleep 1

		if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
			echo "##teamcity[publishArtifacts '${TEST_NOOK} => ${TEST_NOOK}.tar.gz']"
			sleep 1
		fi

		if [ $RC -ne 0 ]; then
			pkill -SIGKILL -s 0 -u $EUID slapd
			echo "##teamcity[buildProblem description='Test(s) failed']"
			find ./@ci-test-* -name all.log | xargs -r grep ' completed OK for '
			find ./@ci-test-* -name all.log | xargs -r grep ' failed for ' >&2
			cleanup
			exit 1
		fi
		echo "##teamcity[blockClosed name='$REOPENLDAP_MODE']"
		sleep 1
	done
	echo "##teamcity[blockClosed name='Round $n of $N']"
done

echo "##teamcity[buildStatus text='Tests passed']"
find ./@ci-test-* -name all.log | xargs -r grep ' completed OK for '
find ./@ci-test-* -name all.log | xargs -r grep ' failed for ' >&2
cleanup
exit 0
