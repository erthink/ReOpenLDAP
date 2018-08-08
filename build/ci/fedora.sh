#!/bin/bash

export EXTRA_CFLAGS="-Wall -Werror -g"

# tcp_wrappers-devel for '--enable-wrappers' was dropped in Fedora28
TCPWRAPPERS_PACKAGE="`yum search -q tcp_wrappers-devel | grep tcp_wrappers-devel`"
echo -n "tcp-wrappers-devel package: "
[ -n "$TCPWRAPPERS_PACKAGE" ] && echo $TCPWRAPPERS_PACKAGE || echo not-available

# Oracle don't provide ndbclient-devel for Fedora
NDBCLIENT_PACKAGE="`yum search -q ndbclient-devel | grep ndbclient-devel`"
echo -n "ndb-client-devel package: "
[ -n "$NDBCLIENT_PACKAGE" ] && echo $NDBCLIENT_PACKAGE || echo not-available

sudo yum install -y \
		@development-tools pkg-config groff-base autoconf automake libtool-ltdl-devel \
		libuuid-devel elfutils-libelf-devel krb5-devel heimdal-devel cracklib-devel \
		cyrus-sasl-devel perl-devel perl-ExtUtils-Embed binutils-devel openssl-devel \
		gnutls-devel unixODBC-devel libdb-devel gdb $TCPWRAPPERS_PACKAGE $NDBCLIENT_PACKAGE \
		libsodium-devel wiredtiger-devel \
	|| exit 1

[ -x ./configure ] || ./bootstrap.sh --dont-cleanup ] \
	|| exit 2

./configure CFLAGS=-Ofast --prefix=${HOME}/_install_mod \
		--enable-contrib --enable-experimental \
		--enable-backends --enable-overlays \
		--enable-check=always --enable-hipagut=always --enable-debug --disable-syslog \
		--disable-maintainer-mode \
		--enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi --enable-rlookups \
		`[ -n "$TCPWRAPPERS_PACKAGE" ] && echo --enable-wrappers || echo --disable-wrappers` \
		`[ -n "$NDBCLIENT_PACKAGE" ] && echo --enable-ndb || echo --disable-ndb` \
	|| exit 3

make && make install \
	|| exit 4

./configure CFLAGS=-O1 --prefix=${HOME}/_install_nomod \
		--enable-experimental --enable-backends --enable-overlays \
		--enable-check=always --enable-hipagut=always --enable-debug --disable-syslog \
		--disable-maintainer-mode \
		--enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi --enable-rlookups \
		`[ -n "$TCPWRAPPERS_PACKAGE" ] && echo --enable-wrappers || echo --disable-wrappers` \
		--disable-bdb --disable-hdb --disable-ndb --disable-sql --disable-modules \
	|| exit 5

make && make install \
	|| exit 6

rm -rf @* && make test \
	|| exit 7
