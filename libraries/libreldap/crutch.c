/* $ReOpenLDAP$ */
/* Copyright 2015-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "lber_hipagut.h"

/* LY: segfault GCC 4.8.5 on RHEL7,
 * https://bugzilla.redhat.com/show_bug.cgi?id=1342862 */
#if __GNUC_PREREQ(4, 5) && 0
/* LY: some magic for ability to build with LTO
 * under modern Fedora/Ubuntu and then run under RHEL6. */
#pragma GCC optimize("-fno-lto")
#endif

#undef memcpy

/*----------------------------------------------------------------------------*/

#if defined(PS_COMPAT_RHEL6)

#if __GLIBC_PREREQ(2, 23) && (defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__))
/* LY: some magic for ability to build with ASAN/TSAN
 * under modern Fedora/Ubuntu and then run under RHEL6. */
LDAP_V(int) signgam;
__attribute__((weak)) int signgam;
#endif /* __GLIBC_PREREQ(2,23) */

#if __GLIBC_PREREQ(2, 14)
/* LY: some magic for ability to build with LTO
 * under modern Fedora/Ubuntu and then run under RHEL6. */

__asm__(".symver memcpy_compat, memcpy@@@GLIBC_2.2.5");
LDAP_V(void *) memcpy_compat(void *dest, const void *src, size_t n);

__asm__(".globl memcpy");
#if LDAP_SAFEMEMCPY
__asm__(".globl ber_memcpy_safe");
__asm__(".set memcpy,ber_memcpy_safe");
#else
__asm__(".globl memcpy_compat");
__asm__(".set memcpy,memcpy_compat");
#endif
#define memcpy memcpy_compat
#endif /* __GLIBC_PREREQ(2,14) */

#endif /* PS_COMPAT_RHEL6 */

/*----------------------------------------------------------------------------*/

__hot __attribute__((__used__)) void *ber_memcpy_safe(void *dest, const void *src, size_t n) {
  long diff = (char *)dest - (char *)src;

  if (unlikely(n > (size_t)__builtin_labs(diff))) {
    if (unlikely(src == dest))
      return dest;
    if (reopenldap_mode_check())
      __ldap_assert_fail("source and destination MUST NOT overlap", __FILE__, __LINE__, __FUNCTION__);
    return memmove(dest, src, n);
  }

  return memcpy(dest, src, n);
}

__cold __attribute__((weak)) __reldap_exportable void __ldap_assert_fail(const char *assertion, const char *file,
                                                                         unsigned line, const char *function) {
  __assert_fail(assertion, file, line, function);
  abort();
}
