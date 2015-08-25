#!/bin/bash

#autoscan && libtoolize --force --automake --copy && aclocal -I build && autoheader && autoconf && automake --add-missing --copy

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

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

export CFLAGS="-Wall -g -O2 -DLDAP_MEMORY_DEBUG -DUSE_VALGRIND $IODBC"
if [ -n "$(which gcc)" ] && gcc -v 2>&1 | grep -q -i lto \
	&& [ -n "$(which gcc-ar)" -a -n "$(which gcc-nm)" -a -n "$(which gcc-ranlib)" ]
then
	export CC=gcc AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib CFLAGS="$CFLAGS -flto=jobserver -fno-fat-lto-objects -fuse-linker-plugin -fwhole-program"
	echo "*** Link-Time Optimization (LTO) will be used" >&2
fi
export CXXFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS"

#======================================================================

if [ "$1" != "--do-not-clean" ]; then
	git clean -x -f -d -e ./ps -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"
fi

if [ ! -s Makefile ]; then
	./configure \
			--enable-debug --enable-backends --enable-overlays $NBD \
			--enable-rewrite --enable-dynacl --enable-aci --enable-slapi \
		|| failure "configure"

	export CFLAGS="-Werror $CFLAGS" CXXFLAGS="-Werror $CFLAGS"
	find ./ -name Makefile | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' || failure "prepare"

	if [ -z "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
		make depend \
			|| failure "depends"
	fi
fi

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
