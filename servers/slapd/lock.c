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

/* lock.c - routines to open and apply an advisory lock to a file */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/time.h>
#include <ac/unistd.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "slap.h"
#include <lutil.h>

FILE *lock_fopen(const char *fname, const char *type, FILE **lfp) {
  FILE *fp;
  char buf[MAXPATHLEN];

  /* open the lock file */
  snprintf(buf, sizeof buf, "%s.lock", fname);

  if ((*lfp = fopen(buf, "w")) == NULL) {
    Debug(LDAP_DEBUG_ANY, "could not open \"%s\"\n", buf);

    return (NULL);
  }

  /* acquire the lock */
  ldap_lockf(fileno(*lfp));

  /* open the log file */
  if ((fp = fopen(fname, type)) == NULL) {
    Debug(LDAP_DEBUG_ANY, "could not open \"%s\"\n", fname);

    ldap_unlockf(fileno(*lfp));
    fclose(*lfp);
    *lfp = NULL;
    return (NULL);
  }

  return (fp);
}

int lock_fclose(FILE *fp, FILE *lfp) {
  int rc = fclose(fp);
  /* unlock */
  ldap_unlockf(fileno(lfp));
  fclose(lfp);

  return (rc);
}
