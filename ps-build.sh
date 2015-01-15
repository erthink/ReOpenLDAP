#!/bin/bash

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

BUILD_NUMBER=${1:-build_number_as_the_first_parameter}
PREFIX=${2:-$(pwd)/install_prefix_as_the_second_parameter}/openldap

echo "BUILD_NUMBER: $BUILD_NUMBER"
echo "PREFIX: $PREFIX"

git clean -x -f -d -e .ccache/ || failure "git clean"
git fetch git://git.openldap.org/openldap.git --prune --tags || failure "git fetch"
BUILD_ID=$(git describe --abbrev=15 --always --long | sed "s/^.\+-\([0-9]\+-g[0-9a-f]\+\)\$/.${BUILD_NUMBER}-\1/")
echo "BUILD_ID: $BUILD_ID"

CFLAGS="-Wall -g -Os" CPPFLAGS="-Wall -g -Os" ./configure \
	--prefix=${PREFIX} --enable-dynacl --enable-ldap \
	--enable-overlays --disable-bdb --disable-hdb \
	--disable-dynamic --disable-shared --enable-static \
	--without-cyrus-sasl --disable-dependency-tracking \
	--disable-spasswd --disable-lmpasswd --disable-rewrite --disable-rwm --disable-relay \
	|| failure "configure"

PACKAGE="$(grep VERSION= Makefile | cut -d ' ' -f 2).${BUILD_NUMBER}"
echo "PACKAGE: $PACKAGE"

mkdir -p ${PREFIX}/bin \
	&& (git log --date=short --pretty=format:"%ad %s" $(git describe --always --abbrev=0 --tags).. \
		| sed 's/lmdb-backend/slapd/g;s/EXTENSION/[+]/g;s/BUGFIX/[-] /g;s/FEATURE/[+]/g;s/CHANGE/[!]/g;s/TRIVIA/[*]/g;s/ - / /g' \
		| tr -s ' ' ' ' | grep -v ' ITS#[0-9]\{4\}$' | sort -r | uniq -u \
	&& /bin/echo -e "\nPackage version: $PACKAGE\nSource code tag: $(git describe --abbrev=15 --long --always)" ) > ${PREFIX}/changelog.txt \
	|| failure "prepare-1"

find ./ -name Makefile -type f | xargs sed -e "s/STRIP = -s/STRIP =/g;s/\(VERSION= .\+\)/\1${BUILD_ID}/g" -i \
	|| failure "prepare-2"

(cd libraries/liblmdb && make -j4 -k && cp mdb_copy mdb_stat ${PREFIX}/bin/) \
	|| failure "build-1"

make -k -j4 install \
	|| failure "build-2"

find ${PREFIX} -name '*.a' -o -name '*.la' | xargs -r rm \
	|| failure "clean-1"
find ${PREFIX} -type d -empty | xargs -r rm -r \
	|| failure "clean-2"
rm -r ${PREFIX}/var ${PREFIX}/include && ln -s /var ${PREFIX}/ \
	|| failure "clean-3"

FILE="openldap.$PACKAGE.tar.xz"
tar -caf $FILE --owner=root -C ${PREFIX}/.. openldap \
	&& echo "##teamcity[publishArtifacts '$FILE']" \
	&& cat ${PREFIX}/changelog.txt >&2 \
	|| failure "tar"
