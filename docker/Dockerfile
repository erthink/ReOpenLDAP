FROM debian:jessie

RUN apt-get update -qq && apt-get install -yqq build-essential\
    git automake uuid-dev bc libelf-dev uuid pkg-config libssl-dev libsasl2-dev\
    groff file libltdl-dev libltdl7 libmagic1 libtool libkrb5-dev &&\
    mkdir /opt/reopenldap &&\
    git clone https://github.com/ReOpen/ReOpenLDAP.git /opt/reopenldap.source &&\
    cd /opt/reopenldap.source && ./configure --prefix /opt/reopenldap &&\
    make -j $(nproc) && make install && cd /opt/reopenldap && rm -rf /opt/reopenldap.source &&\
    apt-get remove -qq -y --purge automake build-essential libssl-dev libsasl2-dev git &&\
    apt-get autoremove -y -qq --purge && apt-get autoclean -y -qq

WORKDIR /opt/reopenldap


