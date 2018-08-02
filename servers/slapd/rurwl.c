/* $ReOpenLDAP$ */
/* Copyright 2017-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
#include "slap.h"

void rurw_lock_init(rurw_lock_t *p) {
  memset(p, 0, sizeof(rurw_lock_t));
  ldap_pvt_thread_mutex_init(&p->rurw_mutex);
  ldap_pvt_thread_cond_init(&p->rurw_wait4r);
  ldap_pvt_thread_cond_init(&p->rurw_wait4w);
  ldap_pvt_thread_cond_init(&p->rurw_wait4u);
}

void rurw_lock_destroy(rurw_lock_t *p) {
  ldap_pvt_thread_mutex_destroy(&p->rurw_mutex);
  ldap_pvt_thread_cond_destroy(&p->rurw_wait4r);
  ldap_pvt_thread_cond_destroy(&p->rurw_wait4w);
  ldap_pvt_thread_cond_destroy(&p->rurw_wait4u);
}

static unsigned rurw_thread_index(void) {
  static __thread unsigned index;

  if (unlikely(index == 0)) {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    LDAP_ENSURE(pthread_mutex_lock(&mutex) == 0);
    static unsigned last;
    index = (last < RURW_LOCK_MAXTHREADS) ? ++last : INT_MAX;
    LDAP_ENSURE(pthread_mutex_unlock(&mutex) == 0);
  }

  return index - 1;
}

static rurw_thr_state_t *rurw_enter(rurw_lock_t *p, int acquire) {
  assert(RURW_LOCK_MAXTHREADS <= 8 * sizeof(p->rurw_fullscan_bitmask));
  rurw_thr_state_t *s;
  ldap_pvt_thread_t current = ldap_pvt_thread_self();
  unsigned index = rurw_thread_index();
  uint64_t mask = 0;

  if (likely(index < RURW_LOCK_MAXTHREADS)) {
    s = p->state + index;
    if (likely(s->thr == current))
      return s;

    mask = (uint64_t)1 << index;
    if (likely(!s->thr)) {
      if (!acquire) {
        if (unlikely(p->rurw_fullscan_bitmask & mask))
          goto fullscan;
        return NULL;
      }

      ldap_pvt_thread_mutex_lock(&p->rurw_mutex);
      if (unlikely(s->thr))
        goto locked;

      assert(s->state.all == 0);
      p->rurw_fullscan_bitmask &= ~mask;
      s->thr = current;
      return s;
    }
  }

fullscan:
  /* LY: fullscan if index approach failed. */
  for (s = p->state; s < p->state + RURW_LOCK_MAXTHREADS; ++s) {
    if (s->thr == current)
      return s;
  }

  if (!acquire)
    return NULL;

  ldap_pvt_thread_mutex_lock(&p->rurw_mutex);
locked:
  for (s = p->state; s < p->state + RURW_LOCK_MAXTHREADS; ++s) {
    if (!s->thr) {
      assert(s->state.all == 0);
      p->rurw_fullscan_bitmask |= mask;
      s->thr = current;
      return s;
    }
  }
  LDAP_BUG();
}

static void rurw_leave(rurw_lock_t *p) {
  if (!p->rurw_writer) {
    if (p->rurw_readers == 0 && p->rurw_upgrades_wait)
      ldap_pvt_thread_cond_signal(&p->rurw_wait4u);
    else if (p->rurw_readers == 0 && p->rurw_writers_wait)
      ldap_pvt_thread_cond_signal(&p->rurw_wait4w);
    else if (p->rurw_readers_wait)
      ldap_pvt_thread_cond_broadcast(&p->rurw_wait4r);
  }
  ldap_pvt_thread_mutex_unlock(&p->rurw_mutex);
}

void rurw_r_lock(rurw_lock_t *p) {
  rurw_thr_state_t *s = rurw_enter(p, 1);
  if (s->state.r) {
    LDAP_ENSURE(s->state.r < 0xFFFFu);
    s->state.r += 1;
    return;
  }

  while (p->rurw_writer) {
    p->rurw_readers_wait += 1;
    ldap_pvt_thread_cond_wait(&p->rurw_wait4r, &p->rurw_mutex);
    p->rurw_readers_wait -= 1;
  }
  s->state.r = 1;
  p->rurw_readers += 1;

  ldap_pvt_thread_mutex_unlock(&p->rurw_mutex);
}

void rurw_r_unlock(rurw_lock_t *p) {
  rurw_thr_state_t *s = rurw_enter(p, 0);
  assert(s != NULL);
  if (!s)
    return;

  ldap_pvt_thread_mutex_lock(&p->rurw_mutex);
  assert(s->state.r > 0 && p->rurw_readers > 0);
  s->state.r -= 1;
  if (s->state.r == 0) {
    if (s->state.w == 0)
      s->thr = 0;
    p->rurw_readers -= 1;
  }
  rurw_leave(p);
}

void rurw_w_lock(rurw_lock_t *p) {
  rurw_thr_state_t *s = rurw_enter(p, 1);
  if (s->state.w) {
    LDAP_ENSURE(s->state.w < 0xFFFFu);
    s->state.w += 1;
    return;
  }

  while (p->rurw_readers || p->rurw_writer) {
    if (s->state.r) {
      p->rurw_upgrades_wait += 1;
      ldap_pvt_thread_cond_wait(&p->rurw_wait4u, &p->rurw_mutex);
      p->rurw_upgrades_wait -= 1;
    } else {
      p->rurw_writers_wait += 1;
      ldap_pvt_thread_cond_wait(&p->rurw_wait4w, &p->rurw_mutex);
      p->rurw_writers_wait -= 1;
    }
  }
  s->state.w = 1;
  p->rurw_writer = 1;

  ldap_pvt_thread_mutex_unlock(&p->rurw_mutex);
}

void rurw_w_unlock(rurw_lock_t *p) {
  rurw_thr_state_t *s = rurw_enter(p, 0);
  assert(s != NULL);
  if (!s)
    return;

  ldap_pvt_thread_mutex_lock(&p->rurw_mutex);
  assert(s->state.w > 0 && p->rurw_writer);
  s->state.w -= 1;
  if (s->state.w == 0) {
    if (s->state.r == 0)
      s->thr = 0;
    p->rurw_writer = 0;
  }
  rurw_leave(p);
}

rurw_lock_deep_t rurw_retreat(rurw_lock_t *p) {
  rurw_thr_state_t *s = rurw_enter(p, 0);
  rurw_lock_deep_t state;
  if (!s) {
    state.all = 0;
    return state;
  }

  ldap_pvt_thread_mutex_lock(&p->rurw_mutex);
  state.all = s->state.all;
  assert(state.all != 0);

  if (s->state.r) {
    assert(p->rurw_readers > 0);
    s->state.r = 0;
    p->rurw_readers -= 1;
  }
  if (s->state.w) {
    assert(p->rurw_writer);
    s->state.w = 0;
    p->rurw_writer = 0;
  }
  s->thr = 0;

  rurw_leave(p);
  return state;
}

void rurw_obtain(rurw_lock_t *p, rurw_lock_deep_t state) {
  if (state.all == 0)
    return;

  rurw_thr_state_t *s = rurw_enter(p, 1);
  s->state = state;
  if (s->state.w) {
    while (p->rurw_readers || p->rurw_writer) {
      p->rurw_writers_wait += 1;
      ldap_pvt_thread_cond_wait(&p->rurw_wait4w, &p->rurw_mutex);
      p->rurw_writers_wait -= 1;
    }
    p->rurw_writer = 1;
    p->rurw_readers += s->state.r > 0;
  } else {
    while (p->rurw_writer) {
      p->rurw_readers_wait += 1;
      ldap_pvt_thread_cond_wait(&p->rurw_wait4r, &p->rurw_mutex);
      p->rurw_readers_wait -= 1;
    }
    p->rurw_readers += 1;
  }
  ldap_pvt_thread_mutex_unlock(&p->rurw_mutex);
}
