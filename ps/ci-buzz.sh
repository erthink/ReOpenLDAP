#!/bin/bash

N=$1
shift
branch_list="$@"

if [ -z "$N" ]; then N=10; fi
if [ -z "$branch_list" ]; then branch_list="devel master"; fi

build="ps/ci-build.sh"
test="ps/ci-test.sh"
build_args="--size --without-bdb --do-not-clean"
test_args="111"

TOP=$(pwd)/@ci-buzz.pool
RAM=$TOP/ramfs
MAINPID=$$
function timestamp {
	date -u +'%F %T'
}

function cleanup {
	if [ $BASHPID = $MAINPID ]; then
		echo "Running cleanup:"
		for ((n=0; n < N; n++)); do
			for branch in $branch_list; do
				dir=$TOP/@$n.$branch
				pid=$([ -s $TOP/@$n.$branch/pid ] && cat $TOP/@$n.$branch/pid)
				if [ -n "$pid" ]; then
					echo "Terminating pid $pid ($n of $branch)..."
					pkill -P $pid && wait $pid 2>/dev/null
				fi
			done
		done
	else
		echo "Skip cleanup for non-main pid $BASHPID"
	fi

	pkill -P $BASHPID && sleep 1
	pkill -9 -P $BASHPID
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
	echo "$(timestamp) Preparing..." >$CIBUZZ_STATUS
	echo $BASHPID > $CIBUZZ_PID4
	root=$(git rev-parse --show-toplevel)
	([ -d src/.git ] || (rm -rf src && git clone --local --share -b $branch $root src)) \
		&& cd src || failure git-clone
	git fetch && git checkout -f $branch && git pull && git checkout origin/ps-build -- . \
		|| failure git-checkout
	echo "==============================================================================="
	echo "$(timestamp) Building..." > $CIBUZZ_STATUS
	nice -n $nice $build $build_args || failure "$here/$build $build_args"
	echo "==============================================================================="
	echo "$(timestamp) Testing..." > $CIBUZZ_STATUS
	NO_COLLECT_SUCCESS=yes TEST_TEMP_DIR=$(readlink -f ${here}/tmp) \
		setsid -w nice -n $nice $test $test_args
}

(mount | grep ${RAM} \
	|| (mkdir -p ${RAM} && sudo mount -t tmpfs RAM ${RAM})) \
	|| failure "mount ${RAM}"

started=$(date +%s)
order=0
for ((n=0; n < N; n++)); do
	for branch in $branch_list; do
		nice=$((5 + order * 2))
		delay=$((order * 199))
		echo "launching $n of $branch, with nice $nice..."
		dir=$TOP/@$n.$branch
		tmp=$(readlink -f ${RAM}/$n.$branch)
		mkdir -p $dir || failure "mkdir -p $dir"
		echo "delay $delay seconds..." >$dir/status
		rm -rf $tmp $dir/tmp && mkdir -p $tmp && ln -s $tmp $dir/tmp || failure "mkdir -p $tmp"
		( \
			( sleep $delay && cd $dir \
				&& doit $branch $nice \
			) >$dir/out.log 2>$dir/err.log </dev/null; \
			wait; echo "$(timestamp) *** exited" >$dir/status \
		) &
		order=$((order + 1))
	done
done

while sleep 5; do
	clear
	/usr/bin/printf "=== %s, running %.2f hours, %d job(s) left\n" \
		 "$(timestamp)" "$((($(date +%s)-started)/36)).0E-02" $(jobs -r | wc -l)
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
		exit 0
	fi
done
exit 1
