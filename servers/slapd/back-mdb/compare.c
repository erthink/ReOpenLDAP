/* $ReOpenLDAP$ */
/* Copyright 2011-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>

#include "back-mdb.h"

int mdb_compare(Operation *op, SlapReply *rs) {
  struct mdb_info *mdb = (struct mdb_info *)op->o_bd->be_private;
  Entry *e = NULL;
  int manageDSAit = get_manageDSAit(op);

  MDBX_txn *rtxn;
  mdb_op_info opinfo = {{{0}}}, *moi = &opinfo;

  rs->sr_err = mdb_opinfo_get(op, mdb, 1, &moi);
  switch (rs->sr_err) {
  case 0:
    break;
  default:
    send_ldap_error(op, rs, LDAP_OTHER, "internal error");
    return rs->sr_err;
  }

  rtxn = moi->moi_txn;

  /* get entry */
  rs->sr_err = mdb_dn2entry(op, rtxn, NULL, &op->o_req_ndn, &e, NULL, 1);
  switch (rs->sr_err) {
  case MDBX_NOTFOUND:
  case 0:
    break;
  case LDAP_BUSY:
    rs->sr_text = "ldap server busy";
    goto return_results;
  default:
    rs->sr_err = LDAP_OTHER;
    rs->sr_text = "internal error";
    goto return_results;
  }

  if (rs->sr_err == MDBX_NOTFOUND) {
    if (e != NULL) {
      /* return referral only if "disclose" is granted on the object */
      if (access_allowed(op, e, slap_schema.si_ad_entry, NULL, ACL_DISCLOSE,
                         NULL)) {
        rs->sr_matched = ch_strdup(e->e_dn);
        if (is_entry_referral(e)) {
          BerVarray ref = get_entry_referrals(op, e);
          rs->sr_ref = referral_rewrite(ref, &e->e_name, &op->o_req_dn,
                                        LDAP_SCOPE_DEFAULT);
          ber_bvarray_free(ref);
        } else {
          rs->sr_ref = NULL;
        }
      }
      mdb_entry_return(op, e);
      e = NULL;
    } else {
      rs->sr_ref = referral_rewrite(default_referral, NULL, &op->o_req_dn,
                                    LDAP_SCOPE_DEFAULT);
    }

    rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
    rs->sr_err = rs->sr_ref ? LDAP_REFERRAL : LDAP_NO_SUCH_OBJECT;
    goto return_results;
  }

  if (!manageDSAit && is_entry_referral(e)) {
    /* return referral only if "disclose" is granted on the object */
    if (access_allowed(op, e, slap_schema.si_ad_entry, NULL, ACL_DISCLOSE,
                       NULL)) {
      /* entry is a referral, don't allow compare */
      rs->sr_ref = get_entry_referrals(op, e);
      rs->sr_matched = e->e_name.bv_val;
      rs->sr_flags = REP_REF_MUSTBEFREED;
    }

    Debug(LDAP_DEBUG_TRACE, "entry is referral\n");
    rs->sr_err = rs->sr_ref ? LDAP_REFERRAL : LDAP_NO_SUCH_OBJECT;
    goto return_results;
  }

  rs->sr_err = slap_compare_entry(op, e, op->orc_ava);

return_results:
  send_ldap_result(op, rs);
  rs_send_cleanup(rs);

  switch (rs->sr_err) {
  case LDAP_COMPARE_FALSE:
  case LDAP_COMPARE_TRUE:
    rs->sr_err = LDAP_SUCCESS;
    break;
  }

  if (moi == &opinfo || --moi->moi_ref < 1) {
    int __maybe_unused rc2 = mdbx_txn_reset(moi->moi_txn);
    assert(rc2 == MDBX_SUCCESS);
    if (moi->moi_oe.oe_key)
      LDAP_SLIST_REMOVE(&op->o_extra, &moi->moi_oe, OpExtra, oe_next);
    if ((moi->moi_flag & (MOI_FREEIT | MOI_KEEPER)) == MOI_FREEIT)
      op->o_tmpfree(moi, op->o_tmpmemctx);
  }
  /* free entry */
  if (e != NULL) {
    mdb_entry_return(op, e);
  }

  return rs->sr_err;
}
