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

#include <ac/stdlib.h>
#include <ac/string.h>

#include "lber-int.h"

/* LY: With respect to http://en.wikipedia.org/wiki/Fail-fast */
#if LDAP_MEMORY_DEBUG

#if LDAP_MEMORY_DEBUG > 3
#include <stdio.h>
#endif

/*
 * LDAP_MEMORY_DEBUG should be enabled for properly memory handling,
 * durability and data consistency.
 *
 * It can't trigger assertion failure in a prefectly valid program!
 *
 * But it should be enabled by an experienced developer as it causes
 * the inclusion of numerous assert()'s due overall poor code quality
 * in the LDAP's world.
 */
#include <lber_hipagut.h>
#define BER_MEM_VALID(p, s) lber_hug_memchk_ensure(p, 0)
#define LIBC_MEM_TAG 0x71BC
#elif defined(HAVE_VALGRIND) || defined(USE_VALGRIND) ||                       \
    defined(__SANITIZE_ADDRESS__)
#define BER_MEM_VALID(p, s)                                                    \
  do {                                                                         \
    if ((s) > 0) {                                                             \
      LDAP_ENSURE(VALGRIND_CHECK_MEM_IS_ADDRESSABLE(p, s) == 0);               \
      LDAP_ENSURE(ASAN_REGION_IS_POISONED(p, s) == 0);                         \
    }                                                                          \
  } while (0)
#else
#define BER_MEM_VALID(p, s) __noop()
#endif /* LDAP_MEMORY_DEBUG */

BerMemoryFunctions *ber_int_memory_fns = NULL;

void ber_memfree_x(void *p, void *ctx) {
  if (unlikely(p == NULL))
    return;

  if (ber_int_memory_fns == NULL || ctx == NULL) {
#if LDAP_MEMORY_DEBUG > 0
    p = lber_hug_memchk_drown(p, LIBC_MEM_TAG);
#if LDAP_MEMORY_DEBUG > 3
    struct lber_hug_memchk *mh = p;
    fprintf(stderr, "%p.%zu -f- %zu ber_memfree %zu\n", mh, mh->hm_sequence,
            mh->hm_length, lber_hug_memchk_info.mi_inuse_bytes);
#endif
#endif /* LDAP_MEMORY_DEBUG */

    free(p);
  } else {
    (*ber_int_memory_fns->bmf_free)(p, ctx);
  }
}

void ber_memfree(void *p) { ber_memfree_x(p, NULL); }

void ber_memvfree_x(void **vec, void *ctx) {
  int i;

  if (unlikely(vec == NULL))
    return;

  for (i = 0; vec[i] != NULL; i++) {
    ber_memfree_x(vec[i], ctx);
  }

  ber_memfree_x(vec, ctx);
}

void ber_memvfree(void **vec) { ber_memvfree_x(vec, NULL); }

void *ber_memalloc_x(ber_len_t s, void *ctx) {
  void *p;

  if (unlikely(s == 0))
    return NULL;

  if (ber_int_memory_fns == NULL || ctx == NULL) {
    size_t bytes = s;
#if LDAP_MEMORY_DEBUG > 0
    bytes += LBER_HUG_MEMCHK_OVERHEAD;
    if (unlikely(bytes < s)) {
      ber_errno = LBER_ERROR_MEMORY;
      return NULL;
    }
#endif /* LDAP_MEMORY_DEBUG */

    p = malloc(bytes);

#if LDAP_MEMORY_DEBUG > 0
    if (p) {
      p = lber_hug_memchk_setup(p, s, LIBC_MEM_TAG, LBER_HUG_POISON_DEFAULT);
#if LDAP_MEMORY_DEBUG > 3
      struct lber_hug_memchk *mh = LBER_HUG_CHUNK(p);
      fprintf(stderr, "%p.%zu -a- %zu ber_memalloc %zu\n", mh, mh->hm_sequence,
              mh->hm_length, lber_hug_memchk_info.mi_inuse_bytes);
#endif
    }
#endif /* LDAP_MEMORY_DEBUG */

  } else {
    p = (*ber_int_memory_fns->bmf_malloc)(s, ctx);
  }

  if (unlikely(p == NULL)) {
    ber_errno = LBER_ERROR_MEMORY;
    return p;
  }

  BER_MEM_VALID(p, s);
  return p;
}

void *ber_memalloc(ber_len_t s) { return ber_memalloc_x(s, NULL); }

#define LIM_SQRT(t) /* some value < sqrt(max value of unsigned type t) */      \
  ((0UL | (t)-1) >> 31 >> 31 > 1                                               \
       ? ((t)1 << 32) - 1                                                      \
       : (0UL | (t)-1) >> 31 ? 65535U : (0UL | (t)-1) >> 15 ? 255U : 15U)

void *ber_memcalloc_x(ber_len_t n, ber_len_t s, void *ctx) {
  void *p;

  if (unlikely(n == 0 || s == 0))
    return NULL;

  if (ber_int_memory_fns == NULL || ctx == NULL) {
#if LDAP_MEMORY_DEBUG > 0
    /* The sqrt test is a slight optimization: often avoids the division */
    if (unlikely((n | s) > LIM_SQRT(size_t) && (size_t)-1 / n > s)) {
      ber_errno = LBER_ERROR_MEMORY;
      return NULL;
    }

    size_t payload_bytes = (size_t)s * (size_t)n;
    size_t total_bytes = payload_bytes + LBER_HUG_MEMCHK_OVERHEAD;
    if (unlikely(total_bytes < payload_bytes)) {
      ber_errno = LBER_ERROR_MEMORY;
      return NULL;
    }

    n = total_bytes;
    s = 1;
#endif /* LDAP_MEMORY_DEBUG */

    p = calloc(n, s);

#if LDAP_MEMORY_DEBUG > 0
    if (p) {
      p = lber_hug_memchk_setup(p, payload_bytes, LIBC_MEM_TAG,
                                LBER_HUG_POISON_CALLOC_ALREADY);
#if LDAP_MEMORY_DEBUG > 3
      struct lber_hug_memchk *mh = LBER_HUG_CHUNK(p);
      fprintf(stderr, "%p.%zu -a- %zu ber_memcalloc %zu\n", mh, mh->hm_sequence,
              mh->hm_length, lber_hug_memchk_info.mi_inuse_bytes);
#endif
    }
#endif /* LDAP_MEMORY_DEBUG */

  } else {
    p = (*ber_int_memory_fns->bmf_calloc)(n, s, ctx);
  }

  if (p == NULL) {
    ber_errno = LBER_ERROR_MEMORY;
    return p;
  }

  BER_MEM_VALID(p, (size_t)n * (size_t)s);
  return p;
}

void *ber_memcalloc(ber_len_t n, ber_len_t s) {
  return ber_memcalloc_x(n, s, NULL);
}

void *ber_memrealloc_x(void *p, ber_len_t s, void *ctx) {
  /* realloc(NULL,s) -> malloc(s) */
  if (unlikely(p == NULL))
    return ber_memalloc_x(s, ctx);

  /* realloc(p,0) -> free(p) */
  if (s == 0) {
    ber_memfree_x(p, ctx);
    return NULL;
  }

  if (ber_int_memory_fns == NULL || ctx == NULL) {
    size_t bytes = s;

#if LDAP_MEMORY_DEBUG > 0
    size_t old_size;
    unsigned undo_key;

    bytes += LBER_HUG_MEMCHK_OVERHEAD;
    if (unlikely(bytes < s)) {
      ber_errno = LBER_ERROR_MEMORY;
      return NULL;
    }
    undo_key = lber_hug_realloc_begin(p, LIBC_MEM_TAG, &old_size);
    p = LBER_HUG_CHUNK(p);
#endif /* LDAP_MEMORY_DEBUG */

    p = realloc(p, bytes);

#if LDAP_MEMORY_DEBUG > 0
    if (p) {
      p = lber_hug_realloc_commit(old_size, p, LIBC_MEM_TAG, s);
#if LDAP_MEMORY_DEBUG > 3
      struct lber_hug_memchk *mh = LBER_HUG_CHUNK(p);
      fprintf(stderr, "%p.%zu -a- %zu ber_memrealloc %zu\n", mh,
              mh->hm_sequence, mh->hm_length,
              lber_hug_memchk_info.mi_inuse_bytes);
#endif
    } else {
      lber_hug_realloc_undo(p, LIBC_MEM_TAG, undo_key);
    }
#endif /* LDAP_MEMORY_DEBUG */

  } else {
    p = (*ber_int_memory_fns->bmf_realloc)(p, s, ctx);
  }

  if (p == NULL) {
    ber_errno = LBER_ERROR_MEMORY;
    return p;
  }

  BER_MEM_VALID(p, s);
  return p;
}

void *ber_memrealloc(void *p, ber_len_t s) {
  return ber_memrealloc_x(p, s, NULL);
}

void ber_bvfree_x(struct berval *bv, void *ctx) {
  if (unlikely(bv == NULL))
    return;

  BER_MEM_VALID(bv, 0);

  if (bv->bv_val != NULL) {
    ber_memfree_x(bv->bv_val, ctx);
  }

  ber_memfree_x((char *)bv, ctx);
}

void ber_bvfree(struct berval *bv) { ber_bvfree_x(bv, NULL); }

void ber_bvecfree_x(struct berval **bv, void *ctx) {
  int i;

  if (unlikely(bv == NULL))
    return;

  BER_MEM_VALID(bv, 0);

  /* count elements */
  for (i = 0; bv[i] != NULL; i++)
    ;

  /* free in reverse order */
  for (i--; i >= 0; i--) {
    ber_bvfree_x(bv[i], ctx);
  }

  ber_memfree_x((char *)bv, ctx);
}

void ber_bvecfree(struct berval **bv) { ber_bvecfree_x(bv, NULL); }

int ber_bvecadd_x(struct berval ***bvec, struct berval *bv, void *ctx) {
  ber_len_t i;
  struct berval **dup;

  if (*bvec == NULL) {
    if (bv == NULL) {
      /* nothing to add */
      return 0;
    }

    *bvec = ber_memalloc_x(2 * sizeof(struct berval *), ctx);

    if (*bvec == NULL) {
      return -1;
    }

    (*bvec)[0] = bv;
    (*bvec)[1] = NULL;

    return 1;
  }

  BER_MEM_VALID(bvec, 0);

  /* count entries */
  for (i = 0; (*bvec)[i] != NULL; i++) {
    /* EMPTY */;
  }

  if (bv == NULL) {
    return i;
  }

  dup = ber_memrealloc_x(*bvec, (i + 2) * sizeof(struct berval *), ctx);

  if (dup == NULL) {
    return -1;
  }

  *bvec = dup;

  (*bvec)[i++] = bv;
  (*bvec)[i] = NULL;

  return i;
}

int ber_bvecadd(struct berval ***bvec, struct berval *bv) {
  return ber_bvecadd_x(bvec, bv, NULL);
}

struct berval *ber_dupbv_x(struct berval *dst, const struct berval *src,
                           void *ctx) {
  struct berval *dup, tmp;

  if (unlikely(src == NULL)) {
    ber_errno = LBER_ERROR_PARAM;
    return NULL;
  }

  if (dst) {
    dup = &tmp;
  } else {
    if ((dup = ber_memalloc_x(sizeof(struct berval), ctx)) == NULL) {
      return NULL;
    }
  }

  if (src->bv_val == NULL) {
    dup->bv_val = NULL;
    dup->bv_len = 0;
  } else {

    if ((dup->bv_val = ber_memalloc_x(src->bv_len + 1, ctx)) == NULL) {
      if (!dst)
        ber_memfree_x(dup, ctx);
      return NULL;
    }

    memcpy(dup->bv_val, src->bv_val, src->bv_len);
    dup->bv_val[src->bv_len] = '\0';
    dup->bv_len = src->bv_len;
  }

  if (dup != &tmp)
    return dup;
  else {
    *dst = *dup;
    return dst;
  }
}

struct berval *ber_dupbv(struct berval *dst, const struct berval *src) {
  return ber_dupbv_x(dst, src, NULL);
}

struct berval *ber_bvdup(struct berval *src) {
  return ber_dupbv_x(NULL, src, NULL);
}

struct berval *ber_str2bv_x(const char *s, ber_len_t len, int dup,
                            struct berval *bv, void *ctx) {
  struct berval *ret;

  if (unlikely(s == NULL)) {
    ber_errno = LBER_ERROR_PARAM;
    return NULL;
  }

  if (bv) {
    ret = bv;
  } else if ((ret = ber_memalloc_x(sizeof(struct berval), ctx)) == NULL) {
    return NULL;
  }

  ret->bv_len = len ? len : strlen(s);
  if (dup) {
    if ((ret->bv_val = ber_memalloc_x(ret->bv_len + 1, ctx)) == NULL) {
      if (!bv)
        ber_memfree_x(ret, ctx);
      return NULL;
    }

    memcpy(ret->bv_val, s, ret->bv_len);
    ret->bv_val[ret->bv_len] = '\0';
  } else {
    ret->bv_val = (char *)s;
  }

  return ret;
}

struct berval *ber_str2bv(const char *s, ber_len_t len, int dup,
                          struct berval *bv) {
  return ber_str2bv_x(s, len, dup, bv, NULL);
}

struct berval *ber_mem2bv_x(const char *s, ber_len_t len, int dup,
                            struct berval *bv, void *ctx) {
  struct berval *ret;

  if (unlikely(s == NULL)) {
    ber_errno = LBER_ERROR_PARAM;
    return NULL;
  }

  if (bv) {
    ret = bv;
  } else {
    if ((ret = ber_memalloc_x(sizeof(struct berval), ctx)) == NULL) {
      return NULL;
    }
  }

  ret->bv_len = len;
  if (dup) {
    if ((ret->bv_val = ber_memalloc_x(ret->bv_len + 1, ctx)) == NULL) {
      if (!bv)
        ber_memfree_x(ret, ctx);
      return NULL;
    }

    memcpy(ret->bv_val, s, ret->bv_len);
    ret->bv_val[ret->bv_len] = '\0';
  } else {
    ret->bv_val = (char *)s;
  }

  return ret;
}

struct berval *ber_mem2bv(const char *s, ber_len_t len, int dup,
                          struct berval *bv) {
  return ber_mem2bv_x(s, len, dup, bv, NULL);
}

char *ber_strdup_x(const char *s, void *ctx) {
  char *p;
  size_t len;

  if (unlikely(s == NULL)) {
    ber_errno = LBER_ERROR_PARAM;
    return NULL;
  }

  len = strlen(s) + 1;
  if ((p = ber_memalloc_x(len, ctx)) != NULL) {
    memcpy(p, s, len);
  }

  return p;
}

char *ber_strdup(const char *s) { return ber_strdup_x(s, NULL); }

ber_len_t ber_strnlen(const char *s, ber_len_t len) {
  ber_len_t l;

  for (l = 0; l < len && s[l] != '\0'; l++)
    ;

  return l;
}

char *ber_strndup_x(const char *s, ber_len_t l, void *ctx) {
  char *p;
  size_t len;

  if (unlikely(s == NULL)) {
    ber_errno = LBER_ERROR_PARAM;
    return NULL;
  }

  len = ber_strnlen(s, l);
  if ((p = ber_memalloc_x(len + 1, ctx)) != NULL) {
    memcpy(p, s, len);
    p[len] = '\0';
  }

  return p;
}

char *ber_strndup(const char *s, ber_len_t l) {
  return ber_strndup_x(s, l, NULL);
}

/*
 * dst is resized as required by src and the value of src is copied into dst
 * dst->bv_val must be NULL (and dst->bv_len must be 0), or it must be
 * alloc'ed with the context ctx
 */
struct berval *ber_bvreplace_x(struct berval *dst, const struct berval *src,
                               void *ctx) {
  if (BER_BVISNULL(dst) || dst->bv_len < src->bv_len) {
    dst->bv_val = ber_memrealloc_x(dst->bv_val, src->bv_len + 1, ctx);
  }

  memcpy(dst->bv_val, src->bv_val, src->bv_len + 1);
  dst->bv_len = src->bv_len;

  return dst;
}

struct berval *ber_bvreplace(struct berval *dst, const struct berval *src) {
  return ber_bvreplace_x(dst, src, NULL);
}

void ber_bvarray_free_x(BerVarray a, void *ctx) {
  int i;

  if (a) {
    BER_MEM_VALID(a, 0);

    /* count elements */
    for (i = 0; a[i].bv_val; i++)
      ;

    /* free in reverse order */
    for (i--; i >= 0; i--) {
      ber_memfree_x(a[i].bv_val, ctx);
    }

    ber_memfree_x(a, ctx);
  }
}

void ber_bvarray_free(BerVarray a) { ber_bvarray_free_x(a, NULL); }

int ber_bvarray_dup_x(BerVarray *dst, BerVarray src, void *ctx) {
  int i, j;
  BerVarray dup;

  if (!src) {
    *dst = NULL;
    return 0;
  }

  for (i = 0; !BER_BVISNULL(&src[i]); i++)
    ;
  dup = ber_memalloc_x((i + 1) * sizeof(BerValue), ctx);
  if (!dup)
    return -1;
  for (j = 0; j < i; j++) {
    ber_dupbv_x(&dup[j], &src[j], ctx);
    if (BER_BVISNULL(&dup[j])) {
      ber_bvarray_free_x(dup, ctx);
      return -1;
    }
  }
  BER_BVZERO(&dup[j]);
  *dst = dup;
  return 0;
}

int ber_bvarray_add_x(BerVarray *a, BerValue *bv, void *ctx) {
  int n;

  if (*a == NULL) {
    if (bv == NULL) {
      return 0;
    }
    n = 0;

    *a = (BerValue *)ber_memalloc_x(2 * sizeof(BerValue), ctx);
    if (*a == NULL) {
      return -1;
    }

  } else {
    BerVarray atmp;

    for (n = 0; *a != NULL && (*a)[n].bv_val != NULL; n++) {
      ; /* just count them */
    }

    if (bv == NULL) {
      return n;
    }

    atmp = (BerValue *)ber_memrealloc_x((char *)*a, (n + 2) * sizeof(BerValue),
                                        ctx);

    if (atmp == NULL) {
      return -1;
    }

    *a = atmp;
  }

  (*a)[n++] = *bv;
  (*a)[n].bv_val = NULL;
  (*a)[n].bv_len = 0;

  return n;
}

int ber_bvarray_add(BerVarray *a, BerValue *bv) {
  return ber_bvarray_add_x(a, bv, NULL);
}
