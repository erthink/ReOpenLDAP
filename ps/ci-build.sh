#!/bin/bash

[ -n "$CIBUZZ_PID4" ] && echo $BASHPID > $CIBUZZ_PID4

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

flag_debug=0
flag_check=1
flag_clean=1
flag_lto=1
flag_O=-O2
flag_clang=0
flag_bdb=1
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
	--clang)
		flag_clang=1
		;;
	--with-bdb)
		flag_bdb=1
		;;
	--without-bdb)
		flag_bdb=0
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

if [ $flag_clang -ne 0 ]; then
	CC=clang
	LLVM_VERSION="$($CC --version | sed -n 's/.\+LLVM \([0-9]\.[0-9]\)\.[0-9].*/\1/p')"
	echo "LLVM_VERSION	= $LLVM_VERSION"
	LTO_PLUGIN="/usr/lib/llvm-${LLVM_VERSION}/lib/LLVMgold.so"
	echo "LTO_PLUGIN	= $LTO_PLUGIN"
	CFLAGS+=" -Wno-pointer-bool-conversion"
elif [ -n "$(which gcc)" ]; then
	CC=gcc
else
	CC=cc
fi

if [ $flag_debug -ne 0 ]; then
	if [ "$CC" = "gcc" ]; then
		CFLAGS+=" -Og"
	else
		CFLAGS+=" -O0"
	fi
else
	CFLAGS+=" ${flag_O}"
fi

if [ $flag_lto -ne 0 ]; then
	if [ "$CC" = "gcc" ] && gcc -v 2>&1 | grep -q -i lto \
	&& [ -n "$(which gcc-ar)" -a -n "$(which gcc-nm)" -a -n "$(which gcc-ranlib)" ]; then
		echo "*** GCC Link-Time Optimization (LTO) will be used" >&2
		CFLAGS+=" -flto=jobserver -fno-fat-lto-objects -fuse-linker-plugin -fwhole-program"
		export AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib
	elif [ "$CC" = "clang" -a -n "$LLVM_VERSION" -a -e "$LTO_PLUGIN" -a -n "$(which ld.gold)" ]; then
		echo "*** CLANG Link-Time Optimization (LTO) will be used" >&2
		HERE=$(readlink -f $(dirname $0))

		(echo "#!/bin/bash" \
		&& echo "firstarg=\${1};shift;exec /usr/bin/ar \"\${firstarg}\" --plugin $LTO_PLUGIN \"\${@}\"") > ${HERE}/ar \
		&& chmod a+x ${HERE}/ar || failure "create thunk-ar"

		(echo "#!/bin/bash" \
		&& echo "exec $(which clang) -flto -fuse-ld=gold \"\${@}\"") > ${HERE}/clang \
		&& chmod a+x ${HERE}/clang || failure "create thunk-clang"

		(echo "#!/bin/bash" \
		&& echo "exec $(which ld.gold) --plugin $LTO_PLUGIN \"\${@}\"") > ${HERE}/ld \
		&& chmod a+x ${HERE}/ld || failure "create thunk-ld"

		(echo "#!/bin/bash" \
		&& echo "exec $(which nm) --plugin $LTO_PLUGIN \"\${@}\"") > ${HERE}/nm \
		&& chmod a+x ${HERE}/nm || failure "create thunk-nm"

		(echo "#!/bin/bash" \
		&& echo "exec $(which ranlib) --plugin $LTO_PLUGIN \"\${@}\"") > ${HERE}/ranlib \
		&& chmod a+x ${HERE}/ranlib || failure "create thunk-ranlib"

		export PATH="$HERE:$PATH"
	fi
fi

if [ $flag_check -ne 0 ]; then
	CFLAGS+=" -DLDAP_MEMORY_CHECK -DLDAP_MEMORY_DEBUG -DUSE_VALGRIND"
fi

if [ -n "$IODBC" ]; then
	CFLAGS+=" $IODBC"
fi

export CC CFLAGS LDFLAGS CXXFLAGS="$CFLAGS"
echo "CFLAGS		= ${CFLAGS}"
echo "PATH		= ${PATH}"
echo "LD		= $(readlink -f $(which ld)) ${LDFLAGS}"

#======================================================================

if [ $flag_clean -ne 0 ]; then
	git clean -x -f -d -e ./ps -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"
fi

if [ ! -s Makefile ]; then
	# autoscan && libtoolize --force --automake --copy && aclocal -I build && autoheader && autoconf && automake --add-missing --copy
	./configure \
			--enable-debug --enable-backends --enable-overlays $NBD \
			--enable-rewrite --enable-dynacl --enable-aci --enable-slapi \
			$([ $flag_bdb -eq 0 ] && echo "--disable-bdb --disable-hdb") \
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
