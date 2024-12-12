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
 * This work was initially developed by Kurt Spanier for inclusion
 * in OpenLDAP Software.
 */

#include "reldap.h"

#include <stdio.h>

#include "ac/stdlib.h"

#include "ac/ctype.h"
#include "ac/param.h"
#include "ac/socket.h"
#include "ac/string.h"
#include "ac/unistd.h"
#include "ac/wait.h"

#include "ldap.h"
#include "ldif.h"
#include "lutil.h"

#include "slapd-common.h"

static void do_addel(struct tester_conn_args *config, LDIFRecord *record, int friendly);

static void usage(char *name, char opt) {
  if (opt) {
    fprintf(stderr, "%s: unable to handle option \'%c\'\n\n", name, opt);
  }

  fprintf(stderr,
          "usage: %s " TESTER_COMMON_HELP "-f <addfile> "
          "[-F]\n",
          name);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  int i;
  char *filename = NULL, *buf = NULL;
  int friendly = 0;
  struct LDIFFP *fp;
  LDIFRecord record = {};
  struct tester_conn_args *config;
  struct berval bv = {};
  unsigned long lineno = 0;

  config = tester_init("slapd-addel", TESTER_ADDEL);

  while ((i = getopt(argc, argv, TESTER_COMMON_OPTS "Ff:")) != EOF) {
    switch (i) {
    case 'F':
      friendly++;
      break;

    case 'i':
      /* ignored (!) by now */
      break;

    case 'f': /* file with entry search request */
      filename = strdup(optarg);
      break;

    default:
      if (tester_config_opt(config, i, optarg) == LDAP_SUCCESS) {
        break;
      }
      usage(argv[0], i);
      break;
    }
  }

  if (filename == NULL)
    usage(argv[0], 0);

  if ((fp = ldif_open(filename, "r")) == NULL) {
    tester_perror(filename, "while reading ldif file");
    exit(EXIT_FAILURE);
  }

  i = 0;
  if (ldif_read_record(fp, &lineno, &buf, &i) < 0) {
    tester_error("ldif_read_record failed");
    exit(EXIT_FAILURE);
  }
  bv.bv_val = buf;
  bv.bv_len = i;

  if (ldap_parse_ldif_record(&bv, lineno, &record, "slapd-addel", LDIF_DEFAULT_ADD | LDIF_ENTRIES_ONLY)) {
    tester_error("ldif_read_record failed");
    exit(EXIT_FAILURE);
  }
  ldif_close(fp);

  if ((record.lr_op != LDAP_REQ_ADD) || (!record.lrop_mods)) {

    fprintf(stderr, "%s: invalid entry DN in file \"%s\".\n", argv[0], filename);
    exit(EXIT_FAILURE);
  }

  tester_config_finish(config);

  for (i = 0; i < config->outerloops; i++) {
    do_addel(config, &record, friendly);
  }

  ber_memfree(buf);
  exit(EXIT_SUCCESS);
}

static void do_addel(struct tester_conn_args *config, LDIFRecord *record, int friendly) {
  LDAP *ld = NULL;
  int i = 0, do_retry = config->retries;
  int rc = LDAP_SUCCESS;

retry:;
  if (ld == NULL) {
    tester_init_ld(&ld, config, 0);
  }

  if (do_retry == config->retries) {
    fprintf(stderr, "PID=%ld - Add/Delete(%d): entry=\"%s\".\n", (long)pid, config->loops, record->lr_dn.bv_val);
  }

  for (; i < config->loops; i++) {

    /* add the entry */
    rc = ldap_add_ext_s(ld, record->lr_dn.bv_val, record->lrop_mods, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
      tester_ldap_error(ld, "ldap_add_ext_s", NULL);
      switch (rc) {
      case LDAP_ALREADY_EXISTS:
        /* NOTE: this likely means
         * the delete failed
         * during the previous round... */
        if (!friendly) {
          goto done;
        }
        break;

      case LDAP_BUSY:
      case LDAP_UNAVAILABLE:
        if (do_retry > 0) {
          do_retry--;
          goto retry;
        }
        /* fall thru */

      default:
        goto done;
      }
    }

#if 0
		/* wait a second for the add to really complete */
		/* This masks some race conditions though. */
		sleep( 1 );
#endif

    /* now delete the entry again */
    rc = ldap_delete_ext_s(ld, record->lr_dn.bv_val, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
      tester_ldap_error(ld, "ldap_delete_ext_s", NULL);
      switch (rc) {
      case LDAP_NO_SUCH_OBJECT:
        /* NOTE: this likely means
         * the add failed
         * during the previous round... */
        if (!friendly) {
          goto done;
        }
        break;

      case LDAP_BUSY:
      case LDAP_UNAVAILABLE:
        if (do_retry > 0) {
          do_retry--;
          goto retry;
        }
        /* fall thru */

      default:
        goto done;
      }
    }
  }

done:;
  fprintf(stderr, "  PID=%ld - Add/Delete done (%d).\n", (long)pid, rc);

  ldap_unbind_ext(ld, NULL, NULL);
}
