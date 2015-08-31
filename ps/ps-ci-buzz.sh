#!/bin/bash

msb_frefix=
failure() {
	echo "${msb_frefix}Oops, $* failed ;(" >&2
	exit 1
}

N=$1
shift
branch_list="$@"

if [ -z "$N" ]; then N=4; fi
if [ -z "$branch_list" ]; then branch_list="devel-2.4 devel-2.5 master"; fi

build="ps/ps-ci-build.sh"
test="ps/ps-ci-test.sh"
build_args="--size --do-not-clean"
test_args="111"

TOP=$(pwd)/@ci-buzz-pid.dbg
mkdir -p $TOP || failure "mkdir -p $TOP"
rm -rf $TOP/@* || failure  "rm -rf $TOP/@*"
[ ! -d $TOP/ramfs ] || rm -rf $TOP/ramfs/* || failure "$TOP/ramfs/*"

function doit {
	branch=$1
	nice=$2
	here=$(pwd)
	echo "branch $branch"
	echo "==============================================================================="
	echo "$(date --rfc-3339=seconds) Preparing..." >&2
	root=$(git rev-parse --show-toplevel)
	([ -d src/.git ] || (rm -rf src && git clone --local --share -b $branch $root src)) \
		&& cd src || failure git-clone
	git fetch && git checkout -f $branch && git pull && git checkout origin/ps-build -- . \
		|| failure git-checkout
	echo "==============================================================================="
	echo "$(date --rfc-3339=seconds) Building..." > .git/@ci-step.log
	nice -n $nice $build $build_args || failure "$here/$build $build_args"
	echo "==============================================================================="
	echo "$(date --rfc-3339=seconds) Testing..." > .git/@ci-step.log
	rm -rf @* || failure "rm -rf @*"
	NO_COLLECT_SUCCESS=yes TEST_TEMP_DIR=$(readlink -f ${here}/tmp) \
		setsid -w nice -n $nice $test $test_args
}

TEMPFS=${TOP}/ramfs
(mount | grep ${TEMPFS} \
	|| (mkdir -p ${TEMPFS} && sudo mount -t tmpfs RAM ${TEMPFS})) \
	|| failure "mount ${TEMPFS}"

nice=5
for ((n=0; n < N; n++)); do
	for branch in $branch_list; do
		echo "launching $n of $branch, with nice $nice..."
		dir="$TOP/@$n.$branch"
		tmp=$(readlink -f ${TEMPFS}/$n.$branch)
		mkdir -p "$dir" || failure "mkdir -p $dir"
		rm -rf $tmp $dir/tmp && mkdir -p $tmp && ln -s $tmp $dir/tmp || failure "mkdir -p $tmp"
		((cd $dir && msg_frefix="#$n of $branch | " doit $branch $nice >out.log 2>err.log) </dev/null; \
			 echo "#$n of $branch:	code $?, $(date '+%F.%H%M%S.%N')") &
		nice=$((nice + 1))
		sleep 1
	done
done

while true; do
	clear
	echo "=== $(date --rfc-3339=seconds) job(s) left $(jobs -r | wc -l)"
	for branch in $branch_list; do
		for ((n=0; n < N; n++)); do
			dir="$TOP/@$n.$branch"
			if [ -s $dir/src/.git/@ci-step.log ]; then
				echo "$branch/$n:	$(tail -n 1 $dir/src/.git/@ci-step.log)"
			elif [ -s $dir/err.log ]; then
				echo "$branch/$n:	$(tail -n 1 $dir/err.log)"
			fi
		done
	done

	echo "==="
	df -h $TOP $TOP/ramfs

	if [ -z "$(jobs -r)" ]; then
		break;
	fi
	sleep 5
done
