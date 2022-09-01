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

#ifdef __SANITIZE_THREAD__
ATTRIBUTE_NO_SANITIZE_THREAD
ID mdb_read_nextid(struct mdb_info *mdb) { return mdb->_mi_nextid; }
#endif

int mdb_next_id(BackendDB *be, MDBX_cursor *mc, ID *out) {
  struct mdb_info *mdb = (struct mdb_info *)be->be_private;
  int rc;
  ID id = 0;
  MDBX_val key;

  rc = mdbx_cursor_get(mc, &key, NULL, MDBX_LAST);

  switch (rc) {
  case MDBX_NOTFOUND:
    rc = 0;
    *out = 1;
    break;
  case 0:
    memcpy(&id, key.iov_base, sizeof(id));
    *out = ++id;
    break;

  default:
    Debug(LDAP_DEBUG_ANY, "=> mdb_next_id: get failed: %s (%d)\n",
          mdbx_strerror(rc), rc);
    goto done;
  }
  mdb->_mi_nextid = *out;

done:
  return rc;
}
