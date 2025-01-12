/* $ReOpenLDAP$ */
/* Copyright 2000-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
#include "back-bdb.h"
#include "idl.h"

/* read a key */
int bdb_key_read(Backend *be, DB *db, DB_TXN *txn, struct berval *k, ID *ids, DBC **saved_cursor, int get_flag) {
  int rc;
  DBT key;

  Debug(LDAP_DEBUG_TRACE, "=> key_read\n");

  DBTzero(&key);
  bv2DBT(k, &key);
  key.ulen = key.size;
  key.flags = DB_DBT_USERMEM;

  rc = bdb_idl_fetch_key(be, db, txn, &key, ids, saved_cursor, get_flag);

  if (rc != LDAP_SUCCESS) {
    Debug(LDAP_DEBUG_TRACE, "<= bdb_index_read: failed (%d)\n", rc);
  } else {
    Debug(LDAP_DEBUG_TRACE, "<= bdb_index_read %ld candidates\n", (long)BDB_IDL_N(ids));
  }

  return rc;
}

/* Add or remove stuff from index files */
int bdb_key_change(Backend *be, DB *db, DB_TXN *txn, struct berval *k, ID id, int op) {
  int rc;
  DBT key;

  Debug(LDAP_DEBUG_TRACE, "=> key_change(%s,%lx)\n", op == SLAP_INDEX_ADD_OP ? "ADD" : "DELETE", (long)id);

  DBTzero(&key);
  bv2DBT(k, &key);
  key.ulen = key.size;
  key.flags = DB_DBT_USERMEM;

  if (op == SLAP_INDEX_ADD_OP) {
    /* Add values */

#ifdef BDB_TOOL_IDL_CACHING
    if (slapMode & SLAP_TOOL_QUICK)
      rc = bdb_tool_idl_add(be, db, txn, &key, id);
    else
#endif
      rc = bdb_idl_insert_key(be, db, txn, &key, id);
    if (rc == DB_KEYEXIST)
      rc = 0;
  } else {
    /* Delete values */
    rc = bdb_idl_delete_key(be, db, txn, &key, id);
    if (rc == DB_NOTFOUND)
      rc = 0;
  }

  Debug(LDAP_DEBUG_TRACE, "<= key_change %d\n", rc);

  return rc;
}
