#!/bin/bash

#autoscan && libtoolize --force --automake --copy && aclocal -I build && autoheader && autoconf && automake --add-missing --copy

failure() {
        echo "Oops, $* failed ;(" >&2
        exit 1
}

git clean -x -f -d -e ./ps -e .ccache/ -e tests/testrun/ -e times.log || failure "cleanup"

IODBC=$([ -d /usr/include/iodbc ] && echo "-I/usr/include/iodbc")
CFLAGS="-Wall -g -Os -DLDAP_MEMORY_DEBUG -DUSE_VALGRIND $IODBC" CPPFLAGS="$CFLAGS" \
	./configure \
		--enable-backends --disable-ndb --enable-overlays \
		--enable-rewrite --enable-dynacl --enable-aci --enable-slapi \
		--disable-dependency-tracking \
	|| failure "configure"

make -j4 && make -j4 -C libraries/liblmdb || failure "build"

echo "DONE"
