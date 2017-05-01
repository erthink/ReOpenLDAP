#!/bin/bash

export EXTRA_CFLAGS="-Wall -Werror -ggdb3 -gdwarf-4"

apt update -q

apt install -y -q \
		build-essential pkg-config groff-base autoconf automake \
		uuid-dev libelf-dev krb5-multidev heimdal-multidev libcrack2-dev \
		libsasl2-dev libperl-dev libtool libltdl-dev binutils-dev libssl-dev \
		libgnutls-dev libwrap0-dev unixodbc-dev libslp-dev libdb-dev bc gdb \
	|| exit 1

if [ -x ./configure ]; then
	./configure CFLAGS=-Ofast --prefix=${HOME}/_install --enable-check=always \
		--enable-hipagut=always --enable-debug --disable-syslog --enable-overlays \
		--disable-maintainer-mode --enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi \
		--enable-rlookups --enable-slp --enable-wrappers --enable-backends \
		--disable-bdb --disable-hdb --disable-ndb --disable-modules \
	|| exit 2
else
	./bootstrap.sh --dont-cleanup && \
	./configure CFLAGS=-Ofast --prefix=${HOME}/_install --enable-check=always \
		--enable-hipagut=always --enable-debug --disable-syslog --enable-overlays \
		--enable-maintainer-mode --enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi \
		--enable-rlookups --enable-slp --enable-wrappers --enable-backends \
		--disable-bdb --disable-hdb --disable-ndb --disable-modules \
	|| exit 2
fi

make && make install \
	|| exit 3

rm -rf @* && make test \
	|| exit 4
