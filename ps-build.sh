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
	./configure "$@" > configure.log
}

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

while grep -q '^--' <<< "$1"; do
	case "$1" in
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


##############################################################################

step_begin "prepare"

BUILD_NUMBER=${1:-$(date '+%y%m%d')}
echo "BUILD_NUMBER: $BUILD_NUMBER"

PREFIX="$(readlink -m ${2:-$(pwd)/install_prefix_as_the_second_parameter})/reopenldap"
echo "PREFIX: $PREFIX"

if [ -d .git ]; then
	git fetch origin --prune --tags || [ -n "$(git tag)" ] || failure "git fetch"
	BUILD_ID=$(git describe --abbrev=15 --always --long --tags | sed -e "s/^.\+-\([0-9]\+-g[0-9a-f]\+\)\$/.${BUILD_NUMBER}-\1/" -e "s/^\([0-9a-f]\+\)\$/.${BUILD_NUMBER}-g\1/")$(git show --abbrev=15 --format=-t%t | head -n 1)
elif [ -n "$BUILD_VCS_NUMBER" ]; then
	notice "No git repository, using BUILD_VCS_NUMBER from Teamcity"
	BUILD_ID=".g${BUILD_VCS_NUMBER}"
else
	notice "No git repository, unable to provide BUILD_ID"
	BUILD_ID=".unknown"
fi
echo "BUILD_ID: $BUILD_ID"

step_finish "prepare"
echo "======================================================================="
step_begin "cleanup"

if [ $flag_clean -ne 0 ]; then
	if [ -d .git ]; then
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
echo "======================================================================="
step_begin "configure"

LIBMDBX_DIR=$([ -d libraries/liblmdb ] && echo "libraries/liblmdb" || echo "libraries/libmdbx")

if [ -s Makefile ]; then
	notice "Makefile present, skip configure"
else
	LDFLAGS="-Wl,--as-needed,-Bsymbolic,--gc-sections,-O,-zignore"
	CFLAGS="-Wall -ggdb3 -DPS_COMPAT_RHEL6=1"
	LIBS="-Wl,--no-as-needed,-lrt"

	if [ $flag_hide -ne 0 ]; then
		CFLAGS+=" -fvisibility=hidden"
		if [ $flag_asan -ne 0 -o $flag_tsan -ne 0 ] && [ $flag_lto -ne 0 ]; then
			notice "*** LTO will be disabled for ASAN/TSN with --hide"
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
			CXX=clang++$([ -n "$CC_VER_SUFF" ] && echo "-$CC_VER_SUFF")
		elif grep -q gcc <<< "$CC"; then
			CXX=g++$([ -n "$CC_VER_SUFF" ] && echo "-$CC_VER_SUFF")
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
		elif grep -q clang <<< "$CC" && [ -n "$LLVM_VERSION" -a -e "$LTO_PLUGIN" -a -n "$(which ld.gold)" ]; then
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
		CFLAGS+=" -DLDAP_MEMORY_CHECK -DLDAP_MEMORY_DEBUG -fstack-protector-all"
	else
		CFLAGS+=" -fstack-protector"
	fi

	if [ $flag_valgrind -ne 0 ]; then
		CFLAGS+=" -DUSE_VALGRIND"
	fi

	if [ $flag_asan -ne 0 ]; then
		if grep -q clang <<< "$CC"; then
			CFLAGS+=" -fsanitize=address -D__SANITIZE_ADDRESS__=1 -pthread"
		elif $CC -v 2>&1 | grep -q -e 'gcc version [5-9]'; then
			CFLAGS+=" -fsanitize=address -D__SANITIZE_ADDRESS__=1 -static-libasan -pthread"
		else
			notice "*** AddressSanitizer is unusable"
		fi
	fi

	if [ $flag_tsan -ne 0 ]; then
		if grep -q clang <<< "$CC"; then
			CFLAGS+=" -fsanitize=thread -D__SANITIZE_THREAD__=1"
		elif $CC -v 2>&1 | grep -q -e 'gcc version [5-9]'; then
			CFLAGS+=" -fsanitize=thread -D__SANITIZE_THREAD__=1 -static-libtsan"
		else
			notice "*** ThreadSanitizer is unusable"
		fi
	fi

	echo "CFLAGS		= ${CFLAGS}"
	echo "PATH		= ${PATH}"
	echo "LD		= $(readlink -f $(which ld)) ${LDFLAGS}"
	echo "TOOLCHAIN	= $CC $CXX $AR $NM $RANLIB"
	export CC CXX CFLAGS LDFLAGS LIBS CXXFLAGS="$CFLAGS"

	if [ $flag_dynamic -ne 0 ]; then
		MOD=mod
		DYNAMIC="--enable-dynamic --enable-shared --enable-modules"
	else
		MOD=yes
		DYNAMIC="--disable-shared --enable-static --disable-dynamic"
	fi

	configure \
		--prefix=${PREFIX} ${DYNAMIC} \
		--enable-dynacl --enable-ldap \
		--enable-overlays --disable-bdb --disable-hdb \
		--enable-debug --with-gnu-ld --without-cyrus-sasl \
		--disable-spasswd --disable-lmpasswd \
		--without-tls --disable-rwm --disable-relay || failure "configure"

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

	if [ -z "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
		make depend \
			|| failure "make-deps"
	fi
fi

PACKAGE="$(grep VERSION= Makefile | cut -d ' ' -f 2)"
echo "PACKAGE: $PACKAGE"

if [ -d .git ]; then
	if [ -n "$PREV_RELEASE" ]; then
		 git describe --abbrev=0 --all "$PREV_RELEASE" > /dev/null \
			|| failure "invalid prev-release ref '$PREV_RELEASE'"
	elif [ "$(git describe --abbrev=0 --tags)" != "$(git describe --exact-match --abbrev=0 --tags 2>/dev/null)" ]; then
		PREV_RELEASE="$(git describe --abbrev=0 --tags)"
	else
		PREV_RELEASE="$(git describe --abbrev=0 --tags HEAD~)"
	fi
	[ -n "$PREV_RELEASE" ] || failure "previous release?"
	echo "PREVIOUS RELEASE: $PREV_RELEASE"

	(git log --no-merges --dense --date=short --pretty=format:"%ad %s" "$PREV_RELEASE".. \
		| tr -s ' ' ' ' | grep -v ' ITS#[0-9]\{4\}$' | sort -r | uniq -u \
		&& /bin/echo -e "\nPackage version: $PACKAGE\nSource code tag: $(git describe --abbrev=15 --long --always --tags)" ) > ${PREFIX}/changelog.txt \
	|| failure "changelog"
else
	notice "No git repository, unable to provide changelog.txt" 2>&1 | tee ${PREFIX}/changelog.txt >&2
fi

[ -s releasenotes.txt ] && cp releasenotes.txt ${PREFIX}/

step_finish "configure"
echo "======================================================================="
step_begin "build mdbx-tools"

mkdir -p ${PREFIX}/bin && \
(cd ${LIBMDBX_DIR} && make -k all && \
	(cp mdbx_chk mdbx_copy mdbx_stat -t ${PREFIX}/bin/ \
	|| cp mdb_chk mdb_copy mdb_stat -t ${PREFIX}/bin/) \
) \
	|| failure "build-1"

step_finish "build mdbx-tools"
echo "======================================================================="
step_begin "build reopenldap"

make -k \
	|| failure "build-2"

step_finish "build reopenldap"
echo "======================================================================="
step_begin "install"

make -k install \
	|| failure "install"

step_finish "install"
echo "======================================================================="
step_begin "sweep"

find ${PREFIX} -name '*.a' -o -name '*.la' | xargs -r rm \
	|| failure "sweep-1"
find ${PREFIX} -type d -empty | xargs -r rm -r \
	|| failure "sweep-2"
rm -r ${PREFIX}/var ${PREFIX}/include && ln -s /var ${PREFIX}/ \
	|| failure "sweep-3"

step_finish "sweep"
echo "======================================================================="

step_begin "packaging"

# LY: '.tgz' could be just changed to 'zip' or '.tar.gz', transparently
FILE="reopenldap.$PACKAGE-src.tgz"
if [ -d .git ]; then
	git archive --prefix=reopenldap.$PACKAGE-sources/ -o $FILE HEAD \
		&& ([ -n "$2" ] && [ $flag_publish -ne 0 ] && echo "##teamcity[publishArtifacts '$FILE']" \
			|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2) \
		|| failure "sources"
else
	notice "No git repository, unable to provide $FILE"
fi

FILE="reopenldap.$PACKAGE.tar.xz"
tar -caf $FILE --owner=root -C ${PREFIX}/.. reopenldap \
	&& sleep 1 && cat ${PREFIX}/changelog.txt >&2 && sleep 1 \
	&& ([ -n "$2" ] && [ $flag_publish -ne 0 ] && echo "##teamcity[publishArtifacts '$FILE']" \
		|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2) \
	|| failure "tar"
step_finish "packaging"
