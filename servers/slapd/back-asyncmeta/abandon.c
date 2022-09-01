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

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"
#include "ldap_rq.h"

/* function is unused */
int asyncmeta_back_abandon(Operation *op, SlapReply *rs) {
  Operation *t_op;

  /* Find the ops being abandoned */
  ldap_pvt_thread_mutex_lock(&op->o_conn->c_mutex);

  LDAP_STAILQ_FOREACH(t_op, &op->o_conn->c_ops, o_next) {
    if (t_op->o_msgid == op->orn_msgid) {
      slap_set_op_abandon(t_op, 1);
    }
  }
  ldap_pvt_thread_mutex_unlock(&op->o_conn->c_mutex);

  return LDAP_SUCCESS;
}
