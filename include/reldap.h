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

#ifndef _LDAP_REOPEN_H
#define _LDAP_REOPEN_H

#include "../reldap_autoconf.h"
#include "ldap_cdefs.h"

#ifndef __maybe_unused
#ifdef ATTRIBUTE_UNUSED
#define __maybe_unused ATTRIBUTE_UNUSED
#elif __has_attribute(__unused__)
#define __maybe_unused __attribute__((__unused__))
#elif __has_attribute(unused)
#define __maybe_unused __attribute__((unused))
#else
#define __maybe_unused __attribute__((__unused__))
#endif
#endif /* __maybe_unused */

#if !defined(__thread) && (defined(_MSC_VER) || defined(__DMC__))
#define __thread __declspec(thread)
#endif

#ifndef __forceinline
#if defined(__GNUC__) || defined(__clang__)
#define __forceinline __inline __attribute__((always_inline))
#elif !defined(_MSC_VER)
#define __forceinline
#endif
#endif /* __forceinline */

#ifndef __noinline
#if defined(__GNUC__) || defined(__clang__)
#define __noinline __attribute__((noinline))
#elif defined(_MSC_VER)
#define __noinline __declspec(noinline)
#endif
#endif /* __noinline */

#ifndef __must_check_result
#if defined(__GNUC__) || defined(__clang__)
#define __must_check_result __attribute__((warn_unused_result))
#else
#define __must_check_result
#endif
#endif /* __must_check_result */

#ifndef __optimize
#if defined(__OPTIMIZE__)
#if defined(__clang__) && !__has_attribute(optimize)
#define __optimize(ops)
#elif defined(__GNUC__) || __has_attribute(optimize)
#define __optimize(ops) __attribute__((optimize(ops)))
#else
#define __optimize(ops)
#endif
#else
#define __optimize(ops)
#endif
#endif /* __optimize */

#ifndef __hot
#if defined(__OPTIMIZE__)
#if defined(__e2k__)
#define __hot __attribute__((hot)) __optimize(3)
#elif defined(__clang__) && !__has_attribute(hot)
/* just put frequently used functions in separate section */
#define __hot __attribute__((section("text.hot"))) __optimize("O3")
#elif defined(__GNUC__) || __has_attribute(hot)
#define __hot __attribute__((hot)) __optimize("O3")
#else
#define __hot __optimize("O3")
#endif
#else
#define __hot
#endif
#endif /* __hot */

#ifndef __cold
#if defined(__OPTIMIZE__)
#if defined(__e2k__)
#define __cold __attribute__((cold)) __optimize(1)
#elif defined(__clang__) && !__has_attribute(cold)
/* just put infrequently used functions in separate section */
#define __cold __attribute__((section("text.unlikely"))) __optimize("Os")
#elif defined(__GNUC__) || __has_attribute(cold)
#define __cold __attribute__((cold)) __optimize("Os")
#else
#define __cold __optimize("Os")
#endif
#else
#define __cold
#endif
#endif /* __cold */

#ifndef __flatten
#if defined(__GNUC__) || defined(__clang__)
#define __flatten __attribute__((flatten))
#else
#define __flatten
#endif
#endif /* __flatten */

#ifndef __aligned
#if defined(__GNUC__) || defined(__clang__)
#define __aligned(N) __attribute__((aligned(N)))
#elif defined(__MSC_VER)
#define __aligned(N) __declspec(align(N))
#else
#define __aligned(N)
#endif
#endif /* __align */

#ifndef __noreturn
#if defined(__GNUC__) || defined(__clang__)
#define __noreturn __attribute__((noreturn))
#elif defined(__MSC_VER)
#define __noreturn __declspec(noreturn)
#else
#define __noreturn
#endif
#endif /* __noreturn */

#ifndef __nothrow
#if defined(__GNUC__) || defined(__clang__)
#define __nothrow __attribute__((nothrow))
#elif defined(__MSC_VER)
#define __nothrow __declspec(nothrow)
#else
#define __nothrow
#endif
#endif /* __nothrow */

#ifndef CACHELINE_SIZE
#if defined(__ia64__) || defined(__ia64) || defined(_M_IA64)
#define CACHELINE_SIZE 128
#else
#define CACHELINE_SIZE 64
#endif
#endif

#ifndef __cache_aligned
#define __cache_aligned __aligned(CACHELINE_SIZE)
#endif

#ifndef likely
#if defined(__GNUC__) || defined(__clang__)
#ifdef __cplusplus
/* LY: workaround for "pretty" boost */
static __inline __attribute__((always_inline)) bool likely(bool cond) { return __builtin_expect(cond, 1); }
#else
#define likely(cond) __builtin_expect(!!(cond), 1)
#endif
#else
#define likely(x) (x)
#endif
#endif /* likely */

#ifndef unlikely
#if defined(__GNUC__) || defined(__clang__)
#ifdef __cplusplus
/* LY: workaround for "pretty" boost */
static __inline __attribute__((always_inline)) bool unlikely(bool cond) { return __builtin_expect(cond, 0); }
#else
#define unlikely(cond) __builtin_expect(!!(cond), 0)
#endif
#else
#define unlikely(x) (x)
#endif
#endif /* unlikely */

#ifndef __ldso_exportable
#if defined(__GNUC__)
#if !defined(__clang__)
#define __ldso_exportable __attribute__((visibility("default"), externally_visible))
#else
#define __ldso_exportable __attribute__((visibility("default")))
#endif
#else
#define __ldso_exportable
#endif
#endif /* __ldso_exportable */

#ifndef __extern_C
#ifdef __cplusplus
#define __extern_C extern "C"
#else
#define __extern_C
#endif
#endif

#ifndef __noop
#define __noop()                                                                                                       \
  do {                                                                                                                 \
  } while (0)
#endif

#ifndef compiler_barrier
#define compiler_barrier()                                                                                             \
  do {                                                                                                                 \
    __asm__ __volatile__("" ::: "memory");                                                                             \
  } while (0)
#endif /* compiler_barrier */

/* -------------------------------------------------------------------------- */

#ifndef LDAP_EXPERIMENTAL
#define LDAP_EXPERIMENTAL 0
#endif

#ifndef LDAP_CHECK
#ifdef LDAP_DEBUG
#define LDAP_CHECK 1
#else
#define LDAP_CHECK 0
#endif
#endif

#include "ldap_cdefs.h"
#ifndef SKIP_LDAP_AC_INCLUDES
#include "ldap_features.h"
#include "ldap_dirs.h"
#endif /* SKIP_LDAP_AC_INCLUDES */

LDAP_BEGIN_DECL

LDAP_F(void)
__ldap_assert_fail(const char *assertion, const char *file, unsigned line, const char *function) __nothrow __noreturn;

/* Prototype should match libc runtime. ISO POSIX (2003) & LSB 3.1 */
__extern_C void __assert_fail(const char *assertion, const char *file, unsigned line,
                              const char *function) __nothrow __noreturn;

#ifndef LDAP_ASSERT_CHECK
#define LDAP_ASSERT_CHECK LDAP_CHECK
#endif

/* LY: ReOpenLDAP operation mode global flags */
#define REOPENLDAP_FLAG_IDDQD 1
#define REOPENLDAP_FLAG_IDKFA 2
#define REOPENLDAP_FLAG_IDCLIP 4
#define REOPENLDAP_FLAG_JITTER 8

LDAP_LUTIL_F(void) reopenldap_flags_setup(int flags);
LDAP_LUTIL_V(int) reopenldap_flags;

#define reopenldap_mode_righteous() likely((reopenldap_flags & REOPENLDAP_FLAG_IDDQD) != 0)
#define reopenldap_mode_check() unlikely(LDAP_CHECK > 2 || (reopenldap_flags & REOPENLDAP_FLAG_IDKFA) != 0)
#define reopenldap_mode_strict() likely((reopenldap_flags & REOPENLDAP_FLAG_IDCLIP) != 0)
#define reopenldap_mode_jitter() unlikely((reopenldap_flags & REOPENLDAP_FLAG_JITTER) != 0)

LDAP_LUTIL_F(void) reopenldap_jitter(int probability_percent);

#define LDAP_JITTER(prob)                                                                                              \
  do                                                                                                                   \
    if (reopenldap_mode_jitter())                                                                                      \
      reopenldap_jitter(prob);                                                                                         \
  while (0)

#define LDAP_ENSURE(condition)                                                                                         \
  do                                                                                                                   \
    if (unlikely(!(condition)))                                                                                        \
      __ldap_assert_fail("ldap: " #condition, __FILE__, __LINE__, __FUNCTION__);                                       \
  while (0)

#if !LDAP_ASSERT_CHECK
#define LDAP_ASSERT(condition) __noop()
#elif LDAP_ASSERT_CHECK > 1
#define LDAP_ASSERT(condition) LDAP_ENSURE(condition)
#else
#define LDAP_ASSERT(condition)                                                                                         \
  do                                                                                                                   \
    if (reopenldap_mode_check())                                                                                       \
      LDAP_ENSURE(condition);                                                                                          \
  while (0)
#endif /* LDAP_ASSERT_CHECK */

#define LDAP_BUG() __ldap_assert_fail("ldap-BUG", __FILE__, __LINE__, __FUNCTION__)

#undef assert
#define assert(expr) LDAP_ASSERT(expr)

LDAP_END_DECL

/* -------------------------------------------------------------------------- */

#include <stdint.h>
#include <time.h>

LDAP_BEGIN_DECL

/* LY: to avoid include <sys/time.h> here */
struct timeval;

LDAP_F(time_t) ldap_time_steady(void);
LDAP_F(time_t) ldap_time_unsteady(void);
LDAP_F(void) ldap_timespec_steady(struct timespec *);
LDAP_F(unsigned) ldap_timeval_realtime(struct timeval *);
LDAP_F(uint64_t) ldap_now_steady_ns(void);

typedef struct {
  uint64_t ns;
} slap_time_t;

#define ldap_now_steady(void)                                                                                          \
  ({                                                                                                                   \
    slap_time_t __t = {ldap_now_steady_ns()};                                                                          \
    __t;                                                                                                               \
  })

#define ldap_from_seconds(S)                                                                                           \
  ({                                                                                                                   \
    slap_time_t __t = {(S) * (uint64_t)1000ul * 1000ul * 1000ul};                                                      \
    __t;                                                                                                               \
  })

#define ldap_from_timeval(T)                                                                                           \
  ({                                                                                                                   \
    slap_time_t __t = {(T)->tv_sec * (uint64_t)1000ul * 1000ul * 1000ul + (T)->tv_usec * (uint64_t)1000ul};            \
    __t;                                                                                                               \
  })

#define ldap_from_timespec(T)                                                                                          \
  ({                                                                                                                   \
    slap_time_t __t = {(T)->tv_sec * (uint64_t)1000ul * 1000ul * 1000ul + (T)->tv_nsec};                               \
    __t;                                                                                                               \
  })

#define ldap_to_milliseconds(TS) ((TS).ns / 1000000ul)
#define ldap_to_seconds(TS) ((TS).ns / 1000000000ull)
#define ldap_to_time(TS) ldap_to_seconds(TS)

LDAP_END_DECL

/* -------------------------------------------------------------------------- */

#if defined(HAVE_VALGRIND) || defined(USE_VALGRIND)
/* Get debugging help from Valgrind */
#include <valgrind/memcheck.h>
#ifndef VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE
/* LY: available since Valgrind 3.10 */
#define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a, s)
#define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a, s)
#endif
#else
#define VALGRIND_CREATE_MEMPOOL(h, r, z)
#define VALGRIND_DESTROY_MEMPOOL(h)
#define VALGRIND_MEMPOOL_TRIM(h, a, s)
#define VALGRIND_MEMPOOL_ALLOC(h, a, s)
#define VALGRIND_MEMPOOL_FREE(h, a)
#define VALGRIND_MEMPOOL_CHANGE(h, a, b, s)
#define VALGRIND_MAKE_MEM_NOACCESS(a, s)
#define VALGRIND_MAKE_MEM_DEFINED(a, s)
#define VALGRIND_MAKE_MEM_UNDEFINED(a, s)
#define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a, s)
#define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a, s)
#define VALGRIND_CHECK_MEM_IS_ADDRESSABLE(a, s) (0)
#define VALGRIND_CHECK_MEM_IS_DEFINED(a, s) (0)
#endif /* ! USE_VALGRIND */

#if __has_feature(thread_sanitizer)
#define __SANITIZE_THREAD__ 1
#endif

#if __has_feature(address_sanitizer)
#define __SANITIZE_ADDRESS__ 1
#endif

#ifdef __SANITIZE_THREAD__
#define ATTRIBUTE_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread, noinline))
#define ATTRIBUTE_NO_SANITIZE_THREAD_INLINE ATTRIBUTE_NO_SANITIZE_THREAD
#else
#define ATTRIBUTE_NO_SANITIZE_THREAD
#define ATTRIBUTE_NO_SANITIZE_THREAD_INLINE __inline
#endif

#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address, noinline))
#define ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE ATTRIBUTE_NO_SANITIZE_ADDRESS
#define ASAN_REGION_IS_POISONED(addr, size) __asan_region_is_poisoned(addr, size)
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#define ASAN_REGION_IS_POISONED(addr, size) ((void)(addr), (void)(size), 0)
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#define ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE __inline
#endif /* __SANITIZE_ADDRESS__ */

#endif /* _LDAP_REOPEN_H */
