#!/bin/bash
rm -rf tests/testrun/*

if [ -z "$AUTORECONF"]; then
	if [ -n "$(which autoreconf)" ] && autoreconf --version | grep -q 'autoreconf (GNU Autoconf) 2\.69'; then
		AUTORECONF=$(which autoreconf)
	elif [ -n "$(which autoreconf-2.69)" ]; then
		AUTORECONF=$(which autoreconf-2.69)
	elif [ -n "$(which autoreconf2.69)" ]; then
		AUTORECONF=$(which autoreconf2.69)
	else
		echo "no suitable autoreconf available" >&2; exit 2
	fi
fi

if git clean -x -f -d -e tests/testrun \
		&& $AUTORECONF --force --install --include=build \
		&& patch -p1 -i build/libltdl.patch; then
	echo "done"; exit 0
else
	echo "failed" >&2; exit 1
fi
