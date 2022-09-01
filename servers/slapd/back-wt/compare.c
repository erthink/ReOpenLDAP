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

int wt_compare(Operation *op, SlapReply *rs) {
  struct wt_info *wi = (struct wt_info *)op->o_bd->be_private;
  Entry *e = NULL;
  int manageDSAit = get_manageDSAit(op);
  int rc;
  wt_ctx *wc = NULL;

  Debug(LDAP_DEBUG_ARGS, "==> wt_compare: %s\n", op->o_req_dn.bv_val);

  wc = wt_ctx_get(op, wi);
  if (!wc) {
    Debug(LDAP_DEBUG_ANY, "wt_compare: wt_ctx_get failed\n");
    rs->sr_err = LDAP_OTHER;
    rs->sr_text = "internal error";
    send_ldap_result(op, rs);
    return rs->sr_err;
  }

  rc = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
  switch (rc) {
  case 0:
  case WT_NOTFOUND:
    break;
  default:
    rs->sr_err = LDAP_OTHER;
    rs->sr_text = "internal error";
    goto return_results;
  }

  if (rc == WT_NOTFOUND || (!manageDSAit && e && is_entry_glue(e))) {

    if (!e) {
      rc = wt_dn2aentry(op->o_bd, wc, &op->o_req_ndn, &e);
      switch (rc) {
      case 0:
        break;
      case WT_NOTFOUND:
        rs->sr_err = LDAP_NO_SUCH_OBJECT;
        goto return_results;
      default:
        Debug(LDAP_DEBUG_ANY, "wt_compare: wt_dn2aentry failed (%d)\n", rc);
        rs->sr_err = LDAP_OTHER;
        rs->sr_text = "internal error";
        goto return_results;
      }
    }

    /* return referral only if "disclose" is granted on the object */
    if (!access_allowed(op, e, slap_schema.si_ad_entry, NULL, ACL_DISCLOSE,
                        NULL)) {
      rs->sr_err = LDAP_NO_SUCH_OBJECT;
    } else {
      rs->sr_matched = ch_strdup(e->e_dn);
      if (is_entry_referral(e)) {
        BerVarray ref = get_entry_referrals(op, e);
        rs->sr_ref = referral_rewrite(ref, &e->e_name, &op->o_req_dn,
                                      LDAP_SCOPE_DEFAULT);
        ber_bvarray_free(ref);
      } else {
        rs->sr_ref = NULL;
      }
      rs->sr_err = LDAP_REFERRAL;
    }
    rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
    send_ldap_result(op, rs);
    goto done;
  }

  if (!manageDSAit && is_entry_referral(e)) {
    /* return referral only if "disclose" is granted on the object */
    if (!access_allowed(op, e, slap_schema.si_ad_entry, NULL, ACL_DISCLOSE,
                        NULL)) {
      rs->sr_err = LDAP_NO_SUCH_OBJECT;
    } else {
      /* entry is a referral, don't allow compare */
      rs->sr_ref = get_entry_referrals(op, e);
      rs->sr_err = LDAP_REFERRAL;
      rs->sr_matched = e->e_name.bv_val;
    }

    Debug(LDAP_DEBUG_TRACE, "entry is referral\n");

    send_ldap_result(op, rs);

    ber_bvarray_free(rs->sr_ref);
    rs->sr_ref = NULL;
    rs->sr_matched = NULL;
    goto done;
  }

  rs->sr_err = slap_compare_entry(op, e, op->orc_ava);

return_results:
  send_ldap_result(op, rs);

  switch (rs->sr_err) {
  case LDAP_COMPARE_FALSE:
  case LDAP_COMPARE_TRUE:
    rs->sr_err = LDAP_SUCCESS;
    break;
  }

done:
  if (e != NULL) {
    wt_entry_return(e);
  }
  return rs->sr_err;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
