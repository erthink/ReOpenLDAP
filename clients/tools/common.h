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
/* ACKNOWLEDGEMENTS:
 * This file was initially created by Hallvard B. Furuseth based (in
 * part) upon argument parsing code for individual tools located in
 * this directory.
 */

#ifndef _CLIENT_TOOLS_COMMON_H_
#define _CLIENT_TOOLS_COMMON_H_

#include <ac/localize.h>

LDAP_BEGIN_DECL

typedef enum tool_type_t {
  TOOL_SEARCH = 0x01U,
  TOOL_COMPARE = 0x02U,
  TOOL_ADD = 0x04U,
  TOOL_DELETE = 0x08U,
  TOOL_MODIFY = 0x10U,
  TOOL_MODRDN = 0x20U,

  TOOL_EXOP = 0x40U,

  TOOL_WHOAMI = TOOL_EXOP | 0x100U,
  TOOL_PASSWD = TOOL_EXOP | 0x200U,
  TOOL_VC = TOOL_EXOP | 0x400U,

  TOOL_WRITE = (TOOL_ADD | TOOL_DELETE | TOOL_MODIFY | TOOL_MODRDN),
  TOOL_READ = (TOOL_SEARCH | TOOL_COMPARE),

  TOOL_ALL = 0xFFU
} tool_type_t;

/* input-related vars */

/* misc. parameters */
extern tool_type_t tool_type;
extern int contoper;
extern int debug;
extern char *infile;
extern int dont;
extern int referrals;
extern int verbose;
extern int ldif;
extern ber_len_t ldif_wrap;
extern char *prog;

/* connection */
extern char *ldapuri;
extern char *ldaphost;
extern int ldapport;
extern int use_tls;
extern int protocol;
extern int version;

/* authc/authz */
extern int authmethod;
extern char *binddn;
extern int want_bindpw;
extern struct berval passwd;
extern char *pw_file;
#ifdef HAVE_CYRUS_SASL
extern unsigned sasl_flags;
extern char *sasl_realm;
extern char *sasl_authc_id;
extern char *sasl_authz_id;
extern char *sasl_mech;
extern char *sasl_secprops;
#endif

/* controls */
extern char *assertion;
extern char *authzid;
extern int manageDIT;
extern int manageDSAit;
extern int noop;
extern int ppolicy;
extern int preread, postread;
extern ber_int_t pr_morePagedResults;
extern struct berval pr_cookie;
#ifdef LDAP_CONTROL_X_CHAINING_BEHAVIOR
extern int chaining;
#endif /* LDAP_CONTROL_X_CHAINING_BEHAVIOR */
extern ber_int_t vlvPos;
extern ber_int_t vlvCount;
extern struct berval *vlvContext;

/* options */
extern struct timeval nettimeout;

/* Defined in common.c, set in main() */
extern const char _Version[];

/* Defined in main program */
extern const char options[];

void usage(void) __attribute__((noreturn));
int handle_private_option(int i);

/* Defined in common.c */
void tool_init(tool_type_t type);
void tool_common_usage(void);
void tool_args(int, char **);
LDAP *tool_conn_setup(int dont, void (*private_setup)(LDAP *));
void tool_bind(LDAP *);
void tool_unbind(LDAP *);
void tool_destroy(void);
void tool_exit(LDAP *ld, int status) __attribute__((noreturn));
void tool_server_controls(LDAP *, LDAPControl *, int);
int tool_check_abandon(LDAP *ld, int msgid);
void tool_perror(const char *func, int err, const char *extra,
                 const char *matched, const char *info, char **refs);
void tool_print_ctrls(LDAP *ld, LDAPControl **ctrls);
int tool_write_ldif(int type, char *name, char *value, ber_len_t vallen);
int tool_is_oid(const char *s);

LDAP_END_DECL

#endif /* _CLIENT_TOOLS_COMMON_H_ */
