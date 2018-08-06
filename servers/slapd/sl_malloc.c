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

/* sl_malloc.c - malloc routines using a per-thread slab */

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"

/* LY: With respect to http://en.wikipedia.org/wiki/Fail-fast */
#if LDAP_MEMORY_DEBUG > 0
#include <lber_hipagut.h>
#define SL_MEM_TAG 0x081E
#endif /* LDAP_MEMORY_DEBUG */

/*
 * This allocator returns temporary memory from a slab in a given memory
 * context, aligned on a 2-int boundary.  It cannot be used for data
 * which will outlive the task allocating it.
 *
 * A new memory context attaches to the creator's thread context, if any.
 * Threads cannot use other threads' memory contexts; there are no locks.
 *
 * The caller of slap_sl_malloc, usually a thread pool task, must
 * slap_sl_free the memory before finishing: New tasks reuse the context
 * and normally reset it, reclaiming memory left over from last task.
 *
 * The allocator helps memory fragmentation, speed and memory leaks.
 * It is not (yet) reliable as a garbage collector:
 *
 * It falls back to context NULL - plain ber_memalloc() - when the
 * context's slab is full.  A reset does not reclaim such memory.
 * Conversely, free/realloc of data not from the given context assumes
 * context NULL.  The data must not belong to another memory context.
 *
 * Code which has lost track of the current memory context can try
 * slap_sl_context() or ch_malloc.c:ch_free/ch_realloc().
 *
 * Allocations cannot yet return failure.  Like ch_malloc, they succeed
 * or abort slapd.  This will change, do fix code which assumes success.
 */

/*
 * The stack-based allocator stores (ber_len_t)sizeof(head+block) at
 * allocated blocks' head - and in freed blocks also at the tail, marked
 * by ORing *next* block's head with 1.  Freed blocks are only reclaimed
 * from the last block forward.  This is fast, but when a block is never
 * freed, older blocks will not be reclaimed until the slab is reset...
 */

#ifdef SLAP_NO_SL_MALLOC /* Useful with memory debuggers like Valgrind */
enum { No_sl_malloc = 1 };
#else
enum { No_sl_malloc = 0 };
#endif

#define SLAP_SLAB_SOBLOCK 64

struct slab_object {
  void *so_ptr;
  int so_blockhead;
  LDAP_LIST_ENTRY(slab_object) so_link;
};

struct slab_heap {
  void *sh_base;
  void *sh_last;
  void *sh_end;
  int sh_stack;
  int sh_maxorder;
  unsigned char **sh_map;
  LDAP_LIST_HEAD(sh_freelist, slab_object) * sh_free;
  LDAP_LIST_HEAD(sh_so, slab_object) sh_sopool;
};

enum {
  Align =
      sizeof(ber_len_t) > 2 * sizeof(int) ? sizeof(ber_len_t) : 2 * sizeof(int),
  Align_log2 = 1 + (Align > 2) + (Align > 4) + (Align > 8) + (Align > 16),
  order_start = Align_log2 - 1,
  pad = Align - 1
};

static struct slab_object *slap_replenish_sopool(struct slab_heap *sh);
#ifdef SLAPD_UNUSED
static void print_slheap(int level, void *ctx);
#endif

/* Keep memory context in a thread-local var, or in a global when no threads */
#ifdef NO_THREADS
static struct slab_heap *slheap;
#define SET_MEMCTX(thrctx, memctx, sfree) ((void)(slheap = (memctx)))
#define GET_MEMCTX(thrctx, memctxp) (*(memctxp) = slheap)
#else
#define memctx_key ((void *)slap_sl_mem_init)
#define SET_MEMCTX(thrctx, memctx, kfree)                                      \
  ldap_pvt_thread_pool_setkey(thrctx, memctx_key, memctx, kfree, NULL, NULL)
#define GET_MEMCTX(thrctx, memctxp)                                            \
  ((void)(*(memctxp) = NULL),                                                  \
   (void)ldap_pvt_thread_pool_getkey(thrctx, memctx_key, memctxp, NULL),       \
   *(memctxp))
#endif /* NO_THREADS */

/* Destroy the context, or if key==NULL clean it up for reuse. */
void slap_sl_mem_destroy(void *key, void *data) {
  struct slab_heap *sh = data;
  struct slab_object *so;
  int i;

  if (!sh)
    return;

  if (!sh->sh_stack) {
    for (i = 0; i <= sh->sh_maxorder - order_start; i++) {
      so = LDAP_LIST_FIRST(&sh->sh_free[i]);
      while (so) {
        struct slab_object *so_tmp = so;
        so = LDAP_LIST_NEXT(so, so_link);
        LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_tmp, so_link);
      }
      ch_free(sh->sh_map[i]);
    }
    ch_free(sh->sh_free);
    ch_free(sh->sh_map);

    so = LDAP_LIST_FIRST(&sh->sh_sopool);
    while (so) {
      struct slab_object *so_tmp = so;
      so = LDAP_LIST_NEXT(so, so_link);
      if (!so_tmp->so_blockhead) {
        LDAP_LIST_REMOVE(so_tmp, so_link);
      }
    }
    so = LDAP_LIST_FIRST(&sh->sh_sopool);
    while (so) {
      struct slab_object *so_tmp = so;
      so = LDAP_LIST_NEXT(so, so_link);
      ch_free(so_tmp);
    }
  }
  ASAN_POISON_MEMORY_REGION(sh->sh_base,
                            (char *)sh->sh_end - (char *)sh->sh_base);
  VALGRIND_MEMPOOL_TRIM(sh, sh->sh_base, 0);

  if (key != NULL) {
    ASAN_UNPOISON_MEMORY_REGION(sh->sh_base,
                                (char *)sh->sh_end - (char *)sh->sh_base);
    VALGRIND_MAKE_MEM_UNDEFINED(sh->sh_base,
                                (char *)sh->sh_end - (char *)sh->sh_base);
    ber_memfree_x(sh->sh_base, NULL);
    VALGRIND_DESTROY_MEMPOOL(sh);
    ber_memfree_x(sh, NULL);
  }
}

const BerMemoryFunctions slap_sl_mfuncs = {slap_sl_malloc, slap_sl_calloc,
                                           slap_sl_realloc, slap_sl_free};

void slap_sl_mem_init() {
  assert(Align == 1 << Align_log2);
  ber_set_option(NULL, LBER_OPT_MEMORY_FNS, &slap_sl_mfuncs);
}

/* Create, reset or just return the memory context of the current thread. */
void *slap_sl_mem_create(ber_len_t size, int stack, void *thrctx, int new) {
  void *memctx;
  struct slab_heap *sh;
  ber_len_t size_shift;
  struct slab_object *so;
  char *base, *newptr;
  enum { Base_offset = (unsigned)-sizeof(ber_len_t) % Align };

  sh = GET_MEMCTX(thrctx, &memctx);
  if (sh && !new)
    return sh;

  /* Round up to doubleword boundary, then make room for initial
   * padding, preserving expected available size for pool version */
  size = ((size + Align - 1) & -Align) + Base_offset;

  if (!sh) {
    sh = ch_malloc(sizeof(struct slab_heap));
    base = ch_malloc(size);
    SET_MEMCTX(thrctx, sh, slap_sl_mem_destroy);
    VALGRIND_CREATE_MEMPOOL(sh, 0, 0);
  } else {
    slap_sl_mem_destroy(NULL, sh);
    base = sh->sh_base;
    if (size > (ber_len_t)((char *)sh->sh_end - base)) {
      newptr = ch_realloc(base, size);
      if (newptr == NULL)
        return NULL;
      base = newptr;
    }
  }
  VALGRIND_MAKE_MEM_NOACCESS(base, size);
  ASAN_POISON_MEMORY_REGION(base, size);
  sh->sh_base = base;
  sh->sh_end = base + size;

  /* Align (base + head of first block) == first returned block */
  base += Base_offset;
  size -= Base_offset;

  sh->sh_stack = stack;
  if (stack) {
    sh->sh_last = base;
  } else {
    int i, order = -1, order_end = -1;

    size_shift = size - 1;
    do {
      order_end++;
    } while (size_shift >>= 1);
    order = order_end - order_start + 1;
    sh->sh_maxorder = order_end;

    sh->sh_free =
        (struct sh_freelist *)ch_malloc(order * sizeof(struct sh_freelist));
    for (i = 0; i < order; i++) {
      LDAP_LIST_INIT(&sh->sh_free[i]);
    }

    LDAP_LIST_INIT(&sh->sh_sopool);

    if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
      slap_replenish_sopool(sh);
    }
    so = LDAP_LIST_FIRST(&sh->sh_sopool);
    LDAP_LIST_REMOVE(so, so_link);
    so->so_ptr = base;

    LDAP_LIST_INSERT_HEAD(&sh->sh_free[order - 1], so, so_link);

    sh->sh_map = (unsigned char **)ch_malloc(order * sizeof(unsigned char *));
    for (i = 0; i < order; i++) {
      int shiftamt = order_start + 1 + i;
      int nummaps = size >> shiftamt;
      assert(nummaps);
      nummaps >>= 3;
      if (!nummaps)
        nummaps = 1;
      sh->sh_map[i] = (unsigned char *)ch_malloc(nummaps);
      memset(sh->sh_map[i], 0, nummaps);
    }
  }

  return sh;
}

/*
 * Assign memory context to thread context. Use NULL to detach
 * current memory context from thread. Future users must
 * know the context, since ch_free/slap_sl_context() cannot find it.
 */
void slap_sl_mem_setctx(void *thrctx, void *memctx) {
  SET_MEMCTX(thrctx, memctx, slap_sl_mem_destroy);
}

static __inline ber_len_t sl_align(ber_len_t bytes) {
  return (bytes + Align - 1) & -Align;
}

static __inline ber_len_t sl_up2block(ber_len_t gross) {
  return sl_align(gross + sizeof(ber_len_t));
}

#define SL_FREE_MASK (1ul << (sizeof(ber_len_t) * 8 - 1))
#define SL_MAX_SIZE (SL_FREE_MASK / 2)

static ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE _Bool
sl_get_free(ber_len_t *block) {
  return (*block & SL_FREE_MASK) != 0;
}

static ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE void sl_set_free(ber_len_t *block) {
  *block |= SL_FREE_MASK;
}

static ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE ber_len_t
sl_get_size(ber_len_t *block) {
  return *block & ~SL_FREE_MASK;
}

static ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE void sl_set_size(ber_len_t *block,
                                                             ber_len_t gross) {
  *block = gross;
}

static ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE void
sl_update_size(ber_len_t *block, ber_len_t gross) {
  *block = (*block & SL_FREE_MASK) | gross;
}

static ATTRIBUTE_NO_SANITIZE_ADDRESS void *
__slap_sl_malloc(struct slab_heap *sh, ber_len_t gross_size) {
  ber_len_t *ptr;

  /* Add room for head, ensure room for tail when freed, and
   * round up to doubleword boundary. */
  size_t sl_size = sl_up2block(gross_size);
  if (sh->sh_stack) {
    if (sl_size < (ber_len_t)((char *)sh->sh_end - (char *)sh->sh_last)) {
      ptr = sh->sh_last;
      sh->sh_last = (char *)sh->sh_last + sl_size;

      ASAN_UNPOISON_MEMORY_REGION(ptr, sl_size);
      VALGRIND_MAKE_MEM_UNDEFINED(ptr, sl_size);
      sl_set_size(ptr, sl_size);
      ASAN_POISON_MEMORY_REGION(ptr, sizeof(*ptr));
      VALGRIND_MAKE_MEM_NOACCESS(ptr, sizeof(*ptr));
      ptr += 1;
      VALGRIND_MEMPOOL_ALLOC(sh, ptr, gross_size);
      return ptr;
    }
  } else {
    struct slab_object *so_new, *so_left, *so_right;
    ber_len_t size_shift;
    unsigned long diff;
    int i, j, order = -1;

    size_shift = sl_size - 1;
    do {
      order++;
    } while (size_shift >>= 1);

    for (i = order;
         i <= sh->sh_maxorder && LDAP_LIST_EMPTY(&sh->sh_free[i - order_start]);
         i++)
      ;

    if (i == order) {
      so_new = LDAP_LIST_FIRST(&sh->sh_free[i - order_start]);
      LDAP_LIST_REMOVE(so_new, so_link);
      ptr = so_new->so_ptr;
      diff = (unsigned long)((char *)ptr - (char *)sh->sh_base) >> (order + 1);
      sh->sh_map[order - order_start][diff >> 3] |= (1 << (diff & 0x7));
      LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_new, so_link);

      ASAN_UNPOISON_MEMORY_REGION(ptr, sl_size);
      VALGRIND_MAKE_MEM_UNDEFINED(ptr, sl_size);
      sl_set_size(ptr, sl_size);
      ASAN_POISON_MEMORY_REGION(ptr, sizeof(*ptr));
      VALGRIND_MAKE_MEM_NOACCESS(ptr, sizeof(*ptr));
      ptr += 1;
      VALGRIND_MEMPOOL_ALLOC(sh, ptr, gross_size);
      return ptr;
    } else if (i <= sh->sh_maxorder) {
      for (j = i; j > order; j--) {
        so_left = LDAP_LIST_FIRST(&sh->sh_free[j - order_start]);
        LDAP_LIST_REMOVE(so_left, so_link);
        if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
          slap_replenish_sopool(sh);
        }
        so_right = LDAP_LIST_FIRST(&sh->sh_sopool);
        LDAP_LIST_REMOVE(so_right, so_link);
        so_right->so_ptr = (void *)((char *)so_left->so_ptr + (1 << j));
        if (j == order + 1) {
          ptr = so_left->so_ptr;
          diff =
              (unsigned long)((char *)ptr - (char *)sh->sh_base) >> (order + 1);
          sh->sh_map[order - order_start][diff >> 3] |= (1 << (diff & 0x7));
          LDAP_LIST_INSERT_HEAD(&sh->sh_free[j - 1 - order_start], so_right,
                                so_link);
          LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, so_left, so_link);

          ASAN_UNPOISON_MEMORY_REGION(ptr, sl_size);
          VALGRIND_MAKE_MEM_UNDEFINED(ptr, sl_size);
          sl_set_size(ptr, sl_size);
          ASAN_POISON_MEMORY_REGION(ptr, sizeof(*ptr));
          VALGRIND_MAKE_MEM_NOACCESS(ptr, sizeof(*ptr));
          ptr += 1;
          VALGRIND_MEMPOOL_ALLOC(sh, ptr, gross_size);
          return ptr;
        } else {
          LDAP_LIST_INSERT_HEAD(&sh->sh_free[j - 1 - order_start], so_right,
                                so_link);
          LDAP_LIST_INSERT_HEAD(&sh->sh_free[j - 1 - order_start], so_left,
                                so_link);
        }
      }
    }
    /* FIXME: missing return; guessing we failed... */
  }

  return NULL;
}

void *slap_sl_malloc(ber_len_t nett_size, void *ctx) {
  struct slab_heap *sh = ctx;
  void *ptr;

  if (unlikely(nett_size == 0))
    return NULL;

  if (!No_sl_malloc && likely(nett_size < SL_MAX_SIZE && sh)) {
    size_t gross_size = nett_size;
#if LDAP_MEMORY_DEBUG > 0
    gross_size += LBER_HUG_MEMCHK_OVERHEAD;
#endif /* LDAP_MEMORY_DEBUG */

    ptr = __slap_sl_malloc(sh, gross_size);

    if (likely(ptr)) {
#if LDAP_MEMORY_DEBUG > 0
      ptr = lber_hug_memchk_setup(ptr, nett_size, SL_MEM_TAG,
                                  LBER_HUG_POISON_DEFAULT);
#endif /* LDAP_MEMORY_DEBUG */
      ASAN_UNPOISON_MEMORY_REGION(ptr, nett_size);
      return ptr;
    }

    Debug(LDAP_DEBUG_TRACE, "sl_malloc %zu: fallback to ch_malloc\n",
          (size_t)nett_size);
  }

  return ch_malloc(nett_size);
}

#define LIM_SQRT(t) /* some value < sqrt(max value of unsigned type t) */      \
  ((0UL | (t)-1) >> 31 >> 31 > 1                                               \
       ? ((t)1 << 32) - 1                                                      \
       : (0UL | (t)-1) >> 31 ? 65535U : (0UL | (t)-1) >> 15 ? 255U : 15U)

void *slap_sl_calloc(ber_len_t n, ber_len_t s, void *ctx) {
  struct slab_heap *sh = ctx;
  void *ptr;
  size_t nett_size;

  if (unlikely((n | s) == 0))
    return NULL;

  /* The sqrt test is a slight optimization: often avoids the division */
  if (unlikely((n | s) > LIM_SQRT(ber_len_t) && (ber_len_t)-1 / n > s)) {
    Debug(LDAP_DEBUG_ANY, "slap_sl_calloc(%zu,%zu) out of range\n", (size_t)n,
          (size_t)s);
    LDAP_BUG();
  }

  nett_size = n * s;
  if (!No_sl_malloc && likely(nett_size < SL_MAX_SIZE && sh)) {
    size_t gross_size = nett_size;
#if LDAP_MEMORY_DEBUG > 0
    gross_size += LBER_HUG_MEMCHK_OVERHEAD;
#endif /* LDAP_MEMORY_DEBUG */

    ptr = __slap_sl_malloc(sh, gross_size);

    if (likely(ptr)) {
      ASAN_UNPOISON_MEMORY_REGION(ptr, nett_size);
#if LDAP_MEMORY_DEBUG > 0
      ptr = lber_hug_memchk_setup(ptr, nett_size, SL_MEM_TAG,
                                  LBER_HUG_POISON_CALLOC_SETUP);
#else
      memset(ptr, 0, nett_size);
#endif /* LDAP_MEMORY_DEBUG */
      return ptr;
    }

    Debug(LDAP_DEBUG_TRACE, "sl_calloc %zu*%zu: fallback to ch_calloc\n",
          (size_t)n, (size_t)s);
  }

  return ch_calloc(n, s);
}

ATTRIBUTE_NO_SANITIZE_ADDRESS void *
slap_sl_realloc(void *ptr, ber_len_t new_nett_size, void *ctx) {
  struct slab_heap *sh = ctx;
  ber_len_t *block;
  void *oldptr, *newptr;
  size_t old_sl_size;

  if (ptr == NULL)
    return slap_sl_malloc(new_nett_size, ctx);

  /* Not our memory? */
  if (No_sl_malloc || !sh || ptr <= sh->sh_base || ptr >= sh->sh_end) {
    /* Like ch_realloc(), except not trying a new context */
    newptr = ber_memrealloc_x(ptr, new_nett_size, NULL);
    if (unlikely(!newptr)) {
#ifndef __SANITIZE_ADDRESS__
      Debug(LDAP_DEBUG_ANY, "slap_sl_realloc of %zu bytes failed\n",
            (size_t)new_nett_size);
#endif
      LDAP_BUG();
    }
    return newptr;
  }

  if (unlikely(new_nett_size == 0)) {
    slap_sl_free(ptr, ctx);
    return NULL;
  }

  size_t old_gross_size, old_nett_size;
  size_t new_gross_size = new_nett_size;
#if LDAP_MEMORY_DEBUG > 0
  new_gross_size += LBER_HUG_MEMCHK_OVERHEAD;

  unsigned undo_key = lber_hug_realloc_begin(ptr, SL_MEM_TAG, &old_nett_size);
  oldptr = LBER_HUG_CHUNK(ptr);
#else
  oldptr = ptr;
#endif /* LDAP_MEMORY_DEBUG */

  block = ((ber_len_t *)oldptr) - 1;
  ASAN_UNPOISON_MEMORY_REGION(block, sizeof(*block));
  VALGRIND_MAKE_MEM_DEFINED(block, sizeof(*block));
  old_sl_size = sl_get_size(block);
#if LDAP_MEMORY_DEBUG > 0
  old_gross_size = old_nett_size + LBER_HUG_MEMCHK_OVERHEAD;
  assert(old_sl_size >= sl_up2block(old_gross_size));
#else
  assert(old_sl_size > sizeof(ber_len_t));
  old_nett_size = old_sl_size - sizeof(ber_len_t);
  old_gross_size = old_nett_size;
#endif

  if (likely(sh->sh_stack && new_nett_size < SL_MAX_SIZE)) {
    /* Add room for head, round up to doubleword boundary */
    size_t new_sl_size = sl_up2block(new_gross_size);

    /* Never shrink blocks */
    if (new_sl_size <= old_sl_size) {
      VALGRIND_MAKE_MEM_NOACCESS(block, sizeof(*block));
      ASAN_POISON_MEMORY_REGION(block, sizeof(*block));

      VALGRIND_MEMPOOL_CHANGE(sh, oldptr, oldptr, new_gross_size);
      if (old_gross_size > new_gross_size) {
        VALGRIND_MAKE_MEM_NOACCESS((char *)ptr + new_gross_size,
                                   old_gross_size - new_gross_size);
        ASAN_POISON_MEMORY_REGION((char *)ptr + new_gross_size,
                                  old_gross_size - new_gross_size);
#if LDAP_MEMORY_DEBUG > 0
      } else {
        VALGRIND_MAKE_MEM_UNDEFINED((char *)ptr + old_nett_size,
                                    new_gross_size - old_nett_size);
        ASAN_UNPOISON_MEMORY_REGION((char *)ptr + old_nett_size,
                                    new_gross_size - old_nett_size);
      }
      lber_hug_realloc_commit(old_nett_size, oldptr, SL_MEM_TAG, new_nett_size);
#else /* LY: avoiding traps because of gross_size alignment and non-shrinking  \
       */
      }
      size_t floor = 1;
#ifdef VALGRIND_GET_VBITS
      /* LY: lookup for non-addressable edge */
      do {
        size_t vbits, probe = (floor + old_gross_size) >> 1;
        if (3 == VALGRIND_GET_VBITS((char *)ptr + probe, &vbits, 1))
          old_gross_size = probe;
        else
          floor = probe + 1;
      } while (floor < old_gross_size);
#endif /* VALGRIND_GET_VBITS */
      /* LY: make it addressable */
      VALGRIND_MAKE_MEM_UNDEFINED((char *)ptr + floor, new_gross_size - floor);
      ASAN_UNPOISON_MEMORY_REGION((char *)ptr + floor, new_gross_size - floor);
#endif /* LDAP_MEMORY_DEBUG */
      return ptr;
    }

    ber_len_t *next_block = (ber_len_t *)((char *)block + old_sl_size);

    /* If reallocing the last block, try to grow it */
    if (likely(next_block == sh->sh_last &&
               new_sl_size < (char *)sh->sh_end - (char *)block)) {
      sh->sh_last = (char *)block + new_sl_size;
      sl_update_size(block, new_sl_size);
      VALGRIND_MAKE_MEM_NOACCESS(block, sizeof(*block));
      ASAN_POISON_MEMORY_REGION(block, sizeof(*block));
      VALGRIND_MEMPOOL_CHANGE(sh, oldptr, oldptr, new_gross_size);

#if LDAP_MEMORY_DEBUG > 0
      VALGRIND_MAKE_MEM_UNDEFINED((char *)ptr + old_nett_size,
                                  new_gross_size - old_nett_size);
      ASAN_UNPOISON_MEMORY_REGION((char *)ptr + old_nett_size,
                                  new_gross_size - old_nett_size);
      lber_hug_realloc_commit(old_nett_size, oldptr, SL_MEM_TAG, new_nett_size);
#else /* LY: avoiding traps because of gross_size alignment and non-shrinking  \
       */
      size_t floor = 1;
#ifdef VALGRIND_GET_VBITS
      /* LY: lookup for non-addressable edge */
      do {
        size_t vbits, probe = (floor + old_gross_size) >> 1;
        if (3 == VALGRIND_GET_VBITS((char *)ptr + probe, &vbits, 1))
          old_gross_size = probe;
        else
          floor = probe + 1;
      } while (floor < old_gross_size);
#endif /* VALGRIND_GET_VBITS */
      /* LY: make it addressable */
      VALGRIND_MAKE_MEM_UNDEFINED((char *)ptr + floor, new_gross_size - floor);
      ASAN_UNPOISON_MEMORY_REGION((char *)ptr + floor, new_gross_size - floor);
#endif /* LDAP_MEMORY_DEBUG */
      return ptr;
    }
    /* Nowhere to grow, need to alloc and copy */
  }

  VALGRIND_MAKE_MEM_NOACCESS(block, sizeof(*block));
  ASAN_POISON_MEMORY_REGION(block, sizeof(*block));
#if LDAP_MEMORY_DEBUG > 0
  lber_hug_realloc_undo(oldptr, SL_MEM_TAG, undo_key);
#endif /* LDAP_MEMORY_DEBUG */

  newptr = slap_sl_malloc(new_nett_size, ctx);
  if (likely(newptr)) {
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(ptr, old_nett_size);
    ASAN_UNPOISON_MEMORY_REGION(ptr, old_nett_size);
    memcpy(newptr, ptr,
           (old_nett_size < new_nett_size) ? old_nett_size : new_nett_size);
    VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(ptr, old_nett_size);
    slap_sl_free(ptr, ctx);
  }
  return newptr;
}

ATTRIBUTE_NO_SANITIZE_ADDRESS void slap_sl_free(void *ptr, void *ctx) {
  struct slab_heap *sh = ctx;
  ber_len_t sl_size;
  ber_len_t *block, *next_block, *tmpp;

  if (!ptr)
    return;

  if (No_sl_malloc || !sh || ptr < sh->sh_base || ptr >= sh->sh_end) {
    ber_memfree_x(ptr, NULL);
    return;
  }

#if LDAP_MEMORY_DEBUG > 0
  ptr = lber_hug_memchk_drown(ptr, SL_MEM_TAG);
#endif /* LDAP_MEMORY_DEBUG */

  VALGRIND_MEMPOOL_FREE(sh, ptr);
  block = ((ber_len_t *)ptr) - 1;
  ASAN_UNPOISON_MEMORY_REGION(block, sizeof(*block));
  VALGRIND_MAKE_MEM_DEFINED(block, sizeof(*block));
  sl_size = sl_get_size(block);
  ASAN_POISON_MEMORY_REGION(block, sl_size);

  if (sh->sh_stack) {
    next_block = (ber_len_t *)((char *)block + sl_size);
    if (sh->sh_last != next_block) {
      /* Mark it free: tail = size, head of next block |= 1 */
      ASAN_UNPOISON_MEMORY_REGION(next_block - 1, sizeof(ber_len_t) * 2);
      VALGRIND_MAKE_MEM_DEFINED(next_block - 1, sizeof(ber_len_t) * 2);
      next_block[-1] = sl_size;
      sl_set_free(next_block);
      VALGRIND_MAKE_MEM_NOACCESS(block, sl_size + sizeof(ber_len_t));
      ASAN_POISON_MEMORY_REGION(block, sl_size + sizeof(ber_len_t));
    } else {
      /* Reclaim freed block(s) off tail */
      while (sl_get_free(block)) {
        ASAN_UNPOISON_MEMORY_REGION(block - 1, sizeof(ber_len_t));
        VALGRIND_MAKE_MEM_DEFINED(block - 1, sizeof(ber_len_t));
        block = (ber_len_t *)((char *)block - block[-1]);
        ASAN_UNPOISON_MEMORY_REGION(block, sizeof(ber_len_t));
        VALGRIND_MAKE_MEM_DEFINED(block, sizeof(ber_len_t));
      }
      sh->sh_last = block;
      VALGRIND_MEMPOOL_TRIM(sh, sh->sh_base,
                            (char *)sh->sh_last - (char *)sh->sh_base);
    }
  } else {
    int size_shift, order_size;
    struct slab_object *so;
    unsigned long diff;
    int i, inserted = 0, order = -1;

    VALGRIND_MAKE_MEM_NOACCESS(block, sizeof(*block));
    ASAN_POISON_MEMORY_REGION(block, sizeof(*block));
    size_shift = sl_size - 1;
    do {
      order++;
    } while (size_shift >>= 1);

    for (i = order, tmpp = block; i <= sh->sh_maxorder; i++) {
      order_size = 1 << (i + 1);
      diff = (unsigned long)((char *)tmpp - (char *)sh->sh_base) >> (i + 1);
      sh->sh_map[i - order_start][diff >> 3] &= (~(1 << (diff & 0x7)));
      if (diff == ((diff >> 1) << 1)) {
        if (!(sh->sh_map[i - order_start][(diff + 1) >> 3] &
              (1 << ((diff + 1) & 0x7)))) {
          so = LDAP_LIST_FIRST(&sh->sh_free[i - order_start]);
          while (so) {
            if ((char *)so->so_ptr == (char *)tmpp) {
              LDAP_LIST_REMOVE(so, so_link);
            } else if ((char *)so->so_ptr == (char *)tmpp + order_size) {
              LDAP_LIST_REMOVE(so, so_link);
              break;
            }
            so = LDAP_LIST_NEXT(so, so_link);
          }
          if (so) {
            if (i < sh->sh_maxorder) {
              inserted = 1;
              so->so_ptr = tmpp;
              LDAP_LIST_INSERT_HEAD(&sh->sh_free[i - order_start + 1], so,
                                    so_link);
            }
            continue;
          } else {
            if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
              slap_replenish_sopool(sh);
            }
            so = LDAP_LIST_FIRST(&sh->sh_sopool);
            LDAP_LIST_REMOVE(so, so_link);
            so->so_ptr = tmpp;
            LDAP_LIST_INSERT_HEAD(&sh->sh_free[i - order_start], so, so_link);
            break;
          }
        } else {
          if (!inserted) {
            if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
              slap_replenish_sopool(sh);
            }
            so = LDAP_LIST_FIRST(&sh->sh_sopool);
            LDAP_LIST_REMOVE(so, so_link);
            so->so_ptr = tmpp;
            LDAP_LIST_INSERT_HEAD(&sh->sh_free[i - order_start], so, so_link);
          }
          break;
        }
      } else {
        if (!(sh->sh_map[i - order_start][(diff - 1) >> 3] &
              (1 << ((diff - 1) & 0x7)))) {
          so = LDAP_LIST_FIRST(&sh->sh_free[i - order_start]);
          while (so) {
            if ((char *)so->so_ptr == (char *)tmpp) {
              LDAP_LIST_REMOVE(so, so_link);
            } else if ((char *)tmpp == (char *)so->so_ptr + order_size) {
              LDAP_LIST_REMOVE(so, so_link);
              tmpp = so->so_ptr;
              break;
            }
            so = LDAP_LIST_NEXT(so, so_link);
          }
          if (so) {
            if (i < sh->sh_maxorder) {
              inserted = 1;
              LDAP_LIST_INSERT_HEAD(&sh->sh_free[i - order_start + 1], so,
                                    so_link);
              continue;
            }
          } else {
            if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
              slap_replenish_sopool(sh);
            }
            so = LDAP_LIST_FIRST(&sh->sh_sopool);
            LDAP_LIST_REMOVE(so, so_link);
            so->so_ptr = tmpp;
            LDAP_LIST_INSERT_HEAD(&sh->sh_free[i - order_start], so, so_link);
            break;
          }
        } else {
          if (!inserted) {
            if (LDAP_LIST_EMPTY(&sh->sh_sopool)) {
              slap_replenish_sopool(sh);
            }
            so = LDAP_LIST_FIRST(&sh->sh_sopool);
            LDAP_LIST_REMOVE(so, so_link);
            so->so_ptr = tmpp;
            LDAP_LIST_INSERT_HEAD(&sh->sh_free[i - order_start], so, so_link);
          }
          break;
        }
      }
    }
  }
}

void slap_sl_release(void *ptr, void *ctx) {
  struct slab_heap *sh = ctx;
  if (sh && ptr >= sh->sh_base && ptr <= sh->sh_end)
    sh->sh_last = ptr;
}

void *slap_sl_mark(void *ctx) {
  struct slab_heap *sh = ctx;
  return sh->sh_last;
}

/*
 * Return the memory context of the current thread if the given block of
 * memory belongs to it, otherwise return NULL.
 */
void *slap_sl_context(void *ptr) {
  void *memctx;
  struct slab_heap *sh;

  if (slapMode & SLAP_TOOL_MODE)
    return NULL;

  sh = GET_MEMCTX(ldap_pvt_thread_pool_context(), &memctx);
  if (sh && ptr >= sh->sh_base && ptr <= sh->sh_end) {
    return sh;
  }
  return NULL;
}

static struct slab_object *slap_replenish_sopool(struct slab_heap *sh) {
  struct slab_object *so_block;
  int i;

  so_block = (struct slab_object *)ch_malloc(SLAP_SLAB_SOBLOCK *
                                             sizeof(struct slab_object));

  if (so_block == NULL) {
    return NULL;
  }

  so_block[0].so_blockhead = 1;
  LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, &so_block[0], so_link);
  for (i = 1; i < SLAP_SLAB_SOBLOCK; i++) {
    so_block[i].so_blockhead = 0;
    LDAP_LIST_INSERT_HEAD(&sh->sh_sopool, &so_block[i], so_link);
  }

  return so_block;
}

#ifdef SLAPD_UNUSED
static void print_slheap(int level, void *ctx) {
  struct slab_heap *sh = ctx;
  struct slab_object *so;
  int i, j, once = 0;

  if (!ctx) {
    Debug(level, "NULL memctx\n");
    return;
  }

  Debug(level, "sh->sh_maxorder=%d\n", sh->sh_maxorder);

  for (i = order_start; i <= sh->sh_maxorder; i++) {
    once = 0;
    Debug(level, "order=%d\n", i);
    for (j = 0; j < (1 << (sh->sh_maxorder - i)) / 8; j++) {
      Debug(level, "%02x ", sh->sh_map[i - order_start][j]);
      once = 1;
    }
    if (!once) {
      Debug(level, "%02x ", sh->sh_map[i - order_start][0]);
    }
    Debug(level, "\n");
    Debug(level, "free list:\n");
    so = LDAP_LIST_FIRST(&sh->sh_free[i - order_start]);
    while (so) {
      Debug(level, "%p\n", so->so_ptr);
      so = LDAP_LIST_NEXT(so, so_link);
    }
  }
}
#endif
