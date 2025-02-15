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

#include "reldap.h"

#include <stdio.h>

#include "ac/stdlib.h"
#include "ac/unistd.h"
#include "ac/string.h"
#include "ac/errno.h"

#include "ldap.h"

#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_pvt.h"
#include "slapd-common.h"

/* global vars */
pid_t pid;

/* static vars */
static char progname[BUFSIZ];
tester_t progtype;

/*
 * ignore_count[] is indexed by result code:
 * negative for OpenLDAP client-side errors, positive for protocol codes.
 */
#define TESTER_CLIENT_FIRST LDAP_REFERRAL_LIMIT_EXCEEDED /* negative */
#define TESTER_SERVER_LAST LDAP_OTHER
static int ignore_base[-TESTER_CLIENT_FIRST + TESTER_SERVER_LAST + 1];
#define ignore_count (ignore_base - TESTER_CLIENT_FIRST)

static const struct {
  const char *name;
  int err;
} ignore_str2err[] = {{"OPERATIONS_ERROR", LDAP_OPERATIONS_ERROR},
                      {"PROTOCOL_ERROR", LDAP_PROTOCOL_ERROR},
                      {"TIMELIMIT_EXCEEDED", LDAP_TIMELIMIT_EXCEEDED},
                      {"SIZELIMIT_EXCEEDED", LDAP_SIZELIMIT_EXCEEDED},
                      {"COMPARE_FALSE", LDAP_COMPARE_FALSE},
                      {"COMPARE_TRUE", LDAP_COMPARE_TRUE},
                      {"AUTH_METHOD_NOT_SUPPORTED", LDAP_AUTH_METHOD_NOT_SUPPORTED},
                      {"STRONG_AUTH_NOT_SUPPORTED", LDAP_STRONG_AUTH_NOT_SUPPORTED},
                      {"STRONG_AUTH_REQUIRED", LDAP_STRONG_AUTH_REQUIRED},
                      {"STRONGER_AUTH_REQUIRED", LDAP_STRONGER_AUTH_REQUIRED},
                      {"PARTIAL_RESULTS", LDAP_PARTIAL_RESULTS},

                      {"REFERRAL", LDAP_REFERRAL},
                      {"ADMINLIMIT_EXCEEDED", LDAP_ADMINLIMIT_EXCEEDED},
                      {"UNAVAILABLE_CRITICAL_EXTENSION", LDAP_UNAVAILABLE_CRITICAL_EXTENSION},
                      {"CONFIDENTIALITY_REQUIRED", LDAP_CONFIDENTIALITY_REQUIRED},
                      {"SASL_BIND_IN_PROGRESS", LDAP_SASL_BIND_IN_PROGRESS},

                      {"NO_SUCH_ATTRIBUTE", LDAP_NO_SUCH_ATTRIBUTE},
                      {"UNDEFINED_TYPE", LDAP_UNDEFINED_TYPE},
                      {"INAPPROPRIATE_MATCHING", LDAP_INAPPROPRIATE_MATCHING},
                      {"CONSTRAINT_VIOLATION", LDAP_CONSTRAINT_VIOLATION},
                      {"TYPE_OR_VALUE_EXISTS", LDAP_TYPE_OR_VALUE_EXISTS},
                      {"INVALID_SYNTAX", LDAP_INVALID_SYNTAX},

                      {"NO_SUCH_OBJECT", LDAP_NO_SUCH_OBJECT},
                      {"ALIAS_PROBLEM", LDAP_ALIAS_PROBLEM},
                      {"INVALID_DN_SYNTAX", LDAP_INVALID_DN_SYNTAX},
                      {"IS_LEAF", LDAP_IS_LEAF},
                      {"ALIAS_DEREF_PROBLEM", LDAP_ALIAS_DEREF_PROBLEM},

                      /* obsolete */
                      {"PROXY_AUTHZ_FAILURE", LDAP_X_PROXY_AUTHZ_FAILURE},
                      {"INAPPROPRIATE_AUTH", LDAP_INAPPROPRIATE_AUTH},
                      {"INVALID_CREDENTIALS", LDAP_INVALID_CREDENTIALS},
                      {"INSUFFICIENT_ACCESS", LDAP_INSUFFICIENT_ACCESS},

                      {"BUSY", LDAP_BUSY},
                      {"UNAVAILABLE", LDAP_UNAVAILABLE},
                      {"UNWILLING_TO_PERFORM", LDAP_UNWILLING_TO_PERFORM},
                      {"LOOP_DETECT", LDAP_LOOP_DETECT},

                      {"NAMING_VIOLATION", LDAP_NAMING_VIOLATION},
                      {"OBJECT_CLASS_VIOLATION", LDAP_OBJECT_CLASS_VIOLATION},
                      {"NOT_ALLOWED_ON_NONLEAF", LDAP_NOT_ALLOWED_ON_NONLEAF},
                      {"NOT_ALLOWED_ON_RDN", LDAP_NOT_ALLOWED_ON_RDN},
                      {"ALREADY_EXISTS", LDAP_ALREADY_EXISTS},
                      {"NO_OBJECT_CLASS_MODS", LDAP_NO_OBJECT_CLASS_MODS},
                      {"RESULTS_TOO_LARGE", LDAP_RESULTS_TOO_LARGE},
                      {"AFFECTS_MULTIPLE_DSAS", LDAP_AFFECTS_MULTIPLE_DSAS},

                      {"OTHER", LDAP_OTHER},

                      {"SERVER_DOWN", LDAP_SERVER_DOWN},
                      {"LOCAL_ERROR", LDAP_LOCAL_ERROR},
                      {"ENCODING_ERROR", LDAP_ENCODING_ERROR},
                      {"DECODING_ERROR", LDAP_DECODING_ERROR},
                      {"TIMEOUT", LDAP_TIMEOUT},
                      {"AUTH_UNKNOWN", LDAP_AUTH_UNKNOWN},
                      {"FILTER_ERROR", LDAP_FILTER_ERROR},
                      {"USER_CANCELLED", LDAP_USER_CANCELLED},
                      {"PARAM_ERROR", LDAP_PARAM_ERROR},
                      {"NO_MEMORY", LDAP_NO_MEMORY},
                      {"CONNECT_ERROR", LDAP_CONNECT_ERROR},
                      {"NOT_SUPPORTED", LDAP_NOT_SUPPORTED},
                      {"CONTROL_NOT_FOUND", LDAP_CONTROL_NOT_FOUND},
                      {"NO_RESULTS_RETURNED", LDAP_NO_RESULTS_RETURNED},
                      {"MORE_RESULTS_TO_RETURN", LDAP_MORE_RESULTS_TO_RETURN},
                      {"CLIENT_LOOP", LDAP_CLIENT_LOOP},
                      {"REFERRAL_LIMIT_EXCEEDED", LDAP_REFERRAL_LIMIT_EXCEEDED},

                      {NULL}};

#define UNKNOWN_ERR (1234567890)

#define RETRIES 0
#define LOOPS 100

static int tester_ignore_str2err(const char *err) {
  int i, ignore = 1;

  if (strcmp(err, "ALL") == 0) {
    for (i = 0; ignore_str2err[i].name != NULL; i++) {
      ignore_count[ignore_str2err[i].err] = 1;
    }
    ignore_count[LDAP_SUCCESS] = 0;

    return 0;
  }

  if (err[0] == '!') {
    ignore = 0;
    err++;

  } else if (err[0] == '*') {
    ignore = -1;
    err++;
  }

  for (i = 0; ignore_str2err[i].name != NULL; i++) {
    if (strcmp(err, ignore_str2err[i].name) == 0) {
      int err = ignore_str2err[i].err;

      if (err != LDAP_SUCCESS) {
        ignore_count[err] = ignore;
      }

      return err;
    }
  }

  return UNKNOWN_ERR;
}

int tester_ignore_str2errlist(const char *err) {
  int i;
  char **errs = ldap_str2charray(err, ",");

  for (i = 0; errs[i] != NULL; i++) {
    /* TODO: allow <err>:<prog> to ignore <err> only when <prog> */
    (void)tester_ignore_str2err(errs[i]);
  }

  ldap_charray_free(errs);

  return 0;
}

int tester_ignore_err(int err) {
  int rc = 1;

  if (err && TESTER_CLIENT_FIRST <= err && err <= TESTER_SERVER_LAST) {
    rc = ignore_count[err];
    if (rc != 0) {
      ignore_count[err] = rc + (rc > 0 ? 1 : -1);
    }
  }

  /* SUCCESS is always "ignored" */
  return rc;
}

struct tester_conn_args *tester_init(const char *pname, tester_t ptype) {
  static struct tester_conn_args config = {
      .authmethod = -1,
      .retries = RETRIES,
      .loops = LOOPS,
      .outerloops = 1,

      .uri = NULL,
      .host = "localhost",
      .port = 389,
  };

  pid = getpid();
  srand48(pid);
  snprintf(progname, sizeof(progname), "%s PID=%d", pname, pid);
  progtype = ptype;

  return &config;
}

void tester_ldap_error(LDAP *ld, const char *fname, const char *msg) {
  int err;
  char *text = NULL;
  LDAPControl **ctrls = NULL;

  ldap_get_option(ld, LDAP_OPT_RESULT_CODE, (void *)&err);
  if (err != LDAP_SUCCESS) {
    ldap_get_option(ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void *)&text);
  }

  fprintf(stderr, "%s: %s: %s (%d) %s %s\n", progname, fname, ldap_err2string(err), err, text == NULL ? "" : text,
          msg ? msg : "");

  if (text) {
    ldap_memfree(text);
    text = NULL;
  }

  ldap_get_option(ld, LDAP_OPT_MATCHED_DN, (void *)&text);
  if (text != NULL) {
    if (text[0] != '\0') {
      fprintf(stderr, "\tmatched: %s\n", text);
    }
    ldap_memfree(text);
    text = NULL;
  }

  ldap_get_option(ld, LDAP_OPT_SERVER_CONTROLS, (void *)&ctrls);
  if (ctrls != NULL) {
    int i;

    fprintf(stderr, "\tcontrols:\n");
    for (i = 0; ctrls[i] != NULL; i++) {
      fprintf(stderr, "\t\t%s\n", ctrls[i]->ldctl_oid);
    }
    ldap_controls_free(ctrls);
    ctrls = NULL;
  }

  if (err == LDAP_REFERRAL) {
    char **refs = NULL;

    ldap_get_option(ld, LDAP_OPT_REFERRAL_URLS, (void *)&refs);

    if (refs) {
      int i;

      fprintf(stderr, "\treferral:\n");
      for (i = 0; refs[i] != NULL; i++) {
        fprintf(stderr, "\t\t%s\n", refs[i]);
      }

      ber_memvfree((void **)refs);
    }
  }
}

void tester_perror(const char *fname, const char *msg) {
  int save_errno = errno;
  char buf[BUFSIZ];

  fprintf(stderr, "%s: %s: (%d) %s %s\n", progname, fname, save_errno, AC_STRERROR_R(save_errno, buf, sizeof(buf)),
          msg ? msg : "");
}

int tester_config_opt(struct tester_conn_args *config, char opt, char *optarg) {
  switch (opt) {
  case 'C':
    config->chaserefs++;
    break;

  case 'D':
    config->binddn = strdup(optarg);
    break;

  case 'd': {
    int debug;
    if (lutil_atoi(&debug, optarg) != 0) {
      return -1;
    }

    if (ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &debug) != LBER_OPT_SUCCESS) {
      fprintf(stderr, "Could not set LBER_OPT_DEBUG_LEVEL %d\n", debug);
    }

    if (ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &debug) != LDAP_OPT_SUCCESS) {
      fprintf(stderr, "Could not set LDAP_OPT_DEBUG_LEVEL %d\n", debug);
    }
    break;
  }

  case 'H':
    config->uri = strdup(optarg);
    break;

  case 'h':
    config->host = strdup(optarg);
    break;

  case 'i':
    tester_ignore_str2errlist(optarg);
    break;

  case 'L':
    if (lutil_atoi(&config->outerloops, optarg) != 0) {
      return -1;
    }
    break;

  case 'l':
    if (lutil_atoi(&config->loops, optarg) != 0) {
      return -1;
    }
    break;

#ifdef HAVE_CYRUS_SASL
  case 'O':
    if (config->secprops != NULL) {
      return -1;
    }
    if (config->authmethod != -1 && config->authmethod != LDAP_AUTH_SASL) {
      return -1;
    }
    config->authmethod = LDAP_AUTH_SASL;
    config->secprops = ber_strdup(optarg);
    break;

  case 'R':
    if (config->realm != NULL) {
      return -1;
    }
    if (config->authmethod != -1 && config->authmethod != LDAP_AUTH_SASL) {
      return -1;
    }
    config->authmethod = LDAP_AUTH_SASL;
    config->realm = ber_strdup(optarg);
    break;

  case 'U':
    if (config->authc_id != NULL) {
      return -1;
    }
    if (config->authmethod != -1 && config->authmethod != LDAP_AUTH_SASL) {
      return -1;
    }
    config->authmethod = LDAP_AUTH_SASL;
    config->authc_id = ber_strdup(optarg);
    break;

  case 'X':
    if (config->authz_id != NULL) {
      return -1;
    }
    if (config->authmethod != -1 && config->authmethod != LDAP_AUTH_SASL) {
      return -1;
    }
    config->authmethod = LDAP_AUTH_SASL;
    config->authz_id = ber_strdup(optarg);
    break;

  case 'Y':
    if (config->mech != NULL) {
      return -1;
    }
    if (config->authmethod != -1 && config->authmethod != LDAP_AUTH_SASL) {
      return -1;
    }
    config->authmethod = LDAP_AUTH_SASL;
    config->mech = ber_strdup(optarg);
    break;
#endif

  case 'p':
    if (lutil_atoi(&config->port, optarg) != 0) {
      return -1;
    }
    break;

  case 'r':
    if (lutil_atoi(&config->retries, optarg) != 0) {
      return -1;
    }
    break;

  case 't':
    if (lutil_atoi(&config->delay, optarg) != 0) {
      return -1;
    }
    break;

  case 'w':
    config->pass.bv_val = strdup(optarg);
    config->pass.bv_len = strlen(optarg);
    memset(optarg, '*', config->pass.bv_len);
    break;

  case 'x':
    if (config->authmethod != -1 && config->authmethod != LDAP_AUTH_SIMPLE) {
      return -1;
    }
    config->authmethod = LDAP_AUTH_SIMPLE;
    break;

  default:
    return -1;
  }

  return LDAP_SUCCESS;
}

void tester_config_finish(struct tester_conn_args *config) {
  if (!config->uri) {
    static char uribuf[BUFSIZ];

    config->uri = uribuf;
    snprintf(uribuf, sizeof(uribuf), "ldap://%s:%d", config->host, config->port);
  }

  if (config->authmethod == -1) {
#ifdef HAVE_CYRUS_SASL
    if (config->binddn != NULL) {
      config->authmethod = LDAP_AUTH_SIMPLE;
    } else {
      config->authmethod = LDAP_AUTH_SASL;
    }
#else
    config->authmethod = LDAP_AUTH_SIMPLE;
#endif
  }

#ifdef HAVE_CYRUS_SASL
  if (config->authmethod == LDAP_AUTH_SASL) {
    config->defaults =
        lutil_sasl_defaults(NULL, config->mech, config->realm, config->authc_id, config->pass.bv_val, config->authz_id);

    if (config->defaults == NULL) {
      tester_error("unable to prepare SASL defaults");
      exit(EXIT_FAILURE);
    }
  }
#endif
}

void tester_init_ld(LDAP **ldp, struct tester_conn_args *config, int flags) {
  LDAP *ld;
  int rc = -1, do_retry = config->retries;
  int version = LDAP_VERSION3;

retry:;
  ldap_initialize(&ld, config->uri);
  if (ld == NULL) {
    tester_perror("ldap_initialize", NULL);
    exit(EXIT_FAILURE);
  }

  (void)ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
  (void)ldap_set_option(ld, LDAP_OPT_REFERRALS, config->chaserefs ? LDAP_OPT_ON : LDAP_OPT_OFF);

  if (!(flags & TESTER_INIT_ONLY)) {
    if (config->authmethod == LDAP_AUTH_SASL) {
#ifdef HAVE_CYRUS_SASL
      if (config->secprops != NULL) {
        rc = ldap_set_option(ld, LDAP_OPT_X_SASL_SECPROPS, config->secprops);

        if (rc != LDAP_OPT_SUCCESS) {
          tester_ldap_error(ld, "ldap_set_option(SECPROPS)", NULL);
          exit(EXIT_FAILURE);
        }
      }

      rc = ldap_sasl_interactive_bind_s(ld, config->binddn, config->mech, NULL, NULL, LDAP_SASL_QUIET,
                                        lutil_sasl_interact, config->defaults);
#else /* HAVE_CYRUS_SASL */
      /* caller shouldn't have allowed this */
      assert(0);
#endif
    } else if (config->authmethod == LDAP_AUTH_SIMPLE) {
      rc = ldap_sasl_bind_s(ld, config->binddn, LDAP_SASL_SIMPLE, &config->pass, NULL, NULL, NULL);
    }

    if (rc != LDAP_SUCCESS) {
      tester_ldap_error(ld, "ldap_sasl_bind_s", NULL);
      switch (rc) {
      case LDAP_BUSY:
      case LDAP_UNAVAILABLE:
        if (do_retry > 0) {
          do_retry--;
          if (config->delay > 0) {
            sleep(config->delay);
          }
          goto retry;
        }
      }
      tester_ldap_error(ld, "ldap_sasl_bind_s", NULL);
      exit(EXIT_FAILURE);
    }
  }

  *ldp = ld;
}

void tester_error(const char *msg) { fprintf(stderr, "%s: %s\n", progname, msg); }
