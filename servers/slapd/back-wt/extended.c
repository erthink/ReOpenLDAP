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
#include "lber_pvt.h"

static struct exop {
  struct berval *oid;
  BI_op_extended *extended;
} exop_table[] = {{NULL, NULL}};

int wt_extended(Operation *op, SlapReply *rs) {
  int i;

  for (i = 0; exop_table[i].extended != NULL; i++) {
    if (ber_bvcmp(exop_table[i].oid, &op->oq_extended.rs_reqoid) == 0) {
      return (exop_table[i].extended)(op, rs);
    }
  }

  rs->sr_text = "not supported within naming context";
  return rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
