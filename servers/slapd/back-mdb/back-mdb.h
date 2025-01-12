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

#ifndef _BACK_MDB_H_
#define _BACK_MDB_H_

#include <reldap.h>
#include "slap.h"

#define MDBX_DEPRECATED
#include "mdbx.h"

#ifndef mdbx_cmp2int
#define mdbx_cmp2int(a, b) (((b) > (a)) ? -1 : (a) > (b))
#endif

LDAP_BEGIN_DECL

#undef MDB_TOOL_IDL_CACHING /* currently no perf gain */

#define DN_BASE_PREFIX SLAP_INDEX_EQUALITY_PREFIX
#define DN_ONE_PREFIX '%'
#define DN_SUBTREE_PREFIX '@'

#define MDB_AD2ID 0
#define MDB_DN2ID 1
#define MDB_ID2ENTRY 2
#define MDB_ID2VAL 3
#define MDB_NDB 4

/* The default search IDL stack cache depth */
#define DEFAULT_SEARCH_STACK_DEPTH 16

/* The minimum we can function with */
#define MINIMUM_SEARCH_STACK_DEPTH 8

#define MDB_INDICES 128

#define MDB_MAXADS 65536

/* Default to 10MB max */
#define DEFAULT_MAPSIZE (10 * 1048576)

/* Most users will never see this */
#define DEFAULT_RTXN_SIZE 10000

#if LDAP_EXPERIMENTAL > 0
#define MDB_MONITOR_IDX 1
#endif /* LDAP_EXPERIMENTAL > 0 */

typedef struct mdb_monitor_t {
  void *mdm_cb;
  struct berval mdm_ndn;
} mdb_monitor_t;

/* From ldap_rq.h */
struct re_s;

struct mdb_info {
  MDBX_env *mi_dbenv;

  /* DB_ENV parameters */
  char *mi_dbenv_home;
  uint32_t mi_dbenv_flags;
  int mi_dbenv_mode;

  size_t mi_mapsize;
  volatile ID _mi_nextid;
  size_t mi_maxentrysize;

  slap_mask_t mi_defaultmask;
  int mi_nattrs;
  struct mdb_attrinfo **mi_attrs;
  void *mi_search_stack;
  int mi_search_stack_depth;
  int mi_readers;

  uint32_t mi_rtxn_size;
  int mi_txn_cp;
  uint32_t mi_txn_cp_period;
  uint32_t mi_txn_cp_kbyte;

  struct re_s *mi_txn_cp_task;
  struct re_s *mi_index_task;
  uint32_t mi_renew_lag;
  uint32_t mi_renew_percent;
#define MDBX_OOM_KILL 1
#define MDBX_OOM_YIELD 2
  int mi_oom_flags;
  uint64_t mi_oom_timestamp_ns;

  mdb_monitor_t mi_monitor;

#ifdef MDB_MONITOR_IDX
  ldap_pvt_thread_mutex_t mi_idx_mutex;
  Avlnode *mi_idx;
#endif /* MDB_MONITOR_IDX */

  int mi_flags;
#define MDB_IS_OPEN 0x01
#define MDB_OPEN_INDEX 0x02
#define MDB_DEL_INDEX 0x08
#define MDB_RE_OPEN 0x10
#define MDB_NEED_UPGRADE 0x20

  ldap_pvt_thread_mutex_t mi_ads_mutex;
  int mi_numads;

  unsigned mi_multi_hi;
  /* more than this many values in an attr goes
   * into a separate DB */
  unsigned mi_multi_lo;
  /* less than this many values in an attr goes
   * back into main blob */

  MDBX_dbi mi_dbis[MDB_NDB];
  AttributeDescription *mi_ads[MDB_MAXADS];
  int mi_adxs[MDB_MAXADS];
};

#ifdef __SANITIZE_THREAD__
ID mdb_read_nextid(struct mdb_info *mdb);
#else
static __inline ID mdb_read_nextid(struct mdb_info *mdb) { return mdb->_mi_nextid; }
#endif

#define mi_id2entry mi_dbis[MDB_ID2ENTRY]
#define mi_dn2id mi_dbis[MDB_DN2ID]
#define mi_ad2id mi_dbis[MDB_AD2ID]
#define mi_id2val mi_dbis[MDB_ID2VAL]

typedef struct mdb_op_info {
  OpExtra moi_oe;
  MDBX_txn *moi_txn;
  int moi_ref;
  char moi_flag;
} mdb_op_info;
#define MOI_READER 0x01
#define MOI_FREEIT 0x02
#define MOI_KEEPER 0x04

LDAP_END_DECL

/* for the cache of attribute information (which are indexed, etc.) */
typedef struct mdb_attrinfo {
  AttributeDescription *ai_desc; /* attribute description cn;lang-en */
  slap_mask_t ai_indexmask;      /* how the attr is indexed	*/
  slap_mask_t ai_newmask;        /* new settings to replace old mask */
#ifdef LDAP_COMP_MATCH
  ComponentReference *ai_cr; /*component indexing*/
#endif
  TAvlnode *ai_root;      /* for tools */
  void *ai_flist;         /* for tools */
  void *ai_clist;         /* for tools */
  MDBX_cursor *ai_cursor; /* for tools */
  int ai_idx;             /* position in AI array */
  MDBX_dbi ai_dbi;
  unsigned ai_multi_hi;
  unsigned ai_multi_lo;
} AttrInfo;

/* tool threaded indexer state */
typedef struct mdb_attrixinfo {
  OpExtra ai_oe;
  void *ai_flist;
  void *ai_clist;
  AttrInfo *ai_ai;
} AttrIxInfo;

/* These flags must not clash with SLAP_INDEX flags or ops in slap.h! */
#define MDB_INDEX_DELETING 0x8000U /* index is being modified */
#define MDB_INDEX_UPDATE_OP 0x03   /* performing an index update */

/* For slapindex to record which attrs in an entry belong to which
 * index database
 */
typedef struct AttrList {
  struct AttrList *next;
  Attribute *attr;
} AttrList;

#ifndef CACHELINE
#define CACHELINE 64
#endif

#if defined(__i386) || defined(__x86_64)
#define MISALIGNED_OK 1
#else
#define ALIGNER (sizeof(size_t) - 1)
#endif

typedef struct IndexRbody {
  AttrInfo *ai;
  AttrList *attrs;
  void *tptr;
  int i;
} IndexRbody;

typedef struct IndexRec {
  union {
    IndexRbody irb;
#define ir_ai iru.irb.ai
#define ir_attrs iru.irb.attrs
#define ir_tptr iru.irb.tptr
#define ir_i iru.irb.i
    /* cache line alignment */
    char pad[(sizeof(IndexRbody) + CACHELINE - 1) & (!CACHELINE - 1)];
  } iru;
} IndexRec;

#define MAXRDNS SLAP_LDAPDN_MAXLEN / 4

#include "proto-mdb.h"

#endif /* _BACK_MDB_H_ */
