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
#include <ac/socket.h>

#include "slap.h"
#include "back-mdb.h"

/*
 * sets *hasSubordinates to LDAP_COMPARE_TRUE/LDAP_COMPARE_FALSE
 * if the entry has children or not.
 */
int mdb_hasSubordinates(Operation *op, Entry *e, int *hasSubordinates) {
  struct mdb_info *mdb = (struct mdb_info *)op->o_bd->be_private;
  MDBX_txn *rtxn;
  mdb_op_info opinfo = {{{0}}}, *moi = &opinfo;
  int rc;

  assert(e != NULL);

  rc = mdb_opinfo_get(op, mdb, 1, &moi);
  switch (rc) {
  case 0:
    break;
  default:
    rc = LDAP_OTHER;
    goto done;
  }

  rtxn = moi->moi_txn;

  rc = mdb_dn2id_children(op, rtxn, e);

  switch (rc) {
  case 0:
    *hasSubordinates = LDAP_COMPARE_TRUE;
    break;

  case MDBX_NOTFOUND:
    *hasSubordinates = LDAP_COMPARE_FALSE;
    rc = LDAP_SUCCESS;
    break;

  default:
    Debug(LDAP_DEBUG_ARGS,
          "<=- " LDAP_XSTRING(
              mdb_hasSubordinates) ": has_children failed: %s (%d)\n",
          mdbx_strerror(rc), rc);
    rc = LDAP_OTHER;
  }

done:;
  if (moi == &opinfo || --moi->moi_ref < 1) {
    int __maybe_unused rc2 = mdbx_txn_reset(moi->moi_txn);
    assert(rc2 == MDBX_SUCCESS);
    if (moi->moi_oe.oe_key)
      LDAP_SLIST_REMOVE(&op->o_extra, &moi->moi_oe, OpExtra, oe_next);
    if ((moi->moi_flag & (MOI_FREEIT | MOI_KEEPER)) == MOI_FREEIT)
      op->o_tmpfree(moi, op->o_tmpmemctx);
  }
  return rc;
}

/*
 * sets the supported operational attributes (if required)
 */
int mdb_operational(Operation *op, SlapReply *rs) {
  Attribute **ap;

  assert(rs->sr_entry != NULL);

  for (ap = &rs->sr_operational_attrs; *ap; ap = &(*ap)->a_next) {
    if ((*ap)->a_desc == slap_schema.si_ad_hasSubordinates) {
      break;
    }
  }

  if (*ap == NULL &&
      attr_find(rs->sr_entry->e_attrs, slap_schema.si_ad_hasSubordinates) ==
          NULL &&
      (SLAP_OPATTRS(rs->sr_attr_flags) ||
       ad_inlist(slap_schema.si_ad_hasSubordinates, rs->sr_attrs))) {
    int hasSubordinates, rc;

    rc = mdb_hasSubordinates(op, rs->sr_entry, &hasSubordinates);
    if (rc == LDAP_SUCCESS) {
      *ap =
          slap_operational_hasSubordinate(hasSubordinates == LDAP_COMPARE_TRUE);
      assert(*ap != NULL);

      ap = &(*ap)->a_next;
    }
  }

  return LDAP_SUCCESS;
}
