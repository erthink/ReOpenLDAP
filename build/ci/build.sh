#!/bin/bash

ncpu=$((grep processor /proc/cpuinfo || echo ?) | wc -l)
lalim=$((ncpu*4))
export LC_ALL=C

srcdir=$(pwd)

[ -n "$CIBUZZ_PID4" ] && echo $BASHPID > $CIBUZZ_PID4

teamcity_sync() {
	[ -z "${TEAMCITY_PROCESS_FLOW_ID}" ] || sleep 1
}

failure() {
	teamcity_sync
	echo "Oops, $* failed ;(" >&2
	teamcity_sync
	exit 1
}

notice() {
	teamcity_sync
	echo "$*" >&2
	teamcity_sync
}

step_begin() {
	echo "##teamcity[blockOpened name='$1']"
	#echo "##teamcity[progressStart '$1']"
}

step_finish() {
	teamcity_sync
	#echo "##teamcity[progressEnd '$1']"
	echo "##teamcity[blockClosed name='$1']"
}

configure() {
	echo "CONFIGURE	= $*"
	${srcdir}/configure "$@" > configure.log
}

flag_forceautoreconf=0
if [ ! -x ${srcdir}/configure ]; then
	notice "info: NOT saw ./configure, will bootstrap"
	flag_forceautoreconf=1
fi

flag_dist=0
if [ -e configure.ac ]; then
	notice "info: saw modern ./configure.ac"
	flag_insrc=0
else
	failure "modern ./configure.ac is required"
fi

flag_debug=0
flag_check=1
flag_clean=1
flag_lto=1
flag_O=-O2
flag_clang=0
flag_bdb=1
flag_mdbx=1
flag_wt=0
flag_ndb=1
flag_valgrind=0
flag_asan=0
flag_tsan=0
flag_hide=1
flag_dynamic=1
flag_exper=0
flag_contrib=1
flag_slapi=1
flag_tls=1
flag_ci=0

flag_nodeps=0
if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
	flag_nodeps=1
fi

CONFIGURE_ARGS="--enable-crypt --enable-spasswd"

for arg in "$@"; do
	case "$arg" in
	--no-tls | --with-tls=no)
		CONFIGURE_ARGS+=" --with-tls=no"
		flag_tls=0
		;;
	--ci)
		flag_ci=1
		flag_check=1
		flag_contrib=1
		flag_exper=1
		flag_slapi=1
		flag_dynamic=1
		flag_bdb=1
		flag_mdb=1
		flag_valgrind=1
		;;
	--with-tls=*)
		CONFIGURE_ARGS+=" $arg"
		if [ $arg != "--with-tls=gnutls" ]; then
			 CONFIGURE_ARGS+=" --enable-lmpasswd"
		fi
		flag_tls=0
		;;
	--contrib)
		flag_contrib=1
		flag_exper=1
		flag_slapi=1
		flag_dynamic=1
		;;
	--no-contrib)
		flag_contrib=0
		;;
	--slapi)
		flag_slapi=1
		;;
	--no-slapi)
		flag_slapi=0
		;;
	--exper*)
		flag_exper=1
		;;
	--no-exper*)
		flag_exper=0
		;;
	--deps)
		flag_nodeps=0
		;;
	--no-deps)
		flag_nodeps=1
		;;
	--dist)
		flag_dist=1
		flag_clean=1
		flag_forceautoreconf=1
		;;
	--no-dist)
		flag_dist=0
		;;
	--insrc | --in-source-build)
		flag_insrc=1
		;;
	--no-insrc | --out-source-build)
		flag_insrc=0
		;;
	--autoconf)
		flag_forceautoreconf=1
		;;
	--no-autoconf)
		flag_forceautoreconf=0
		flag_dist=0
		;;
	--hide)
		flag_hide=1
		;;
	--no-hide)
		flag_hide=0
		;;
	--dynamic)
		flag_dynamic=1
		flag_hide=1
		;;
	--no-dynamic)
		flag_dynamic=0
		flag_contrib=0
		;;
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
	--clean | --do-clean)
		flag_clean=1
		;;
	--no-clean | --do-not-clean)
		flag_clean=0
		flag_dist=0
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
	--with-mdbx)
		flag_mdbx=1
		;;
	--without-mdbx)
		flag_mdbx=0
		;;
	--with-bdb)
		flag_bdb=1
		;;
	--without-bdb)
		flag_bdb=0
		;;
	--with-wt)
		flag_wt=1
		;;
	--without-wt)
		flag_wt=0
		;;
	--with-ndb)
		flag_nbd=1
		;;
	--without-ndb)
		flag_ndb=0
		;;
	--valg)
		flag_valgrind=1
		flag_asan=0
		flag_tsan=0

		flag_ndb=0
		flag_wt=0
		flag_bdb=0
		flag_lto=0
		flag_O=-Og
		;;
	--asan)
		flag_valgrind=0
		flag_asan=1
		flag_tsan=0
		flag_bdb=0
		;;
	--tsan)
		flag_valgrind=0
		flag_asan=0
		flag_tsan=1
		flag_bdb=0
		;;
	*)
		failure "unknown option '$arg'"
		;;
	esac
done

#======================================================================

if [ $flag_tls -ne 0 ]; then
	CONFIGURE_ARGS+=" --with-tls=yes --enable-lmpasswd"
fi

if [ $flag_contrib -ne 0 ]; then
	CONFIGURE_ARGS+=" --enable-contrib"
else
	CONFIGURE_ARGS+=" --disable-contrib"
fi

if [ $flag_exper -ne 0 ]; then
	CONFIGURE_ARGS+=" --enable-experimental"
else
	CONFIGURE_ARGS+=" --disable-experimental"
fi

if [ $flag_slapi -ne 0 ]; then
	CONFIGURE_ARGS+=" --enable-slapi"
else
	CONFIGURE_ARGS+=" --disable-slapi"
fi

if [ $flag_dynamic -ne 0 ]; then
	MOD=mod
	CONFIGURE_ARGS+=" --enable-shared --disable-static --enable-modules"
else
	MOD=yes
	CONFIGURE_ARGS+=" --disable-shared --enable-static --disable-modules"
fi

CONFIGURE_ARGS+=" --enable-backends=${MOD}"

if [ $flag_ci -ne 0 ]; then
	CONFIGURE_ARGS+=" --enable-rlookups --enable-wrappers"
	CONFIGURE_ARGS+=" --enable-dynacl --enable-aci --enable-cleartext --enable-crypt --enable-lmpasswd --enable-spasswd --enable-rewrite"

## LY:	Вызывает ряд серьезных проблем на RHEL6/RHEL7, видимо дело в "росписи" памяти.
#	CONFIGURE_ARGS+=" --enable-slp"

fi

#======================================================================

NDB_CONFIG="--disable-ndb"
NDB_PATH_ADD=
NDB_LDFLAGS=
NDB_INCLUDES=
if [ $flag_ndb -ne 0 ]; then
	MYSQL_CLUSTER="$(find -L /opt /usr/local -maxdepth 2 -name 'mysql-cluster*' -type d | sort -r | head -1)"
	if [ -n "${MYSQL_CLUSTER}" -a -x ${MYSQL_CLUSTER}/bin/mysql_config ]; then
		echo "MYSQL_CLUSTER	= $MYSQL_CLUSTER"
		NDB_PATH_ADD=${MYSQL_CLUSTER}/bin:
	fi
	MYSQL_CONFIG="$(PATH=$NDB_PATH_ADD$PATH which mysql_config)"
	if [ -n "${MYSQL_CONFIG}" ]; then
		echo "MYSQL_CONFIG	= $MYSQL_CONFIG"
		MYSQL_NDB_API="$($MYSQL_CONFIG --include | sed 's|-I/|/|g')/storage/ndb/ndbapi/NdbApi.hpp"
		if [ -s "${MYSQL_NDB_API}" ]; then
			echo "MYSQL_NDB_API	= $MYSQL_NDB_API"
			NDB_RPATH="$($MYSQL_CONFIG --libs_r | sed -n 's|-L\(/\S\+\)\s.*$|\1|p')"
			echo "NDB_RPATH	= $NDB_RPATH"
			NDB_INCLUDES="$($MYSQL_CONFIG --include)"
			NDB_LDFLAGS="-Wl,-rpath,${NDB_RPATH}"
			NDB_CONFIG="--enable-ndb=${MOD}"
		fi
	fi
fi

IODBC_INCLUDES=$([ -d /usr/include/iodbc ] && echo "-I/usr/include/iodbc")

#======================================================================

CPPFLAGS=
EXTRA_CFLAGS="-Wall -ggdb3 -gdwarf-4"
if [ $flag_hide -ne 0 ]; then
	EXTRA_CFLAGS+=" -fvisibility=hidden"
	if [ $flag_asan -ne 0 -o $flag_tsan -ne 0 ] && [ $flag_lto -ne 0 ]; then
		notice "*** LTO will be disabled for ASAN/TSAN with --hide"
		flag_lto=0
	fi
fi

if [ -z "$CC" ]; then
	if [ $flag_clang -ne 0 ]; then
		CC=clang
	elif [ -n "$(which gcc)" ]; then
		CC=gcc
	else
		CC=cc
	fi
fi
CC_VER_SUFF=$(sed -nre 's/^(gcc|clang)-(.*)/-\2/p' <<< "$CC")

if [ -z "$CXX" ]; then
	if grep -q clang <<< "$CC"; then
		CXX=clang++$CC_VER_SUFF
	elif grep -q gcc <<< "$CC"; then
		CXX=g++$CC_VER_SUFF
	else
		CXX=c++
	fi
fi

if grep -q gcc <<< "$CC"; then
	EXTRA_CFLAGS+=" -fvar-tracking-assignments"
elif grep -q clang <<< "$CC"; then
	LLVM_VERSION="$($CC --version | sed -n 's/.\+ version \([0-9]\.[0-9]\)\.[0-9]-.*/\1/p')"
	echo "LLVM_VERSION	= $LLVM_VERSION"
	LTO_PLUGIN="/usr/lib/llvm-${LLVM_VERSION}/lib/LLVMgold.so"
	if [ ! -e $LTO_PLUGIN -a $CC = 'clang' ]; then
		LTO_PLUGIN=/usr/lib/LLVMgold.so
	fi
	echo "LTO_PLUGIN	= $LTO_PLUGIN"
	EXTRA_CFLAGS+=" -Wno-pointer-bool-conversion"
fi

if [ $flag_debug -ne 0 ]; then
	EXTRA_CFLAGS+=" -O0"
else
	EXTRA_CFLAGS+=" ${flag_O}"
fi

if [ $flag_lto -ne 0 ]; then
	if grep -q gcc <<< "$CC" && $CC -v 2>&1 | grep -q -i lto \
	&& [ -n "$(which gcc-ar$CC_VER_SUFF)" -a -n "$(which gcc-nm$CC_VER_SUFF)" -a -n "$(which gcc-ranlib$CC_VER_SUFF)" ]; then
		notice "*** GCC Link-Time Optimization (LTO) will be used"
		EXTRA_CFLAGS+=" -flto=jobserver -fno-fat-lto-objects -fuse-linker-plugin -fwhole-program"
		export AR=gcc-ar$CC_VER_SUFF NM=gcc-nm$CC_VER_SUFF RANLIB=gcc-ranlib$CC_VER_SUFF
	elif grep -q clang <<< "$CC" && [ -e "$LTO_PLUGIN" -a -n "$(which ld.gold)" ]; then
		notice "*** CLANG Link-Time Optimization (LTO) will be used"
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
	EXTRA_CFLAGS+=" -fstack-protector-all"
	CONFIGURE_ARGS+=" --enable-check=default --enable-hipagut=yes --enable-debug"
else
	EXTRA_CFLAGS+=" -fstack-protector"
	CONFIGURE_ARGS+=" --disable-debug"
fi

if [ $flag_valgrind -ne 0 ]; then
	CONFIGURE_ARGS+=" --enable-valgrind"
else
	CONFIGURE_ARGS+=" --disable-valgrind"
fi

if [ $flag_asan -ne 0 ]; then
	if grep -q clang <<< "$CC"; then
		EXTRA_CFLAGS+=" -fsanitize=address -D__SANITIZE_ADDRESS__=1 -pthread"
	elif $CC -v 2>&1 | grep -q -e 'gcc version [5-9]'; then
		EXTRA_CFLAGS+=" -fsanitize=address -D__SANITIZE_ADDRESS__=1 -pthread"
		if [ $flag_dynamic -eq 0 ]; then
			EXTRA_CFLAGS+=" -static-libasan"
		fi
	else
		notice "*** AddressSanitizer is unusable"
	fi
fi

if [ $flag_tsan -ne 0 ]; then
	if grep -q clang <<< "$CC"; then
		EXTRA_CFLAGS+=" -fsanitize=thread -D__SANITIZE_THREAD__=1"
	elif $CC -v 2>&1 | grep -q -e 'gcc version [5-9]'; then
		EXTRA_CFLAGS+=" -fsanitize=thread -D__SANITIZE_THREAD__=1"
		if [ $flag_dynamic -eq 0 ]; then
			EXTRA_CFLAGS+=" -static-libtsan"
		fi
	else
		notice "*** ThreadSanitizer is unusable"
	fi
fi

MDBX_NICK=mdb
export CC CXX EXTRA_CFLAGS CPPFLAGS LDFLAGS CXXFLAGS="$EXTRA_CFLAGS"
export LIBTOOL_SUPPRESS_DEFAULT=no
echo "EXTRA_CFLAGS	= ${EXTRA_CFLAGS}"
echo "CPPFLAGS	= ${CPPFLAGS}"
echo "PATH		= ${PATH}"
echo "LD		= $(readlink -f $(which ld)) ${LDFLAGS}"
echo "TOOLCHAIN	= $CC $CXX $AR $NM $RANLIB"

#======================================================================

if [ $flag_clean -ne 0 ]; then
	notice "info: cleaning"
	git clean -x -f -d -e .ccache/ -e tests/testrun/ -e times.log >/dev/null || failure "cleanup"
	git submodule foreach --recursive git clean -x -f -d >/dev/null || failure "cleanup-submodules"
fi

#======================================================================

if [ $flag_forceautoreconf -ne 0 -o ! -x ${srcdir}/configure ]; then
	if [ -s ${srcdir}/bootstrap.sh ]; then
		${srcdir}/bootstrap.sh
	elif [ -n "$(which autoreconf)" ] && autoreconf --version | grep -q 'autoreconf (GNU Autoconf) 2\.69'; then
		notice "info: use autoreconf"
		autoreconf --force --install --include=build || failure "autoreconf"
	elif [ -n "$(which autoreconf-2.69)" ]; then
		notice "info: use autoreconf-2.69"
		autoreconf-2.69 --force --install --include=build || failure "autoreconf-2.69"
	elif [ -n "$(which autoreconf2.69)" ]; then
		notice "info: use autoreconf2.69"
		autoreconf2.69 --force --install --include=build || failure "autoreconf2.69"
	else
		notice "warning: no autoreconf-2.69, skip autoreconf"
	fi
fi

#======================================================================

if [ $flag_dist -ne 0 ]; then
	notice "info: make dist"
	${srcdir}/configure || failure "configure dist"
	make dist || failure "make dist"
	dist=$(ls *.tar.* | sed 's/^\(.\+\)\.tar\..\+$/\1/g')
	tar xaf *.tar.* || failure "untar dist"
	[ -n "$dist" ] && [ -d "$dist" ] && cd "$dist" || failure "dir dist"
	srcdir=$(pwd)
fi

#======================================================================

if [ $flag_insrc -ne 0 ]; then
	notice "info: in-source build"
	build="$srcdir"
else
	notice "info: out-of-source build"
	build="$(pwd)/_build"
fi

if [ ! -s ${build}/Makefile ]; then
	mkdir -p ${build} && \
	( cd ${build} && ${srcdir}/configure \
			$(if [ $flag_nodeps -ne 0 ]; then echo "--disable-dependency-tracking"; fi) \
			--prefix=$(pwd)/@install_here \
			$CONFIGURE_ARGS --enable-overlays=${MOD} $NDB_CONFIG \
			--enable-rewrite --enable-dynacl --enable-aci \
			$(if [ $flag_mdbx -eq 0 ]; then echo "--disable-${MDBX_NICK}"; else echo "--enable-${MDBX_NICK}=${MOD}"; fi) \
			$(if [ $flag_bdb -eq 0 ]; then echo "--disable-bdb --disable-hdb"; else echo "--enable-bdb=${MOD} --enable-hdb=${MOD}"; fi) \
			$(if [ $flag_wt -eq 0 ]; then echo "--disable-wt"; else echo "--enable-wt=${MOD}"; fi) \
	) || failure "configure"
fi

make -C ${build} install || failure "install"

echo "DONE"
