FROM debian:jessie
MAINTAINER Pavel Tsaregorodtsev <me@ixpict.com>

RUN useradd -m -d /var/lib/ldap -s /bin/false ldap

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -qq && apt-get install -yqq build-essential\
    git automake uuid-dev bc libelf-dev uuid pkg-config libssl-dev libsasl2-dev\
    groff file libltdl-dev libltdl7 libmagic1 libtool libkrb5-dev &&\
    mkdir /opt/reopenldap &&\
    git clone https://github.com/ReOpen/ReOpenLDAP.git /opt/reopenldap.source &&\
    cd /opt/reopenldap.source && ./configure --prefix /opt/reopenldap &&\
    make -j $(nproc) && make install && cd /opt/reopenldap && rm -rf /opt/reopenldap.source &&\
    apt-get remove -qq -y --purge automake build-essential libssl-dev libsasl2-dev git &&\
    apt-get autoremove -y -qq --purge && apt-get clean -y -qq && chown ldap.ldap -R /opt/reopenldap

WORKDIR /opt/reopenldap

VOLUME ["/var/lib/ldap"]

COPY slapd.test.conf /opt/reopenldap/etc/
COPY entrypoint.sh /opt/reopenldap/

EXPOSE 389

ENTRYPOINT ["./entrypoint.sh"]