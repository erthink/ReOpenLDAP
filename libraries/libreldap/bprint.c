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

#include <ac/ctype.h>
#include <ac/stdarg.h>
#include <ac/string.h>

#include "lber-int.h"

#define ber_log_check(errlvl, loglvl) ((errlvl) & (loglvl))

BER_LOG_FN ber_int_log_proc = NULL;

/*
 * We don't just set ber_pvt_err_file to stderr here, because in NT,
 * stderr is a symbol imported from a DLL. As such, the compiler
 * doesn't recognize the symbol as having a constant address. Thus
 * we set ber_pvt_err_file to stderr later, when it first gets
 * referenced.
 */
FILE *ber_pvt_err_file = NULL;

/*
 * ber errno
 */
BER_ERRNO_FN ber_int_errno_fn = NULL;

int *ber_errno_addr(void) {
  static int ber_int_errno = LBER_ERROR_NONE;

  if (ber_int_errno_fn) {
    return (*ber_int_errno_fn)();
  }

  return &ber_int_errno;
}

/*
 * Print stuff
 */
void __attribute__((weak)) ber_debug_print(const char *data) {
  assert(data != NULL);

  if (!ber_pvt_err_file)
    ber_pvt_err_file = stderr;

  fputs(data, ber_pvt_err_file);

  /* Print to both streams */
  if (ber_pvt_err_file != stderr) {
    fputs(data, stderr);
    fflush(stderr);
  }

  fflush(ber_pvt_err_file);
}

BER_LOG_PRINT_FN ber_pvt_log_print = ber_debug_print;

int ber_pvt_log_printf(int errlvl, int loglvl, const char *fmt, ...) {
  char buf[1024];
  va_list ap;

  assert(fmt != NULL);

  if (!ber_log_check(errlvl, loglvl)) {
    return 0;
  }

  va_start(ap, fmt);

  buf[sizeof(buf) - 1] = '\0';
  vsnprintf(buf, sizeof(buf) - 1, fmt, ap);

  va_end(ap);

  (*ber_pvt_log_print)(buf);
  return 1;
}

/*
 * Print arbitrary stuff, for debugging.
 */

int ber_log_bprint(int errlvl, int loglvl, const char *data, ber_len_t len) {
  assert(data != NULL);

  if (!ber_log_check(errlvl, loglvl)) {
    return 0;
  }

  ber_bprint(data, len);
  return 1;
}

void ber_bprint(const char *data, ber_len_t len) {
  static const char hexdig[] = "0123456789abcdef";
#define BP_OFFSET 9
#define BP_GRAPH 60
#define BP_LEN 80
  char line[BP_LEN];
  ber_len_t i;

  assert(data != NULL);

  /* in case len is zero */
  line[0] = '\n';
  line[1] = '\0';

  for (i = 0; i < len; i++) {
    int n = i % 16;
    unsigned off;

    if (!n) {
      if (i)
        (*ber_pvt_log_print)(line);
      memset(line, ' ', sizeof(line) - 2);
      line[sizeof(line) - 2] = '\n';
      line[sizeof(line) - 1] = '\0';

      off = i % 0x0ffffU;

      line[2] = hexdig[0x0f & (off >> 12)];
      line[3] = hexdig[0x0f & (off >> 8)];
      line[4] = hexdig[0x0f & (off >> 4)];
      line[5] = hexdig[0x0f & off];
      line[6] = ':';
    }

    off = BP_OFFSET + n * 3 + ((n >= 8) ? 1 : 0);
    line[off] = hexdig[0x0f & (data[i] >> 4)];
    line[off + 1] = hexdig[0x0f & data[i]];

    off = BP_GRAPH + n + ((n >= 8) ? 1 : 0);

    if (isprint((unsigned char)data[i])) {
      line[BP_GRAPH + n] = data[i];
    } else {
      line[BP_GRAPH + n] = '.';
    }
  }

  (*ber_pvt_log_print)(line);
}

int ber_log_dump(int errlvl, int loglvl, BerElement *ber, int inout) {
  assert(ber != NULL);
  assert(LBER_VALID(ber));

  if (!ber_log_check(errlvl, loglvl)) {
    return 0;
  }

  ber_dump(ber, inout);
  return 1;
}

void ber_dump(BerElement *ber, int inout) {
  char buf[132];
  ber_len_t len;

  assert(ber != NULL);
  assert(LBER_VALID(ber));

  if (inout == 1) {
    len = ber_pvt_ber_remaining(ber);
  } else {
    len = ber_pvt_ber_write(ber);
  }

  sprintf(buf, "ber_dump: buf=%p ptr=%p end=%p len=%ld\n", ber->ber_buf,
          ber->ber_ptr, ber->ber_end, (long)len);

  (void)(*ber_pvt_log_print)(buf);

  ber_bprint(ber->ber_ptr, len);
}

typedef struct seqorset Seqorset;

/* Exists for binary compatibility with OpenLDAP 2.4.17-- */
int ber_log_sos_dump(int errlvl, int loglvl, Seqorset *sos) { return 0; }

/* Exists for binary compatibility with OpenLDAP 2.4.17-- */
void ber_sos_dump(Seqorset *sos) {}
