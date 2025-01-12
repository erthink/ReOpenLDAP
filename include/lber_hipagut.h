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

/*
 * Imported from 1Hippeus project at 2015-01-19.
 * Portions Copyright (c) 2010-2014 Leonid Yuriev <leo@yuriev.ru>.
 */

#ifndef _LBER_HUG_H
#define _LBER_HUG_H

#include <stddef.h>
#include <ldap_cdefs.h>

/* LDAP_MEMORY_DEBUG - control checking/debugging of memory allocation:
 *  = 0	- completely disabled;
 *  = 1 - enabled in code, but by default disabled in run time,
 *		  e.g. may be enabled by 'reopenldap idkfa'
 *        or REOPENLDAP_FORCE_IDKFA in environment;
 *  = 2 - enabled in code and always enabled in run time;
 *  = 3 - enabled, moreover provide memory usage statistics
 *        via LBER_OPT_MEMORY_INUSE.
 */
#ifndef LDAP_MEMORY_DEBUG
#define LDAP_MEMORY_DEBUG 0
#endif

/* ----------------------------------------------------------------------------
 * LY: Originally (in 1Hippeus) calles "Hipagut" = sister in law, nurse
 * (Tagalog language, Philippines). */

LDAP_BEGIN_DECL

struct lber_hipagut {
  char opaque[8];
};
typedef struct lber_hipagut lber_hug_t;

LBER_F(void) lber_hug_setup(lber_hug_t *, const unsigned n42);

LBER_F(void) lber_hug_drown(lber_hug_t *);

/* Return zero when no corruption detected, otherwise returns -1. */
LBER_F(int) lber_hug_probe(const lber_hug_t *, const unsigned n42);

LBER_F(void) lber_hug_setup_link(lber_hug_t *slave, const lber_hug_t *master);

LBER_F(void) lber_hug_drown_link(lber_hug_t *slave, const lber_hug_t *master);

/* Return zero when no corruption detected, otherwise returns -1. */
LBER_F(int)
lber_hug_probe_link(const lber_hug_t *slave, const lber_hug_t *master);

#define LBER_HUG_TETRAD(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

#define LBER_HUG_N42(label, tag) ((tag) + LBER_HUG_TETRAD(#label[0], #label[1], #label[2], #label[3]))

#define LBER_HUG_DECLARE(gizmo) struct lber_hipagut gizmo

/* -------------------------------------------------------------------------- */

#define LBER_HUG_SETUP(gizmo, label, tag) lber_hug_setup(&(gizmo), LBER_HUG_N42(label, tag))

#define LBER_HUG_DROWN(gizmo) lber_hug_drown(&(gizmo))

#define LBER_HUG_PROBE(gizmo, label, tag) lber_hug_probe(&(gizmo), LBER_HUG_N42(label, tag))

#define LBER_HUG_SETUP_LINK(slave, master) lber_hug_setup_link(&(slave), &(master))

#define LBER_HUG_PROBE_LINK(slave, master) lber_hug_probe_link(&(slave), &(master))

/* -------------------------------------------------------------------------- */

#define LBER_HUG_SPACE ((ptrdiff_t)sizeof(lber_hug_t))

#define LBER_HUG_ASIDE(base, offset) ((lber_hug_t *)(((char *)(base)) + (offset)))

#define LBER_HUG_BEFORE(address) LBER_HUG_ASIDE(address, -LBER_HUG_SPACE)

/* -------------------------------------------------------------------------- */

#define LBER_HUG_SETUP_ASIDE(base, label, tag, offset)                                                                 \
  lber_hug_setup(LBER_HUG_ASIDE(base, offset), LBER_HUG_N42(label, tag))

#define LBER_HUG_DROWN_ASIDE(base, offset) lber_hug_drown(LBER_HUG_ASIDE(base, offset))

#define LBER_HUG_PROBE_ASIDE(base, label, tag, offset)                                                                 \
  lber_hug_probe(LBER_HUG_ASIDE(base, offset), LBER_HUG_N42(label, tag))

#define LBER_HUG_SETUP_LINK_ASIDE(base, master, offset) lber_hug_setup_link(LBER_HUG_ASIDE(base, offset), &(master))

#define LBER_HUG_PROBE_LINK_ASIDE(base, master, offset) lber_hug_probe_link(LBER_HUG_ASIDE(base, offset), &(master))

/* -------------------------------------------------------------------------- */

#define lber_hug_throw(info) __assert_fail("hipagut: guard " info, __FILE__, __LINE__, __FUNCTION__)

#define LBER_HUG_ENSURE(gizmo, label, tag)                                                                             \
  do                                                                                                                   \
    if (unlikely(!LBER_HUG_PROBE(gizmo, label, tag)))                                                                  \
      lber_hug_throw(#label "." #tag "@" #gizmo);                                                                      \
  while (0)

#define LBER_HUG_ENSURE_ASIDE(base, label, tag, offset)                                                                \
  if (unlikely(!LBER_HUG_PROBE_ASIDE(base, label, tag, offset)))                                                       \
    lber_hug_throw(#label "." #tag "@" #base "[" #offset "]");                                                         \
  while (0)

#define LBER_HUG_ENSURE_LINK(slave, master)                                                                            \
  do                                                                                                                   \
    if (unlikely(!LBER_HUG_PROBE_LINK(slave, master)))                                                                 \
      lber_hug_throw(#master "~" #slave);                                                                              \
  while (0)

#define LBER_HUG_ENSURE_LINK_ASIDE(base, master, offset)                                                               \
  if (unlikely(!LBER_HUG_PROBE_LINK_ASIDE(base, master, offset)))                                                      \
    lber_hug_throw(#master "~" #base "[" #offset "]");                                                                 \
  while (0)

#if LDAP_MEMORY_DEBUG > 0
#define LBER_HUG_ASSERT(gizmo, label, tag) LBER_HUG_ENSURE(gizmo, label, tag)
#define LBER_HUG_ASSERT_ASIDE(base, label, tag, offset) LBER_HUG_ENSURE_ASIDE(base, label, tag, offset)
#define LBER_HUG_ASSERT_LINK(slave, maser) LBER_HUG_ENSURE_LINK(slave, master)
#define LBER_HUG_ASSERT_LINK_ASIDE(base, master, offset) LBER_HUG_ENSURE_LINK_ASIDE(base, master, offset)
#else
#define LBER_HUG_ASSERT(gizmo, label, tag)                                                                             \
  do {                                                                                                                 \
  } while (0)
#define LBER_HUG_ASSERT_ASIDE(base, label, tag, offset)                                                                \
  do {                                                                                                                 \
  } while (0)
#define LBER_HUG_ASSERT_LINK(slave, master)                                                                            \
  do {                                                                                                                 \
  } while (0)
#define LBER_HUG_ASSERT_LINK_ASIDE(base, master, offset)                                                               \
  do {                                                                                                                 \
  } while (0)
#endif /* LDAP_MEMORY_DEBUG > 0 */

/* -------------------------------------------------------------------------- */

#if LDAP_MEMORY_DEBUG > 0

#define LBER_HUG_DISABLED 0xfea51b1eu /* feasible */
#if LDAP_MEMORY_DEBUG == 1
LBER_V(unsigned) lber_hug_nasty_disabled;
#else
#define lber_hug_nasty_disabled 0
#endif /* LDAP_MEMORY_DEBUG == 1 */

LBER_V(unsigned) lber_hug_memchk_poison_alloc;
LBER_V(unsigned) lber_hug_memchk_poison_free;

#if LDAP_MEMORY_DEBUG > 2
struct _lber_hug_memchk_info {
  volatile size_t mi_inuse_bytes;
  volatile size_t mi_inuse_chunks;
  volatile size_t mi_sequence;
};

LBER_V(struct _lber_hug_memchk_info) lber_hug_memchk_info;
#endif /* LDAP_MEMORY_DEBUG > 2 */

struct lber_hug_memchk {
  LBER_HUG_DECLARE(hm_guard_head);
  size_t hm_length;
  size_t hm_sequence; /* Allocation sequence number */
  LBER_HUG_DECLARE(hm_guard_bottom);
};

#define LBER_HUG_MEMCHK_OVERHEAD (sizeof(struct lber_hug_memchk) + sizeof(struct lber_hipagut))

#define LBER_HUG_CHUNK(ptr) ((struct lber_hug_memchk *)(((char *)(ptr)) - sizeof(struct lber_hug_memchk)))

#define LBER_HUG_PAYLOAD(memchunk) ((void *)(((char *)(memchunk)) + sizeof(struct lber_hug_memchk)))

/* Defines for 'poison_mode' of lber_hug_memchk_setup() */
#define LBER_HUG_POISON_DEFAULT 0xDEFA0178u
#define LBER_HUG_POISON_CALLOC_SETUP 0xCA110C00u
#define LBER_HUG_POISON_CALLOC_ALREADY (LBER_HUG_POISON_CALLOC_SETUP | 0xFFu)
#define LBER_HUG_POISON_DISABLED LBER_HUG_DISABLED

LBER_F(void *)
lber_hug_memchk_setup(struct lber_hug_memchk *memchunk, size_t payload_size, unsigned tag, unsigned poison_mode);

LBER_F(void *) lber_hug_memchk_drown(void *payload, unsigned tag);
LBER_F(void) lber_hug_memchk_ensure(const void *payload, unsigned tag);
LBER_F(size_t) lber_hug_memchk_size(const void *payload, unsigned tag);

LBER_F(unsigned)
lber_hug_realloc_begin(const void *payload, unsigned tag, size_t *old_size);
LBER_F(void *)
lber_hug_realloc_undo(struct lber_hug_memchk *old_memchunk, unsigned tag, unsigned undo_key);
LBER_F(void *)
lber_hug_realloc_commit(size_t old_size, struct lber_hug_memchk *new_memchunk, unsigned tag, size_t new_size);

/* Return zero when no corruption detected,
 * otherwise returns combination of OR'ed bits: 1, 2, 3. */
LBER_F(int)
lber_hug_memchk_probe(const void *payload, unsigned tag, size_t *length, size_t *sequence);

#endif /* LDAP_MEMORY_DEBUG > 0 */

LDAP_F(void *) ber_memcpy_safe(void *dest, const void *src, size_t n);

LDAP_END_DECL

#endif /* _LBER_HUG_H */
