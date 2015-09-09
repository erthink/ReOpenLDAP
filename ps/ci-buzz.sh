#!/bin/bash

N=$1
shift
branch_list="$@"

if [ -z "$N" ]; then N=4; fi
if [ -z "$branch_list" ]; then branch_list="devel-2.4 devel-2.5"; fi

build="ps/ci-build.sh"
test="ps/ci-test.sh"
build_args="--size --without-bdb --do-not-clean"
test_args="111"

TOP=$(pwd)/@ci-buzz.pool
RAM=$TOP/ramfs
MAIN=$$

function cleanup {
	if [ $$ = $MAIN ]; then
		for ((n=0; n < N; n++)); do
			for branch in $branch_list; do
				echo "launching $n of $branch, with nice $nice..."
				dir=$TOP/@$n.$branch
				pid=$([ -s $TOP/@$n.$branch/pid ] && echo $TOP/@$n.$branch/pid)
				if [ -n "$pid" ]; then
					echo "terminating pid $pid ($n of $branch)..."
					pkill -P $pid && wait $pid
				fi
			done
		done

		pkill -P $$ && sleep 1
		pkill -9 -P $$
	fi
}

trap cleanup TERM INT QUIT HUP

function failure {
	echo "Oops, $* failed ;(" >&2
	cleanup
	exit 1
}

mkdir -p $TOP || failure "mkdir -p $TOP"
rm -rf $TOP/@* || failure  "rm -rf $TOP/@*"
[ ! -d $RAM ] || rm -rf $RAM/* || failure "$RAM/*"

function doit {
	branch=$1
	nice=$2
	here=$(pwd)
	export CIBUZZ_STATUS=$here/status CIBUZZ_PID4=$here/pid
	echo "branch $branch"
	echo "==============================================================================="
	echo "$(date --rfc-3339=seconds) Preparing..." >$CIBUZZ_STATUS
	root=$(git rev-parse --show-toplevel)
	([ -d src/.git ] || (rm -rf src && git clone --local --share -b $branch $root src)) \
		&& cd src || failure git-clone
	git fetch && git checkout -f $branch && git pull && git checkout origin/ps-build -- . \
		|| failure git-checkout
	echo "==============================================================================="
	echo "$(date --rfc-3339=seconds) Building..." > $CIBUZZ_STATUS
	nice -n $nice $build $build_args || failure "$here/$build $build_args"
	echo "==============================================================================="
	echo "$(date --rfc-3339=seconds) Testing..." > $CIBUZZ_STATUS
	NO_COLLECT_SUCCESS=yes TEST_TEMP_DIR=$(readlink -f ${here}/tmp) \
		setsid -w nice -n $nice $test $test_args
}

(mount | grep ${RAM} \
	|| (mkdir -p ${RAM} && sudo mount -t tmpfs RAM ${RAM})) \
	|| failure "mount ${RAM}"

order=0
for ((n=0; n < N; n++)); do
	for branch in $branch_list; do
		echo "launching $n of $branch, with nice $nice..."
		dir=$TOP/@$n.$branch
		tmp=$(readlink -f ${RAM}/$n.$branch)
		mkdir -p $dir || failure "mkdir -p $dir"
		echo "launching..." >$dir/status
		rm -rf $tmp $dir/tmp && mkdir -p $tmp && ln -s $tmp $dir/tmp || failure "mkdir -p $tmp"
		( \
			( sleep $((order * 41)) && cd $dir \
				&& doit $branch $((5 + order * 2)) \
			) >$dir/out.log 2>$dir/err.log </dev/null; \
			wait; echo "$(date --rfc-3339=seconds) *** exited" >$dir/status \
		) &
		order=$((order + 1))
	done
done

while true; do
	clear
	echo "=== $(date --rfc-3339=seconds) job(s) left $(jobs -r | wc -l)"
	for branch in $branch_list; do
		for ((n=0; n < N; n++)); do
			dir="$TOP/@$n.$branch"
			echo "$branch/$n: $(tail -n 1 $dir/status) | $(tail -n 1 $dir/err.log)"
		done
	done

	echo "==="
	df -h $TOP $TOP/ramfs
	echo "==="
	vmstat -w; vmstat -w -a
	echo "==="
	uptime

	if [ -z "$(jobs -r)" ]; then
		break;
	fi
	sleep 5
done
