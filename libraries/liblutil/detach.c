/* $ReOpenLDAP$ */
/* Copyright 1992-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* This work was originally developed by the University of Michigan
 * and distributed as part of U-MICH LDAP.
 */

#include "reldap.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "lutil.h"

int lutil_detach(int debug, int do_close) {
  int i, sd, nbits, pid;

#ifdef HAVE_SYSCONF
  nbits = sysconf(_SC_OPEN_MAX);
#elif defined(HAVE_GETDTABLESIZE)
  nbits = getdtablesize();
#else
  nbits = FD_SETSIZE;
#endif

#ifdef FD_SETSIZE
  if (nbits > FD_SETSIZE) {
    nbits = FD_SETSIZE;
  }
#endif /* FD_SETSIZE */

  if (debug == 0) {
    for (i = 0; i < 5; i++) {
#ifdef HAVE_THR
      pid = fork1();
#else
      pid = fork();
#endif
      switch (pid) {
      case -1:
        sleep(5);
        continue;

      case 0:
        break;

      default:
        return pid;
      }
      break;
    }

    if ((sd = open("/dev/null", O_RDWR)) == -1 && (sd = open("/dev/null", O_RDONLY)) == -1 &&
        /* Panic -- open *something* */
        (sd = open("/", O_RDONLY)) == -1) {
      perror("/dev/null");
    } else {
      /* redirect stdin, stdout, stderr to /dev/null */
      dup2(sd, STDIN_FILENO);
      dup2(sd, STDOUT_FILENO);
      dup2(sd, STDERR_FILENO);

      switch (sd) {
      default:
        close(sd);
      case STDIN_FILENO:
      case STDOUT_FILENO:
      case STDERR_FILENO:
        break;
      }
    }

    if (do_close) {
      /* close everything else */
      for (i = 0; i < nbits; i++) {
        if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
          close(i);
        }
      }
    }

#ifdef CHDIR_TO_ROOT
    (void)chdir("/");
#endif

#ifdef HAVE_SETSID
    (void)setsid();
#elif defined(TIOCNOTTY)
    if ((sd = open("/dev/tty", O_RDWR)) != -1) {
      (void)ioctl(sd, TIOCNOTTY, NULL);
      (void)close(sd);
    }
#endif
  }

#ifdef SIGPIPE
  (void)SIGNAL(SIGPIPE, SIG_IGN);
#endif
  return 0;
}
