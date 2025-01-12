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

/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "shell.h"

int shell_back_delete(Operation *op, SlapReply *rs) {
  struct shellinfo *si = (struct shellinfo *)op->o_bd->be_private;
  AttributeDescription *entry = slap_schema.si_ad_entry;
  Entry e;
  FILE *rfp, *wfp;

  if (si->si_delete == NULL) {
    send_ldap_error(op, rs, LDAP_UNWILLING_TO_PERFORM, "delete not implemented");
    return (-1);
  }

  e.e_id = NOID;
  e.e_name = op->o_req_dn;
  e.e_nname = op->o_req_ndn;
  e.e_attrs = NULL;
  e.e_ocflags = 0;
  e.e_bv.bv_len = 0;
  e.e_bv.bv_val = NULL;
  e.e_private = NULL;

  if (!access_allowed(op, &e, entry, NULL, ACL_WDEL, NULL)) {
    send_ldap_error(op, rs, LDAP_INSUFFICIENT_ACCESS, NULL);
    return -1;
  }

  if (forkandexec(si->si_delete, &rfp, &wfp) == (pid_t)-1) {
    send_ldap_error(op, rs, LDAP_OTHER, "could not fork/exec");
    return (-1);
  }

  /* write out the request to the delete process */
  fprintf(wfp, "DELETE\n");
  fprintf(wfp, "msgid: %ld\n", (long)op->o_msgid);
  print_suffixes(wfp, op->o_bd);
  fprintf(wfp, "dn: %s\n", op->o_req_dn.bv_val);
  fclose(wfp);

  /* read in the results and send them along */
  read_and_send_results(op, rs, rfp);
  fclose(rfp);
  return (0);
}
