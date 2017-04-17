FROM debian:jessie
MAINTAINER Pavel Tsaregorodtsev <ixpict@gmail.com>

RUN useradd -m -d /var/lib/ldap -s /bin/false ldap

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -qq && apt-get install -yqq --no-install-recommends build-essential\
    git automake uuid-dev bc libelf-dev pkg-config libssl-dev libsasl2-dev\
    groff file libltdl-dev libltdl7 libmagic1 libtool libkrb5-dev man ca-certificates &&\
    mkdir /opt/reopenldap &&\
    git clone https://github.com/ReOpen/ReOpenLDAP.git /opt/reopenldap.source &&\
    cd /opt/reopenldap.source && ./configure --prefix /opt/reopenldap &&\
    make -j $(nproc) && make install && cd /opt/reopenldap && rm -rf /opt/reopenldap.source &&\
    apt-get remove -qq -y --purge automake build-essential libssl-dev libsasl2-dev git \
    groff imagemagick cpp gcc-4.9 cpp-4.9 libtool libmagic1 &&\
    apt-get autoremove -y -qq --purge && apt-get clean && chown ldap.ldap -R /opt/reopenldap

ENV MANPATH /opt/reopenldap/share/man:/usr/share/man

RUN mandb -u

ENV PATH /opt/reopenldap/bin:/opt/reopenldap/sbin:$PATH

WORKDIR /opt/reopenldap

RUN mkdir -p /opt/reopenldap/var/run /var/lib/reopenldap /etc/reopenldap &&\
    chown ldap.ldap -R /opt/reopenldap/var /var/lib/reopenldap /etc/reopenldap

COPY slapd.sample.conf /opt/reopenldap/etc/slapd.conf

COPY entrypoint.sh /opt/reopenldap/

EXPOSE 389

VOLUME ["/var/lib/reopenldap"]
VOLUME ["/etc/reopenldap"]

ENTRYPOINT ["./entrypoint.sh"]