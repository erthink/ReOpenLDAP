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

#ifndef _PROTO_MDB_H
#define _PROTO_MDB_H

LDAP_BEGIN_DECL

#define MDB_UCTYPE "MDB"

/*
 * attr.c
 */

AttrInfo *mdb_attr_mask(struct mdb_info *mdb, AttributeDescription *desc);

void mdb_attr_flush(struct mdb_info *mdb);

int mdb_attr_slot(struct mdb_info *mdb, AttributeDescription *desc, int *insert);

int mdb_attr_dbs_open(BackendDB *be, MDBX_txn *txn, struct config_reply_s *cr);
void mdb_attr_dbs_close(struct mdb_info *mdb);

int mdb_attr_index_config(struct mdb_info *mdb, const char *fname, int lineno, int argc, char **argv,
                          struct config_reply_s *cr);

void mdb_attr_index_unparse(struct mdb_info *mdb, BerVarray *bva);
void mdb_attr_index_destroy(struct mdb_info *mdb);
void mdb_attr_index_free(struct mdb_info *mdb, AttributeDescription *ad);

int mdb_attr_multi_config(struct mdb_info *mdb, const char *fname, int lineno, int argc, char **argv,
                          struct config_reply_s *cr);

void mdb_attr_multi_unparse(struct mdb_info *mdb, BerVarray *bva);

void mdb_attr_multi_thresh(struct mdb_info *mdb, AttributeDescription *ad, unsigned *hi, unsigned *lo);

void mdb_attr_info_free(AttrInfo *ai);

int mdb_ad_read(struct mdb_info *mdb, MDBX_txn *txn);
int mdb_ad_get(struct mdb_info *mdb, MDBX_txn *txn, AttributeDescription *ad);

/*
 * config.c
 */

int mdb_back_init_cf(BackendInfo *bi);

/*
 * dn2entry.c
 */

int mdb_dn2entry(Operation *op, MDBX_txn *tid, MDBX_cursor *mc, struct berval *dn, Entry **e, ID *nsubs, int matched);

/*
 * dn2id.c
 */

int mdb_dn2id(Operation *op, MDBX_txn *txn, MDBX_cursor *mc, struct berval *ndn, ID *id, ID *nsubs,
              struct berval *matched, struct berval *nmatched);

int mdb_dn2id_add(Operation *op, MDBX_cursor *mcp, MDBX_cursor *mcd, ID pid, ID nsubs, int upsub, Entry *e);

int mdb_dn2id_delete(Operation *op, MDBX_cursor *mc, ID id, ID nsubs);

int mdb_dn2id_children(Operation *op, MDBX_txn *tid, Entry *e);

int mdb_dn2sups(Operation *op, MDBX_txn *tid, struct berval *dn, ID *sups);

int mdb_dn2idl(Operation *op, MDBX_txn *txn, struct berval *ndn, ID eid, ID *ids, ID *stack);

int mdb_dn2id_parent(Operation *op, MDBX_txn *txn, ID eid, ID *idp);

int mdb_id2name(Operation *op, MDBX_txn *txn, MDBX_cursor **cursp, ID eid, struct berval *name, struct berval *nname);

int mdb_idscope(Operation *op, MDBX_txn *txn, ID base, ID *ids, ID *res);

struct IdScopes;

int mdb_idscopes(Operation *op, struct IdScopes *isc);

int mdb_idscopechk(Operation *op, struct IdScopes *isc);

int mdb_dn2id_walk(Operation *op, struct IdScopes *isc);

void mdb_dn2id_wrestore(Operation *op, struct IdScopes *isc);

MDBX_cmp_func mdb_dup_compare;

/*
 * filterentry.c
 */

int mdb_filter_candidates(Operation *op, MDBX_txn *txn, Filter *f, ID *ids, ID *tmp, ID *stack);

/*
 * id2entry.c
 */

MDBX_cmp_func mdb_id2v_compare;
MDBX_cmp_func mdb_id2v_dupsort;

int mdb_id2entry_add(Operation *op, MDBX_txn *tid, MDBX_cursor *mc, Entry *e);

int mdb_id2entry_update(Operation *op, MDBX_txn *tid, MDBX_cursor *mc, Entry *e);

int mdb_id2entry_delete(BackendDB *be, MDBX_txn *tid, Entry *e);

int mdb_id2entry(Operation *op, MDBX_cursor *mc, ID id, Entry **e);

int mdb_id2edata(Operation *op, MDBX_cursor *mc, ID id, MDBX_val *data);

int mdb_entry_return(Operation *op, Entry *e);
BI_entry_release_rw mdb_entry_release;
BI_entry_get_rw mdb_entry_get;
#ifdef LDAP_X_TXN
BI_op_txn mdb_txn;
#endif

int mdb_entry_decode(Operation *op, MDBX_txn *txn, MDBX_val *data, ID id, Entry **e);

void mdb_reader_flush(MDBX_env *env);
int mdb_opinfo_get(Operation *op, struct mdb_info *mdb, int rdonly, mdb_op_info **moi);

int mdb_mval_put(Operation *op, MDBX_cursor *mc, ID id, Attribute *a);
int mdb_mval_del(Operation *op, MDBX_cursor *mc, ID id, Attribute *a);

/*
 * idl.c
 */

unsigned mdb_idl_search(ID *ids, ID id);

int mdb_idl_fetch_key(BackendDB *be, MDBX_txn *txn, MDBX_dbi dbi, MDBX_val *key, ID *ids, MDBX_cursor **saved_cursor,
                      int get_flag);

int mdb_idl_insert(ID *ids, ID id);

typedef int(mdb_idl_keyfunc)(BackendDB *be, MDBX_cursor *mc, struct berval *key, ID id);

mdb_idl_keyfunc mdb_idl_insert_keys;
mdb_idl_keyfunc mdb_idl_delete_keys;

int mdb_idl_intersection(ID *a, ID *b);

int mdb_idl_union(ID *a, ID *b);

ID mdb_idl_first(ID *ids, ID *cursor);
ID mdb_idl_next(ID *ids, ID *cursor);

void mdb_idl_sort(ID *ids, ID *tmp);
int mdb_idl_append(ID *a, ID *b);
int mdb_idl_append_one(ID *ids, ID id);

/*
 * index.c
 */

extern AttrInfo *mdb_index_mask(Backend *be, AttributeDescription *desc, struct berval *name);

extern int mdb_index_param(Backend *be, AttributeDescription *desc, int ftype, MDBX_dbi *dbi, slap_mask_t *mask,
                           struct berval *prefix);

extern int mdb_index_values(Operation *op, MDBX_txn *txn, AttributeDescription *desc, BerVarray vals, ID id, int opid);

extern int mdb_index_recset(struct mdb_info *mdb, Attribute *a, AttributeType *type, struct berval *tags, IndexRec *ir);

extern int mdb_index_recrun(Operation *op, MDBX_txn *txn, struct mdb_info *mdb, IndexRec *ir, ID id, int base);

int mdb_index_entry(Operation *op, MDBX_txn *t, int r, Entry *e);

#define mdb_index_entry_add(op, t, e) mdb_index_entry((op), (t), SLAP_INDEX_ADD_OP, (e))
#define mdb_index_entry_del(op, t, e) mdb_index_entry((op), (t), SLAP_INDEX_DELETE_OP, (e))

/*
 * key.c
 */

extern int mdb_key_read(Backend *be, MDBX_txn *txn, MDBX_dbi dbi, struct berval *k, ID *ids, MDBX_cursor **saved_cursor,
                        int get_flags);

/*
 * nextid.c
 */

int mdb_next_id(BackendDB *be, MDBX_cursor *mc, ID *id);

/*
 * modify.c
 */

int mdb_modify_internal(Operation *op, MDBX_txn *tid, Modifications *modlist, Entry *e, const char **text,
                        char *textbuf, size_t textlen);

/*
 * monitor.c
 */

int mdb_monitor_db_init(BackendDB *be);
int mdb_monitor_db_open(BackendDB *be);
int mdb_monitor_db_close(BackendDB *be);
int mdb_monitor_db_destroy(BackendDB *be);

#ifdef MDB_MONITOR_IDX
int mdb_monitor_idx_add(struct mdb_info *mdb, AttributeDescription *desc, slap_mask_t type);
#endif /* MDB_MONITOR_IDX */

/*
 * former external.h
 */

extern BI_init mdb_back_initialize;

extern BI_db_config mdb_db_config;

extern BI_op_add mdb_add;
extern BI_op_bind mdb_bind;
extern BI_op_compare mdb_compare;
extern BI_op_delete mdb_delete;
extern BI_op_modify mdb_modify;
extern BI_op_modrdn mdb_modrdn;
extern BI_op_search mdb_search;
extern BI_op_extended mdb_extended;

extern BI_chk_referrals mdb_referrals;

extern BI_operational mdb_operational;

extern BI_has_subordinates mdb_hasSubordinates;

/* tools.c */
extern BI_tool_entry_open mdb_tool_entry_open;
extern BI_tool_entry_close mdb_tool_entry_close;
extern BI_tool_entry_first_x mdb_tool_entry_first_x;
extern BI_tool_entry_next mdb_tool_entry_next;
extern BI_tool_entry_get mdb_tool_entry_get;
extern BI_tool_entry_put mdb_tool_entry_put;
extern BI_tool_entry_reindex mdb_tool_entry_reindex;
extern BI_tool_dn2id_get mdb_tool_dn2id_get;
extern BI_tool_entry_modify mdb_tool_entry_modify;
extern BI_tool_entry_delete mdb_tool_entry_delete;

extern mdb_idl_keyfunc mdb_tool_idl_add;

LDAP_END_DECL

#endif /* _PROTO_MDB_H */
