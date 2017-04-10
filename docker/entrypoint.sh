#!/bin/bash

exec /opt/reopenldap/sbin/slapd -4 -h ldap:/// -g ldap -u ldap -f /opt/reopenldap/etc/slapd.conf -d1