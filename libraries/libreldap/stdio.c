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

#include "reldap.h"

#include <stdio.h>
#include <unistd.h>
#include <ac/stdarg.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <lutil.h>

#ifndef HAVE_VSNPRINTF
/* Write at most n characters to the buffer in str, return the
 * number of chars written or -1 if the buffer would have been
 * overflowed.
 *
 * This is portable to any POSIX-compliant system. We use pipe()
 * to create a valid file descriptor, and then fdopen() it to get
 * a valid FILE pointer. The user's buffer and size are assigned
 * to the FILE pointer using setvbuf. Then we close the read side
 * of the pipe to invalidate the descriptor.
 *
 * If the write arguments all fit into size n, the write will
 * return successfully. If the write is too large, the stdio
 * buffer will need to be flushed to the underlying file descriptor.
 * The flush will fail because it is attempting to write to a
 * broken pipe, and the write will be terminated.
 * -- hyc, 2002-07-19
 */
/* This emulation uses vfprintf; on OS/390 we're also emulating
 * that function so it's more efficient just to have a separate
 * version of vsnprintf there.
 */
#include <ac/signal.h>
int ber_pvt_vsnprintf(char *str, size_t n, const char *fmt, va_list ap) {
  int fds[2], res;
  FILE *f;
  RETSIGTYPE (*sig)();

  if (pipe(fds))
    return -1;

  f = fdopen(fds[1], "w");
  if (!f) {
    close(fds[1]);
    close(fds[0]);
    return -1;
  }
  setvbuf(f, str, _IOFBF, n);
  sig = signal(SIGPIPE, SIG_IGN);
  close(fds[0]);

  res = vfprintf(f, fmt, ap);

  fclose(f);
  signal(SIGPIPE, sig);
  if (res > 0 && res < n) {
    res = vsprintf(str, fmt, ap);
  }
  return res;
}
#endif /* !HAVE_VSNPRINTF */

#ifndef HAVE_SNPRINTF
int ber_pvt_snprintf(char *str, size_t n, const char *fmt, ...) {
  va_list ap;
  int res;

  va_start(ap, fmt);
  res = vsnprintf(str, n, fmt, ap);
  va_end(ap);
  return res;
}
#endif /* !HAVE_SNPRINTF */
