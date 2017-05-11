#!/bin/bash

REOPENLDAP_RUN_CMD=${REOPENLDAP_RUN_CMD:=-4 -h ldap:/// -g ldap -u ldap -f /opt/reopenldap/etc/slapd.conf -d1}
REOPENLDAP_SLAPD_PATH=${REOPENLDAP_SLAPD_PATH:=/opt/reopenldap/sbin/slapd}

echo "Trying run ReOpenLDAP: ${REOPENLDAP_SLAPD_PATH} ${REOPENLDAP_RUN_CMD}"

chown ldap.ldap -R /var/lib/reopenldap

exec $REOPENLDAP_SLAPD_PATH $REOPENLDAP_RUN_CMD