#!/bin/bash

export EXTRA_CFLAGS="-Wall -Wno-error -g"

# tcp_wrappers-devel for '--enable-wrappers' was dropped in Fedora28
sudo yum install -y -q \
                @development-tools pkg-config groff-base autoconf automake libtool-ltdl-devel \
                libuuid-devel elfutils-libelf-devel krb5-devel heimdal-devel cracklib-devel \
                cyrus-sasl-devel perl-devel perl-ExtUtils-Embed binutils-devel openssl-devel \
                gnutls-devel unixODBC-devel libdb-devel gdb \
	|| exit 1

if [ -x ./configure ]; then
	./configure CFLAGS=-Ofast --prefix=${HOME}/_install --enable-check=always \
		--enable-hipagut=always --enable-debug --disable-syslog --enable-overlays \
		--disable-maintainer-mode --enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi \
                --enable-rlookups --enable-backends \
		--disable-bdb --disable-hdb --disable-ndb --disable-modules \
	|| exit 2
else
	./bootstrap.sh --dont-cleanup && \
	./configure CFLAGS=-Ofast --prefix=${HOME}/_install --enable-check=always \
		--enable-hipagut=always --enable-debug --disable-syslog --enable-overlays \
		--enable-maintainer-mode --enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi \
                --enable-rlookups --enable-backends \
		--disable-bdb --disable-hdb --disable-ndb --disable-modules \
	|| exit 2
fi

make && make install \
	|| exit 3

rm -rf @* && make test \
	|| exit 4
