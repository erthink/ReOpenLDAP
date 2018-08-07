#!/bin/bash

export EXTRA_CFLAGS="-Wall -Werror -g"

sudo apt update -q

# ndbclient-dev (MySQL Cluster Client for NDB backend) = https://dev.mysql.com/doc/mysql-apt-repo-quick-guide/en/
# i.e. add "deb http://repo.mysql.com/apt/ubuntu/ bionic mysql-cluster-7.6" into /etc/apt/sources

sudo apt install -y \
		build-essential pkg-config groff-base autoconf automake \
		uuid-dev libelf-dev krb5-multidev heimdal-multidev libcrack2-dev \
		libsasl2-dev libperl-dev libtool libltdl-dev binutils-dev libssl-dev \
		libgnutls28-dev libwrap0-dev unixodbc-dev libdb-dev gdb ndbclient-dev \
		libsodium-dev \
	|| exit 1

[ -x ./configure ] || ./bootstrap.sh --dont-cleanup ] \
	|| exit 2

./configure CFLAGS=-Ofast --prefix=${HOME}/_install_mod \
		--enable-contrib --enable-experimental \
		--enable-backends --enable-overlays \
		--enable-check=always --enable-hipagut=always --enable-debug --disable-syslog \
		--disable-maintainer-mode \
		--enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi --enable-rlookups --enable-wrappers \
	|| exit 3

make && make install \
	|| exit 4

./configure CFLAGS=-O1 --prefix=${HOME}/_install_nomod \
		--enable-experimental --enable-backends --enable-overlays \
		--enable-check=always --enable-hipagut=always --enable-debug --disable-syslog \
		--disable-maintainer-mode \
		--enable-dynacl --enable-aci --enable-crypt --enable-lmpasswd \
		--enable-spasswd --enable-slapi --enable-rlookups --enable-wrappers \
		--disable-bdb --disable-hdb --disable-ndb --disable-sql --disable-modules \
	|| exit 5

make && make install \
	|| exit 6

rm -rf @* && make test \
	|| exit 7
