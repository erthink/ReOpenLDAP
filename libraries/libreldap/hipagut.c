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

/*
 * Memory checker and other utils for bug detection (e.g. jitter).
 *
 * Imported from 1Hippeus project at 2015-01-19.
 * Copyright (c) 2010-2014 Leonid Yuriev <leo@yuriev.ru>.
 */

#include "reldap.h"

#include "ac/time.h"
#include "ac/unistd.h"
#include "ac/string.h"
#include "ac/stdlib.h"
#include <stdint.h>
#include <sched.h>
#include <pthread.h>

#include "lber_hipagut.h"
#include "ldap_log.h"
#include "ldap-int.h"

/* LY: only for self debugging
#include <stdio.h> */

#if !defined(UNALIGNED_OK) /* FIXME: detection by configure/cmake */
#if defined(__i386) || defined(__x86_64__) || defined(_M_IX86) ||              \
    defined(_M_X64) || defined(i386) || defined(_X86_) || defined(__i386__) || \
    defined(_X86_64_)
#define UNALIGNED_OK 1
#else
#define UNALIGNED_OK 0
#endif
#endif

/* -------------------------------------------------------------------------- */

static __inline
#ifdef __SANITIZE_ADDRESS__
    __attribute__((no_sanitize_address))
#endif
    __attribute__((always_inline)) __maybe_unused uint64_t
    unaligned_load_noasan(const volatile void *ptr) {
#if UNALIGNED_OK
  return *(const volatile uint64_t *)ptr;
#else
  uint64_t local;
#if defined(__GNUC__) || defined(__clang__)
  __builtin_memcpy(&local, (const void *)ptr, 8);
#else
  memcpy(&local, (const void *)ptr, 8);
#endif /* __GNUC__ || __clang__ */
  return local;
#endif /* arch selector */
}

static __forceinline __maybe_unused uint64_t
unaligned_load(const volatile void *ptr) {
#if UNALIGNED_OK
  return *(const volatile uint64_t *)ptr;
#else
  uint64_t local;
#if defined(__GNUC__) || defined(__clang__)
  __builtin_memcpy(&local, (const void *)ptr, 8);
#else
  memcpy(&local, (const void *)ptr, 8);
#endif /* __GNUC__ || __clang__ */
  return local;
#endif /* arch selector */
}

static __forceinline __maybe_unused void unaligned_store(volatile void *ptr,
                                                         uint64_t value) {
#if UNALIGNED_OK
  *(volatile uint64_t *)ptr = value;
#else
#if defined(__GNUC__) || defined(__clang__)
  __builtin_memcpy((void *)ptr, &value, 8);
#else
  memcpy((void *)ptr, &value, 8);
#endif /* __GNUC__ || __clang__ */
#endif /* arch selector */
}

#if defined(__GNUC__) || defined(__clang__)
/* LY: noting needed */
#elif defined(_MSC_VER)
#define __sync_fetch_and_add(p, v) _InterlockedExchangeAdd(p, v)
#define __sync_fetch_and_sub(p, v) _InterlockedExchangeAdd(p, -(v))
#elif defined(__APPLE__)
#include <libkern/OSAtomic.h>
static size_t __sync_fetch_and_add(volatile size_t *p, size_t v) {
  switch (sizeof(size_t)) {
  case 4:
    return OSAtomicAdd32(v, (volatile int32_t *)p);
  case 8:
    return OSAtomicAdd64(v, (volatile int64_t *)p);
  default: {
    size_t snap = *p;
    *p += v;
    return snap;
  }
  }
#define __sync_fetch_and_sub(p, v) __sync_fetch_and_add(p, -(v))
#else
static size_t __sync_fetch_and_add(volatile size_t *p, size_t v) {
  size_t snap = *p;
  *p += v;
  return snap;
}
#define __sync_fetch_and_sub(p, v) __sync_fetch_and_add(p, -(v))
#endif

/* -------------------------------------------------------------------------- */

static __forceinline uint64_t entropy_ticks() {
#if defined(__GNUC__) || defined(__clang__)
#if defined(__ia64__)
  uint64_t ticks;
  __asm("mov %0=ar.itc" : "=r"(ticks));
  return ticks;
#elif defined(__hppa__)
  uint64_t ticks;
  __asm("mfctl 16, %0" : "=r"(ticks));
  return ticks;
#elif defined(__s390__)
  uint64_t ticks;
  __asm("stck 0(%0)" : : "a"(&(ticks)) : "memory", "cc");
  return ticks;
#elif defined(__alpha__)
  uint64_t ticks;
  __asm("rpcc %0" : "=r"(ticks));
  return ticks;
#elif defined(__sparc_v9__)
  uint64_t ticks;
  __asm("rd %%tick, %0" : "=r"(ticks));
  return ticks;
#elif defined(__powerpc64__) || defined(__ppc64__)
  uint64_t ticks;
  __asm("mfspr %0, 268" : "=r"(ticks));
  return ticks;
#elif defined(__ppc__) || defined(__powerpc__)
  unsigned tbl, tbu;

  /* LY: Here not a problem if a high-part (tbu)
   * would been updated during reading. */
  __asm("mftb %0" : "=r"(tbl));
  __asm("mftbu %0" : "=r"(tbu));

  return (((uin64_t)tbu0) << 32) | tbl;
/* #elif defined(__mips__)
        return hippeus_mips_rdtsc(); */
#elif defined(__x86_64__) || defined(__i386__)
  unsigned lo, hi;

  /* LY: Using the "a" and "d" constraints is important for correct code. */
  __asm("rdtsc" : "=a"(lo), "=d"(hi));

  return (((uint64_t)hi) << 32) + lo;
#endif /* arch selector */
#endif /* __GNUC__ || __clang__ */

  struct timespec ts;
#if defined(CLOCK_MONOTONIC_COARSE)
  clockid_t clock = CLOCK_MONOTONIC_COARSE;
#elif defined(CLOCK_MONOTONIC_RAW)
  clockid_t clock = CLOCK_MONOTONIC_RAW;
#else
    clockid_t clock = CLOCK_MONOTONIC;
#endif
  LDAP_ENSURE(clock_gettime(clock, &ts) == 0);

  return (((uint64_t)ts.tv_sec) << 32) + ts.tv_nsec;
}

/* -------------------------------------------------------------------------- */

static __forceinline unsigned canary() {
  uint64_t ticks = entropy_ticks();
  return ticks ^ (ticks >> 32);
}

#if LDAP_MEMORY_DEBUG > 0

#if LDAP_MEMORY_DEBUG == 1
unsigned lber_hug_nasty_disabled;
#endif /* LDAP_MEMORY_DEBUG == 1 */

static __forceinline uint32_t linear_congruential(uint32_t value) {
  return value * 1664525u + 1013904223u;
}

/*! Calculate magic mesh from a variable chirp and the constant salt n42 */
static __inline uint32_t mixup(unsigned chirp, const unsigned n42) {
  uint64_t caramba;

  /* LY: just multiplication by a prime number. */
  if (sizeof(unsigned long) > 4)
    caramba = 1445174711lu * (unsigned long)(chirp + n42);
  else {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
    __asm("mull %2"
          : "=A"(caramba)
          : "a"(1445174711u), "r"(chirp + n42)
          : "cc");
#else
    caramba = ((uint64_t)chirp + n42) * 1445174711u;
#endif
  }

  /* LY: significally less collisions when only the few bits damaged. */
  return caramba ^ (caramba >> 32);
}

static __inline int fairly(uint32_t value) {
  if (unlikely((value >= 0xFFFF0000u) || (value <= 0x0000FFFFu)))
    return 0;

  return 1;
}

__hot __flatten void lber_hug_setup(lber_hug_t *self, const unsigned n42) {
  uint32_t chirp = canary();
  for (;;) {
    if (likely(fairly(chirp))) {
      uint32_t sign = mixup(chirp, n42);
      if (likely(fairly(sign))) {
        uint64_t tale = chirp + (((uint64_t)sign) << 32);
        /* fprintf(stderr, "hipagut_setup: ptr %p | n42 %08x, "
                        "tale %016lx, chirp %08x, sign %08x ? %08x\n",
                        self, n42, tale, chirp, sign, mixup(chirp, n42)); */
        unaligned_store(self->opaque, tale);
        break;
      }
    }
    chirp = linear_congruential(chirp);
  }
}

__hot __flatten ATTRIBUTE_NO_SANITIZE_ADDRESS int
lber_hug_probe(const lber_hug_t *self, const unsigned n42) {
  if (unlikely(lber_hug_nasty_disabled == LBER_HUG_DISABLED))
    return 0;

  uint64_t tale = unaligned_load_noasan(self->opaque);
  uint32_t chirp = tale;
  uint32_t sign = tale >> 32;
  /* fprintf(stderr, "hipagut_probe: ptr %p | n42 %08x, "
                  "tale %016lx, chirp %08x, sign %08x ? %08x\n",
                  self, n42, tale, chirp, sign, mixup(chirp, n42)); */
  return likely(fairly(chirp) && fairly(sign) && sign == mixup(chirp, n42))
             ? 0
             : -1;
}

__hot __flatten void lber_hug_drown(lber_hug_t *gizmo) {
  /* LY: This notable value would always bite,
   * because (chirp == 0) != (sign == 0). */
  unaligned_store(gizmo->opaque, 0xDEADB0D1Ful);
}

void lber_hug_setup_link(lber_hug_t *slave, const lber_hug_t *master) {
  lber_hug_setup(slave, unaligned_load(master->opaque) >> 32);
}

void lber_hug_drown_link(lber_hug_t *slave, const lber_hug_t *master) {
  lber_hug_drown(slave);
}

int lber_hug_probe_link(const lber_hug_t *slave, const lber_hug_t *master) {
  return lber_hug_probe(slave, unaligned_load(master->opaque) >> 32);
}

#if LDAP_MEMORY_DEBUG > 2
struct _lber_hug_memchk_info lber_hug_memchk_info __cache_aligned;
#endif /* LDAP_MEMORY_DEBUG > 2 */

unsigned lber_hug_memchk_poison_alloc;
unsigned lber_hug_memchk_poison_free;

#define MEMCHK_TAG_HEADER header
#define MEMCHK_TAG_BOTTOM bottom
#define MEMCHK_TAG_COVER cover

#define MEMCHK_TAG_HEADER_REALLOC _Header
#define MEMCHK_TAG_BOTTOM_REALLOC _Bottom
#define MEMCHK_TAG_COVER_REALLOC _Cover

static void lber_hug_memchk_throw(const void *payload, unsigned bits) {
  const char *trouble;
  switch (bits) {
  case 0:
    return;
  case 1:
    trouble = "hipagut: corrupted memory chunk"
              " (likely overruned by other)";
    break;
  case 2:
    trouble = "hipagut: corrupted memory chunk"
              " (likely underrun)";
    break;
  case 4:
    trouble = "hipagut: corrupted memory chunk"
              " (likely overrun)";
    break;
  /* case 3: */
  default:
    trouble = "hipagut: corrupted memory chunk"
              " (control header was destroyed)";
  }
  __ldap_assert_fail(trouble, __FILE__, __LINE__, __FUNCTION__);
}

__hot __flatten size_t lber_hug_memchk_size(const void *payload, unsigned tag) {
  size_t size = 0;
  unsigned bits = lber_hug_memchk_probe(payload, tag, &size, NULL);

  if (unlikely(bits != 0))
    lber_hug_memchk_throw(payload, bits);

  return size;
}

#define VALGRIND_CLOSE(memchunk)                                               \
  do {                                                                         \
    VALGRIND_MAKE_MEM_NOACCESS((char *)memchunk + sizeof(*memchunk) +          \
                                   memchunk->hm_length,                        \
                               sizeof(struct lber_hipagut));                   \
    VALGRIND_MAKE_MEM_NOACCESS(memchunk, sizeof(*memchunk));                   \
    ASAN_POISON_MEMORY_REGION((char *)memchunk + sizeof(*memchunk) +           \
                                  memchunk->hm_length,                         \
                              sizeof(struct lber_hipagut));                    \
    ASAN_POISON_MEMORY_REGION(memchunk, sizeof(*memchunk));                    \
  } while (0)

#define VALGRIND_OPEN(memchunk)                                                \
  do {                                                                         \
    ASAN_UNPOISON_MEMORY_REGION(memchunk, sizeof(*memchunk));                  \
    ASAN_UNPOISON_MEMORY_REGION((char *)memchunk + sizeof(*memchunk) +         \
                                    memchunk->hm_length,                       \
                                sizeof(struct lber_hipagut));                  \
    VALGRIND_MAKE_MEM_DEFINED(memchunk, sizeof(*memchunk));                    \
    VALGRIND_MAKE_MEM_DEFINED((char *)memchunk + sizeof(*memchunk) +           \
                                  memchunk->hm_length,                         \
                              sizeof(struct lber_hipagut));                    \
  } while (0)

__hot __flatten ATTRIBUTE_NO_SANITIZE_ADDRESS int
lber_hug_memchk_probe(const void *payload, unsigned tag, size_t *length,
                      size_t *sequence) {
  struct lber_hug_memchk *memchunk = LBER_HUG_CHUNK(payload);
  unsigned bits = 0;

  if (likely(lber_hug_nasty_disabled != LBER_HUG_DISABLED)) {
#if defined(HAVE_VALGRIND) || defined(USE_VALGRIND) ||                         \
    defined(__SANITIZE_ADDRESS__)
#define N_ASAN_MUTEX 7
    static pthread_mutex_t asan_mutex[N_ASAN_MUTEX] = {
        PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_MUTEX_INITIALIZER};
    pthread_mutex_t *mutex =
        asan_mutex + ((unsigned)(uintptr_t)payload) % N_ASAN_MUTEX;
    LDAP_ENSURE(pthread_mutex_lock(mutex) == 0);
    VALGRIND_OPEN(memchunk);
#endif
    if (tag && unlikely(LBER_HUG_PROBE(memchunk->hm_guard_head,
                                       MEMCHK_TAG_HEADER, tag)))
      bits |= 1;
    if (unlikely(
            LBER_HUG_PROBE(memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM, 0)))
      bits |= 2;
    if (likely(bits == 0)) {
      if (length)
        *length = memchunk->hm_length;
      if (sequence)
        *sequence = memchunk->hm_sequence;
      if (unlikely(LBER_HUG_PROBE_ASIDE(memchunk, MEMCHK_TAG_COVER, 0,
                                        memchunk->hm_length +
                                            sizeof(struct lber_hug_memchk))))
        bits |= 4;
      else {
        LDAP_ENSURE(
            VALGRIND_CHECK_MEM_IS_ADDRESSABLE(
                memchunk, memchunk->hm_length + LBER_HUG_MEMCHK_OVERHEAD) == 0);
#ifdef __SANITIZE_ADDRESS__
        char *bug = ASAN_REGION_IS_POISONED(
            memchunk, memchunk->hm_length + LBER_HUG_MEMCHK_OVERHEAD);
        if (unlikely(bug)) {
          __asan_report_error(
              /* PC */ __builtin_extract_return_addr(
                  __builtin_return_address(0)),
              /* BP */ __builtin_frame_address(0),
              /* SP */ __builtin_frame_address(0), bug, 0, 0);
          bits |= 8;
        }
#endif
      }
    }
#if defined(HAVE_VALGRIND) || defined(USE_VALGRIND) ||                         \
    defined(__SANITIZE_ADDRESS__)
    VALGRIND_CLOSE(memchunk);
    LDAP_ENSURE(pthread_mutex_unlock(mutex) == 0);
#endif
  }
  return bits;
}

__hot __flatten void lber_hug_memchk_ensure(const void *payload, unsigned tag) {
  unsigned bits = lber_hug_memchk_probe(payload, tag, NULL, NULL);

  if (unlikely(bits != 0))
    lber_hug_memchk_throw(payload, bits);
}

__hot __flatten void *lber_hug_memchk_setup(struct lber_hug_memchk *memchunk,
                                            size_t payload_size, unsigned tag,
                                            unsigned poison_mode) {
  void *payload = LBER_HUG_PAYLOAD(memchunk);

  size_t sequence = LBER_HUG_DISABLED;
#if LDAP_MEMORY_DEBUG > 2
  sequence = __sync_fetch_and_add(&lber_hug_memchk_info.mi_sequence, 1);
  __sync_fetch_and_add(&lber_hug_memchk_info.mi_inuse_chunks, 1);
  __sync_fetch_and_add(&lber_hug_memchk_info.mi_inuse_bytes, payload_size);
#endif /* LDAP_MEMORY_DEBUG > 2 */

  assert(tag != 0);
  LBER_HUG_SETUP(memchunk->hm_guard_head, MEMCHK_TAG_HEADER, tag);
  memchunk->hm_length = payload_size;
  memchunk->hm_sequence = sequence;
  LBER_HUG_SETUP(memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM, 0);
  LBER_HUG_SETUP_ASIDE(memchunk, MEMCHK_TAG_COVER, 0,
                       payload_size + sizeof(struct lber_hug_memchk));
  VALGRIND_CLOSE(memchunk);

  if (poison_mode == LBER_HUG_POISON_DEFAULT)
    poison_mode = lber_hug_memchk_poison_alloc;
  ASAN_UNPOISON_MEMORY_REGION(payload, payload_size);
  switch (poison_mode) {
  default:
    memset(payload, (char)poison_mode, payload_size);
  case LBER_HUG_POISON_DISABLED:
    VALGRIND_MAKE_MEM_UNDEFINED(payload, payload_size);
    break;
  case LBER_HUG_POISON_CALLOC_SETUP:
    memset(payload, 0, payload_size);
  case LBER_HUG_POISON_CALLOC_ALREADY:
    VALGRIND_MAKE_MEM_DEFINED(payload, payload_size);
    break;
  }
  return payload;
}

__hot __flatten void *lber_hug_memchk_drown(void *payload, unsigned tag) {
  size_t payload_size;
  struct lber_hug_memchk *memchunk;

  lber_hug_memchk_ensure(payload, tag);
  memchunk = LBER_HUG_CHUNK(payload);
  VALGRIND_OPEN(memchunk);
  payload_size = memchunk->hm_length;
  LBER_HUG_DROWN(memchunk->hm_guard_head);
  /* memchunk->hm_length = ~memchunk->hm_length; */
  LBER_HUG_DROWN(memchunk->hm_guard_bottom);
  LBER_HUG_DROWN_ASIDE(memchunk, payload_size + sizeof(struct lber_hug_memchk));

#if LDAP_MEMORY_DEBUG > 2
  __sync_fetch_and_sub(&lber_hug_memchk_info.mi_inuse_chunks, 1);
  __sync_fetch_and_sub(&lber_hug_memchk_info.mi_inuse_bytes, payload_size);
#endif /* LDAP_MEMORY_DEBUG > 2 */

  if (lber_hug_memchk_poison_free != LBER_HUG_POISON_DISABLED)
    memset(payload, (char)lber_hug_memchk_poison_free, payload_size);

  VALGRIND_MAKE_MEM_NOACCESS(memchunk, LBER_HUG_MEMCHK_OVERHEAD + payload_size);
  ASAN_POISON_MEMORY_REGION(memchunk, LBER_HUG_MEMCHK_OVERHEAD + payload_size);
  return memchunk;
}

static ATTRIBUTE_NO_SANITIZE_ADDRESS int
lber_hug_memchk_probe_realloc(struct lber_hug_memchk *memchunk, unsigned key) {
  unsigned bits = 0;

  if (likely(lber_hug_nasty_disabled != LBER_HUG_DISABLED)) {
    if (unlikely(lber_hug_probe(&memchunk->hm_guard_head, key)))
      bits |= 1;
    if (unlikely(lber_hug_probe(&memchunk->hm_guard_bottom, key + 1)))
      bits |= 2;
    if (likely(bits == 0)) {
      if (unlikely(lber_hug_probe(
              LBER_HUG_ASIDE(memchunk, memchunk->hm_length +
                                           sizeof(struct lber_hug_memchk)),
              key + 2)))
        bits |= 4;
      else {
        LDAP_ENSURE(
            VALGRIND_CHECK_MEM_IS_ADDRESSABLE(
                memchunk, memchunk->hm_length + LBER_HUG_MEMCHK_OVERHEAD) == 0);
#ifdef __SANITIZE_ADDRESS__
        char *bug = ASAN_REGION_IS_POISONED(
            memchunk, memchunk->hm_length + LBER_HUG_MEMCHK_OVERHEAD);
        if (unlikely(bug)) {
          __asan_report_error(
              /* PC */ __builtin_extract_return_addr(
                  __builtin_return_address(0)),
              /* BP */ __builtin_frame_address(0),
              /* SP */ __builtin_frame_address(0), bug, 0, 0);
          bits |= 8;
        }
#endif
      }
    }
  }
  return bits;
}

unsigned lber_hug_realloc_begin(const void *payload, unsigned tag,
                                size_t *old_size) {
  struct lber_hug_memchk *memchunk;
  unsigned key = canary();

  lber_hug_memchk_ensure(payload, tag);
  memchunk = LBER_HUG_CHUNK(payload);
  VALGRIND_OPEN(memchunk);
  *old_size = memchunk->hm_length;
  lber_hug_setup(&memchunk->hm_guard_head, key);
  lber_hug_setup(&memchunk->hm_guard_bottom, key + 1);
  lber_hug_setup(LBER_HUG_ASIDE(memchunk, memchunk->hm_length +
                                              sizeof(struct lber_hug_memchk)),
                 key + 2);

  assert(lber_hug_memchk_probe_realloc(memchunk, key) == 0);
  return key;
}

void *lber_hug_realloc_undo(struct lber_hug_memchk *memchunk, unsigned tag,
                            unsigned key) {
  unsigned bits = lber_hug_memchk_probe_realloc(memchunk, key);

  if (unlikely(bits != 0)) {
    void *payload = LBER_HUG_PAYLOAD(memchunk);
    lber_hug_memchk_throw(payload, bits);
  }

  LBER_HUG_SETUP(memchunk->hm_guard_head, MEMCHK_TAG_HEADER, tag);
  LBER_HUG_SETUP(memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM, 0);
  LBER_HUG_SETUP_ASIDE(memchunk, MEMCHK_TAG_COVER, 0,
                       memchunk->hm_length + sizeof(struct lber_hug_memchk));
  VALGRIND_CLOSE(memchunk);

  return LBER_HUG_PAYLOAD(memchunk);
}

void *lber_hug_realloc_commit(size_t old_size,
                              struct lber_hug_memchk *new_memchunk,
                              unsigned tag, size_t new_size) {
  void *new_payload = LBER_HUG_PAYLOAD(new_memchunk);

  LDAP_ENSURE(VALGRIND_CHECK_MEM_IS_ADDRESSABLE(
                  new_memchunk, new_size + LBER_HUG_MEMCHK_OVERHEAD) == 0);
  LDAP_ENSURE(ASAN_REGION_IS_POISONED(
                  new_memchunk, new_size + LBER_HUG_MEMCHK_OVERHEAD) == 0);

  size_t sequence = LBER_HUG_DISABLED;
#if LDAP_MEMORY_DEBUG > 2
  sequence = __sync_fetch_and_add(&lber_hug_memchk_info.mi_sequence, 1);
  __sync_fetch_and_add(&lber_hug_memchk_info.mi_inuse_bytes,
                       new_size - old_size);
#endif /* LDAP_MEMORY_DEBUG > 2 */

  LBER_HUG_SETUP(new_memchunk->hm_guard_head, MEMCHK_TAG_HEADER, tag);
  new_memchunk->hm_length = new_size;
  new_memchunk->hm_sequence = sequence;
  LBER_HUG_SETUP(new_memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM, 0);
  LBER_HUG_SETUP_ASIDE(new_memchunk, MEMCHK_TAG_COVER, 0,
                       new_size + sizeof(struct lber_hug_memchk));
  VALGRIND_CLOSE(new_memchunk);

  if (new_size > old_size) {
    if (lber_hug_memchk_poison_alloc != LBER_HUG_POISON_DISABLED)
      memset((char *)new_payload + old_size, (char)lber_hug_memchk_poison_alloc,
             new_size - old_size);
    VALGRIND_MAKE_MEM_UNDEFINED((char *)new_payload + old_size,
                                new_size - old_size);
  }

  assert(lber_hug_memchk_probe(new_payload, tag, NULL, NULL) == 0);
  return new_payload;
}

#endif /* LDAP_MEMORY_DEBUG */

int reopenldap_flags
#if LDAP_CHECK > 1
    = REOPENLDAP_FLAG_IDKFA
#endif
    ;

void __attribute__((constructor)) reopenldap_flags_init() {
  int flags = reopenldap_flags;
  if (getenv("REOPENLDAP_FORCE_IDDQD"))
    flags |= REOPENLDAP_FLAG_IDDQD;
  if (getenv("REOPENLDAP_FORCE_IDKFA"))
    flags |= REOPENLDAP_FLAG_IDKFA;
  if (getenv("REOPENLDAP_FORCE_IDCLIP"))
    flags |= REOPENLDAP_FLAG_IDCLIP;
  if (getenv("REOPENLDAP_FORCE_JITTER"))
    flags |= REOPENLDAP_FLAG_JITTER;
  reopenldap_flags_setup(flags);
}

__cold void reopenldap_flags_setup(int flags) {
  reopenldap_flags = flags & (REOPENLDAP_FLAG_IDDQD | REOPENLDAP_FLAG_IDKFA |
                              REOPENLDAP_FLAG_IDCLIP | REOPENLDAP_FLAG_JITTER);

#if LDAP_MEMORY_DEBUG > 0

#if LDAP_MEMORY_DEBUG == 1
  lber_hug_nasty_disabled = reopenldap_mode_check() ? 0 : LBER_HUG_DISABLED;
#endif /* LDAP_MEMORY_DEBUG == 1 */

  const char *poison_alloc = getenv("REOPENLDAP_POISON_ALLOC");
  lber_hug_memchk_poison_alloc = (lber_hug_nasty_disabled == LBER_HUG_DISABLED)
                                     ? LBER_HUG_POISON_DISABLED
                                     : 0xCC;
  if (poison_alloc) {
    if (*poison_alloc == 0 || strcasecmp(poison_alloc, "disabled") == 0)
      lber_hug_memchk_poison_alloc = LBER_HUG_POISON_DISABLED;
    else {
      char *end;
      unsigned long val = strtoul(poison_alloc, &end, 16);
      if (*end == 0)
        lber_hug_memchk_poison_alloc = val;
    }
  }

  const char *poison_free = getenv("REOPENLDAP_POISON_FREE");
  lber_hug_memchk_poison_free = (lber_hug_nasty_disabled == LBER_HUG_DISABLED)
                                    ? LBER_HUG_POISON_DISABLED
                                    : 0xDD;
  if (poison_free) {
    if (*poison_free == 0 || strcasecmp(poison_free, "disabled") == 0)
      lber_hug_memchk_poison_free = LBER_HUG_POISON_DISABLED;
    else {
      char *end;
      unsigned long val = strtoul(poison_free, &end, 16);
      if (*end == 0)
        lber_hug_memchk_poison_free = val;
    }
  }
#endif /* LDAP_MEMORY_DEBUG */
}

__hot void reopenldap_jitter(int probability_percent) {
  LDAP_ASSERT(probability_percent < 113);
#if defined(__x86_64__) || defined(__i386__)
  __asm __volatile("pause");
#endif
  for (;;) {
    unsigned randomness = canary();
    unsigned salt = (randomness << 13) | (randomness >> 19);
    unsigned twister = (3023231633u * randomness) ^ salt;
    if ((int)(twister % 101u) > --probability_percent)
      break;
    sched_yield();
  }
}

/*----------------------------------------------------------------------------*/

#ifdef CLOCK_BOOTTIME
static clockid_t clock_mono_id;
#else
#define clock_mono_id CLOCK_MONOTONIC
#endif /* CLOCK_BOOTTIME */
static pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
static int64_t clock_mono2real_ns;

__hot static uint64_t clock_ns(clockid_t clk_id) {
  struct timespec ts;
  LDAP_ENSURE(clock_gettime(clk_id, &ts) == 0);
  return ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

__cold static uint64_t sqr(int64_t v) { return v * v; }

__cold static int64_t clock_mono2real_delta() {
  uint64_t real_a, mono_a, real_b, mono_b;
  int64_t delta_a, delta_b, best_delta;
  uint64_t best_dist, dist;
  int i;

#ifdef CLOCK_BOOTTIME
  if (!clock_mono_id) {
    struct timespec ts;

    clock_mono_id = (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) ? CLOCK_BOOTTIME
                                                              : CLOCK_MONOTONIC;
  }
#endif /* CLOCK_BOOTTIME */

  for (best_dist = best_delta = i = 0; i < 42;) {
    real_a = clock_ns(CLOCK_REALTIME);
    mono_a = clock_ns(clock_mono_id);
    real_b = clock_ns(CLOCK_REALTIME);
    mono_b = clock_ns(clock_mono_id);

    delta_a = real_a - mono_a;
    delta_b = real_b - mono_b;

    dist = sqr(real_b - real_a) + sqr(mono_b - mono_a) +
           sqr(delta_b - delta_a) * 2;

    if (i == 0 || best_dist > dist) {
      best_dist = dist;
      best_delta = (delta_a + delta_b) / 2;
      i = 1;
      continue;
    }
    ++i;
  }

  LDAP_ENSURE(best_delta > 0);
  return best_delta;
}

__hot uint64_t ldap_now_steady_ns(void) {
  LDAP_ENSURE(pthread_mutex_lock(&clock_mutex) == 0);

  if (unlikely(!clock_mono2real_ns))
    clock_mono2real_ns = clock_mono2real_delta();

  uint64_t ns = clock_ns(clock_mono_id) + clock_mono2real_ns;
  static uint64_t previous_clock, previous_returned;
  if (unlikely(ns < previous_clock)) {
    Log(LDAP_DEBUG_ANY, LDAP_LEVEL_ERROR,
        "Detected system steady-time adjusted backwards %e seconds\n",
        (previous_clock - ns) * 1e-9);
    if (reopenldap_mode_strict()) {
      Log(LDAP_DEBUG_ANY, LDAP_LEVEL_CRIT,
          "Bailing out due %s-clock backwards in the strict mode.\n", "steady");
      abort();
    }
  }
  previous_clock = ns;

  ns = likely(ns > previous_returned) ? ns : previous_returned + 1;
  previous_returned = ns;

  LDAP_ENSURE(pthread_mutex_unlock(&clock_mutex) == 0);
  return ns;
}

__hot void ldap_timespec_steady(struct timespec *ts) {
  uint64_t clock_now = ldap_now_steady_ns();
  ts->tv_sec = clock_now / 1000000000ul;
  ts->tv_nsec = clock_now % 1000000000ul;
}

__hot unsigned ldap_timeval_realtime(struct timeval *tv) {
  LDAP_ENSURE(pthread_mutex_lock(&clock_mutex) == 0);

  uint64_t us;
  if (reopenldap_mode_righteous()) {
    if (unlikely(!clock_mono2real_ns))
      clock_mono2real_ns = clock_mono2real_delta();
    us = (clock_ns(clock_mono_id) + clock_mono2real_ns) / 1000;
  } else
    us = clock_ns(CLOCK_REALTIME) / 1000;

  static uint64_t previous;
  static unsigned previous_subtick;
  if (unlikely(us < previous)) {
    Log(LDAP_DEBUG_ANY, LDAP_LEVEL_WARNING,
        "Detected system real-time adjusted backwards %e seconds\n",
        (previous - us) * 1e-6);
    if (reopenldap_mode_strict()) {
      Log(LDAP_DEBUG_ANY, LDAP_LEVEL_CRIT,
          "Bailing out due %s-clock backwards in the strict mode.\n",
          "realtime");
      abort();
    }
    us = previous;
  }

  unsigned subtick = likely(previous != us) ? 0 : previous_subtick + 1;
  previous = us;
  previous_subtick = subtick;
  LDAP_ENSURE(pthread_mutex_unlock(&clock_mutex) == 0);

  tv->tv_sec = us / 1000000u;
  tv->tv_usec = us % 1000000u;
  return subtick;
}

__hot time_t ldap_time_steady(void) { return ldap_to_time(ldap_now_steady()); }

__hot time_t ldap_time_unsteady(void) {
#undef time
  return time(NULL);
}
