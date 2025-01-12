/* $ReOpenLDAP$ */
/* Copyright 1990-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was initially developed by Kurt Zeilenga for inclusion
 * in OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <ldap.h>
#include <lber_pvt.h>
#include <lutil.h>
#include <lutil_sha1.h>

#include "ldap_defaults.h"
#include "slap.h"
#include "slapcommon.h"

static char *modulepath = NULL;
static char *moduleload = NULL;

static void usage(const char *s) {
  fprintf(stderr,
          "Usage: %s [options]\n"
          "  -d debuglevel"
          "  -c format\tcrypt(3) salt format\n"
          "  -g\t\tgenerate random password\n"
          "  -h hash\tpassword scheme\n"
          "  -n\t\tomit trailing newline\n"
          "  -o <opt>[=val] specify an option with a(n optional) value\n"
          "  \tmodule-path=<pathspec>\n"
          "  \tmodule-load=<filename>\n"
          "  -s secret\tnew password\n"
          "  -u\t\tgenerate RFC2307 values (default)\n"
          "  -v\t\tincrease verbosity\n"
          "  -T file\tread file for new password\n",
          s);

  exit(EXIT_FAILURE);
}

static int parse_slappasswdopt(void) {
  size_t len = 0;
  char *p;

  p = strchr(optarg, '=');
  if (p != NULL) {
    len = p - optarg;
    p++;
  }

  if (strncasecmp(optarg, "module-path", len) == 0) {
    if (modulepath)
      ch_free(modulepath);
    modulepath = ch_strdup(p);

  } else if (strncasecmp(optarg, "module-load", len) == 0) {
    if (moduleload)
      ch_free(moduleload);
    moduleload = ch_strdup(p);

  } else {
    return -1;
  }

  return 0;
}

int slappasswd(int argc, char *argv[]) {
  int rc = EXIT_SUCCESS;
#ifdef LUTIL_SHA1_BYTES
  char *default_scheme = "{SSHA}";
#else
  char *default_scheme = "{SMD5}";
#endif
  char *scheme = default_scheme;

  char *newpw = NULL;
  char *pwfile = NULL;
  const char *text = NULL;
  const char *progname = "slappasswd";

  int i;
  char *newline = "\n";
  struct berval passwd = BER_BVNULL;
  struct berval hash;
  char **debug_unknowns = NULL;

#ifdef LDAP_SYSLOG
  slap_syslog_mask = 0;
#endif /* LDAP_SYSLOG */

  while ((i = getopt(argc, argv, "c:d:gh:no:s:T:vu")) != EOF) {
    switch (i) {
    case 'd': { /* turn on debugging */
      int mask = 0;

      if (parse_debug_level(optarg, &mask, &debug_unknowns)) {
        usage(progname);
      }
#ifdef LDAP_DEBUG
      slap_set_debug_level(mask);
#else
      if (mask != 0 || debug_unknowns)
        fputs("must be configured with '--enable-debug' for debugging\n", stderr);
#endif
    } break;

    case 'c': /* crypt salt format */
      if (scheme == default_scheme)
        scheme = ch_strdup("{CRYPT}");
      lutil_salt_format(optarg);
      break;

    case 'g': /* new password (generate) */
      if (pwfile != NULL) {
        fprintf(stderr, "Option -g incompatible with -T\n");
        return EXIT_FAILURE;

      } else if (newpw != NULL) {
        fprintf(stderr, "New password already provided\n");
        return EXIT_FAILURE;

      } else if (lutil_passwd_generate(&passwd, 8)) {
        fprintf(stderr, "Password generation failed\n");
        return EXIT_FAILURE;
      }
      break;

    case 'h': /* scheme */
      if (scheme != default_scheme) {
        fprintf(stderr, "Scheme already provided\n");
        return EXIT_FAILURE;

      } else {
        scheme = ch_strdup(optarg);
      }
      break;

    case 'n':
      newline = "";
      break;

    case 'o':
      if (parse_slappasswdopt()) {
        usage(progname);
      }
      break;

    case 's': /* new password (secret) */
      if (pwfile != NULL) {
        fprintf(stderr, "Option -s incompatible with -T\n");
        return EXIT_FAILURE;

      } else if (newpw != NULL) {
        fprintf(stderr, "New password already provided\n");
        return EXIT_FAILURE;

      } else {
        char *p;
        newpw = ch_strdup(optarg);

        for (p = optarg; *p != '\0'; p++) {
          *p = '\0';
        }
      }
      break;

    case 'T': /* password file */
      if (pwfile != NULL) {
        fprintf(stderr, "Password file already provided\n");
        return EXIT_FAILURE;

      } else if (newpw != NULL) {
        fprintf(stderr, "Option -T incompatible with -s/-g\n");
        return EXIT_FAILURE;
      }
      pwfile = optarg;
      break;

    case 'u': /* RFC2307 userPassword */
      break;

    case 'v': /* verbose */
      verbose++;
      break;

    default:
      usage(progname);
    }
  }

  if (argc - optind != 0) {
    usage(progname);
  }

  if (debug_unknowns) {
    int mask = LDAP_DEBUG_NONE;
    rc = parse_debug_unknowns(debug_unknowns, &mask);
    slap_set_debug_level(mask);
    ldap_charray_free(debug_unknowns);
    debug_unknowns = NULL;
    if (rc)
      goto destroy;
  }

  slapMode = SLAP_TOOL_MODE;

#ifdef SLAPD_DYNAMIC_MODULES
  if (module_init() != 0) {
    fprintf(stderr, "%s: module_init failed\n", progname);
    rc = EXIT_FAILURE;
    goto destroy;
  }

  if (modulepath && module_path(modulepath)) {
    rc = EXIT_FAILURE;
    goto destroy;
  }

  if (moduleload && module_load(moduleload, 0, NULL)) {
    rc = EXIT_FAILURE;
    goto destroy;
  }
#endif

  if (pwfile != NULL) {
    if (passwd.bv_val)
      free(passwd.bv_val);
    if (lutil_get_filed_password(pwfile, &passwd)) {
      rc = EXIT_FAILURE;
      goto destroy;
    }
  } else if (BER_BVISEMPTY(&passwd)) {
    if (newpw == NULL) {
      /* prompt for new password */
      char *cknewpw;
      newpw = ch_strdup(getpassphrase("New password: "));
      cknewpw = getpassphrase("Re-enter new password: ");

      if (strcmp(newpw, cknewpw)) {
        fprintf(stderr, "Password values do not match\n");
        rc = EXIT_FAILURE;
        goto destroy;
      }
    }

    passwd.bv_val = newpw;
    passwd.bv_len = strlen(passwd.bv_val);
  } else {
    hash = passwd;
    goto print_pw;
  }

  lutil_passwd_hash(&passwd, scheme, &hash, &text);
  if (hash.bv_val == NULL) {
    fprintf(stderr, "Password generation failed for scheme %s: %s\n", scheme, text ? text : "");
    rc = EXIT_FAILURE;
    goto destroy;
  }

  if (lutil_passwd(&hash, &passwd, NULL, &text)) {
    fprintf(stderr, "Password verification failed. %s\n", text ? text : "");
    rc = EXIT_FAILURE;
    goto destroy;
  }

print_pw:;
  printf("%s%s", hash.bv_val, newline);

destroy:;
#ifdef SLAPD_DYNAMIC_MODULES
  module_kill();
#endif
  lutil_passwd_destroy();

  if (text && text != passwd.bv_val && text != hash.bv_val)
    free((void *)text);
  if (scheme != default_scheme)
    free(scheme);
  if (passwd.bv_val)
    free(passwd.bv_val);
  if (hash.bv_val && hash.bv_val != passwd.bv_val)
    free(hash.bv_val);
  if (newpw && newpw != passwd.bv_val)
    free(newpw);

  return rc;
}
