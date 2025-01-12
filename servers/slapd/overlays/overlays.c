/* $ReOpenLDAP$ */
/* Copyright 2003-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

/* overlays.c - Static overlay framework */

#include "reldap.h"

#include "slap.h"

extern OverlayInit slap_oinfo[];

int overlay_init(void) {
  int i, rc = 0;

  for (i = 0; slap_oinfo[i].ov_type; i++) {
    rc = slap_oinfo[i].ov_init();
    if (rc) {
      Debug(LDAP_DEBUG_ANY, "%s overlay setup failed, err %d\n", slap_oinfo[i].ov_type, rc);
      break;
    }
  }

  return rc;
}
