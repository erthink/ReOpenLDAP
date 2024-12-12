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
#include "idl.h"

/* read a key */
int mdb_key_read(Backend *be, MDBX_txn *txn, MDBX_dbi dbi, struct berval *k, ID *ids, MDBX_cursor **saved_cursor,
                 int get_flag) {
  int rc;
  MDBX_val key;
#ifndef MISALIGNED_OK
  int kbuf[2];
#endif

  Debug(LDAP_DEBUG_TRACE, "=> key_read\n");

#ifndef MISALIGNED_OK
  if (k->bv_len & ALIGNER) {
    key.iov_len = sizeof(kbuf);
    key.iov_base = kbuf;
    kbuf[1] = 0;
    memcpy(kbuf, k->bv_val, k->bv_len);
  } else
#endif
  {
    key.iov_len = k->bv_len;
    key.iov_base = k->bv_val;
  }

  rc = mdb_idl_fetch_key(be, txn, dbi, &key, ids, saved_cursor, get_flag);

  if (rc != LDAP_SUCCESS) {
    Debug(LDAP_DEBUG_TRACE, "<= mdb_index_read: failed (%d)\n", rc);
  } else {
    Debug(LDAP_DEBUG_TRACE, "<= mdb_index_read %ld candidates\n", (long)MDB_IDL_N(ids));
  }

  return rc;
}
