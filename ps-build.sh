#!/bin/bash

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
	[ -z "${TEAMCITY_PROCESS_FLOW_ID}" ] \
		&& echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> $1"
	echo "##teamcity[blockOpened name='$1']"
	#echo "##teamcity[progressStart '$1']"
}

step_finish() {
	teamcity_sync
	#echo "##teamcity[progressEnd '$1']"
	echo "##teamcity[blockClosed name='$1']"
	[ -z "${TEAMCITY_PROCESS_FLOW_ID}" ] \
		&& echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< $1"
}

configure() {
	echo "CONFIGURE	= $*"
	./configure "$@" > configure.log
}

##############################################################################

flag_debug=0
flag_check=0
flag_clean=1
flag_lto=1
flag_O=-Os
flag_clang=0
flag_valgrind=0
flag_asan=0
flag_tsan=0
flag_hide=1
flag_dynamic=0
flag_publish=1
PREV_RELEASE=""
HERE=$(pwd)
SUBDIR=

flag_nodeps=0
if [ -n "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
	flag_nodeps=1
fi

[ -s build/BRANDING ] || failure "modern ./configure.ac required"
flag_dist=1

while grep -q '^--' <<< "$1"; do
	case "$1" in
	--dist)
		flag_dist=1
		;;
	--no-dist)
		flag_dist=0
		;;
	--deps)
		flag_nodeps=0
		;;
	--no-deps)
		flag_nodeps=1
		;;
	--prev-release)
		PREV_RELEASE="$2"
		shift ;;
	--hide)
		flag_hide=1
		;;
	--no-hide)
		flag_hide=0
		;;
	--dynamic)
		flag_dynamic=1
		;;
	--no-dynamic)
		flag_dynamic=0
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
	--valg)
		flag_valgrind=1
		flag_asan=0
		flag_tsan=0

		flag_lto=0
		flag_O=-Og
		;;
	--asan)
		flag_valgrind=0
		flag_asan=1
		flag_tsan=0
		;;
	--tsan)
		flag_valgrind=0
		flag_asan=0
		flag_tsan=1
		;;
	--publish)
		flag_publish=1
		;;
	--no-publish)
		flag_publish=0
		;;
	--*)
		failure "unknown option '$1'"
		;;
	*)
		break
		;;
	esac
	shift
done

CONFIGURE_ARGS=

##############################################################################

step_begin "prepare"

BUILD_NUMBER=${1:-$(date '+%y%m%d')}
echo "BUILD_NUMBER: $BUILD_NUMBER"

PREFIX="$(readlink -m ${2:-$(pwd)/install_prefix_as_the_second_parameter})/reopenldap"
echo "PREFIX: $PREFIX"

if [ -d $HERE/.git ]; then
	git fetch origin --prune --tags || [ -n "$(git tag)" ] || failure "git fetch"
	if [ -n "$PREV_RELEASE" ]; then
		 git describe --abbrev=0 --all "$PREV_RELEASE" > /dev/null \
			|| failure "invalid prev-release ref '$PREV_RELEASE'"
		 git tag | grep -v "^$PREV_RELEASE" | xargs -r git tag -d
	elif [ "$(git describe --abbrev=0 --tags)" != "$(git describe --exact-match --abbrev=0 --tags 2>/dev/null)" ]; then
		PREV_RELEASE="$(git describe --abbrev=0 --tags)"
	else
		PREV_RELEASE="$(git describe --abbrev=0 --tags HEAD~)"
	fi
	[ -n "$PREV_RELEASE" ] || failure "previous release?"
	echo "PREVIOUS RELEASE: $PREV_RELEASE"

	BUILD_ID=$(git describe --abbrev=15 --always --long --tags | sed -e "s/^.\+-\([0-9]\+-g[0-9a-f]\+\)\$/.${BUILD_NUMBER}-\1/" -e "s/^\([0-9a-f]\+\)\$/.${BUILD_NUMBER}-g\1/")$(git show -s --abbrev=15 --format=-t%t | head -n 1)
elif [ -n "$BUILD_VCS_NUMBER" ]; then
	notice "No git repository, using BUILD_VCS_NUMBER from Teamcity"
	BUILD_ID=".g${BUILD_VCS_NUMBER}"
else
	notice "No git repository, unable to provide BUILD_ID"
	BUILD_ID=".unknown"
fi
echo "BUILD_ID: $BUILD_ID"

PACKAGE="$(build/BRANDING --version)-$(build/BRANDING --stamp)"
echo "PACKAGE: $PACKAGE"

step_finish "prepare"
##############################################################################
step_begin "cleanup"

if [ $flag_clean -ne 0 ]; then
	if [ -d $HERE/.git ]; then
		git clean -x -f -d -q -e '*.pdf' \
			$( \
				[ -d .ccache ] && echo " -e .ccache/"; \
				[ -d tests/testrun ] && echo " -e tests/testrun/"; \
				[ -f times.log ] && echo " -e times.log"; \
			) || failure "cleanup"
		git submodule foreach --recursive git clean -q -x -f -d || failure "cleanup-submodules"
	else
		notice "No git repository, skip cleanup step"
	fi
fi

if [ -n "${PREFIX}" ]; then
	rm -rf "${PREFIX}"/* && mkdir -p "${PREFIX}" || failure "cleanup of '${PREFIX}/*'"
fi

step_finish "cleanup"
##############################################################################
step_begin "distrib"

# LY: '.tgz' could be just changed to 'zip' or '.tar.gz', transparently
FILE="reopenldap.$PACKAGE-src.tgz"
if [ $flag_dist -ne 0 ]; then
	[ -s Makefile ] || CFLAGS=-std=gnu99 ./configure || failure "configure dist"
	make dist || failure "make dist"
	dist=$(ls *.tar.* | sed 's/^\(.\+\)\.tar\..\+$/\1/g')
	[ -n "$dist" ] && tar xaf *.tar.* && rm *.tar.* || failure "untar dist"
	tar caf $FILE $dist || failure "tar dist"
	SUBDIR=$dist
	[ -d "$SUBDIR" ] && cd "$SUBDIR" || failure "chdir dist"
else
	if [ -d $HERE/.git ]; then
		git archive --prefix=reopenldap.$PACKAGE-sources/ -o $FILE HEAD || failure "sources"
	else
		notice "No git repository, unable to provide $FILE"
	fi
fi

if [ -s $FILE ]; then
	[ -n "$2" ] && [ $flag_publish -ne 0 ] \
		&& echo "##teamcity[publishArtifacts '$FILE']" \
		|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2
fi

step_finish "distrib"
##############################################################################
step_begin "configure"

LIBMDBX_DIR=$([ -d libraries/liblmdb ] && echo "libraries/liblmdb" || echo "libraries/libmdbx")

if [ -s Makefile ]; then
	notice "Makefile present, skip configure"
else
	LDFLAGS="-Wl,--as-needed,-Bsymbolic,--gc-sections,-O,-zignore"
	CFLAGS="-std=gnu99 -Wall -ggdb3 -DPS_COMPAT_RHEL6=1"
	LIBS="-Wl,--no-as-needed,-lrt,--as-needed"

	if [ $flag_hide -ne 0 ]; then
		CFLAGS+=" -fvisibility=hidden"
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
		CFLAGS+=" -fvar-tracking-assignments -gstrict-dwarf"
	elif grep -q clang <<< "$CC"; then
		LLVM_VERSION="$($CC --version | sed -n 's/.\+ version \([0-9]\.[0-9]\)\.[0-9]-.*/\1/p')"
		echo "LLVM_VERSION	= $LLVM_VERSION"
		LTO_PLUGIN="/usr/lib/llvm-${LLVM_VERSION}/lib/LLVMgold.so"
		if [ ! -e $LTO_PLUGIN -a $CC = 'clang' ]; then
			LTO_PLUGIN=/usr/lib/LLVMgold.so
		fi
		echo "LTO_PLUGIN	= $LTO_PLUGIN"
		CFLAGS+=" -Wno-pointer-bool-conversion"
	fi

	if [ $flag_debug -ne 0 ]; then
		if grep -q gcc <<< "$CC" ; then
			CFLAGS+=" -Og"
		else
			CFLAGS+=" -O0"
		fi
	else
		CFLAGS+=" ${flag_O}"
	fi

	if [ $flag_lto -ne 0 ]; then
		if grep -q gcc <<< "$CC" && $CC -v 2>&1 | grep -q -i lto \
		&& [ -n "$(which gcc-ar$CC_VER_SUFF)" -a -n "$(which gcc-nm$CC_VER_SUFF)" -a -n "$(which gcc-ranlib$CC_VER_SUFF)" ]; then
			notice "*** GCC Link-Time Optimization (LTO) will be used"
			CFLAGS+=" -flto=jobserver -fno-fat-lto-objects -fuse-linker-plugin -fwhole-program"
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
		CFLAGS+=" -fstack-protector-all"
		CONFIGURE_ARGS+=" --enable-check=default --enable-hipagut=yes"
	else
		CFLAGS+=" -fstack-protector"
	fi

	if [ $flag_valgrind -ne 0 ]; then
		CONFIGURE_ARGS+=" --enable-valgrind"
	else
		CONFIGURE_ARGS+=" --disable-valgrind"
	fi

	if [ $flag_asan -ne 0 ]; then
		if grep -q clang <<< "$CC"; then
			CFLAGS+=" -fsanitize=address -D__SANITIZE_ADDRESS__=1 -pthread"
		elif $CC -v 2>&1 | grep -q -e 'gcc version [5-9]'; then
			CFLAGS+=" -fsanitize=address -D__SANITIZE_ADDRESS__=1 -pthread"
			if [ $flag_dynamic -eq 0 ]; then
				CFLAGS+=" -static-libasan"
			fi
		else
			notice "*** AddressSanitizer is unusable"
		fi
	fi

	if [ $flag_tsan -ne 0 ]; then
		if grep -q clang <<< "$CC"; then
			CFLAGS+=" -fsanitize=thread -D__SANITIZE_THREAD__=1"
		elif $CC -v 2>&1 | grep -q -e 'gcc version [5-9]'; then
			CFLAGS+=" -fsanitize=thread -D__SANITIZE_THREAD__=1"
			if [ $flag_dynamic -eq 0 ]; then
				CFLAGS+=" -static-libtsan"
			fi
		else
			notice "*** ThreadSanitizer is unusable"
		fi
	fi

	echo "CFLAGS		= ${CFLAGS}"
	echo "PATH		= ${PATH}"
	echo "LD		= $(readlink -f $(which ld)) ${LDFLAGS}"
	echo "LIBS		= ${LIBS}"
	echo "TOOLCHAIN	= $CC $CXX $AR $NM $RANLIB"
	export CC CXX CFLAGS LDFLAGS LIBS CXXFLAGS="$CFLAGS"

	if [ $flag_dynamic -ne 0 ]; then
		MOD=mod
		CONFIGURE_ARGS+=" --enable-shared --disable-static --enable-modules"
	else
		MOD=yes
		CONFIGURE_ARGS+=" --disable-shared --enable-static --disable-modules"
	fi

	configure \
		$(if [ $flag_nodeps -ne 0 ]; then echo "--disable-dependency-tracking"; fi) \
		--prefix=${PREFIX} --enable-overlays --disable-bdb --disable-hdb \
		--enable-debug --with-gnu-ld --without-cyrus-sasl \
		--disable-spasswd --disable-lmpasswd \
		--without-tls --disable-rwm --disable-relay \
		--enable-mdb=${MOD} \
		$CONFIGURE_ARGS || failure "configure"

	find ./ -name Makefile -type f | xargs sed -e "s/STRIP = -s/STRIP =/g;s/\(VERSION= .\+\)/\1${BUILD_ID}/g" -i \
		|| failure "fixup build-id"

	if [ -e ${LIBMDBX_DIR}/mdbx.h ]; then
		find ./ -name Makefile | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' \
			|| failure "fix-1"
	else
		find ./ -name Makefile | grep -v liblmdb | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' \
			|| failure "fix-2"
	fi

	sed -e 's/ -lrt/ -Wl,--no-as-needed,-lrt/g' -i ${LIBMDBX_DIR}/Makefile
fi

if [ -d $HERE/.git ]; then
	(git log --no-merges --dense --date=short --pretty=format:"%cd %s" "$PREV_RELEASE".. \
		| tr -s ' ' ' ' | grep -v ' ITS#[0-9]\{4\}$' | sort -r | uniq -u \
		&& /bin/echo -e "\nPackage version: $PACKAGE\nSource code tag: $(git describe --abbrev=15 --long --always --tags)" ) > ${PREFIX}/changelog.txt \
	|| failure "changelog"
else
	notice "No git repository, unable to provide changelog.txt" 2>&1 | tee ${PREFIX}/changelog.txt >&2
fi

[ -s releasenotes.txt ] && cp releasenotes.txt ${PREFIX}/

step_finish "configure"
##############################################################################
step_begin "build reopenldap"

make -k || failure "build"
[ ! -d tests/progs ] || make -C tests/progs || failure "build-tests"

step_finish "build reopenldap"
##############################################################################
step_begin "install"
make -k install && cd $HERE || failure "install"
step_finish "install"
##############################################################################
step_begin "sweep"

find ${PREFIX} -name '*.a' -o -name '*.la' | xargs -r rm \
	|| failure "sweep-1"
find ${PREFIX} -type d -empty | xargs -r rm -r \
	|| failure "sweep-2"
rm -rf ${PREFIX}/var ${PREFIX}/include && ln -s /var ${PREFIX}/ \
	|| failure "sweep-3"

step_finish "sweep"
##############################################################################
step_begin "packaging"

FILE="reopenldap.$PACKAGE.tar.xz"
tar -caf $FILE --owner=root -C ${PREFIX}/.. reopenldap \
	&& sleep 1 && cat ${PREFIX}/changelog.txt >&2 && sleep 1 \
	&& ([ -n "$2" ] && [ $flag_publish -ne 0 ] && echo "##teamcity[publishArtifacts '$FILE']" \
		|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2) \
	|| failure "tar"
step_finish "packaging"
exit 0
