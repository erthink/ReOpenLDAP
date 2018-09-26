/* $ReOpenLDAP$ */
/* Copyright 1990-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* unbind.c - decode an ldap unbind operation and pass it to a backend db */

#include "reldap.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"

int do_unbind(Operation *op, SlapReply *rs) {
  Debug(LDAP_DEBUG_TRACE, "%s do_unbind\n", op->o_log_prefix);

  /*
   * Parse the unbind request.  It looks like this:
   *
   *	UnBindRequest ::= NULL
   */

  Statslog(LDAP_DEBUG_STATS, "%s UNBIND\n", op->o_log_prefix);

  if (frontendDB->be_unbind) {
    op->o_bd = frontendDB;
    (void)frontendDB->be_unbind(op, rs);
    op->o_bd = NULL;
  }

  /* pass the unbind to all backends */
  (void)backend_unbind(op, rs);

  return 0;
}
