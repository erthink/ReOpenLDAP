%global systemctl_bin /usr/bin/systemctl
%define packaging_dir contrib/packaging/CentOS/7
%define owner %(git config --get remote.origin.url | sed -n -e 's!^git@github.com:\(.*\)\/.*$!\1!p')
%global commit0 %(git log -n 1 --pretty=format:"%H")
%global gittag0 v1.1.5
%global shortcommit0 %(c=%{commit0}; echo ${c:0:7})

Name:		reopenldap
Version:	1.1.5
Release:	%{?dist}
Summary:	The fork of OpenLDAP with a few new features (mostly for highload and multi-master clustering), additional bug fixing and code quality improvement.

Group:		System Environment/Daemons
License:	AGPLv3
URL:		https://github.com/%{owner}/ReOpenLDAP
Source0:	https://github.com/%{owner}/%{name}/archive/%{commit0}.tar.gz#/%{name}-%{shortcommit0}.tar.gz

BuildRequires:	cyrus-sasl-devel, krb5-devel, tcp_wrappers-devel, unixODBC-devel libuuid-devel elfutils-libelf-devel
BuildRequires:	glibc-devel, libtool, libtool-ltdl-devel, groff, perl, perl-devel, perl(ExtUtils::Embed)
BuildRequires:	openssl-devel, nss-devel
BuildRequires:	bc git
Requires:	rpm, coreutils, nss-tools
Conflicts:      openldap-servers, openldap-servers-sql, openldap-clients, openldap-devel

%description
The fork of OpenLDAP with a few new features (mostly for highload and multi-master clustering), additional bug fixing and code quality improvement.

%package devel
Summary: LDAP development libraries and header files
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}, cyrus-sasl-devel%{?_isa}
Provides: ldap-devel

%description devel
The openldap-devel package includes the development libraries and
header files needed for compiling applications that use LDAP
(Lightweight Directory Access Protocol) internals. LDAP is a set of
protocols for enabling directory services over the Internet. Install
this package only if you plan to develop or will need to compile
customized LDAP clients.

%package servers
Summary: LDAP server
License: AGPLv3
Requires: %{name}%{?_isa} = %{version}-%{release}, libdb-utils
Requires(pre): shadow-utils
Requires(post): systemd, systemd-sysv, chkconfig
Requires(preun): systemd
Requires(postun): systemd
BuildRequires: libdb-devel
BuildRequires: systemd-units
BuildRequires: cracklib-devel
Group: System Environment/Daemons
# migrationtools (slapadd functionality):
Provides: ldif2ldbm, ldap-server

%description servers
OpenLDAP is an open-source suite of LDAP (Lightweight Directory Access
Protocol) applications and development tools. LDAP is a set of
protocols for accessing directory services (usually phone book style
information, but other information is possible) over the Internet,
similar to the way DNS (Domain Name System) information is propagated
over the Internet. This package contains the slapd server and related files.

%package clients
Summary: LDAP client utilities
Requires: %{name}%{?_isa} = %{version}-%{release}
Group: Applications/Internet

%description clients
OpenLDAP is an open-source suite of LDAP (Lightweight Directory Access
Protocol) applications and development tools. LDAP is a set of
protocols for accessing directory services (usually phone book style
information, but other information is possible) over the Internet,
similar to the way DNS (Domain Name System) information is propagated
over the Internet. The openldap-clients package contains the client
programs needed for accessing and modifying OpenLDAP directories.

%prep
%autosetup -n %{name}-%{commit0}
#%setup -q -n %{name}-%{version}
# alternative include paths for Mozilla NSS
ln -s %{_includedir}/nss3 include/nss
ln -s %{_includedir}/nspr4 include/nspr

%build
%ifarch s390 s390x
  export CFLAGS="-fPIE"
%else
  export CFLAGS="-fpie"
%endif
export LDFLAGS="-pie"
# avoid stray dependencies (linker flag --as-needed)
# enable experimental support for LDAP over UDP (LDAP_CONNECTIONLESS)
export CFLAGS="${CFLAGS} %{optflags} -Wl,--as-needed -DLDAP_CONNECTIONLESS"
%configure \
   --sysconfdir=%{_sysconfdir}/openldap \
   --enable-deprecated \
   --enable-syslog \
   --enable-proctitle \
   --enable-ipv6 \
   --enable-local \
   \
   --enable-slapd \
   --enable-dynacl \
   --disable-aci \
   --enable-cleartext \
   --enable-crypt \
   --enable-lmpasswd=no \
   --enable-spasswd \
   --enable-modules \
   --enable-rewrite \
   --enable-rlookups \
   --enable-slapi \
   --disable-slp \
   --enable-wrappers \
   \
   --enable-backends=mod \
   --enable-mdb=yes \
   --disable-hdb \
   --disable-bdb \
   --disable-dnssrv \
   --enable-ldap=mod \
   --enable-meta=mod \
   --enable-monitor=yes \
   --disable-ndb \
   --enable-null=mod \
   --disable-passwd \
   --disable-perl \
   --disable-relay \
   --disable-shell \
   --disable-sock \
   --disable-sql \
   \
   --enable-overlays=mod \
   \
   --disable-static \
   --enable-shared \
   \
   --with-cyrus-sasl \
   --with-gssapi \
   --without-fetch \
   --with-pic \
   --with-gnu-ld \
   --with-tls=moznss \
   \
   --libexecdir=%{_libdir}
make %{?_smp_mflags}


%install
mkdir -p %{buildroot}%{_libdir}/
make install DESTDIR=%{buildroot} STRIP=""

# setup directories for TLS certificates
mkdir -p %{buildroot}%{_sysconfdir}/openldap/certs

# setup data and runtime directories
mkdir -p %{buildroot}%{_sharedstatedir}
mkdir -p %{buildroot}%{_localstatedir}
install -m 0700 -d %{buildroot}%{_sharedstatedir}/ldap
install -m 0755 -d %{buildroot}%{_localstatedir}/run/reopenldap

# setup autocreation of runtime directories on tmpfs
mkdir -p %{buildroot}%{_tmpfilesdir}/
install -m 0644 %{packaging_dir}/slapd.tmpfiles %{buildroot}%{_tmpfilesdir}/slapd.conf

# install default ldap.conf (customized)
rm -f %{buildroot}%{_sysconfdir}/openldap/ldap.conf
install -m 0644 %{packaging_dir}/ldap.conf %{buildroot}%{_sysconfdir}/openldap/ldap.conf

# Надо разобраться, что нам нужно из этих самых скриптов и кого из них запускать в %post.
## setup maintainance scripts
mkdir -p %{buildroot}%{_libexecdir}
install -m 0755 -d %{buildroot}%{_libexecdir}/reopenldap
install -m 0644 %{packaging_dir}/libexec-functions %{buildroot}%{_libexecdir}/reopenldap/functions
install -m 0755 %{packaging_dir}/libexec-check-config.sh %{buildroot}%{_libexecdir}/reopenldap/check-config.sh
install -m 0755 %{packaging_dir}/libexec-upgrade-db.sh %{buildroot}%{_libexecdir}/reopenldap/upgrade-db.sh

# remove build root from config files and manual pages
perl -pi -e "s|%{buildroot}||g" %{buildroot}%{_sysconfdir}/openldap/*.conf
perl -pi -e "s|%{buildroot}||g" %{buildroot}%{_mandir}/*/*.*

# install an init script for the servers
mkdir -p %{buildroot}%{_unitdir}
install -m 0644 %{packaging_dir}/slapd.service %{buildroot}%{_unitdir}/slapd.service

# install syconfig/slapd
mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
install -m 644 %{packaging_dir}/slapd.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/slapd

# ldapadd point to buildroot.
rm -f %{buildroot}%{_bindir}/ldapadd 
pushd %{buildroot}%{_bindir}
ln -s ldapmodify ldapadd
popd

# tweak permissions on the libraries to make sure they're correct
chmod 0755 %{buildroot}%{_libdir}/reopenldap/lib*.so*
chmod 0644 %{buildroot}%{_libdir}/reopenldap/lib*.*a

# slapd.conf(5) is obsoleted since 2.3, see slapd-config(5)
# new configuration will be generated in %%post
mkdir -p %{buildroot}%{_datadir}
install -m 0755 -d %{buildroot}%{_datadir}/reopenldap-servers
install -m 0644 %{packaging_dir}/slapd.ldif %{buildroot}%{_datadir}/reopenldap-servers/slapd.ldif
install -m 0644 %{packaging_dir}/db_config.example %{buildroot}%{_datadir}/reopenldap-servers/DB_CONFIG.example
install -m 0700 -d %{buildroot}%{_sysconfdir}/openldap/slapd.d
rm -f %{buildroot}%{_sysconfdir}/openldap/slapd.conf
rm -f %{buildroot}%{_sysconfdir}/openldap/slapd.ldif

# move doc files out of _sysconfdir
mv %{buildroot}%{_sysconfdir}/openldap/schema/README README.schema
#mv %{buildroot}%{_sysconfdir}/schema %{buildroot}%{_sysconfdir}/openldap

# remove files which we don't want packaged
rm -f %{buildroot}%{_libdir}/reopenldap/*.la
rm -f %{buildroot}%{_mandir}/man5/ldif.5*
rm -f %{buildroot}%{_mandir}/man5/ldap.conf.5*

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%pre servers
# create ldap user and group
getent group ldap &>/dev/null || groupadd -r -g 55 ldap
getent passwd ldap &>/dev/null || \
   useradd -r -g ldap -u 55 -d %{_sharedstatedir}/ldap -s /sbin/nologin -c "OpenLDAP server" ldap
if [ $1 -eq 2 ]; then
   # package upgrade
   old_version=$(rpm -q --qf=%%{version} reopenldap-servers)
   new_version=%{version}
   if [ "$old_version" != "$new_version" ]; then
       touch %{_sharedstatedir}/ldap/rpm_upgrade_reopenldap &>/dev/null
   fi
fi
exit 0

%post servers

/sbin/ldconfig -n %{_libdir}/reopenldap
%systemd_post slapd.service

# generate configuration if necessary
if [[ ! -f %{_sysconfdir}/openldap/slapd.d/cn=config.ldif && \
      ! -f %{_sysconfdir}/openldap/slapd.conf
   ]]; then
      # if there is no configuration available, generate one from the defaults
      mkdir -p %{_sysconfdir}/openldap/slapd.d/ &>/dev/null || :
      /usr/sbin/slapadd -F %{_sysconfdir}/openldap/slapd.d/ -n0 -l %{_datadir}/reopenldap-servers/slapd.ldif
      chown -R ldap:ldap %{_sysconfdir}/openldap/slapd.d/
      %{systemctl_bin} try-restart slapd.service &>/dev/null
fi
start_slapd=0

# upgrade the database
if [ -f %{_sharedstatedir}/ldap/rpm_upgrade_reopenldap ]; then
   if %{systemctl_bin} --quiet is-active slapd.service; then
       %{systemctl_bin} stop slapd.service
       start_slapd=1
   fi

   %{_libexecdir}/reopenldap/upgrade-db.sh &>/dev/null
   rm -f %{_sharedstatedir}/ldap/rpm_upgrade_reopenldap
fi

# restart after upgrade
if [ $1 -ge 1 ]; then
   if [ $start_slapd -eq 1 ]; then
       %{systemctl_bin} start slapd.service &>/dev/null || :
   else
       %{systemctl_bin} condrestart slapd.service &>/dev/null || :
   fi
fi
exit 0

%preun servers
%systemd_preun slapd.service

%postun servers
/sbin/ldconfig
%systemd_postun_with_restart slapd.service

%triggerin servers -- libdb

# libdb upgrade (setup for %%triggerun)
if [ $2 -eq 2 ]; then
   # we are interested in minor version changes (both versions of libdb are installed at this moment)
   if [ "$(rpm -q --qf="%%{version}\n" libdb | sed 's/\.[0-9]*$//' | sort -u | wc -l)" != "1" ]; then
       touch %{_sharedstatedir}/ldap/rpm_upgrade_libdb
   else
       rm -f %{_sharedstatedir}/ldap/rpm_upgrade_libdb
   fi
fi
exit 0
%triggerun servers -- libdb

# libdb upgrade (finish %%triggerin)
if [ -f %{_sharedstatedir}/ldap/rpm_upgrade_libdb ]; then
   if %{systemctl_bin} --quiet is-active slapd.service; then
       %{systemctl_bin} stop slapd.service
       start=1
   else
       start=0
   fi
   %{_libexecdir}/reopenldap/upgrade-db.sh &>/dev/null
   rm -f %{_sharedstatedir}/ldap/rpm_upgrade_libdb
   [ $start -eq 1 ] && %{systemctl_bin} start slapd.service &>/dev/null
fi
exit 0


%files
%doc ANNOUNCEMENT.OpenLDAP
%doc CHANGES.OpenLDAP
%doc ChangeLog
%doc COPYRIGHT
%doc LICENSE
%doc README
%doc README.md
%doc README.OpenLDAP
%dir %{_sysconfdir}/openldap
%dir %{_sysconfdir}/openldap/certs
%config(noreplace) %{_sysconfdir}/openldap/ldap.conf
%{_libdir}/reopenldap/libreldap*.so*
%{_libdir}/reopenldap/libreslapi*.so*
#%{_mandir}/man5/ldif.5*
#%{_mandir}/man5/ldap.conf.5*

%files servers
%doc contrib/slapd-modules/smbk5pwd/README
%doc README.schema
%config(noreplace) %dir %attr(0750,ldap,ldap) %{_sysconfdir}/openldap/slapd.d
%config(noreplace) %{_sysconfdir}/openldap/schema
%config(noreplace) %{_sysconfdir}/sysconfig/slapd
%config(noreplace) %{_tmpfilesdir}/slapd.conf
%dir %attr(0700,ldap,ldap) %{_sharedstatedir}/ldap
%dir %attr(-,ldap,ldap) %{_localstatedir}/run/reopenldap
%{_unitdir}/slapd.service
%{_bindir}/mdbx_*
%{_datadir}/reopenldap-servers/
%{_libdir}/reopenldap/accesslog*.so*
%{_libdir}/reopenldap/auditlog*.so*
%{_libdir}/reopenldap/back_ldap*.so*
%{_libdir}/reopenldap/back_meta*.so*
%{_libdir}/reopenldap/back_null*.so*
%{_libdir}/reopenldap/collect*.so*
%{_libdir}/reopenldap/constraint*.so*
%{_libdir}/reopenldap/dds*.so*
%{_libdir}/reopenldap/deref*.so*
%{_libdir}/reopenldap/dyngroup*.so*
%{_libdir}/reopenldap/dynlist*.so*
%{_libdir}/reopenldap/memberof*.so*
%{_libdir}/reopenldap/pcache*.so*
%{_libdir}/reopenldap/ppolicy*.so*
%{_libdir}/reopenldap/refint*.so*
%{_libdir}/reopenldap/retcode*.so*
%{_libdir}/reopenldap/rwm*.so*
%{_libdir}/reopenldap/seqmod*.so*
%{_libdir}/reopenldap/sssvlv*.so*
%{_libdir}/reopenldap/syncprov*.so*
%{_libdir}/reopenldap/translucent*.so*
%{_libdir}/reopenldap/unique*.so*
%{_libdir}/reopenldap/valsort*.so*
%dir %{_libexecdir}/reopenldap/
%{_libexecdir}/reopenldap/functions
%{_libexecdir}/reopenldap/check-config.sh
%{_libexecdir}/reopenldap/upgrade-db.sh
%{_sbindir}/slap*
%{_mandir}/man5/slap*
%{_mandir}/man8/*
%{_mandir}/ru/man5/*
%{_mandir}/ru/man8/*
# obsolete configuration
%ghost %config(noreplace,missingok) %attr(0640,ldap,ldap) %{_sysconfdir}/openldap/slapd.conf
%ghost %config(noreplace,missingok) %attr(0640,ldap,ldap) %{_sysconfdir}/openldap/slapd.conf.bak

%files clients
%{_bindir}/ldap*
%{_mandir}/man1/*
%{_mandir}/ru/man1/*

%files devel
%{_includedir}/reopenldap/*
%{_mandir}/man3/*


%changelog

