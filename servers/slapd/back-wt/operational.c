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

int wt_hasSubordinates(Operation *op, Entry *e, int *hasSubordinates) {
  struct wt_info *wi = (struct wt_info *)op->o_bd->be_private;
  wt_ctx *wc = NULL;
  int rc;

  assert(e != NULL);

  wc = wt_ctx_get(op, wi);
  if (!wc) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(wt_hasSubordinates) ": wt_ctx_get failed\n");
    return LDAP_OTHER;
  }

  rc = wt_dn2id_has_children(op, wc, e->e_id);
  switch (rc) {
  case 0:
    *hasSubordinates = LDAP_COMPARE_TRUE;
    break;
  case WT_NOTFOUND:
    *hasSubordinates = LDAP_COMPARE_FALSE;
    rc = LDAP_SUCCESS;
    break;
  default:
    Debug(LDAP_DEBUG_ANY,
          "<=- " LDAP_XSTRING(
              wt_hasSubordinates) ": has_children failed: %s (%d)\n",
          wiredtiger_strerror(rc), rc);
    rc = LDAP_OTHER;
  }
  return rc;
}

/*
 * sets the supported operational attributes (if required)
 */
int wt_operational(Operation *op, SlapReply *rs) {
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

    rc = wt_hasSubordinates(op, rs->sr_entry, &hasSubordinates);
    if (rc == LDAP_SUCCESS) {
      *ap =
          slap_operational_hasSubordinate(hasSubordinates == LDAP_COMPARE_TRUE);
      assert(*ap != NULL);

      ap = &(*ap)->a_next;
    }
  }

  return LDAP_SUCCESS;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
