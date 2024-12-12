/* $ReOpenLDAP$ */
/* Copyright 2011-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "back-mdb.h"
#include <lutil.h>
#include <ldap_rq.h>
#include "slapconfig.h"

static const struct berval mdmi_databases[] = {BER_BVC("ad2i"), BER_BVC("dn2i"),
                                               BER_BVC("id2e"), BER_BVC("id2v"),
                                               BER_BVNULL};

static int mdb_id_compare(const MDBX_val *a, const MDBX_val *b) {
  return mdbx_cmp2int(*(ID *)a->iov_base, *(ID *)b->iov_base);
}

static void mdbx_debug(MDBX_log_level_t log, const char *function, int line,
                       const char *msg, va_list args) {
  int level;
  if (log < MDBX_LOG_VERBOSE)
    level = LDAP_DEBUG_ANY;
  else if (log == MDBX_LOG_VERBOSE)
    level = LDAP_DEBUG_NONE;
  else
    level = LDAP_DEBUG_TRACE;

  vLog(level, (int)log, msg, args);
}

/* perform kick/kill a laggard readers */
static int mdb_oom_handler(const MDBX_env *env, const MDBX_txn *txn,
                           mdbx_pid_t pid, mdbx_tid_t tid, uint64_t laggard,
                           unsigned gap, size_t space, int retry) {
  uint64_t now_ns = ldap_now_steady_ns();
  struct mdb_info *mdb = mdbx_env_get_userctx(env);
  uint64_t txnid = mdbx_txn_id(txn);
  (void)tid;

  if (retry < 0) {
    double elapsed = (now_ns - mdb->mi_oom_timestamp_ns) * 1e-9;
    const char *suffix = "s";
    if (elapsed < 1) {
      elapsed *= 1000;
      suffix = "ms";
    }

    if (gap) {
      Debug(LDAP_DEBUG_ANY,
            "oom-handler: done, txnid %zu, got/forward %u, elapsed/waited "
            "%.3f%s, %d retries\n",
            txnid, gap, elapsed, suffix, -retry);
    } else {
      Debug(
          LDAP_DEBUG_ANY,
          "oom-handler: unable, txnid %zu, elapsed/waited %.3f%s, %d retries\n",
          txnid, elapsed, suffix, -retry);
    }

    mdb->mi_oom_timestamp_ns = 0;
    return -1;
  }

  if (!mdb->mi_oom_flags)
    return -1;
  else if (!mdb->mi_oom_timestamp_ns) {
    mdb->mi_oom_timestamp_ns = now_ns;
    Debug(LDAP_DEBUG_ANY, "oom-handler: begin, txnid %zu lag %u\n", txnid, gap);
  }

  if ((mdb->mi_oom_flags & MDBX_OOM_KILL) && pid != getpid()) {
    if (kill(pid, SIGKILL) == 0) {
      Debug(LDAP_DEBUG_ANY, "oom-handler: txnid %zu lag %u, KILLED pid %i\n",
            txnid, gap, pid);
      ldap_pvt_thread_yield();
      return 2;
    }
    if (errno == ESRCH)
      return 0;
    Debug(LDAP_DEBUG_ANY, "oom-handler: retry %d, UNABLE kill pid %i: %s\n",
          retry, pid, STRERROR(errno));
  }

  if ((mdb->mi_oom_flags & MDBX_OOM_YIELD) && !slapd_shutdown) {
    if (retry == 0)
      Debug(LDAP_DEBUG_ANY, "oom-handler: txnid %zu lag %u, WAITING\n", txnid,
            gap);
    if (retry < 42)
      ldap_pvt_thread_yield();
    else
      usleep(retry);
    return 0;
  }

  mdb->mi_oom_timestamp_ns = 0;
  return -1;
}

static int mdb_db_init(BackendDB *be, ConfigReply *cr) {
  struct mdb_info *mdb;
  int rc;

  Debug(LDAP_DEBUG_TRACE,
        LDAP_XSTRING(mdb_db_init) ": Initializing mdb database\n");

  /* TODO: MDBX_DBG_AUDIT, MDBX_DBG_DUMP */
  int bits =
      mdbx_setup_debug(MDBX_LOG_DONTCHANGE, MDBX_DBG_DONTCHANGE, mdbx_debug);
  int loglevel = bits & 7;
  int dbgflags = bits - loglevel;

  dbgflags &= ~(MDBX_DBG_ASSERT | MDBX_DBG_JITTER);
  dbgflags |= MDBX_DBG_LEGACY_OVERLAP;
  if (reopenldap_mode_check())
    dbgflags |= MDBX_DBG_ASSERT;
  if (reopenldap_mode_jitter())
    dbgflags |= MDBX_DBG_JITTER;

  if (loglevel < MDBX_LOG_WARN)
    loglevel = MDBX_LOG_WARN;
#if LDAP_DEBUG > 1
  if (loglevel < MDBX_LOG_VERBOSE)
    MDBX_LOG_VERBOSE = MDBX_LOG_VERBOSE;
#endif /* LDAP_DEBUG > 1 */
#if LDAP_DEBUG > 2
  if (loglevel < MDBX_LOG_EXTRA)
    loglevel = MDBX_LOG_EXTRA;
#endif /* LDAP_DEBUG > 2 */
  mdbx_setup_debug((MDBX_log_level_t)loglevel, (MDBX_debug_flags_t)dbgflags,
                   mdbx_debug);

  /* allocate backend-database-specific stuff */
  mdb = (struct mdb_info *)ch_calloc(1, sizeof(struct mdb_info));

  /* DBEnv parameters */
  mdb->mi_dbenv_home = ch_strdup(SLAPD_DEFAULT_DB_DIR);
  mdb->mi_dbenv_flags = 0;
  mdb->mi_dbenv_mode = SLAPD_DEFAULT_DB_MODE;

  mdb->mi_search_stack_depth = DEFAULT_SEARCH_STACK_DEPTH;
  mdb->mi_search_stack = NULL;

  mdb->mi_mapsize = DEFAULT_MAPSIZE;
  mdb->mi_rtxn_size = DEFAULT_RTXN_SIZE;
  mdb->mi_multi_hi = UINT_MAX;
  mdb->mi_multi_lo = UINT_MAX;

  be->be_private = mdb;
  be->be_cf_ocs = be->bd_info->bi_cf_ocs;

#ifndef MDB_MULTIPLE_SUFFIXES
  SLAP_DBFLAGS(be) |= SLAP_DBFLAG_ONE_SUFFIX;
#endif

  slap_backtrace_set_dir(mdb->mi_dbenv_home);

  ldap_pvt_thread_mutex_init(&mdb->mi_ads_mutex);

  rc = mdb_monitor_db_init(be);

  return rc;
}

static int mdb_db_close(BackendDB *be, ConfigReply *cr);

static int mdb_db_open(BackendDB *be, ConfigReply *cr) {
  int rc, i;
  struct mdb_info *mdb = (struct mdb_info *)be->be_private;
  struct stat stat1;
  uint32_t flags;
  char *dbhome;
  MDBX_txn *txn;

  if (be->be_suffix == NULL) {
    Debug(LDAP_DEBUG_ANY, LDAP_XSTRING(mdb_db_open) ": need suffix.\n");
    return -1;
  }

  Debug(LDAP_DEBUG_ARGS, LDAP_XSTRING(mdb_db_open) ": \"%s\"\n",
        be->be_suffix[0].bv_val);

  /* Check existence of dbenv_home. Any error means trouble */
  rc = stat(mdb->mi_dbenv_home, &stat1);
  if (rc != 0) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(
              mdb_db_open) ": database \"%s\": "
                           "cannot access database directory \"%s\" (%d).\n",
          be->be_suffix[0].bv_val, mdb->mi_dbenv_home, errno);
    return -1;
  }

  /* mdb is always clean */
  be->be_flags |= SLAP_DBFLAG_CLEAN;

  rc = mdbx_env_create(&mdb->mi_dbenv);
  if (rc != 0) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
                                    "mdbx_env_create failed: %s (%d).\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
    goto fail;
  }

  mdbx_env_set_userctx(mdb->mi_dbenv, mdb);

  if (mdb->mi_readers) {
    rc = mdbx_env_set_maxreaders(mdb->mi_dbenv, mdb->mi_readers);
    if (rc != 0) {
      Debug(LDAP_DEBUG_ANY,
            LDAP_XSTRING(
                mdb_db_open) ": database \"%s\": "
                             "mdbx_env_set_maxreaders failed: %s (%d).\n",
            be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
      goto fail;
    }
  }

  /* FIXME: use mdbx_env_set_geometry() */
  rc = mdbx_env_set_mapsize(mdb->mi_dbenv, mdb->mi_mapsize);
  if (rc != 0) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
                                    "mdbx_env_set_mapsize failed: %s (%d).\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
    goto fail;
  }

  rc = mdbx_env_set_maxdbs(mdb->mi_dbenv, MDB_INDICES);
  if (rc != 0) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
                                    "mdbx_env_set_maxdbs failed: %s (%d).\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
    goto fail;
  }

  mdbx_env_set_hsr(mdb->mi_dbenv, mdb_oom_handler);
  if ((slapMode & SLAP_SERVER_MODE) && SLAP_MULTIMASTER(be) &&
      ((MDBX_OOM_YIELD & mdb->mi_oom_flags) == 0 || mdb->mi_renew_lag == 0)) {
    snprintf(cr->msg, sizeof(cr->msg),
             "database \"%s\": "
             "for properly operation in multi-master mode"
             " 'oom-handler yield' and 'dreamcatcher' are recommended",
             be->be_suffix[0].bv_val);
    Debug(LDAP_DEBUG_ANY, LDAP_XSTRING(mdb_db_open) ": %s\n", cr->msg);
  }

  dbhome = mdb->mi_dbenv_home;

  Debug(LDAP_DEBUG_TRACE,
        LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
                                  "dbenv_open(%s).\n",
        be->be_suffix[0].bv_val, mdb->mi_dbenv_home);

  flags = mdb->mi_dbenv_flags;

  if (reopenldap_mode_check())
    flags |= MDBX_PAGEPERTURB;

  if (slapMode & SLAP_TOOL_QUICK)
    flags |= MDBX_SAFE_NOSYNC | MDBX_WRITEMAP;

  if (slapMode & SLAP_TOOL_READONLY)
    flags |= MDBX_RDONLY;

  rc = mdbx_env_open(mdb->mi_dbenv, dbhome, flags, mdb->mi_dbenv_mode);

  if (rc) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(
              mdb_db_open) ": database \"%s\" cannot be opened: %s (%d). "
                           "Restore from backup!\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
    goto fail;
  }

  if ((slapMode & SLAP_TOOL_READONLY) == 0) {
    rc = mdbx_env_set_syncbytes(mdb->mi_dbenv, mdb->mi_txn_cp_kbyte * 1024ul);
    if (rc != 0) {
      Debug(
          LDAP_DEBUG_ANY,
          LDAP_XSTRING(mdb_db_open) ": database \"%s\": "
                                    "mdbx_env_set_syncbytes failed: %s (%d).\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
      goto fail;
    }
  }

  rc = mdbx_txn_begin(mdb->mi_dbenv, NULL, flags & MDBX_RDONLY, &txn);
  if (rc) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(
              mdb_db_open) ": database \"%s\" cannot be opened: %s (%d). "
                           "Restore from backup!\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
    goto fail;
  }

  /* open (and create) main databases */
  for (i = 0; mdmi_databases[i].bv_val; i++) {
    flags = MDBX_INTEGERKEY;
    if (i == MDB_ID2ENTRY) {
      if (!(slapMode & (SLAP_TOOL_READMAIN | SLAP_TOOL_READONLY)))
        flags |= MDBX_CREATE;
    } else {
      if (i == MDB_DN2ID)
        flags |= MDBX_DUPSORT;
      if (i == MDB_ID2VAL)
        flags ^= MDBX_INTEGERKEY | MDBX_DUPSORT;
      if (!(slapMode & SLAP_TOOL_READONLY))
        flags |= MDBX_CREATE;
    }

    MDBX_cmp_func *keycmp = NULL;
    MDBX_cmp_func *datacmp = NULL;
    if (i == MDB_ID2ENTRY)
      keycmp = mdb_id_compare;
    else if (i == MDB_ID2VAL) {
      keycmp = mdb_id2v_compare;
      datacmp = mdb_id2v_dupsort;
    } else if (i == MDB_DN2ID)
      datacmp = mdb_dup_compare;

    rc = mdbx_dbi_open_ex(txn, mdmi_databases[i].bv_val, flags,
                          &mdb->mi_dbis[i], keycmp, datacmp);

    if (rc != 0) {
      snprintf(cr->msg, sizeof(cr->msg),
               "database \"%s\": "
               "mdbx_dbi_open(%s/%s) failed: %s (%d).",
               be->be_suffix[0].bv_val, mdb->mi_dbenv_home,
               mdmi_databases[i].bv_val, mdbx_strerror(rc), rc);
      Debug(LDAP_DEBUG_ANY, LDAP_XSTRING(mdb_db_open) ": %s\n", cr->msg);
      goto fail;
    }

    if (i == MDB_DN2ID) {
      MDBX_cursor *mc;
      MDBX_val key, data;
      /* check for old dn2id format */
      rc = mdbx_cursor_open(txn, mdb->mi_dbis[i], &mc);
      /* first record is always ID 0 */
      rc = mdbx_cursor_get(mc, &key, &data, MDBX_FIRST);
      if (rc == 0) {
        rc = mdbx_cursor_get(mc, &key, &data, MDBX_NEXT);
        if (rc == 0) {
          int len;
          unsigned char *ptr;
          ptr = data.iov_base;
          len = (ptr[0] & 0x7f) << 8 | ptr[1];
          if (data.iov_len < 2 * len + 4 + 2 * sizeof(ID)) {
            snprintf(cr->msg, sizeof(cr->msg),
                     "database \"%s\": DN index needs upgrade, "
                     "run \"slapindex entryDN\".",
                     be->be_suffix[0].bv_val);
            Debug(LDAP_DEBUG_ANY, LDAP_XSTRING(mdb_db_open) ": %s\n", cr->msg);
            if (!(slapMode & SLAP_TOOL_READMAIN))
              rc = LDAP_OTHER;
            mdb->mi_flags |= MDB_NEED_UPGRADE;
          }
        }
      }
      mdbx_cursor_close(mc);
      if (rc == LDAP_OTHER)
        goto fail;
    }
  }

  rc = mdb_ad_read(mdb, txn);
  if (rc) {
    mdbx_txn_abort(txn);
    goto fail;
  }

  /* slapcat doesn't need indexes. avoid a failure if
   * a configured index wasn't created yet.
   */
  if (!(slapMode & SLAP_TOOL_READONLY)) {
    rc = mdb_attr_dbs_open(be, txn, cr);
    if (rc) {
      mdbx_txn_abort(txn);
      goto fail;
    }
  }

  rc = mdbx_txn_commit(txn);
  if (rc != 0) {
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(mdb_db_open) ": database %s: "
                                    "txn_commit failed: %s (%d)\n",
          be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
    goto fail;
  }

  /* monitor setup */
  rc = mdb_monitor_db_open(be);
  if (rc != 0) {
    goto fail;
  }

  mdb->mi_flags |= MDB_IS_OPEN;

  return 0;

fail:
  mdb_db_close(be, NULL);
  return rc;
}

static int mdb_db_close(BackendDB *be, ConfigReply *cr) {
  int rc;
  struct mdb_info *mdb = (struct mdb_info *)be->be_private;

  /* monitor handling */
  (void)mdb_monitor_db_close(be);

  mdb->mi_flags &= ~MDB_IS_OPEN;

  if (mdb->mi_dbenv) {
    mdb_reader_flush(mdb->mi_dbenv);
  }

  if (mdb->mi_dbenv) {
    if (mdb->mi_dbis[0]) {
      int i;

      mdb_attr_dbs_close(mdb);
      for (i = 0; i < MDB_NDB; i++)
        mdbx_dbi_close(mdb->mi_dbenv, mdb->mi_dbis[i]);

      /* force a sync, but not if we were ReadOnly,
       * and not in Quick mode.
       */
      if (!(slapMode & (SLAP_TOOL_QUICK | SLAP_TOOL_READONLY))) {
        rc = mdbx_env_sync(mdb->mi_dbenv);
        if (rc != MDBX_SUCCESS && rc != MDBX_RESULT_TRUE) {
          Debug(LDAP_DEBUG_ANY,
                "mdb_db_close: database \"%s\": "
                "mdb_env_sync failed: %s (%d).\n",
                be->be_suffix[0].bv_val, mdbx_strerror(rc), rc);
        }
      }
    }

    mdbx_env_close(mdb->mi_dbenv);
    mdb->mi_dbenv = NULL;
  }

  if (mdb->mi_search_stack) {
    ch_free(mdb->mi_search_stack);
    mdb->mi_search_stack = NULL;
  }

  return 0;
}

static int mdb_db_destroy(BackendDB *be, ConfigReply *cr) {
  struct mdb_info *mdb = (struct mdb_info *)be->be_private;

  /* stop and remove checkpoint task */
  if (mdb->mi_txn_cp_task) {
    struct re_s *re = mdb->mi_txn_cp_task;
    mdb->mi_txn_cp_task = NULL;
    ldap_pvt_thread_mutex_lock(&slapd_rq.rq_mutex);
    if (ldap_pvt_runqueue_isrunning(&slapd_rq, re))
      ldap_pvt_runqueue_stoptask(&slapd_rq, re);
    ldap_pvt_runqueue_remove(&slapd_rq, re);
    ldap_pvt_thread_mutex_unlock(&slapd_rq.rq_mutex);
  }

  /* monitor handling */
  (void)mdb_monitor_db_destroy(be);

  if (mdb->mi_dbenv_home)
    ch_free(mdb->mi_dbenv_home);

  mdb_attr_index_destroy(mdb);

  ch_free(mdb);
  be->be_private = NULL;

  return 0;
}

int mdb_back_initialize(BackendInfo *bi) {
  int rc;

  static const char *const controls[] = {LDAP_CONTROL_ASSERT,
                                         LDAP_CONTROL_MANAGEDSAIT,
                                         LDAP_CONTROL_NOOP,
                                         LDAP_CONTROL_PAGEDRESULTS,
                                         LDAP_CONTROL_PRE_READ,
                                         LDAP_CONTROL_POST_READ,
                                         LDAP_CONTROL_SUBENTRIES,
                                         LDAP_CONTROL_X_PERMISSIVE_MODIFY,
#ifdef LDAP_X_TXN
                                         LDAP_CONTROL_X_TXN_SPEC,
#endif
                                         NULL};

  /* initialize the underlying database system */
  Debug(LDAP_DEBUG_TRACE,
        LDAP_XSTRING(mdb_back_initialize) ": initialize " MDB_UCTYPE
                                          " backend\n");

  bi->bi_flags |= SLAP_BFLAG_INCREMENT | SLAP_BFLAG_SUBENTRIES |
                  SLAP_BFLAG_ALIASES | SLAP_BFLAG_REFERRALS;

  bi->bi_controls = controls;

  /* version check */
  if (mdbx_version.major != MDBX_VERSION_MAJOR ||
      mdbx_version.minor != MDBX_VERSION_MINOR) {
    /* fail if a versions don't match */
    Debug(LDAP_DEBUG_ANY,
          LDAP_XSTRING(mdb_back_initialize) ": "
                                            "MDB library version mismatch:"
                                            " expected %u.%u, got %u.%u\n",
          MDBX_VERSION_MAJOR, MDBX_VERSION_MINOR, mdbx_version.major,
          mdbx_version.minor);
    return -1;
  }
  Debug(LDAP_DEBUG_TRACE, LDAP_XSTRING(mdb_back_initialize) ": %u.%u.%u.%u\n",
        mdbx_version.major, mdbx_version.minor, mdbx_version.patch,
        mdbx_version.tweak);

  bi->bi_open = 0;
  bi->bi_close = 0;
  bi->bi_config = 0;
  bi->bi_destroy = 0;

  bi->bi_db_init = mdb_db_init;
  bi->bi_db_config = config_generic_wrapper;
  bi->bi_db_open = mdb_db_open;
  bi->bi_db_close = mdb_db_close;
  bi->bi_db_destroy = mdb_db_destroy;

  bi->bi_op_add = mdb_add;
  bi->bi_op_bind = mdb_bind;
  bi->bi_op_compare = mdb_compare;
  bi->bi_op_delete = mdb_delete;
  bi->bi_op_modify = mdb_modify;
  bi->bi_op_modrdn = mdb_modrdn;
  bi->bi_op_search = mdb_search;

  bi->bi_op_unbind = 0;
#ifdef LDAP_X_TXN
  bi->bi_op_txn = mdb_txn;
#endif

  bi->bi_extended = mdb_extended;

  bi->bi_chk_referrals = 0;
  bi->bi_operational = mdb_operational;

  bi->bi_has_subordinates = mdb_hasSubordinates;
  bi->bi_entry_release_rw = mdb_entry_release;
  bi->bi_entry_get_rw = mdb_entry_get;

  /*
   * hooks for slap tools
   */
  bi->bi_tool_entry_open = mdb_tool_entry_open;
  bi->bi_tool_entry_close = mdb_tool_entry_close;
  bi->bi_tool_entry_first = backend_tool_entry_first;
  bi->bi_tool_entry_first_x = mdb_tool_entry_first_x;
  bi->bi_tool_entry_next = mdb_tool_entry_next;
  bi->bi_tool_entry_get = mdb_tool_entry_get;
  bi->bi_tool_entry_put = mdb_tool_entry_put;
  bi->bi_tool_entry_reindex = mdb_tool_entry_reindex;
  bi->bi_tool_sync = 0;
  bi->bi_tool_dn2id_get = mdb_tool_dn2id_get;
  bi->bi_tool_entry_modify = mdb_tool_entry_modify;
  bi->bi_tool_entry_delete = mdb_tool_entry_delete;

  bi->bi_connection_init = 0;
  bi->bi_connection_destroy = 0;

  rc = mdb_back_init_cf(bi);

  return rc;
}

#if (SLAPD_MDBX == SLAPD_MOD_DYNAMIC)
SLAP_BACKEND_INIT_MODULE(mdb)
#endif /* SLAPD_MDBX == SLAPD_MOD_DYNAMIC */
