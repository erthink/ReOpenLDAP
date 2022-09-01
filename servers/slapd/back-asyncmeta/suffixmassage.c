/* $ReOpenLDAP$ */
/* Copyright 2016-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was developed by Symas Corporation
 * based on back-meta module for inclusion in OpenLDAP Software.
 * This work was sponsored by Ericsson. */

/* This is an altered version */

/*
 * Copyright 1999, Howard Chu, All rights reserved. <hyc@highlandsun.com>
 * Copyright 2000, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 *
 * Module back-ldap, originally developed by Howard Chu
 *
 * has been modified by Pierangelo Masarati. The original copyright
 * notice has been maintained.
 *
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 *
 * 4. This notice may not be removed or altered.
 */

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

int asyncmeta_dn_massage(a_dncookie *dc, struct berval *dn,
                         struct berval *res) {
  int rc = 0;
  static char *dmy = "";

  switch (rewrite_session(dc->target->mt_rwmap.rwm_rw, dc->ctx,
                          (dn->bv_val ? dn->bv_val : dmy), dc->conn,
                          &res->bv_val)) {
  case REWRITE_REGEXEC_OK:
    if (res->bv_val != NULL) {
      res->bv_len = strlen(res->bv_val);
    } else {
      *res = *dn;
    }
    Debug(LDAP_DEBUG_ARGS, "[rw] %s: \"%s\" -> \"%s\"\n", dc->ctx,
          BER_BVISNULL(dn) ? "" : dn->bv_val,
          BER_BVISNULL(res) ? "" : res->bv_val);
    rc = LDAP_SUCCESS;
    break;

  case REWRITE_REGEXEC_UNWILLING:
    if (dc->rs) {
      dc->rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
      dc->rs->sr_text = "Operation not allowed";
    }
    rc = LDAP_UNWILLING_TO_PERFORM;
    break;

  case REWRITE_REGEXEC_ERR:
    if (dc->rs) {
      dc->rs->sr_err = LDAP_OTHER;
      dc->rs->sr_text = "Rewrite error";
    }
    rc = LDAP_OTHER;
    break;
  }

  if (res->bv_val == dmy) {
    BER_BVZERO(res);
  }

  return rc;
}
