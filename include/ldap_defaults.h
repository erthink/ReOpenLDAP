/* $ReOpenLDAP$ */
/* Copyright 1992-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
 * All rights reserved.
 *
 * This file is part of ReOpenLDAP.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/*
 * This file controls defaults for ReOpenLDAP package.
 * You probably do not need to edit the defaults provided by this file.
 */

#ifndef _LDAP_DEFAULTS_H
#define _LDAP_DEFAULTS_H

#include <ldap_dirs.h>

#define LDAP_CONF_FILE LDAP_SYSCONFDIR LDAP_DIRSEP "ldap.conf"
#define LDAP_USERRC_FILE "ldaprc"
#define LDAP_ENV_PREFIX "LDAP"
#define SASL_CONFIGPATH LDAP_SYSCONFDIR LDAP_DIRSEP "sasl2"

#ifndef LDAPI_SOCK
/* default ldapi:// socket */
#define LDAPI_SOCK LDAP_RUNDIR LDAP_DIRSEP "slapd" LDAP_DIRSEP "ldapi"
#endif

/*
 * SLAPD DEFINITIONS
 */
/* location of the default slapd config file */
#define SLAPD_DEFAULT_CONFIGFILE LDAP_SYSCONFDIR LDAP_DIRSEP "slapd.conf"
#define SLAPD_DEFAULT_CONFIGDIR LDAP_SYSCONFDIR LDAP_DIRSEP "slapd.d"
#define SLAPD_DEFAULT_DB_DIR LDAP_VARDIR LDAP_DIRSEP "reopenldap-data"
#define SLAPD_DEFAULT_DB_MODE 0600
#define SLAPD_DEFAULT_UCDATA LDAP_DATADIR LDAP_DIRSEP "ucdata"
/* default max deref depth for aliases */
#define SLAPD_DEFAULT_MAXDEREFDEPTH 15
/* default sizelimit on number of entries from a search */
#define SLAPD_DEFAULT_SIZELIMIT 500
/* default timelimit to spend on a search */
#define SLAPD_DEFAULT_TIMELIMIT 3600

/* the following DNs must be normalized! */
/* dn of the default subschema subentry */
#define SLAPD_SCHEMA_DN "cn=Subschema"
/* dn of the default "monitor" subentry */
#define SLAPD_MONITOR_DN "cn=Monitor"

#ifndef SLAPD_BIGLOCK_TRACELATENCY
#define SLAPD_BIGLOCK_TRACELATENCY 0
#endif

#endif /* _LDAP_DEFAULTS_H */
