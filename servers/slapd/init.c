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

/* init.c - initialize various things */

#include "reldap.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lber_pvt.h"

#include "ldap_rq.h"

/*
 * read-only global variables or variables only written by the listener
 * thread (after they are initialized) - no need to protect them with a mutex.
 */
#ifdef LDAP_DEBUG
/* LY: debug level, controlled only by '-d' command line option */
LDAP_SLAPD_V(int) slap_debug_mask;
int slap_debug_mask = LDAP_DEBUG_NONE;
#endif /* LDAP_DEBUG */

#ifdef LDAP_SYSLOG
/* LY: syslog debug level, controlled by 'loglevel' in slapd.conf */
int slap_syslog_mask = LDAP_DEBUG_NONE;
/* LY: syslog severity for debug messages, controlled by 'syslog-severity' in
 * slapd.conf */
int slap_syslog_severity = LOG_DEBUG;
#endif /* LDAP_SYSLOG */

BerVarray default_referral = NULL;

/*
 * global variables that need mutex protection
 */
ldap_pvt_thread_pool_t connection_pool;
int connection_pool_max = SLAP_MAX_WORKER_THREADS;
int connection_pool_queues = 1;
int slap_tool_thread_max = 1;

slap_counters_t slap_counters, *slap_counters_list;

static const char *slap_name = NULL;
int slapMode = SLAP_UNDEFINED_MODE;

int slap_init(int mode, const char *name) {
  int rc;

  assert(mode);

  if (slapMode != SLAP_UNDEFINED_MODE) {
    /* Make sure we write something to stderr */
    Debug(LDAP_DEBUG_ANY, "%s init: init called twice (old=%d, new=%d)\n", name,
          slapMode, mode);

    return 1;
  }

  slapMode = mode;

  slap_op_init();

#ifdef SLAPD_DYNAMIC_MODULES
  if (module_init() != 0) {
    Debug(LDAP_DEBUG_ANY, "%s: module_init failed\n", name);
    return 1;
  }
#endif

  if (slap_schema_init() != 0) {
    Debug(LDAP_DEBUG_ANY, "%s: slap_schema_init failed\n", name);
    return 1;
  }

  quorum_global_init();

  if (filter_init() != 0) {
    Debug(LDAP_DEBUG_ANY, "%s: filter_init failed\n", name);
    return 1;
  }

  if (entry_init() != 0) {
    Debug(LDAP_DEBUG_ANY, "%s: entry_init failed\n", name);
    return 1;
  }

  switch (slapMode & SLAP_MODE) {
  case SLAP_SERVER_MODE:
    root_dse_init();

    /* FALLTHRU */
  case SLAP_TOOL_MODE:
    Debug(LDAP_DEBUG_TRACE, "%s init: initiated %s.\n", name,
          (mode & SLAP_MODE) == SLAP_TOOL_MODE ? "tool" : "server");

    slap_name = name;

    ldap_pvt_thread_pool_init_q(&connection_pool, connection_pool_max, 0,
                                connection_pool_queues);

    slap_counters_init(&slap_counters);

    ldap_pvt_thread_mutex_init(&slapd_rq.rq_mutex);
    LDAP_STAILQ_INIT(&slapd_rq.task_list);
    LDAP_STAILQ_INIT(&slapd_rq.run_list);

    slap_passwd_init();

    rc = slap_sasl_init();

    if (rc == 0) {
      rc = backend_init();
    }
    if (rc)
      return rc;

    break;

  default:
    Debug(LDAP_DEBUG_ANY, "%s init: undefined mode (%d).\n", name, mode);

    rc = 1;
    break;
  }

  if (slap_controls_init() != 0) {
    Debug(LDAP_DEBUG_ANY, "%s: slap_controls_init failed\n", name);
    return 1;
  }

  if (frontend_init()) {
    Debug(LDAP_DEBUG_ANY, "%s: frontend_init failed\n", name);
    return 1;
  }

  if (overlay_init()) {
    Debug(LDAP_DEBUG_ANY, "%s: overlay_init failed\n", name);
    return 1;
  }

  if (glue_sub_init()) {
    Debug(LDAP_DEBUG_ANY, "%s: glue/subordinate init failed\n", name);

    return 1;
  }

  if (acl_init()) {
    Debug(LDAP_DEBUG_ANY, "%s: acl_init failed\n", name);
    return 1;
  }

  return rc;
}

int slap_startup(Backend *be) {
  int rc;
  Debug(LDAP_DEBUG_TRACE, "%s startup: initiated.\n", slap_name);

  rc = backend_startup(be);
  if (!rc && (slapMode & SLAP_SERVER_MODE))
    slapMode |= SLAP_SERVER_RUNNING;
  return rc;
}

int slap_shutdown(Backend *be) {
  Debug(LDAP_DEBUG_TRACE, "%s shutdown: initiated\n", slap_name);

  /* Make sure the pool stops now even if we did not start up fully */
  ldap_pvt_thread_pool_close(&connection_pool, 1);

  /* let backends do whatever cleanup they need to do */
  return backend_shutdown(be);
}

int slap_destroy(void) {
  int rc;

  Debug(LDAP_DEBUG_TRACE, "%s destroy: freeing system resources.\n", slap_name);

  if (default_referral) {
    ber_bvarray_free(default_referral);
  }

  ldap_pvt_thread_pool_free(&connection_pool);

  /* clear out any thread-keys for the main thread */
  ldap_pvt_thread_pool_context_reset(ldap_pvt_thread_pool_context());

  rc = backend_destroy();

  slap_sasl_destroy();

  /* rootdse destroy goes before entry_destroy()
   * because it may use entry_free() */
  root_dse_destroy();
  entry_destroy();

  switch (slapMode & SLAP_MODE) {
  case SLAP_SERVER_MODE:
  case SLAP_TOOL_MODE:
    slap_counters_destroy(&slap_counters);
    break;

  default:
    Debug(LDAP_DEBUG_ANY, "slap_destroy(): undefined mode (%d).\n", slapMode);

    rc = 1;
    break;
  }

  slap_op_destroy();

  ldap_pvt_thread_destroy();

  /* should destroy the above mutex */
  return rc;
}

void slap_counters_init(slap_counters_t *sc) {
  int i;

  ldap_pvt_thread_mutex_init(&sc->sc_mutex);
  ldap_pvt_mp_init(sc->sc_bytes);
  ldap_pvt_mp_init(sc->sc_pdu);
  ldap_pvt_mp_init(sc->sc_entries);
  ldap_pvt_mp_init(sc->sc_refs);

  ldap_pvt_mp_init(sc->sc_ops_initiated);
  ldap_pvt_mp_init(sc->sc_ops_completed);

#ifdef SLAPD_MONITOR
  for (i = 0; i < SLAP_OP_LAST; i++) {
    ldap_pvt_mp_init(sc->sc_ops_initiated_[i]);
    ldap_pvt_mp_init(sc->sc_ops_completed_[i]);
  }
#endif /* SLAPD_MONITOR */
}

void slap_counters_destroy(slap_counters_t *sc) {
  int i;

  ldap_pvt_thread_mutex_destroy(&sc->sc_mutex);
  ldap_pvt_mp_clear(sc->sc_bytes);
  ldap_pvt_mp_clear(sc->sc_pdu);
  ldap_pvt_mp_clear(sc->sc_entries);
  ldap_pvt_mp_clear(sc->sc_refs);

  ldap_pvt_mp_clear(sc->sc_ops_initiated);
  ldap_pvt_mp_clear(sc->sc_ops_completed);

#ifdef SLAPD_MONITOR
  for (i = 0; i < SLAP_OP_LAST; i++) {
    ldap_pvt_mp_clear(sc->sc_ops_initiated_[i]);
    ldap_pvt_mp_clear(sc->sc_ops_completed_[i]);
  }
#endif /* SLAPD_MONITOR */
}
