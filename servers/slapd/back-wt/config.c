/* $ReOpenLDAP$ */
/* Copyright 2002-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "slapconfig.h"
#include "lutil.h"
#include "ldap_rq.h"

static ConfigDriver wt_cf_gen;

enum {
  WT_DIRECTORY = 1,
  WT_CONFIG,
  WT_INDEX,
  WT_MODE,
};

static ConfigTable wtcfg[] = {
    {"directory", "dir", 2, 2, 0, ARG_STRING | ARG_MAGIC | WT_DIRECTORY,
     wt_cf_gen,
     "( OLcfgDbAt:0.1 NAME 'olcDbDirectory' "
     "DESC 'Directory for database content' "
     "EQUALITY caseIgnoreMatch "
     "SYNTAX OMsDirectoryString SINGLE-VALUE )",
     NULL, NULL},
    {"wtconfig", "config", 2, 2, 0, ARG_STRING | ARG_MAGIC | WT_CONFIG,
     wt_cf_gen,
     "( OLcfgDbAt:13.1 NAME 'olcWtConfig' "
     "DESC 'Configuration for WiredTiger' "
     "EQUALITY caseIgnoreMatch "
     "SYNTAX OMsDirectoryString SINGLE-VALUE )",
     NULL, NULL},
    {"index", "attr> <[pres,eq,approx,sub]", 2, 3, 0, ARG_MAGIC | WT_INDEX,
     wt_cf_gen,
     "( OLcfgDbAt:0.2 NAME 'olcDbIndex' "
     "DESC 'Attribute index parameters' "
     "EQUALITY caseIgnoreMatch "
     "SYNTAX OMsDirectoryString )",
     NULL, NULL},
    {"mode", "mode", 2, 2, 0, ARG_MAGIC | WT_MODE, wt_cf_gen,
     "( OLcfgDbAt:0.3 NAME 'olcDbMode' "
     "DESC 'Unix permissions of database files' "
     "SYNTAX OMsDirectoryString SINGLE-VALUE )",
     NULL, NULL},
    {NULL, NULL, 0, 0, 0, ARG_IGNORED, NULL, NULL, NULL, NULL}};

static ConfigOCs wtocs[] = {{"( OLcfgDbOc:13.1 "
                             "NAME 'olcWtConfig' "
                             "DESC 'Wt backend configuration' "
                             "SUP olcDatabaseConfig "
                             "MUST olcDbDirectory "
                             "MAY ( olcWtConfig $ olcDbIndex $ olcDbMode ) )",
                             Cft_Database, wtcfg},
                            {NULL, 0, NULL}};

/* reindex entries on the fly */
static void *wt_online_index(void *ctx, void *arg) {
  // Not implement yet
  return NULL;
}

/* Cleanup loose ends after Modify completes */
static int wt_cf_cleanup(ConfigArgs *c) {
  // Not implement yet
  return 0;
}

static int wt_cf_gen(ConfigArgs *c) {
  struct wt_info *wi = (struct wt_info *)c->be->be_private;
  int rc;

  if (c->op == SLAP_CONFIG_EMIT) {
    rc = 0;
    switch (c->type) {
    case WT_DIRECTORY:
      if (wi->wi_home) {
        c->value_string = ch_strdup(wi->wi_home);
      } else {
        rc = 1;
      }
      break;
    case WT_INDEX:
      wt_attr_index_unparse(wi, &c->rvalue_vals);
      if (!c->rvalue_vals)
        rc = 1;
      break;
    }
    return rc;
  }

  switch (c->type) {
  case WT_DIRECTORY:
    ch_free(wi->wi_home);
    wi->wi_home = c->value_string;
    break;
  case WT_CONFIG:
    if (strlen(wi->wi_config) + 1 + strlen(c->value_string) > WT_CONFIG_MAX) {
      fprintf(stderr,
              "%s: "
              "\"wtconfig\" are too long. Increase WT_CONFIG_MAX or you may "
              "realloc the buffer.\n",
              c->log);
      return 1;
    }
    /* size of wi->wi_config is WT_CONFIG_MAX + 1, it's guaranteed with
     * NUL-terminate. */
    strcat(wi->wi_config, ",");
    strcat(wi->wi_config, c->value_string);
    break;

  case WT_INDEX:
    rc = wt_attr_index_config(wi, c->fname, c->lineno, c->argc - 1, &c->argv[1],
                              &c->reply);

    if (rc != LDAP_SUCCESS)
      return 1;
    wi->wi_flags |= WT_OPEN_INDEX;

    if (wi->wi_flags & WT_IS_OPEN) {
      c->cleanup = wt_cf_cleanup;

      if (!wi->wi_index_task) {
        /* Start the task as soon as we finish here. Set a long
         * interval (10 hours) so that it only gets scheduled once.
         */
        if (c->be->be_suffix == NULL || BER_BVISNULL(&c->be->be_suffix[0])) {
          fprintf(stderr,
                  "%s: "
                  "\"index\" must occur after \"suffix\".\n",
                  c->log);
          return 1;
        }
        ldap_pvt_thread_mutex_lock(&slapd_rq.rq_mutex);
        wi->wi_index_task = ldap_pvt_runqueue_insert(
            &slapd_rq, 36000, wt_online_index, c->be,
            LDAP_XSTRING(wt_online_index), c->be->be_suffix[0].bv_val);
        ldap_pvt_thread_mutex_unlock(&slapd_rq.rq_mutex);
      }
    }
    break;
  }
  return LDAP_SUCCESS;
}

int wt_back_init_cf(BackendInfo *bi) {
  int rc;
  bi->bi_cf_ocs = wtocs;

  rc = config_register_schema(wtcfg, wtocs);
  if (rc)
    return rc;
  return 0;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
