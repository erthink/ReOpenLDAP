/* $ReOpenLDAP$ */
/* Copyright 1999-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#ifndef SLAPD_COMMON_H
#define SLAPD_COMMON_H

#include <ac/localize.h>

typedef enum {
  TESTER_TESTER,
  TESTER_ADDEL,
  TESTER_BIND,
  TESTER_MODIFY,
  TESTER_MODRDN,
  TESTER_READ,
  TESTER_SEARCH,
  TESTER_LAST
} tester_t;

extern struct tester_conn_args *tester_init(const char *pname, tester_t ptype);
extern char *tester_uri(char *uri, char *host, int port);
extern void tester_error(const char *msg);
extern void tester_perror(const char *fname, const char *msg);
extern void tester_ldap_error(LDAP *ld, const char *fname, const char *msg);
extern int tester_ignore_str2errlist(const char *err);
extern int tester_ignore_err(int err);

struct tester_conn_args {
  char *uri, *host;
  int port;

  int outerloops;
  int loops;
  int retries;
  int delay;

  int chaserefs;

  int authmethod;

  char *binddn;
  struct berval pass;

#ifdef HAVE_CYRUS_SASL
  char *mech;
  char *realm;
  char *authz_id;
  char *authc_id;
  char *secprops;
  void *defaults;
#endif
};

#define TESTER_INIT_ONLY (1 << 0)
#define TESTER_COMMON_OPTS "CD:d:H:h:L:l:i:O:p:R:U:X:Y:r:t:w:x"
#define TESTER_COMMON_HELP                                                                                             \
  "[-C] "                                                                                                              \
  "[-D <dn> [-w <passwd>]] "                                                                                           \
  "[-d <level>] "                                                                                                      \
  "[-H uri | -h <host> [-p port]] "                                                                                    \
  "[-i <ignore>] "                                                                                                     \
  "[-l <loops>] "                                                                                                      \
  "[-L <outerloops>] "                                                                                                 \
  "[-r <maxretries>] "                                                                                                 \
  "[-t <delay>] "                                                                                                      \
  "[-O <SASL secprops>] "                                                                                              \
  "[-R <SASL realm>] "                                                                                                 \
  "[-U <SASL authcid> [-X <SASL authzid>]] "                                                                           \
  "[-x | -Y <SASL mech>] "

extern int tester_config_opt(struct tester_conn_args *config, char opt, char *optarg);
extern void tester_config_finish(struct tester_conn_args *config);
extern void tester_init_ld(LDAP **ldp, struct tester_conn_args *conf, int flags);

extern pid_t pid;

#endif /* SLAPD_COMMON_H */
