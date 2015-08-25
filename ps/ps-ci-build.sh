#!/bin/bash

#autoscan && libtoolize --force --automake --copy && aclocal -I build && autoheader && autoconf && automake --add-missing --copy

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

flag_debug=0
flag_check=1
flag_clean=1
flag_lto=1
flag_O=-O2
for arg in "$@"; do
	case "$arg" in
	--debug)
		flag_debug=1
		;;
	--no-debug)
		flag_debug=0
		;;
	--check)
		flag_check=1
		;;
	--no-check)
		flag_check=0
		;;
	--lto)
		flag_lto=1
		;;
	--no-lto)
		flag_lto=0
		;;
	--do-clean)
		flag_clean=1
		;;
	--do-not-clean)
		flag_clean=0
		;;
	--size)
		flag_O=-Os
		;;
	--speed)
		flag_O=-Ofast
		;;
	*)
		failure "unknown option '$arg'"
		;;
	esac
done

#======================================================================

NBD="--disable-ndb"
MYSQL_CLUSTER="$(find -L /opt /usr/local -maxdepth 2 -name 'mysql-cluster*' -type d | sort -r | head -1)"
if [ -n "${MYSQL_CLUSTER}" -a -x ${MYSQL_CLUSTER}/bin/mysql_config ]; then
	echo "MYSQL_CLUSTER	= ${MYSQL_CLUSTER}"
	PATH=${MYSQL_CLUSTER}/bin:$PATH
fi
MYSQL_CONFIG="$(which mysql_config)"
if [ -n "${MYSQL_CONFIG}" ]; then
	echo "MYSQL_CONFIG	= ${MYSQL_CONFIG}"
	MYSQL_NDB_API="$(${MYSQL_CONFIG} --include | sed 's|-I/|/|g')/storage/ndb/ndbapi/NdbApi.hpp"
	if [ -s "${MYSQL_NDB_API}" ]; then
		echo "MYSQL_NDB_API	= ${MYSQL_NDB_API}"
		MYSQL_NDB_RPATH="$(${MYSQL_CONFIG} --libs_r | sed -n 's|-L\(/\S\+\)\s.*$|\1|p')"
		echo "MYSQL_NDB_RPATH	= ${MYSQL_NDB_RPATH}"
		LDFLAGS="-Xlinker -rpath=${MYSQL_NDB_RPATH}"
		NBD="--enable-ndb"
	fi
fi

IODBC=$([ -d /usr/include/iodbc ] && echo "-I/usr/include/iodbc")

#======================================================================

CFLAGS="-Wall -g"
if [ $flag_debug -ne 0 ]; then
	CFLAGS+=" -Og"
else
	CFLAGS+=" ${flag_O}"
fi

if [ $flag_check -ne 0 ]; then
	CFLAGS+=" -DLDAP_MEMORY_CHECK -DLDAP_MEMORY_DEBUG -DUSE_VALGRIND"
fi

if [ $flag_lto -ne 0 -a -n "$(which gcc)" ] && gcc -v 2>&1 | grep -q -i lto \
	&& [ -n "$(which gcc-ar)" -a -n "$(which gcc-nm)" -a -n "$(which gcc-ranlib)" ]
then
	echo "*** Link-Time Optimization (LTO) will be used" >&2
	CFLAGS+=" -flto=jobserver -fno-fat-lto-objects -fuse-linker-plugin -fwhole-program"
	export CC=gcc AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib
fi

if [ -n "$IODBC" ]; then
	CFLAGS+=" $IODBC"
fi

export CFLAGS LDFLAGS CXXFLAGS="$CFLAGS"
echo "CFLAGS		= ${CFLAGS}"

#======================================================================

if [ $flag_clean -ne 0 ]; then
	git clean -x -f -d -e ./ps -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"
fi

if [ ! -s Makefile ]; then
	./configure \
			--enable-debug --enable-backends --enable-overlays $NBD \
			--enable-rewrite --enable-dynacl --enable-aci --enable-slapi \
		|| failure "configure"

	find ./ -name Makefile | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' || failure "prepare"

	if [ -z "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
		make depend \
			|| failure "depends"
	fi
fi

export CFLAGS="-Werror $CFLAGS" CXXFLAGS="-Werror $CXXFLAGS"
make -j4 && make -j4 -C libraries/liblmdb || failure "build"

for m in $(find contrib/slapd-modules -name Makefile -printf '%h\n'); do
	if [ -e $m/BROKEN ]; then
		echo "----------- EXTENTIONS: $m - expecting is BROKEN"
		make -C $m &>$m/build.log && failure "contrib-module '$m' is NOT broken as expected"
	else
		echo "----------- EXTENTIONS: $m - expecting is NOT broken"
		make -C $m || failure "contrib-module '$m' is BROKEN"
	fi
done

echo "DONE"
