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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-ldap.h"

int ldap_back_delete(Operation *op, SlapReply *rs) {
  ldapinfo_t *li = (ldapinfo_t *)op->o_bd->be_private;

  ldapconn_t *lc = NULL;
  ber_int_t msgid;
  LDAPControl **ctrls = NULL;
  ldap_back_send_t retrying = LDAP_BACK_RETRYING;
  int rc = LDAP_SUCCESS;

  if (!ldap_back_dobind(&lc, op, rs, LDAP_BACK_SENDERR)) {
    return rs->sr_err;
  }

retry:
  ctrls = op->o_ctrls;
  rc = ldap_back_controls_add(op, rs, lc, &ctrls);
  if (rc != LDAP_SUCCESS) {
    send_ldap_result(op, rs);
    goto cleanup;
  }

  rs->sr_err = ldap_delete_ext(lc->lc_ld, op->o_req_dn.bv_val, ctrls, NULL, &msgid);
  rc = ldap_back_op_result(lc, op, rs, msgid, li->li_timeout[SLAP_OP_DELETE], (LDAP_BACK_SENDRESULT | retrying));
  if (rs->sr_err == LDAP_UNAVAILABLE && retrying) {
    retrying &= ~LDAP_BACK_RETRYING;
    if (ldap_back_retry(&lc, op, rs, LDAP_BACK_SENDERR)) {
      /* if the identity changed, there might be need to re-authz */
      (void)ldap_back_controls_free(op, rs, &ctrls);
      goto retry;
    }
  }

  ldap_pvt_thread_mutex_lock(&li->li_counter_mutex);
  ldap_pvt_mp_add(li->li_ops_completed[SLAP_OP_DELETE], 1);
  ldap_pvt_thread_mutex_unlock(&li->li_counter_mutex);

cleanup:
  (void)ldap_back_controls_free(op, rs, &ctrls);

  if (lc != NULL) {
    ldap_back_release_conn(li, lc);
  }

  return rs->sr_err;
}
