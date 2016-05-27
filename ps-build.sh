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
	sleep 1
	#echo "##teamcity[progressEnd '$1']"
	echo "##teamcity[blockClosed name='$1']"
}

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

if [ -d .git ]; then
	git clean -x -f -d \
		$( \
			[ -d .ccache ] && echo " -e .ccache/"; \
			[ -d tests/testrun ] && echo " -e tests/testrun/"; \
			[ -f times.log ] && echo " -e times.log"; \
		) || failure "cleanup"
	git submodule foreach --recursive git clean -x -f -d || failure "cleanup-submodules"
else
	notice "No git repository, skip cleanup step"
fi

if [ -n "${PREFIX}" ]; then
	rm -rf "${PREFIX}"/* && mkdir -p "${PREFIX}" || failure "cleanup of '${PREFIX}/*'"
fi

step_finish "cleanup"
echo "======================================================================="
step_begin "configure"

if [ -s Makefile ]; then
	notice "Makefile present, skip configure"
else
	LDFLAGS="-Wl,--as-needed,-Bsymbolic,--gc-sections,-O,-zignore"
	CFLAGS="-Wall -ggdb3 -gstrict-dwarf -fvisibility=hidden -Os -include $(readlink -f $(dirname $0)/ps/glibc-225.h)"
	LIBS="-Wl,--no-as-needed,-lrt"

	if [ -n "$(which gcc)" ] && gcc -v 2>&1 | grep -q -i lto \
		&& [ -n "$(which gcc-ar)" -a -n "$(which gcc-nm)" -a -n "$(which gcc-ranlib)" ]
	then
	#	gcc -fpic -O3 -c ps/glibc-225.c -o ps/glibc-225.o
	#	CFLAGS+=" -D_LTO_BUG_WORKAROUND -save-temps"
	#	LDFLAGS+=",$(readlink -f ps/glibc-225.o)"
		sleep 0.5 && echo "*** Link-Time Optimization (LTO) will be used" >&2
		CFLAGS+=" -free -flto -fno-fat-lto-objects -flto-partition=none"
		export CC=gcc AR=gcc-ar NM=gcc-nm RANLIB=gcc-ranlib
	fi

	export CFLAGS LDFLAGS LIBS CXXFLAGS="$CFLAGS"

	./configure \
		--prefix=${PREFIX} --enable-dynacl --enable-ldap \
		--enable-overlays --disable-bdb --disable-hdb \
		--disable-dynamic --disable-shared --enable-static \
		--enable-debug --with-gnu-ld --without-cyrus-sasl \
		--disable-spasswd --disable-lmpasswd \
		--without-tls --disable-rwm --disable-relay > configure.log \
		|| failure "configure"

	find ./ -name Makefile -type f | xargs sed -e "s/STRIP = -s/STRIP =/g;s/\(VERSION= .\+\)/\1${BUILD_ID}/g" -i \
		|| failure "fixup build-id"

	if [ -e libraries/liblmdb/mdbx.h ]; then
		find ./ -name Makefile | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' \
			|| failure "fix-1"
	else
		find ./ -name Makefile | grep -v liblmdb | xargs -r sed -i 's/-Wall -g/-Wall -Werror -g/g' \
			|| failure "fix-2"
	fi

	sed -e 's/ -lrt/ -Wl,--no-as-needed,-lrt/g' -i libraries/liblmdb/Makefile

	if [ -z "${TEAMCITY_PROCESS_FLOW_ID}" ]; then
		make depend \
			|| failure "make-deps"
	fi
fi

PACKAGE="$(grep VERSION= Makefile | cut -d ' ' -f 2).${BUILD_NUMBER}"
echo "PACKAGE: $PACKAGE"

if [ -d .git ]; then
	if [ "$(git describe --abbrev=0 --tags)" != "$(git describe --exact-match --abbrev=0 --tags 2>/dev/null)" ]; then
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

step_finish "configure"
echo "======================================================================="
step_begin "build mdbx-tools"

mkdir -p ${PREFIX}/bin && \
(cd libraries/liblmdb && make -k all && \
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
		&& ([ -n "$2" ] && echo "##teamcity[publishArtifacts '$FILE']" \
			|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2) \
		|| failure "sources"
else
	notice "No git repository, unable to provide $FILE"
fi

FILE="reopenldap.$PACKAGE.tar.xz"
tar -caf $FILE --owner=root -C ${PREFIX}/.. reopenldap \
	&& sleep 1 && cat ${PREFIX}/changelog.txt >&2 && sleep 1 \
	&& ([ -n "$2" ] && echo "##teamcity[publishArtifacts '$FILE']" \
		|| echo "Skip publishing of artifact ($(ls -hs $FILE))" >&2) \
	|| failure "tar"

step_finish "packaging"
