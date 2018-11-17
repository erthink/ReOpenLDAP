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

#ifndef PROTO_SLAP_H
#define PROTO_SLAP_H

#include <ldap_cdefs.h>
#include "ldap_pvt.h"

LDAP_BEGIN_DECL

struct config_args_s;  /* slapconfig.h */
struct config_reply_s; /* slapconfig.h */

/*
 * rurwlock.c
 */

LDAP_SLAPD_F(void) rurw_lock_init(rurw_lock_t *p);
LDAP_SLAPD_F(void) rurw_lock_destroy(rurw_lock_t *p);
LDAP_SLAPD_F(void) rurw_r_lock(rurw_lock_t *p);
LDAP_SLAPD_F(void) rurw_r_unlock(rurw_lock_t *p);
LDAP_SLAPD_F(void) rurw_w_lock(rurw_lock_t *p);
LDAP_SLAPD_F(void) rurw_w_unlock(rurw_lock_t *p);
LDAP_SLAPD_F(rurw_lock_deep_t) rurw_retreat(rurw_lock_t *p);
LDAP_SLAPD_F(void) rurw_obtain(rurw_lock_t *p, rurw_lock_deep_t state);

/*
 * biglock.c
 */
LDAP_SLAPD_F(void) slap_biglock_init(BackendDB *be);
LDAP_SLAPD_F(void) slap_biglock_destroy(BackendDB *be);
LDAP_SLAPD_F(slap_biglock_t *) slap_biglock_get(BackendDB *bd);
LDAP_SLAPD_F(void) slap_biglock_acquire(slap_biglock_t *bl);
LDAP_SLAPD_F(void) slap_biglock_release(slap_biglock_t *bl);
LDAP_SLAPD_F(int)
slap_biglock_call_be(slap_operation_t which, Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) slap_biglock_deep(BackendDB *be);
LDAP_SLAPD_F(int) slap_biglock_owned(BackendDB *be);
LDAP_SLAPD_F(int) slap_biglock_pool_pausing(BackendDB *bd);
LDAP_SLAPD_F(int) slap_biglock_pool_pause(BackendDB *be);
LDAP_SLAPD_F(int) slap_biglock_pool_pausecheck(BackendDB *be);
LDAP_SLAPD_F(int) slap_biglock_pool_resume(BackendDB *be);

LDAP_SLAPD_F(void) memleak_crutch_push(void *p);
LDAP_SLAPD_F(void) memleak_crutch_pop(void *p);
LDAP_SLAPD_F(void) rs_send_cleanup(SlapReply *rs);

/*
 * aci.c
 */
#ifdef SLAP_DYNACL
#ifdef SLAPD_ACI_ENABLED
LDAP_SLAPD_F(int) aci_over_initialize(void);
#endif /* SLAPD_ACI_ENABLED */
#endif /* SLAP_DYNACL */

/*
 * acl.c
 */
LDAP_SLAPD_F(int)
access_allowed_mask(Operation *op, Entry *e, AttributeDescription *desc,
                    struct berval *val, slap_access_t access,
                    AccessControlState *state, slap_mask_t *mask);
#define access_allowed(op, e, desc, val, access, state)                        \
  access_allowed_mask(op, e, desc, val, access, state, NULL)
LDAP_SLAPD_F(int)
slap_access_allowed(Operation *op, Entry *e, AttributeDescription *desc,
                    struct berval *val, slap_access_t access,
                    AccessControlState *state, slap_mask_t *maskp);
LDAP_SLAPD_F(int)
slap_access_always_allowed(Operation *op, Entry *e, AttributeDescription *desc,
                           struct berval *val, slap_access_t access,
                           AccessControlState *state, slap_mask_t *maskp);

LDAP_SLAPD_F(int) acl_check_modlist(Operation *op, Entry *e, Modifications *ml);

LDAP_SLAPD_F(void) acl_append(AccessControl **l, AccessControl *a, int pos);

#ifdef SLAP_DYNACL
LDAP_SLAPD_F(int) slap_dynacl_register(slap_dynacl_t *da);
LDAP_SLAPD_F(slap_dynacl_t *) slap_dynacl_get(const char *name);
#endif /* SLAP_DYNACL */
LDAP_SLAPD_F(int) acl_init(void);

LDAP_SLAPD_F(int)
acl_get_part(struct berval *list, int ix, char sep, struct berval *bv);
LDAP_SLAPD_F(int)
acl_match_set(struct berval *subj, Operation *op, Entry *e,
              struct berval *default_set_attribute);
LDAP_SLAPD_F(int)
acl_string_expand(struct berval *newbuf, struct berval *pattern,
                  struct berval *dnmatch, struct berval *valmatch,
                  AclRegexMatches *matches);

/*
 * aclparse.c
 */
LDAP_SLAPD_V(const char *) style_strings[];

LDAP_SLAPD_F(int)
parse_acl(Backend *be, const char *fname, int lineno, int argc, char **argv,
          int pos);

LDAP_SLAPD_F(char *) access2str(slap_access_t access);
LDAP_SLAPD_F(slap_access_t) str2access(const char *str);

#define ACCESSMASK_MAXLEN sizeof("unknown (+wrscan)")
LDAP_SLAPD_F(char *) accessmask2str(slap_mask_t mask, char *, int debug);
LDAP_SLAPD_F(slap_mask_t) str2accessmask(const char *str);
LDAP_SLAPD_F(void) acl_unparse(AccessControl *, struct berval *);
LDAP_SLAPD_F(void) acl_destroy(AccessControl *);
LDAP_SLAPD_F(void) acl_free(AccessControl *a);

/*
 * ad.c
 */
LDAP_SLAPD_F(int)
slap_str2ad(const char *, AttributeDescription **ad, const char **text);

LDAP_SLAPD_F(int)
slap_bv2ad(struct berval *bv, AttributeDescription **ad, const char **text);

LDAP_SLAPD_F(void) ad_destroy(AttributeDescription *);
LDAP_SLAPD_F(int) ad_keystring(struct berval *bv);

#define ad_cmp(l, r)                                                           \
  (((l)->ad_cname.bv_len < (r)->ad_cname.bv_len)                               \
       ? -1                                                                    \
       : (((l)->ad_cname.bv_len > (r)->ad_cname.bv_len)                        \
              ? 1                                                              \
              : strcasecmp((l)->ad_cname.bv_val, (r)->ad_cname.bv_val)))

LDAP_SLAPD_F(int)
is_ad_subtype(AttributeDescription *sub, AttributeDescription *super);

LDAP_SLAPD_F(int) ad_inlist(AttributeDescription *desc, AttributeName *attrs);

LDAP_SLAPD_F(int)
slap_str2undef_ad(const char *, AttributeDescription **ad, const char **text,
                  unsigned proxied);

LDAP_SLAPD_F(int)
slap_bv2undef_ad(struct berval *bv, AttributeDescription **ad,
                 const char **text, unsigned proxied);

LDAP_SLAPD_F(AttributeDescription *)
slap_bv2tmp_ad(struct berval *bv, void *memctx);

LDAP_SLAPD_F(int) slap_ad_undef_promote(char *name, AttributeType *nat);

LDAP_SLAPD_F(AttributeDescription *)
ad_find_tags(AttributeType *type, struct berval *tags);

LDAP_SLAPD_F(AttributeName *)
str2anlist(AttributeName *an, char *str, const char *brkstr);
LDAP_SLAPD_F(void) anlist_free(AttributeName *an, int freename, void *ctx);

LDAP_SLAPD_F(char **) anlist2charray_x(AttributeName *an, int dup, void *ctx);
LDAP_SLAPD_F(char **) anlist2charray(AttributeName *an, int dup);
LDAP_SLAPD_F(char **) anlist2attrs(AttributeName *anlist);
LDAP_SLAPD_F(AttributeName *)
file2anlist(AttributeName *, const char *, const char *);
LDAP_SLAPD_F(int) an_find(AttributeName *a, struct berval *s);
LDAP_SLAPD_F(int)
ad_define_option(const char *name, const char *fname, int lineno);
LDAP_SLAPD_F(void) ad_unparse_options(BerVarray *res);

LDAP_SLAPD_F(MatchingRule *) ad_mr(AttributeDescription *ad, unsigned usage);

LDAP_SLAPD_V(AttributeName *) slap_anlist_no_attrs;
LDAP_SLAPD_V(AttributeName *) slap_anlist_all_user_attributes;
LDAP_SLAPD_V(AttributeName *) slap_anlist_all_operational_attributes;
LDAP_SLAPD_V(AttributeName *) slap_anlist_all_attributes;

LDAP_SLAPD_V(struct berval *) slap_bv_no_attrs;
LDAP_SLAPD_V(struct berval *) slap_bv_all_user_attrs;
LDAP_SLAPD_V(struct berval *) slap_bv_all_operational_attrs;

/* deprecated; only defined for backward compatibility */
#define NoAttrs (*slap_bv_no_attrs)
#define AllUser (*slap_bv_all_user_attrs)
#define AllOper (*slap_bv_all_operational_attrs)

/*
 * add.c
 */
LDAP_SLAPD_F(int)
slap_mods2entry(Modifications *mods, Entry **e, int initial, int dup,
                const char **text, char *textbuf, size_t textlen);

LDAP_SLAPD_F(int)
slap_entry2mods(Entry *e, Modifications **mods, const char **text,
                char *textbuf, size_t textlen);
LDAP_SLAPD_F(int)
slap_add_opattrs(Operation *op, const char **text, char *textbuf,
                 size_t textlen, int manage_ctxcsn);

/*
 * at.c
 */
LDAP_SLAPD_V(int) at_oc_cache;
LDAP_SLAPD_F(void)
at_config(const char *fname, int lineno, int argc, char **argv);
LDAP_SLAPD_F(AttributeType *) at_find(const char *name);
LDAP_SLAPD_F(AttributeType *) at_bvfind(struct berval *name);
LDAP_SLAPD_F(int) at_find_in_list(AttributeType *sat, AttributeType **list);
LDAP_SLAPD_F(int) at_append_to_list(AttributeType *sat, AttributeType ***listp);
LDAP_SLAPD_F(int) at_delete_from_list(int pos, AttributeType ***listp);
LDAP_SLAPD_F(int) at_schema_info(Entry *e);
LDAP_SLAPD_F(int)
at_add(LDAPAttributeType *at, int user, AttributeType **sat,
       AttributeType *prev, const char **err);
LDAP_SLAPD_F(void) at_destroy(void);

LDAP_SLAPD_F(int) is_at_subtype(AttributeType *sub, AttributeType *super);

LDAP_SLAPD_F(const char *) at_syntax(AttributeType *at);
LDAP_SLAPD_F(int) is_at_syntax(AttributeType *at, const char *oid);

LDAP_SLAPD_F(int) at_start(AttributeType **at);
LDAP_SLAPD_F(int) at_next(AttributeType **at);
LDAP_SLAPD_F(void) at_delete(AttributeType *at);

LDAP_SLAPD_F(void)
at_unparse(BerVarray *bva, AttributeType *start, AttributeType *end,
           int system);

LDAP_SLAPD_F(int)
register_at(const char *at, AttributeDescription **ad, int dupok);

/*
 * attr.c
 */
LDAP_SLAPD_F(void) attr_free(Attribute *a);
LDAP_SLAPD_F(Attribute *) attr_dup(Attribute *a);

#ifdef LDAP_COMP_MATCH
LDAP_SLAPD_F(void) comp_tree_free(Attribute *a);
#endif

#define attr_mergeit(e, d, v) attr_merge(e, d, v, NULL /* FIXME */)
#define attr_mergeit_one(e, d, v) attr_merge_one(e, d, v, NULL /* FIXME */)

LDAP_SLAPD_F(Attribute *) attr_alloc(AttributeDescription *ad);
LDAP_SLAPD_F(Attribute *) attrs_alloc(int num);
LDAP_SLAPD_F(int) attr_prealloc(int num);
LDAP_SLAPD_F(int)
attr_valfind(Attribute *a, unsigned flags, struct berval *val, unsigned *slot,
             void *ctx);
LDAP_SLAPD_F(int)
attr_valadd(Attribute *a, BerVarray vals, BerVarray nvals, int num);
LDAP_SLAPD_F(int)
attr_merge(Entry *e, AttributeDescription *desc, BerVarray vals,
           BerVarray nvals);
LDAP_SLAPD_F(int)
attr_merge_one(Entry *e, AttributeDescription *desc, struct berval *val,
               struct berval *nval);
LDAP_SLAPD_F(int)
attr_normalize(AttributeDescription *desc, BerVarray vals, BerVarray *nvalsp,
               void *memctx);
LDAP_SLAPD_F(int)
attr_normalize_one(AttributeDescription *desc, struct berval *val,
                   struct berval *nval, void *memctx);
LDAP_SLAPD_F(int)
attr_merge_normalize(Entry *e, AttributeDescription *desc, BerVarray vals,
                     void *memctx);
LDAP_SLAPD_F(int)
attr_merge_normalize_one(Entry *e, AttributeDescription *desc,
                         struct berval *val, void *memctx);
LDAP_SLAPD_F(Attribute *) attrs_find(Attribute *a, AttributeDescription *desc);
LDAP_SLAPD_F(Attribute *) attr_find(Attribute *a, AttributeDescription *desc);
LDAP_SLAPD_F(int) attr_delete(Attribute **attrs, AttributeDescription *desc);

LDAP_SLAPD_F(void) attr_clean(Attribute *a);
LDAP_SLAPD_F(void) attrs_free(Attribute *a);
LDAP_SLAPD_F(Attribute *) attrs_dup(Attribute *a);
LDAP_SLAPD_F(int) attr_init(void);
LDAP_SLAPD_F(int) attr_destroy(void);

LDAP_SLAPD_F(void) slap_backtrace_debug(void);
LDAP_SLAPD_F(void)
slap_backtrace_debug_ex(int skip, int deep, const char *caption);
LDAP_SLAPD_F(void)
slap_backtrace_log(void *array[], int nentries, const char *caption);
LDAP_SLAPD_F(void) slap_backtrace_set_enable(int value);
LDAP_SLAPD_F(int) slap_backtrace_get_enable();
LDAP_SLAPD_F(void) slap_backtrace_set_dir(const char *path);
LDAP_SLAPD_F(int) slap_limit_coredump_set(int mbytes);
LDAP_SLAPD_F(int) slap_limit_memory_set(int mbytes);
LDAP_SLAPD_F(int) slap_limit_coredump_get();
LDAP_SLAPD_F(int) slap_limit_memory_get();

#ifdef SLAPD_ENABLE_CI
/* simplify testing for Continuous Integration */
LDAP_SLAPD_F(void) slap_setup_ci(void);
#endif /* SLAPD_ENABLE_CI */

/*
 * ava.c
 */
LDAP_SLAPD_F(int)
get_ava(Operation *op, BerElement *ber, Filter *f, unsigned usage,
        const char **text);
LDAP_SLAPD_F(void) ava_free(Operation *op, AttributeAssertion *ava, int freeit);

/*
 * backend.c
 */

#define be_match(be1, be2)                                                     \
  ((be1) == (be2) || ((be1) && (be2) && (be1)->be_nsuffix == (be2)->be_nsuffix))

LDAP_SLAPD_F(int) backend_init(void);
LDAP_SLAPD_F(int) backend_add(BackendInfo *aBackendInfo);
LDAP_SLAPD_F(int) backend_num(Backend *be);
LDAP_SLAPD_F(int) backend_startup(Backend *be);
LDAP_SLAPD_F(int) backend_startup_one(Backend *be, struct config_reply_s *cr);
LDAP_SLAPD_F(int) backend_sync(Backend *be);
LDAP_SLAPD_F(int) backend_shutdown(Backend *be);
LDAP_SLAPD_F(int) backend_destroy(void);
LDAP_SLAPD_F(void) backend_stopdown_one(BackendDB *bd);
LDAP_SLAPD_F(void) backend_destroy_one(BackendDB *bd, int dynamic);

LDAP_SLAPD_F(BackendInfo *) backend_info(const char *type);
LDAP_SLAPD_F(BackendDB *)
backend_db_init(const char *type, BackendDB *be, int idx,
                struct config_reply_s *cr);
LDAP_SLAPD_F(void) backend_db_insert(BackendDB *bd, int idx);
LDAP_SLAPD_F(void) backend_db_move(BackendDB *bd, int idx);

LDAP_SLAPD_F(BackendDB *) select_backend(struct berval *dn, int noSubordinates);

LDAP_SLAPD_F(int) be_issuffix(Backend *be, struct berval *suffix);
LDAP_SLAPD_F(int) be_issubordinate(Backend *be, struct berval *subordinate);
LDAP_SLAPD_F(int) be_isroot(Operation *op);
LDAP_SLAPD_F(int) be_isroot_dn(Backend *be, struct berval *ndn);
LDAP_SLAPD_F(int) be_isroot_pw(Operation *op);
LDAP_SLAPD_F(int) be_rootdn_bind(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) be_slurp_update(Operation *op);
#define be_isupdate(op) be_slurp_update((op))
LDAP_SLAPD_F(int) be_shadow_update(Operation *op);
LDAP_SLAPD_F(int) be_isupdate_dn(Backend *be, struct berval *ndn);
LDAP_SLAPD_F(struct berval *) be_root_dn(Backend *be);
LDAP_SLAPD_F(int)
be_entry_get_rw(Operation *o, struct berval *ndn, ObjectClass *oc,
                AttributeDescription *at, int rw, Entry **e);

#ifndef USE_RS_ASSERT
#define USE_RS_ASSERT LDAP_CHECK
#endif

/* "backend->ophandler(op,rs)" wrappers, applied by contrib:wrap_slap_ops */
#define SLAP_OP(which, op, rs) slap_bi_op((op)->o_bd->bd_info, which, op, rs)
#define slap_be_op(be, which, op, rs) slap_bi_op((be)->bd_info, which, op, rs)
#if !(defined(USE_RS_ASSERT) && (USE_RS_ASSERT))
#define slap_bi_op(bi, which, op, rs) ((&(bi)->bi_op_bind)[which](op, rs))
#endif
LDAP_SLAPD_F(int)
(slap_bi_op)(BackendInfo *bi, slap_operation_t which, Operation *op,
             SlapReply *rs);

LDAP_SLAPD_F(int) be_entry_release_rw(Operation *o, Entry *e, int rw);
#define be_entry_release_r(o, e) be_entry_release_rw(o, e, 0)
#define be_entry_release_w(o, e) be_entry_release_rw(o, e, 1)

LDAP_SLAPD_F(int) backend_unbind(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) backend_connection_init(Connection *conn);
LDAP_SLAPD_F(int) backend_connection_destroy(Connection *conn);

LDAP_SLAPD_F(int) backend_check_controls(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int)
backend_check_restrictions(Operation *op, SlapReply *rs, struct berval *opdata);

LDAP_SLAPD_F(int) backend_check_referrals(Operation *op, SlapReply *rs);

LDAP_SLAPD_F(int)
backend_group(Operation *op, Entry *target, struct berval *gr_ndn,
              struct berval *op_ndn, ObjectClass *group_oc,
              AttributeDescription *group_at);

LDAP_SLAPD_F(int)
backend_attribute(Operation *op, Entry *target, struct berval *entry_ndn,
                  AttributeDescription *entry_at, BerVarray *vals,
                  slap_access_t access);

LDAP_SLAPD_F(int)
backend_access(Operation *op, Entry *target, struct berval *edn,
               AttributeDescription *entry_at, struct berval *nval,
               slap_access_t access, slap_mask_t *mask);

LDAP_SLAPD_F(int) backend_operational(Operation *op, SlapReply *rs);

LDAP_SLAPD_F(ID) backend_tool_entry_first(BackendDB *be);

LDAP_SLAPD_V(BackendInfo) slap_binfo[];

/*
 * backglue.c
 */

LDAP_SLAPD_F(int) glue_sub_init(void);
LDAP_SLAPD_F(int) glue_sub_attach(int online);
LDAP_SLAPD_F(int) glue_sub_add(BackendDB *be, int advert, int online);
LDAP_SLAPD_F(int) glue_sub_del(BackendDB *be);

/*
 * backover.c
 */
LDAP_SLAPD_F(int) overlay_register(slap_overinst *on);
LDAP_SLAPD_F(int)
overlay_config(BackendDB *be, const char *ov, int idx, BackendInfo **res,
               ConfigReply *cr);
LDAP_SLAPD_F(void) overlay_destroy_one(BackendDB *be, slap_overinst *on);
LDAP_SLAPD_F(slap_overinst *) overlay_next(slap_overinst *on);
LDAP_SLAPD_F(slap_overinst *) overlay_find(const char *name);
LDAP_SLAPD_F(int) overlay_is_over(BackendDB *be);
LDAP_SLAPD_F(int) overlay_is_inst(BackendDB *be, const char *name);
LDAP_SLAPD_F(int) overlay_register_control(BackendDB *be, const char *oid);
LDAP_SLAPD_F(int)
overlay_op_walk(Operation *op, SlapReply *rs, slap_operation_t which,
                slap_overinfo *oi, slap_overinst *on);
LDAP_SLAPD_F(int)
overlay_entry_get_ov(Operation *op, struct berval *dn, ObjectClass *oc,
                     AttributeDescription *ad, int rw, Entry **e,
                     slap_overinst *ov);
LDAP_SLAPD_F(int)
overlay_entry_release_ov(Operation *op, Entry *e, int rw, slap_overinst *ov);
LDAP_SLAPD_F(void)
overlay_insert(BackendDB *be, slap_overinst *on, slap_overinst ***prev,
               int idx);
LDAP_SLAPD_F(void) overlay_move(BackendDB *be, slap_overinst *on, int idx);
#ifdef SLAP_CONFIG_DELETE
LDAP_SLAPD_F(void)
overlay_remove(BackendDB *be, slap_overinst *on, Operation *op);
LDAP_SLAPD_F(void) overlay_unregister_control(BackendDB *be, const char *oid);
#endif /* SLAP_CONFIG_DELETE */
LDAP_SLAPD_F(int)
overlay_callback_after_backover(Operation *op, slap_callback *sc, int append);

/*
 * bconfig.c
 */
LDAP_SLAPD_F(int) slap_loglevel_register(slap_mask_t m, struct berval *s);
LDAP_SLAPD_F(int) slap_loglevel_get(struct berval *s, int *l);
LDAP_SLAPD_F(int) str2loglevel(const char *s, int *l);
LDAP_SLAPD_F(int) loglevel2bvarray(int l, BerVarray *bva);
LDAP_SLAPD_F(const char *) loglevel2str(int l);
LDAP_SLAPD_F(int) loglevel2bv(int l, struct berval *bv);
LDAP_SLAPD_F(int) loglevel_print(FILE *out);
LDAP_SLAPD_F(int)
slap_cf_aux_table_parse(const char *word, void *bc, slap_cf_aux_table *tab0,
                        const char *tabmsg);
LDAP_SLAPD_F(int)
slap_cf_aux_table_unparse(void *bc, struct berval *bv, slap_cf_aux_table *tab0);

/*
 * ch_malloc.c
 */
LDAP_SLAPD_V(BerMemoryFunctions) ch_mfuncs;
LDAP_SLAPD_F(void *) ch_malloc(ber_len_t size);
LDAP_SLAPD_F(void *) ch_realloc(void *block, ber_len_t size);
LDAP_SLAPD_F(void *) ch_calloc(ber_len_t nelem, ber_len_t size);
LDAP_SLAPD_F(char *) ch_strdup(const char *string);
LDAP_SLAPD_F(void) ch_free(void *);

#ifndef CH_FREE
#undef free
#define free ch_free
#endif

/*
 * compare.c
 */

LDAP_SLAPD_F(int)
slap_compare_entry(Operation *op, Entry *e, AttributeAssertion *ava);

/*
 * component.c
 */
#ifdef LDAP_COMP_MATCH
struct comp_attribute_aliasing;

LDAP_SLAPD_F(int)
test_comp_filter_entry(Operation *op, Entry *e, MatchingRuleAssertion *mr);

LDAP_SLAPD_F(int)
dup_comp_filter(Operation *op, struct berval *bv, ComponentFilter *in_f,
                ComponentFilter **out_f);

LDAP_SLAPD_F(int)
get_aliased_filter_aa(Operation *op, AttributeAssertion *a_assert,
                      struct comp_attribute_aliasing *aa, const char **text);

LDAP_SLAPD_F(int)
get_aliased_filter(Operation *op, MatchingRuleAssertion *ma,
                   struct comp_attribute_aliasing *aa, const char **text);

LDAP_SLAPD_F(int)
get_comp_filter(Operation *op, BerValue *bv, ComponentFilter **filt,
                const char **text);

LDAP_SLAPD_F(int)
insert_component_reference(ComponentReference *cr,
                           ComponentReference **cr_list);

LDAP_SLAPD_F(int) is_component_reference(char *attr);

LDAP_SLAPD_F(int)
extract_component_reference(char *attr, ComponentReference **cr);

LDAP_SLAPD_F(int)
componentFilterMatch(int *matchp, slap_mask_t flags, Syntax *syntax,
                     MatchingRule *mr, struct berval *value,
                     void *assertedValue);

LDAP_SLAPD_F(int)
directoryComponentsMatch(int *matchp, slap_mask_t flags, Syntax *syntax,
                         MatchingRule *mr, struct berval *value,
                         void *assertedValue);

LDAP_SLAPD_F(int)
allComponentsMatch(int *matchp, slap_mask_t flags, Syntax *syntax,
                   MatchingRule *mr, struct berval *value, void *assertedValue);

LDAP_SLAPD_F(ComponentReference *)
dup_comp_ref(Operation *op, ComponentReference *cr);

LDAP_SLAPD_F(int) componentFilterValidate(Syntax *syntax, struct berval *bv);

LDAP_SLAPD_F(int) allComponentsValidate(Syntax *syntax, struct berval *bv);

LDAP_SLAPD_F(void) component_free(ComponentFilter *f);

LDAP_SLAPD_F(void) free_ComponentData(Attribute *a);

LDAP_SLAPD_V(test_membership_func *) is_aliased_attribute;

LDAP_SLAPD_V(free_component_func *) component_destructor;

LDAP_SLAPD_V(get_component_info_func *) get_component_description;

LDAP_SLAPD_V(component_encoder_func *) component_encoder;

LDAP_SLAPD_V(convert_attr_to_comp_func *) attr_converter;

LDAP_SLAPD_V(alloc_nibble_func *) nibble_mem_allocator;

LDAP_SLAPD_V(free_nibble_func *) nibble_mem_free;
#endif

/*
 * controls.c
 */
LDAP_SLAPD_V(struct slap_control_ids) slap_cids;
LDAP_SLAPD_F(void) slap_free_ctrls(Operation *op, LDAPControl **ctrls);
LDAP_SLAPD_F(int)
slap_add_ctrls(Operation *op, SlapReply *rs, LDAPControl **ctrls);
LDAP_SLAPD_F(int)
slap_parse_ctrl(Operation *op, SlapReply *rs, LDAPControl *control,
                const char **text);
LDAP_SLAPD_F(int) get_ctrls(Operation *op, SlapReply *rs, int senderrors);
LDAP_SLAPD_F(int)
get_ctrls2(Operation *op, SlapReply *rs, int senderrors, ber_tag_t ctag);
LDAP_SLAPD_F(int)
register_supported_control2(const char *controloid, slap_mask_t controlmask,
                            const char *const *controlexops,
                            SLAP_CTRL_PARSE_FN *controlparsefn, unsigned flags,
                            int *controlcid);
#define register_supported_control(oid, mask, exops, fn, cid)                  \
  register_supported_control2((oid), (mask), (exops), (fn), 0, (cid))
#ifdef SLAP_CONFIG_DELETE
LDAP_SLAPD_F(int) unregister_supported_control(const char *controloid);
#endif /* SLAP_CONFIG_DELETE */
LDAP_SLAPD_F(int) register_control_exop(const char *controloid, char *exopoid);
LDAP_SLAPD_F(int) slap_controls_init(void);
LDAP_SLAPD_F(void) controls_destroy(void);
LDAP_SLAPD_F(int) controls_root_dse_info(Entry *e);
LDAP_SLAPD_F(int)
get_supported_controls(char ***ctrloidsp, slap_mask_t **ctrlmasks);
LDAP_SLAPD_F(int) slap_find_control_id(const char *oid, int *cid);
LDAP_SLAPD_F(int) slap_global_control(Operation *op, const char *oid, int *cid);
LDAP_SLAPD_F(int)
slap_remove_control(Operation *op, SlapReply *rs, int ctrl,
                    BI_chk_controls fnc);

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
LDAP_SLAPD_F(int)
slap_ctrl_session_tracking_add(Operation *op, SlapReply *rs, struct berval *ip,
                               struct berval *name, struct berval *id,
                               LDAPControl *ctrl);
LDAP_SLAPD_F(int)
slap_ctrl_session_tracking_request_add(Operation *op, SlapReply *rs,
                                       LDAPControl *ctrl);
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */
#ifdef SLAP_CONTROL_X_WHATFAILED
LDAP_SLAPD_F(int)
slap_ctrl_whatFailed_add(Operation *op, SlapReply *rs, char **oids);
#endif /* SLAP_CONTROL_X_WHATFAILED */

/*
 * config.c
 */
LDAP_SLAPD_F(int) read_config(const char *fname, const char *dir);
LDAP_SLAPD_F(void) config_destroy(void);
LDAP_SLAPD_F(char **) slap_str2clist(char ***, char *, const char *);
LDAP_SLAPD_F(int) bverb_to_mask(struct berval *bword, const slap_verbmasks *v);
LDAP_SLAPD_F(int) verb_to_mask(const char *word, const slap_verbmasks *v);
LDAP_SLAPD_F(int)
verbs_to_mask(int argc, char *argv[], const slap_verbmasks *v, slap_mask_t *m);
LDAP_SLAPD_F(int)
mask_to_verbs(const slap_verbmasks *v, slap_mask_t m, BerVarray *bva);
LDAP_SLAPD_F(int)
mask_to_verbstring(const slap_verbmasks *v, slap_mask_t m, char delim,
                   struct berval *bv);
LDAP_SLAPD_F(int)
verbstring_to_mask(const slap_verbmasks *v, char *str, char delim,
                   slap_mask_t *m);
LDAP_SLAPD_F(int)
enum_to_verb(const slap_verbmasks *v, slap_mask_t m, struct berval *bv);
LDAP_SLAPD_F(int)
slap_verbmasks_init(slap_verbmasks **vp, const slap_verbmasks *v);
LDAP_SLAPD_F(int) slap_verbmasks_destroy(slap_verbmasks *v);
LDAP_SLAPD_F(int)
slap_verbmasks_append(slap_verbmasks **vp, slap_mask_t m, struct berval *v,
                      slap_mask_t *ignore);
LDAP_SLAPD_F(int) slap_tls_get_config(LDAP *ld, int opt, char **val);
LDAP_SLAPD_F(void) bindconf_tls_defaults(slap_bindconf *bc);
LDAP_SLAPD_F(int) bindconf_tls_parse(const char *word, slap_bindconf *bc);
LDAP_SLAPD_F(int) bindconf_tls_unparse(slap_bindconf *bc, struct berval *bv);
LDAP_SLAPD_F(int) bindconf_parse(const char *word, slap_bindconf *bc);
LDAP_SLAPD_F(int) bindconf_unparse(slap_bindconf *bc, struct berval *bv);
LDAP_SLAPD_F(int) bindconf_tls_set(slap_bindconf *bc, LDAP *ld);
LDAP_SLAPD_F(void) bindconf_free(slap_bindconf *bc);
LDAP_SLAPD_F(int) slap_client_connect(LDAP **ldp, slap_bindconf *sb);
LDAP_SLAPD_F(int)
config_generic_wrapper(Backend *be, const char *fname, int lineno, int argc,
                       char **argv);
LDAP_SLAPD_F(char *) anlist_unparse(AttributeName *, char *, ber_len_t buflen);
LDAP_SLAPD_F(int)
slap_keepalive_parse(struct berval *val, void *bc, slap_cf_aux_table *tab0,
                     const char *tabmsg, int unparse);
LDAP_SLAPD_F(void) slap_client_keepalive(LDAP *ld, slap_keepalive *sk);

#ifdef LDAP_SLAPI
LDAP_SLAPD_V(int) slapi_plugins_used;
#endif

/*
 * connection.c
 */
LDAP_SLAPD_F(int) connections_init(void);
LDAP_SLAPD_F(int) connections_shutdown(int gentle_shutdown_only);
LDAP_SLAPD_F(int) connections_destroy(void);
LDAP_SLAPD_F(int) connections_timeout_idle(slap_time_t);
LDAP_SLAPD_F(void) connections_drop(void);

LDAP_SLAPD_F(Connection *)
connection_client_setup(ber_socket_t s, ldap_pvt_thread_start_t *func,
                        void *arg);
LDAP_SLAPD_F(void) connection_client_enable(Connection *c);
LDAP_SLAPD_F(void) connection_client_stop(Connection *c);

#ifdef LDAP_PF_LOCAL_SENDMSG
#define LDAP_PF_LOCAL_SENDMSG_ARG(arg) , arg
#else
#define LDAP_PF_LOCAL_SENDMSG_ARG(arg)
#endif

LDAP_SLAPD_F(Connection *)
connection_init(ber_socket_t s, Listener *url, const char *dnsname,
                const char *peername, int use_tls, slap_ssf_t ssf,
                struct berval *id
                    LDAP_PF_LOCAL_SENDMSG_ARG(struct berval *peerbv));

LDAP_SLAPD_F(void) connection_closing(Connection *c, const char *why);
LDAP_SLAPD_F(int) connection_valid(Connection *c);
LDAP_SLAPD_F(const char *)
connection_state2str(int state) __attribute__((const));
int connections_socket_trouble(ber_socket_t s);

LDAP_SLAPD_F(int) connection_read_activate(ber_socket_t s);
LDAP_SLAPD_F(int) connection_write(ber_socket_t s);

LDAP_SLAPD_F(void) connection_op_finish(Operation *op);

LDAP_SLAPD_F(unsigned long) connections_nextid(void);

LDAP_SLAPD_F(Connection *) connection_first(ber_socket_t *);
LDAP_SLAPD_F(Connection *) connection_next(Connection *, ber_socket_t *);
LDAP_SLAPD_F(void) connection_done(Connection *);

LDAP_SLAPD_F(void) connection2anonymous(Connection *);
LDAP_SLAPD_F(void)
connection_fake_init(Connection *conn, OperationBuffer *opbuf, void *threadctx);
LDAP_SLAPD_F(void)
connection_fake_init2(Connection *conn, OperationBuffer *opbuf, void *threadctx,
                      int newmem);
LDAP_SLAPD_F(void)
operation_fake_init(Connection *conn, Operation *op, void *threadctx,
                    int newmem);
LDAP_SLAPD_F(void) connection_assign_nextid(Connection *);

/*
 * cr.c
 */
LDAP_SLAPD_F(int) cr_schema_info(Entry *e);
LDAP_SLAPD_F(void)
cr_unparse(BerVarray *bva, ContentRule *start, ContentRule *end, int system);

LDAP_SLAPD_F(int)
cr_add(LDAPContentRule *oc, int user, ContentRule **scr, const char **err);

LDAP_SLAPD_F(void) cr_destroy(void);

LDAP_SLAPD_F(ContentRule *) cr_find(const char *crname);
LDAP_SLAPD_F(ContentRule *) cr_bvfind(struct berval *crname);

/*
 * ctxcsn.c
 */

LDAP_SLAPD_V(int) slap_serverID;
LDAP_SLAPD_V(const struct berval) slap_ldapsync_bv;
LDAP_SLAPD_V(const struct berval) slap_ldapsync_cn_bv;
LDAP_SLAPD_F(int) slap_get_commit_csn(Operation *, struct berval *maxcsn);
LDAP_SLAPD_F(int) slap_graduate_commit_csn(Operation *);
LDAP_SLAPD_F(Entry *) slap_create_context_csn_entry(Backend *, struct berval *);
LDAP_SLAPD_F(int) slap_get_csn(Operation *, struct berval *);
LDAP_SLAPD_F(void) slap_queue_csn(Operation *, const struct berval *);
LDAP_SLAPD_F(void) slap_free_commit_csn_list(struct be_pcl *list);
LDAP_SLAPD_F(void) slap_op_csn_free(Operation *op);
LDAP_SLAPD_F(void) slap_op_csn_clean(Operation *op);
LDAP_SLAPD_F(void) slap_op_csn_assign(Operation *op, BerValue *csn);

/*
 * quorum.c
 */

LDAP_SLAPD_F(void) quorum_global_init();
LDAP_SLAPD_F(void) quorum_global_destroy();
LDAP_SLAPD_F(void) quorum_be_destroy(BackendDB *bd);
LDAP_SLAPD_F(void) quorum_notify_self_sid();
LDAP_SLAPD_F(void) quorum_add_rid(BackendDB *bd, void *key, int rid);
LDAP_SLAPD_F(void) quorum_remove_rid(BackendDB *bd, void *key);
LDAP_SLAPD_F(void) quorum_notify_sid(BackendDB *bd, void *key, int sid);

#define QS_DIRTY 0
#define QS_DEAD 1
#define QS_REFRESH 2
#define QS_READY 3
#define QS_PROCESS 4
#define QS_MAX 5
LDAP_SLAPD_F(void) quorum_notify_status(BackendDB *bd, void *key, int status);

LDAP_SLAPD_F(int) quorum_query(BackendDB *bd);
LDAP_SLAPD_F(void) quorum_notify_csn(BackendDB *bd, int csnsid);
LDAP_SLAPD_F(int)
quorum_syncrepl_gate(BackendDB *bd, void *instance_key, int in);
LDAP_SLAPD_F(int)
quorum_query_status(BackendDB *bd, int running_only, BerValue *, Operation *op);
LDAP_SLAPD_F(int) quorum_syncrepl_maxrefresh(BackendDB *bd);

/*
 * daemon.c
 */
LDAP_SLAPD_F(void) slapd_add_internal(ber_socket_t s, int isactive);
LDAP_SLAPD_F(int) slapd_daemon_init(const char *urls);
LDAP_SLAPD_F(int) slapd_daemon_destroy(void);
LDAP_SLAPD_F(int) slapd_daemon(void);
LDAP_SLAPD_F(Listener **) slapd_get_listeners(void);
LDAP_SLAPD_F(void)
slapd_remove(ber_socket_t s, Sockbuf *sb, int wasactive, int wake, int locked);

LDAP_SLAPD_F(void) slap_sig_shutdown(int sig);
LDAP_SLAPD_F(void) slap_sig_wake(int sig);
LDAP_SLAPD_F(void) slap_wake_listener(void);

LDAP_SLAPD_F(void) slap_suspend_listeners(void);
LDAP_SLAPD_F(void) slap_resume_listeners(void);

LDAP_SLAPD_F(int) slap_pause_server(void);
LDAP_SLAPD_F(int) slap_unpause_server(void);

LDAP_SLAPD_F(void) slapd_set_write(ber_socket_t s, int wake);
LDAP_SLAPD_F(void) slapd_clr_write(ber_socket_t s, int wake);
LDAP_SLAPD_F(void) slapd_set_read(ber_socket_t s, int wake);
LDAP_SLAPD_F(int) slapd_clr_read(ber_socket_t s, int wake);
LDAP_SLAPD_F(int) slapd_wait_writer(ber_socket_t sd);
LDAP_SLAPD_F(void) slapd_shutsock(ber_socket_t sd);
#ifdef SLAPD_WRITE_TIMEOUT
LDAP_SLAPD_F(void) slapd_clr_writetime(slap_time_t old);
LDAP_SLAPD_F(slap_time_t) slapd_get_writetime(void);
#endif /* SLAPD_WRITE_TIMEOUT */

#ifdef __SANITIZE_THREAD__

LDAP_SLAPD_F(int) get_shutdown();
LDAP_SLAPD_F(int) get_gentle_shutdown();
LDAP_SLAPD_F(int) get_abrupt_shutdown();
LDAP_SLAPD_F(int) set_shutdown(int v);
LDAP_SLAPD_F(int) set_gentle_shutdown(int v);
LDAP_SLAPD_F(int) set_abrupt_shutdown(int v);

#else

LDAP_SLAPD_V(volatile sig_atomic_t)
_slapd_shutdown, _slapd_gentle_shutdown, _slapd_abrupt_shutdown;

static __inline int get_shutdown() { return _slapd_shutdown; }

static __inline int get_gentle_shutdown() { return _slapd_gentle_shutdown; }

static __inline int get_abrupt_shutdown() { return _slapd_abrupt_shutdown; }

static __inline int set_shutdown(int v) { return _slapd_shutdown = v; }

static __inline int set_gentle_shutdown(int v) {
  return _slapd_gentle_shutdown = v;
}

static __inline int set_abrupt_shutdown(int v) {
  return _slapd_abrupt_shutdown = v;
}

#endif /* __SANITIZE_THREAD__ */

#define slapd_shutdown get_shutdown()
#define slapd_gentle_shutdown get_gentle_shutdown()
#define slapd_abrupt_shutdown get_abrupt_shutdown()

LDAP_SLAPD_V(int) slapd_register_slp;
LDAP_SLAPD_V(const char *) slapd_slp_attrs;
LDAP_SLAPD_V(slap_ssf_t) local_ssf;
LDAP_SLAPD_V(struct runqueue_s) slapd_rq;
LDAP_SLAPD_V(int) slapd_daemon_threads;
LDAP_SLAPD_V(int) slapd_daemon_mask;
#ifdef LDAP_TCP_BUFFER
LDAP_SLAPD_V(int) slapd_tcp_rmem;
LDAP_SLAPD_V(int) slapd_tcp_wmem;
#endif /* LDAP_TCP_BUFFER */

/*
 * dn.c
 */

#define dn_match(dn1, dn2) (ber_bvcmp((dn1), (dn2)) == 0)
#define bvmatch(bv1, bv2)                                                      \
  (((bv1)->bv_len == (bv2)->bv_len) &&                                         \
   (memcmp((bv1)->bv_val, (bv2)->bv_val, (bv1)->bv_len) == 0))

LDAP_SLAPD_F(int) dnValidate(Syntax *syntax, struct berval *val);
LDAP_SLAPD_F(int) rdnValidate(Syntax *syntax, struct berval *val);

LDAP_SLAPD_F(slap_mr_normalize_func) dnNormalize;

LDAP_SLAPD_F(slap_mr_normalize_func) rdnNormalize;

LDAP_SLAPD_F(slap_syntax_transform_func) dnPretty;

LDAP_SLAPD_F(slap_syntax_transform_func) rdnPretty;

LDAP_SLAPD_F(int)
dnPrettyNormal(Syntax *syntax, struct berval *val, struct berval *pretty,
               struct berval *normal, void *ctx);

LDAP_SLAPD_F(int)
dnMatch(int *matchp, slap_mask_t flags, Syntax *syntax, MatchingRule *mr,
        struct berval *value, void *assertedValue);

LDAP_SLAPD_F(int)
dnRelativeMatch(int *matchp, slap_mask_t flags, Syntax *syntax,
                MatchingRule *mr, struct berval *value, void *assertedValue);

LDAP_SLAPD_F(int)
rdnMatch(int *matchp, slap_mask_t flags, Syntax *syntax, MatchingRule *mr,
         struct berval *value, void *assertedValue);

LDAP_SLAPD_F(int)
dnIsSuffix(const struct berval *dn, const struct berval *suffix);

LDAP_SLAPD_F(int)
dnIsWithinScope(struct berval *ndn, struct berval *nbase, int scope);

LDAP_SLAPD_F(int)
dnIsSuffixScope(struct berval *ndn, struct berval *nbase, int scope);

LDAP_SLAPD_F(int) dnIsOneLevelRDN(struct berval *rdn);

LDAP_SLAPD_F(int)
dnExtractRdn(struct berval *dn, struct berval *rdn, void *ctx);

LDAP_SLAPD_F(int) rdn_validate(struct berval *rdn);

LDAP_SLAPD_F(ber_len_t) dn_rdnlen(Backend *be, struct berval *dn);

LDAP_SLAPD_F(void)
build_new_dn(struct berval *new_dn, struct berval *parent_dn,
             struct berval *newrdn, void *memctx);

LDAP_SLAPD_F(void) dnParent(struct berval *dn, struct berval *pdn);
LDAP_SLAPD_F(void) dnRdn(struct berval *dn, struct berval *rdn);

LDAP_SLAPD_F(int) dnX509normalize(void *x509_name, struct berval *out);

LDAP_SLAPD_F(int) dnX509peerNormalize(void *ssl, struct berval *dn);

LDAP_SLAPD_F(int)
dnPrettyNormalDN(Syntax *syntax, struct berval *val, LDAPDN *dn, int flags,
                 void *ctx);
#define dnPrettyDN(syntax, val, dn, ctx)                                       \
  dnPrettyNormalDN((syntax), (val), (dn), SLAP_LDAPDN_PRETTY, ctx)
#define dnNormalDN(syntax, val, dn, ctx)                                       \
  dnPrettyNormalDN((syntax), (val), (dn), 0, ctx)

typedef int(SLAP_CERT_MAP_FN)(void *ssl, struct berval *dn);
LDAP_SLAPD_F(int) register_certificate_map_function(SLAP_CERT_MAP_FN *fn);

/*
 * entry.c
 */
LDAP_SLAPD_V(const Entry) slap_entry_root;

LDAP_SLAPD_F(int) entry_init(void);
LDAP_SLAPD_F(int) entry_destroy(void);

LDAP_SLAPD_F(Entry *) str2entry(char *s, int *rc);
LDAP_SLAPD_F(Entry *) str2entry2(char *s, int checkvals, int *rc);
LDAP_SLAPD_F(char *) entry2str(Entry *e, int *len);
LDAP_SLAPD_F(char *) entry2str_wrap(Entry *e, int *len, ber_len_t wrap);

LDAP_SLAPD_F(ber_len_t) entry_flatsize(Entry *e, int norm);
LDAP_SLAPD_F(void)
entry_partsize(Entry *e, ber_len_t *len, int *nattrs, int *nvals, int norm);

LDAP_SLAPD_F(int) entry_header(EntryHeader *eh);
LDAP_SLAPD_F(int)
entry_decode_dn(EntryHeader *eh, struct berval *dn, struct berval *ndn);
LDAP_SLAPD_F(int) entry_decode(EntryHeader *eh, Entry **e);
LDAP_SLAPD_F(int) entry_encode(Entry *e, struct berval *bv);

LDAP_SLAPD_F(void) entry_clean(Entry *e);
LDAP_SLAPD_F(void) entry_free(Entry *e);
LDAP_SLAPD_F(int) entry_cmp(Entry *a, Entry *b);
LDAP_SLAPD_F(int) entry_dn_cmp(const void *v_a, const void *v_b);
LDAP_SLAPD_F(int) entry_id_cmp(const void *v_a, const void *v_b);
LDAP_SLAPD_F(Entry *) entry_dup(Entry *e);
LDAP_SLAPD_F(Entry *) entry_dup2(Entry *dest, Entry *src);
LDAP_SLAPD_F(Entry *) entry_dup_bv(Entry *e);
LDAP_SLAPD_F(Entry *) entry_alloc(void);
LDAP_SLAPD_F(int) entry_prealloc(int num);

/*
 * extended.c
 */
LDAP_SLAPD_F(int) exop_root_dse_info(Entry *e);

#define exop_is_write(op) ((op->ore_flags & SLAP_EXOP_WRITES) != 0)

LDAP_SLAPD_V(const struct berval) slap_EXOP_CANCEL;
LDAP_SLAPD_V(const struct berval) slap_EXOP_WHOAMI;
LDAP_SLAPD_V(const struct berval) slap_EXOP_MODIFY_PASSWD;
LDAP_SLAPD_V(const struct berval) slap_EXOP_START_TLS;
#ifdef LDAP_X_TXN
LDAP_SLAPD_V(const struct berval) slap_EXOP_TXN_START;
LDAP_SLAPD_V(const struct berval) slap_EXOP_TXN_END;
#endif

typedef int(SLAP_EXTOP_MAIN_FN)(Operation *op, SlapReply *rs);

typedef int(SLAP_EXTOP_GETOID_FN)(int index, struct berval *oid, int blen);

LDAP_SLAPD_F(int)
extop_register_ex(const struct berval *exop_oid, slap_mask_t exop_flags,
                  SLAP_EXTOP_MAIN_FN *exop_main, unsigned do_not_replace);

#define extop_register(exop_oid, exop_flags, exop_main)                        \
  extop_register_ex((exop_oid), (exop_flags), (exop_main), 0)

LDAP_SLAPD_F(int)
extop_unregister(const struct berval *ext_oid, SLAP_EXTOP_MAIN_FN *ext_main,
                 unsigned unused_flags);

LDAP_SLAPD_F(int) extops_init(void);

LDAP_SLAPD_F(int) extops_destroy(void);

LDAP_SLAPD_F(struct berval *) get_supported_extop(int index);

/*
 * txn.c
 */
#ifdef LDAP_X_TXN
LDAP_SLAPD_F(SLAP_CTRL_PARSE_FN) txn_spec_ctrl;
LDAP_SLAPD_F(SLAP_EXTOP_MAIN_FN) txn_start_extop;
LDAP_SLAPD_F(SLAP_EXTOP_MAIN_FN) txn_end_extop;
LDAP_SLAPD_F(int) txn_preop(Operation *op, SlapReply *rs);
#endif

/*
 * cancel.c
 */
LDAP_SLAPD_F(SLAP_EXTOP_MAIN_FN) cancel_extop;

/*
 * filter.c
 */
LDAP_SLAPD_F(int)
get_filter(Operation *op, BerElement *ber, Filter **filt, const char **text);

LDAP_SLAPD_F(void) filter_free(Filter *f);
LDAP_SLAPD_F(void) filter_free_x(Operation *op, Filter *f, int freeme);
LDAP_SLAPD_F(void) filter2bv(Filter *f, struct berval *bv);
LDAP_SLAPD_F(void) filter2bv_x(Operation *op, Filter *f, struct berval *bv);
LDAP_SLAPD_F(void) filter2bv_undef(Filter *f, int noundef, struct berval *bv);
LDAP_SLAPD_F(void)
filter2bv_undef_x(Operation *op, Filter *f, int noundef, struct berval *bv);
LDAP_SLAPD_F(Filter *) filter_dup(Filter *f, void *memctx);

LDAP_SLAPD_F(int)
get_vrFilter(Operation *op, BerElement *ber, ValuesReturnFilter **f,
             const char **text);

LDAP_SLAPD_F(void) vrFilter_free(Operation *op, ValuesReturnFilter *f);
LDAP_SLAPD_F(void)
vrFilter2bv(Operation *op, ValuesReturnFilter *f, struct berval *fstr);

LDAP_SLAPD_F(int) filter_has_subordinates(Filter *filter);
#define filter_escape_value(in, out)                                           \
  ldap_bv2escaped_filter_value_x((in), (out), 0, NULL)
#define filter_escape_value_x(in, out, ctx)                                    \
  ldap_bv2escaped_filter_value_x((in), (out), 0, ctx)

LDAP_SLAPD_V(const Filter *) slap_filter_objectClass_pres;
LDAP_SLAPD_V(const struct berval *) slap_filterstr_objectClass_pres;

LDAP_SLAPD_F(int) filter_init(void);
LDAP_SLAPD_F(void) filter_destroy(void);
/*
 * filterentry.c
 */

LDAP_SLAPD_F(int) test_filter(Operation *op, Entry *e, Filter *f);

/*
 * frontend.c
 */
LDAP_SLAPD_F(int) frontend_init(void);

/*
 * globals.c
 */

LDAP_SLAPD_V(const struct berval) slap_empty_bv;
LDAP_SLAPD_V(const struct berval) slap_unknown_bv;
LDAP_SLAPD_V(const struct berval) slap_true_bv;
LDAP_SLAPD_V(const struct berval) slap_false_bv;
extern pthread_mutex_t slap_sync_cookie_mutex;
LDAP_SLAPD_V(struct slap_sync_cookie_s) slap_sync_cookie;
LDAP_SLAPD_V(void *) slap_tls_ctx;
LDAP_SLAPD_V(LDAP *) slap_tls_ld;
LDAP_SLAPD_F(void) slap_set_debug_level(int mask);

/*
 * index.c
 */
LDAP_SLAPD_F(int) slap_str2index(const char *str, slap_mask_t *idx);
LDAP_SLAPD_F(void) slap_index2bvlen(slap_mask_t idx, struct berval *bv);
LDAP_SLAPD_F(void) slap_index2bv(slap_mask_t idx, struct berval *bv);

/*
 * init.c
 */
LDAP_SLAPD_F(int) slap_init(int mode, const char *name);
LDAP_SLAPD_F(int) slap_startup(Backend *be);
LDAP_SLAPD_F(int) slap_shutdown(Backend *be);
LDAP_SLAPD_F(int) slap_destroy(void);
LDAP_SLAPD_F(void) slap_counters_init(slap_counters_t *sc);
LDAP_SLAPD_F(void) slap_counters_destroy(slap_counters_t *sc);

LDAP_SLAPD_V(const char *) slap_known_controls[];

/*
 * ldapsync.c
 */
LDAP_SLAPD_F(void)
slap_insert_csn_sids(struct sync_cookie *ck, int, int, struct berval *);
LDAP_SLAPD_F(int) slap_build_syncUUID_set(Operation *, BerVarray *, Entry *);
LDAP_SLAPD_F(int) slap_check_same_server(BackendDB *bd, int sid);

LDAP_SLAPD_F(int) slap_csn_verify_lite(const BerValue *csn);
LDAP_SLAPD_F(int) slap_csn_verify_full(const BerValue *csn);
LDAP_SLAPD_F(int) slap_csn_match(const BerValue *a, const BerValue *b);
LDAP_SLAPD_F(int) slap_csn_compare_sr(const BerValue *a, const BerValue *b);
LDAP_SLAPD_F(int) slap_csn_compare_ts(const BerValue *a, const BerValue *b);
LDAP_SLAPD_F(int) slap_csn_get_sid(const BerValue *csn);
LDAP_SLAPD_F(void) slap_csn_shift(BerValue *csn, int delta_points);

LDAP_SLAPD_F(int) slap_csns_validate_and_sort(BerVarray vals);
LDAP_SLAPD_F(int) slap_csns_match(BerVarray a, BerVarray b);
LDAP_SLAPD_F(int) slap_csns_length(BerVarray vals);
LDAP_SLAPD_F(int) slap_csns_compare(BerVarray next, BerVarray base);
LDAP_SLAPD_F(void) slap_csns_debug(const char *prefix, const BerVarray csns);
LDAP_SLAPD_F(void)
slap_csns_backward_debug(const char *prefix, const BerVarray current,
                         const BerVarray next);

LDAP_SLAPD_F(void) slap_cookie_verify(const struct sync_cookie *);
LDAP_SLAPD_F(void) slap_cookie_init(struct sync_cookie *);
LDAP_SLAPD_F(void) slap_cookie_clean_all(struct sync_cookie *);
LDAP_SLAPD_F(void) slap_cookie_clean_csns(struct sync_cookie *, void *memctx);
LDAP_SLAPD_F(void)
slap_cookie_copy(struct sync_cookie *dst, const struct sync_cookie *src);
LDAP_SLAPD_F(void)
slap_cookie_move(struct sync_cookie *dst, struct sync_cookie *src);
LDAP_SLAPD_F(int)
slap_cookie_merge(BackendDB *bd, struct sync_cookie *dst,
                  const struct sync_cookie *src);
LDAP_SLAPD_F(void) slap_cookie_free(struct sync_cookie *, int free_cookie);
LDAP_SLAPD_F(void) slap_cookie_fetch(struct sync_cookie *dst, BerVarray src);
LDAP_SLAPD_F(int)
slap_cookie_pull(struct sync_cookie *dst, BerVarray src, int consume);
LDAP_SLAPD_F(int)
slap_cookie_merge_csn(BackendDB *bd, struct sync_cookie *dst, int sid,
                      BerValue *src);
LDAP_SLAPD_F(int)
slap_cookie_compare_csn(const struct sync_cookie *cookie, BerValue *csn);
LDAP_SLAPD_F(int)
slap_cookie_parse(struct sync_cookie *dst, const BerValue *src, void *memctx);
LDAP_SLAPD_F(int) slap_cookie_stubself(struct sync_cookie *dst);
LDAP_SLAPD_F(void)
slap_cookie_compose(BerValue *dst, BerVarray csns, int rid, int sid,
                    void *memctx);
LDAP_SLAPD_F(void)
slap_cookie_debug(const char *prefix, const struct sync_cookie *sc);
LDAP_SLAPD_F(void)
slap_cookie_debug_pair(const char *prefix, const char *x_name,
                       const struct sync_cookie *x_cookie, const char *y_name,
                       const struct sync_cookie *y_cookie, int y_marker);
LDAP_SLAPD_F(void)
slap_cookie_backward_debug(const char *prefix,
                           const struct sync_cookie *current,
                           const struct sync_cookie *next);
LDAP_SLAPD_F(int)
slap_cookie_merge_csnset(BackendDB *bd, struct sync_cookie *dst, BerVarray src);
LDAP_SLAPD_F(int)
slap_cookie_compare_csnset(struct sync_cookie *base, BerVarray next);
LDAP_SLAPD_F(int)
slap_cookie_is_sid_here(const struct sync_cookie *cookie, int sid);

/*
 * limits.c
 */
LDAP_SLAPD_F(int)
limits_parse(Backend *be, const char *fname, int lineno, int argc, char **argv);
LDAP_SLAPD_F(int)
limits_parse_one(const char *arg, struct slap_limits_set *limit);
LDAP_SLAPD_F(int) limits_check(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int)
limits_unparse_one(struct slap_limits_set *limit, int which, struct berval *bv,
                   ber_len_t buflen);
LDAP_SLAPD_F(int)
limits_unparse(struct slap_limits *limit, struct berval *bv, ber_len_t buflen);
LDAP_SLAPD_F(void) limits_free_one(struct slap_limits *lm);
LDAP_SLAPD_F(void) limits_destroy(struct slap_limits **lm);

/*
 * lock.c
 */
LDAP_SLAPD_F(FILE *)
lock_fopen(const char *fname, const char *type, FILE **lfp);
LDAP_SLAPD_F(int) lock_fclose(FILE *fp, FILE *lfp);

/*
 * main.c
 */
LDAP_SLAPD_F(int)
parse_debug_level(const char *arg, int *levelp, char ***unknowns);
LDAP_SLAPD_F(int)
parse_syslog_severity(const char *arg, int *severity);
LDAP_SLAPD_F(int)
parse_syslog_user(const char *arg, int *syslogUser);
LDAP_SLAPD_F(int)
parse_debug_unknowns(char **unknowns, int *levelp);

/*
 * matchedValues.c
 */
LDAP_SLAPD_F(int)
filter_matched_values(Operation *op, Attribute *a, char ***e_flags);

/*
 * modrdn.c
 */
LDAP_SLAPD_F(int) slap_modrdn2mods(Operation *op, SlapReply *rs);

/*
 * modify.c
 */
LDAP_SLAPD_F(int)
slap_mods_obsolete_check(Operation *op, Modifications *ml, const char **text,
                         char *textbuf, size_t textlen);

LDAP_SLAPD_F(int)
slap_mods_no_user_mod_check(Operation *op, Modifications *ml, const char **text,
                            char *textbuf, size_t textlen);

LDAP_SLAPD_F(int)
slap_mods_no_repl_user_mod_check(Operation *op, Modifications *ml,
                                 const char **text, char *textbuf,
                                 size_t textlen);

LDAP_SLAPD_F(int)
slap_mods_check(Operation *op, Modifications *ml, const char **text,
                char *textbuf, size_t textlen, void *ctx);

LDAP_SLAPD_F(int)
slap_sort_vals(Modifications *ml, const char **text, int *dup, void *ctx);

LDAP_SLAPD_F(void) slap_timestamp(time_t tm, struct berval *bv);

LDAP_SLAPD_F(void)
slap_mods_opattrs(Operation *op, Modifications **modsp, int manage_ctxcsn);

LDAP_SLAPD_F(int)
slap_parse_modlist(Operation *op, SlapReply *rs, BerElement *ber,
                   req_modify_s *ms);

/*
 * mods.c
 */
LDAP_SLAPD_F(int)
modify_add_values(Entry *e, Modification *mod, int permissive,
                  const char **text, char *textbuf, size_t textlen);
LDAP_SLAPD_F(int)
modify_delete_values(Entry *e, Modification *mod, int permissive,
                     const char **text, char *textbuf, size_t textlen);
LDAP_SLAPD_F(int)
modify_delete_vindex(Entry *e, Modification *mod, int permissive,
                     const char **text, char *textbuf, size_t textlen,
                     int *idx);
LDAP_SLAPD_F(int)
modify_replace_values(Entry *e, Modification *mod, int permissive,
                      const char **text, char *textbuf, size_t textlen);
LDAP_SLAPD_F(int)
modify_increment_values(Entry *e, Modification *mod, int permissive,
                        const char **text, char *textbuf, size_t textlen);

LDAP_SLAPD_F(void) slap_mod_free(Modification *mod, int freeit);
LDAP_SLAPD_F(void) slap_mods_free(Modifications *mods, int freevals);
LDAP_SLAPD_F(void) slap_modlist_free(LDAPModList *ml);

/*
 * module.c
 */
#ifdef SLAPD_DYNAMIC_MODULES

LDAP_SLAPD_F(int) module_init(void);
LDAP_SLAPD_F(int) module_kill(void);

LDAP_SLAPD_F(int) module_load(const char *file_name, int argc, char *argv[]);
LDAP_SLAPD_F(int) module_path(const char *path);
LDAP_SLAPD_F(int) module_unload(const char *file_name);

typedef struct module module_t;

LDAP_SLAPD_F(module_t *) module_handle(const char *file_name);
LDAP_SLAPD_F(void *) module_resolve(const module_t *module, const char *name);

#endif /* SLAPD_DYNAMIC_MODULES */

/* mr.c */
LDAP_SLAPD_F(MatchingRule *) mr_bvfind(struct berval *mrname);
LDAP_SLAPD_F(MatchingRule *) mr_find(const char *mrname);
LDAP_SLAPD_F(int)
mr_add(LDAPMatchingRule *mr, slap_mrule_defs_rec *def, MatchingRule *associated,
       const char **err);
LDAP_SLAPD_F(void) mr_destroy(void);

LDAP_SLAPD_F(int) register_matching_rule(slap_mrule_defs_rec *def);

LDAP_SLAPD_F(void) mru_destroy(void);
LDAP_SLAPD_F(int) matching_rule_use_init(void);

LDAP_SLAPD_F(int) mr_schema_info(Entry *e);
LDAP_SLAPD_F(int) mru_schema_info(Entry *e);

LDAP_SLAPD_F(int) mr_usable_with_at(MatchingRule *mr, AttributeType *at);
LDAP_SLAPD_F(int) mr_make_syntax_compat_with_mr(Syntax *syn, MatchingRule *mr);
LDAP_SLAPD_F(int)
mr_make_syntax_compat_with_mrs(const char *syntax, char *const *mrs);

/*
 * mra.c
 */
LDAP_SLAPD_F(int)
get_mra(Operation *op, BerElement *ber, Filter *f, const char **text);
LDAP_SLAPD_F(void)
mra_free(Operation *op, MatchingRuleAssertion *mra, int freeit);

/* oc.c */
LDAP_SLAPD_F(int)
oc_add(LDAPObjectClass *oc, int user, ObjectClass **soc, ObjectClass *prev,
       const char **err);
LDAP_SLAPD_F(void) oc_destroy(void);

LDAP_SLAPD_F(ObjectClass *) oc_find(const char *ocname);
LDAP_SLAPD_F(ObjectClass *) oc_bvfind(struct berval *ocname);
LDAP_SLAPD_F(ObjectClass *) oc_bvfind_undef(struct berval *ocname);
LDAP_SLAPD_F(int) is_object_subclass(ObjectClass *sup, ObjectClass *sub);

LDAP_SLAPD_F(int)
is_entry_objectclass(Entry *, ObjectClass *oc, unsigned flags);
#define is_entry_objectclass_or_sub(e, oc)                                     \
  (is_entry_objectclass((e), (oc), SLAP_OCF_CHECK_SUP))
#define is_entry_alias(e)                                                      \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_ALIAS) != 0)                               \
       : is_entry_objectclass((e), slap_schema.si_oc_alias,                    \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_referral(e)                                                   \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_REFERRAL) != 0)                            \
       : is_entry_objectclass((e), slap_schema.si_oc_referral,                 \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_subentry(e)                                                   \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_SUBENTRY) != 0)                            \
       : is_entry_objectclass((e), slap_schema.si_oc_subentry,                 \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_collectiveAttributeSubentry(e)                                \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY) != 0)         \
       : is_entry_objectclass((e),                                             \
                              slap_schema.si_oc_collectiveAttributeSubentry,   \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_dynamicObject(e)                                              \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_DYNAMICOBJECT) != 0)                       \
       : is_entry_objectclass((e), slap_schema.si_oc_dynamicObject,            \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_glue(e)                                                       \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_GLUE) != 0)                                \
       : is_entry_objectclass((e), slap_schema.si_oc_glue,                     \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_syncProviderSubentry(e)                                       \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_SYNCPROVIDERSUBENTRY) != 0)                \
       : is_entry_objectclass((e), slap_schema.si_oc_syncProviderSubentry,     \
                              SLAP_OCF_SET_FLAGS))
#define is_entry_syncConsumerSubentry(e)                                       \
  (((e)->e_ocflags & SLAP_OC__END)                                             \
       ? (((e)->e_ocflags & SLAP_OC_SYNCCONSUMERSUBENTRY) != 0)                \
       : is_entry_objectclass((e), slap_schema.si_oc_syncConsumerSubentry,     \
                              SLAP_OCF_SET_FLAGS))

LDAP_SLAPD_F(int) oc_schema_info(Entry *e);

LDAP_SLAPD_F(int) oc_start(ObjectClass **oc);
LDAP_SLAPD_F(int) oc_next(ObjectClass **oc);
LDAP_SLAPD_F(void) oc_delete(ObjectClass *oc);

LDAP_SLAPD_F(void)
oc_unparse(BerVarray *bva, ObjectClass *start, ObjectClass *end, int system);

LDAP_SLAPD_F(int) register_oc(const char *desc, ObjectClass **oc, int dupok);

/*
 * oidm.c
 */
LDAP_SLAPD_F(char *) oidm_find(char *oid);
LDAP_SLAPD_F(void) oidm_destroy(void);
LDAP_SLAPD_F(void)
oidm_unparse(BerVarray *bva, OidMacro *start, OidMacro *end, int system);
LDAP_SLAPD_F(int) parse_oidm(struct config_args_s *ca, int user, OidMacro **om);

/*
 * operation.c
 */
LDAP_SLAPD_F(void) slap_op_init(void);
LDAP_SLAPD_F(void) slap_op_destroy(void);
LDAP_SLAPD_F(void) slap_op_groups_free(Operation *op);
LDAP_SLAPD_F(void) slap_op_free(Operation *op, void *ctx);
LDAP_SLAPD_F(void) slap_op_time(time_t *t, int *n);
LDAP_SLAPD_F(Operation *)
slap_op_alloc(BerElement *ber, ber_int_t msgid, ber_tag_t tag, ber_int_t id,
              void *ctx);

LDAP_SLAPD_F(slap_op_t) slap_req2op(ber_tag_t tag);

/*
 * operational.c
 */
LDAP_SLAPD_F(Attribute *) slap_operational_subschemaSubentry(Backend *be);
LDAP_SLAPD_F(Attribute *) slap_operational_entryDN(Entry *e);
LDAP_SLAPD_F(Attribute *) slap_operational_hasSubordinate(int has);

/*
 * overlays.c
 */
LDAP_SLAPD_F(int) overlay_init(void);

/*
 * passwd.c
 */
LDAP_SLAPD_F(SLAP_EXTOP_MAIN_FN) passwd_extop;

LDAP_SLAPD_F(int)
slap_passwd_check(Operation *op, Entry *e, Attribute *a, struct berval *cred,
                  const char **text);

LDAP_SLAPD_F(void) slap_passwd_generate(struct berval *);

LDAP_SLAPD_F(void)
slap_passwd_hash(struct berval *cred, struct berval *hash, const char **text);

LDAP_SLAPD_F(void)
slap_passwd_hash_type(struct berval *cred, struct berval *hash, char *htype,
                      const char **text);

LDAP_SLAPD_F(struct berval *) slap_passwd_return(struct berval *cred);

LDAP_SLAPD_F(int)
slap_passwd_parse(struct berval *reqdata, struct berval *id,
                  struct berval *oldpass, struct berval *newpass,
                  const char **text);

LDAP_SLAPD_F(void) slap_passwd_init(void);

/*
 * phonetic.c
 */
LDAP_SLAPD_F(char *) phonetic(char *s);

/*
 * referral.c
 */
LDAP_SLAPD_F(int) validate_global_referral(const char *url);

LDAP_SLAPD_F(BerVarray) get_entry_referrals(Operation *op, Entry *e);

LDAP_SLAPD_F(BerVarray)
referral_rewrite(BerVarray refs, struct berval *base, struct berval *target,
                 int scope);

LDAP_SLAPD_F(int)
get_alias_dn(Entry *e, struct berval *ndn, int *err, const char **text);

/*
 * result.c
 */
#if USE_RS_ASSERT
#ifdef __GNUC__
#define RS_FUNC_ __FUNCTION__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__) >= 199901L
#define RS_FUNC_ __func__
#else
#define rs_assert_(file, line, func, cond) rs_assert__(file, line, cond)
#endif /* __GNUC__ */

LDAP_SLAPD_F(void)
rs_assert_(const char *, unsigned, const char *, const char *);
#define RS_ASSERT(cond)                                                        \
  do {                                                                         \
    if (reopenldap_mode_check() && unlikely(!(cond)))                          \
      rs_assert_(__FILE__, __LINE__, RS_FUNC_, #cond);                         \
  } while (0)
#else
#define RS_ASSERT(cond) ((void)0)
#define rs_assert_ok(rs) ((void)(rs))
#define rs_assert_ready(rs) ((void)(rs))
#define rs_assert_done(rs) ((void)(rs))
#endif /* USE_RS_ASSERT */

LDAP_SLAPD_F(void)(rs_assert_ok)(const SlapReply *rs);
LDAP_SLAPD_F(void)(rs_assert_ready)(const SlapReply *rs);
LDAP_SLAPD_F(void)(rs_assert_done)(const SlapReply *rs);

#define rs_reinit(rs, type)                                                    \
  do {                                                                         \
    SlapReply *const rsRI = (rs);                                              \
    rs_assert_done(rsRI);                                                      \
    rsRI->sr_type = (type);                                                    \
    /* Got type before memset in case of rs_reinit(rs, rs->sr_type) */         \
    assert(!offsetof(SlapReply, sr_type));                                     \
    memset((slap_reply_t *)rsRI + 1, 0, sizeof(*rsRI) - sizeof(slap_reply_t)); \
  } while (0)
LDAP_SLAPD_F(void)(rs_reinit)(SlapReply *rs, slap_reply_t type);
LDAP_SLAPD_F(void)
rs_flush_entry(Operation *op, SlapReply *rs, slap_overinst *on);
LDAP_SLAPD_F(void)
rs_replace_entry(Operation *op, SlapReply *rs, slap_overinst *on, Entry *e);
LDAP_SLAPD_F(int)
rs_entry2modifiable(Operation *op, SlapReply *rs, slap_overinst *on);
#define rs_ensure_entry_modifiable rs_entry2modifiable /* older name */
LDAP_SLAPD_F(void) slap_send_ldap_result(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(void) send_ldap_sasl(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(void) send_ldap_disconnect(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(void) slap_send_ldap_extended(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(void) slap_send_ldap_intermediate(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(void) slap_send_search_result(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) slap_send_search_reference(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) slap_send_search_entry(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) slap_null_cb(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) slap_freeself_cb(Operation *op, SlapReply *rs);

LDAP_SLAPD_V(const struct berval) slap_pre_read_bv;
LDAP_SLAPD_V(const struct berval) slap_post_read_bv;
LDAP_SLAPD_F(int)
slap_read_controls(Operation *op, SlapReply *rs, Entry *e,
                   const struct berval *oid, LDAPControl **ctrl);

LDAP_SLAPD_F(int) str2result(char *s, int *code, char **matched, char **info);
LDAP_SLAPD_F(int) slap_map_api2result(SlapReply *rs);
LDAP_SLAPD_F(slap_mask_t) slap_attr_flags(AttributeName *an);
LDAP_SLAPD_F(ber_tag_t) slap_req2res(ber_tag_t tag);

LDAP_SLAPD_V(const struct berval) slap_dummy_bv;

/*
 * root_dse.c
 */
LDAP_SLAPD_F(int) root_dse_init(void);
LDAP_SLAPD_F(int) root_dse_destroy(void);

LDAP_SLAPD_F(int) root_dse_info(Connection *conn, Entry **e, const char **text);

LDAP_SLAPD_F(int) root_dse_read_file(const char *file);

LDAP_SLAPD_F(int)
slap_discover_feature(slap_bindconf *sb, const char *attr, const char *val);

LDAP_SLAPD_F(int) supported_feature_load(struct berval *f);
LDAP_SLAPD_F(int) supported_feature_destroy(void);

LDAP_SLAPD_F(int) entry_info_register(SLAP_ENTRY_INFO_FN func, void *arg);
LDAP_SLAPD_F(int) entry_info_unregister(SLAP_ENTRY_INFO_FN func, void *arg);
LDAP_SLAPD_F(void) entry_info_destroy(void);

/*
 * sasl.c
 */
LDAP_SLAPD_F(int) slap_sasl_init(void);
LDAP_SLAPD_F(char *) slap_sasl_secprops(const char *);
LDAP_SLAPD_F(void) slap_sasl_secprops_unparse(struct berval *);
LDAP_SLAPD_F(int) slap_sasl_destroy(void);

LDAP_SLAPD_F(int) slap_sasl_open(Connection *c, int reopen);
LDAP_SLAPD_F(char **) slap_sasl_mechs(Connection *c);

LDAP_SLAPD_F(int)
slap_sasl_external(Connection *c,
                   slap_ssf_t ssf, /* relative strength of external security */
                   struct berval *authid); /* asserted authentication id */

LDAP_SLAPD_F(int) slap_sasl_cbinding(Connection *c, struct berval *);

LDAP_SLAPD_F(int) slap_sasl_reset(Connection *c);
LDAP_SLAPD_F(int) slap_sasl_close(Connection *c);

LDAP_SLAPD_F(int) slap_sasl_bind(Operation *op, SlapReply *rs);

LDAP_SLAPD_F(int) slap_sasl_setpass(Operation *op, SlapReply *rs);

LDAP_SLAPD_F(int)
slap_sasl_getdn(Connection *conn, Operation *op, struct berval *id,
                char *user_realm, struct berval *dn, int flags);

/*
 * saslauthz.c
 */
LDAP_SLAPD_F(int)
slap_parse_user(struct berval *id, struct berval *user, struct berval *realm,
                struct berval *mech);
LDAP_SLAPD_F(int)
slap_sasl_matches(Operation *op, BerVarray rules, struct berval *assertDN,
                  struct berval *authc);
LDAP_SLAPD_F(void)
slap_sasl2dn(Operation *op, struct berval *saslname, struct berval *dn,
             int flags);
LDAP_SLAPD_F(int)
slap_sasl_authorized(Operation *op, struct berval *authcid,
                     struct berval *authzid);
LDAP_SLAPD_F(int)
slap_sasl_regexp_config(const char *match, const char *replace);
LDAP_SLAPD_F(void) slap_sasl_regexp_unparse(BerVarray *bva);
LDAP_SLAPD_F(int) slap_sasl_setpolicy(const char *);
LDAP_SLAPD_F(const char *) slap_sasl_getpolicy(void);
#ifdef SLAP_AUTH_REWRITE
LDAP_SLAPD_F(int)
slap_sasl_rewrite_config(const char *fname, int lineno, int argc, char **argv);
#endif /* SLAP_AUTH_REWRITE */
LDAP_SLAPD_F(void) slap_sasl_regexp_destroy(void);
LDAP_SLAPD_F(int) authzValidate(Syntax *syn, struct berval *in);
#if 0
LDAP_SLAPD_F (int) authzMatch (
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue );
#endif
LDAP_SLAPD_F(int)
authzPretty(Syntax *syntax, struct berval *val, struct berval *out, void *ctx);
LDAP_SLAPD_F(int)
authzNormalize(slap_mask_t usage, Syntax *syntax, MatchingRule *mr,
               struct berval *val, struct berval *normalized, void *ctx);

/*
 * schema.c
 */
LDAP_SLAPD_F(int) schema_info(Entry **entry, const char **text);

/*
 * schema_check.c
 */
LDAP_SLAPD_F(int)
oc_check_allowed(AttributeType *type, ObjectClass **socs, ObjectClass *sc);

LDAP_SLAPD_F(int)
structural_class(BerVarray ocs, ObjectClass **sc, ObjectClass ***socs,
                 const char **text, char *textbuf, size_t textlen, void *ctx);

LDAP_SLAPD_F(int)
entry_schema_check(Operation *op, Entry *e, Attribute *attrs, int manage,
                   int add, Attribute **socp, const char **text, char *textbuf,
                   size_t textlen);

LDAP_SLAPD_F(int)
mods_structural_class(Modifications *mods, struct berval *oc, const char **text,
                      char *textbuf, size_t textlen, void *ctx);

/*
 * schema_init.c
 */
LDAP_SLAPD_V(int) schema_init_done;
LDAP_SLAPD_F(int) slap_schema_init(void);
LDAP_SLAPD_F(void) schema_destroy(void);

LDAP_SLAPD_F(int) slap_hash64(int);

LDAP_SLAPD_F(slap_mr_indexer_func) octetStringIndexer;
LDAP_SLAPD_F(slap_mr_filter_func) octetStringFilter;

LDAP_SLAPD_F(int) numericoidValidate(Syntax *syntax, struct berval *in);
LDAP_SLAPD_F(int) numericStringValidate(Syntax *syntax, struct berval *in);
LDAP_SLAPD_F(int)
octetStringMatch(int *matchp, slap_mask_t flags, Syntax *syntax,
                 MatchingRule *mr, struct berval *value, void *assertedValue);
LDAP_SLAPD_F(int)
octetStringOrderingMatch(int *matchp, slap_mask_t flags, Syntax *syntax,
                         MatchingRule *mr, struct berval *value,
                         void *assertedValue);

/*
 * schema_prep.c
 */
LDAP_SLAPD_V(struct slap_internal_schema) slap_schema;
LDAP_SLAPD_F(int) slap_schema_load(void);
LDAP_SLAPD_F(int) slap_schema_check(void);

/*
 * schemaparse.c
 */
LDAP_SLAPD_F(int) slap_valid_descr(const char *);

LDAP_SLAPD_F(int) parse_cr(struct config_args_s *ca, ContentRule **scr);
LDAP_SLAPD_F(int)
parse_oc(struct config_args_s *ca, ObjectClass **soc, ObjectClass *prev);
LDAP_SLAPD_F(int)
parse_at(struct config_args_s *ca, AttributeType **sat, AttributeType *prev);
LDAP_SLAPD_F(char *) scherr2str(int code) __attribute__((const));
LDAP_SLAPD_F(int) dscompare(const char *s1, const char *s2del, char delim);
LDAP_SLAPD_F(int)
parse_syn(struct config_args_s *ca, Syntax **sat, Syntax *prev);

/*
 * sessionlog.c
 */
LDAP_SLAPD_F(int) slap_send_session_log(Operation *, Operation *, SlapReply *);
LDAP_SLAPD_F(int) slap_add_session_log(Operation *, Operation *, Entry *);

/*
 * sl_malloc.c
 */
LDAP_SLAPD_F(void *) slap_sl_malloc(ber_len_t size, void *ctx);
LDAP_SLAPD_F(void *) slap_sl_realloc(void *block, ber_len_t size, void *ctx);
LDAP_SLAPD_F(void *) slap_sl_calloc(ber_len_t nelem, ber_len_t size, void *ctx);
LDAP_SLAPD_F(void) slap_sl_free(void *, void *ctx);
LDAP_SLAPD_F(void) slap_sl_release(void *, void *ctx);
LDAP_SLAPD_F(void *) slap_sl_mark(void *ctx);

LDAP_SLAPD_V(const BerMemoryFunctions) slap_sl_mfuncs;

LDAP_SLAPD_F(void) slap_sl_mem_init(void);
LDAP_SLAPD_F(void *)
slap_sl_mem_create(ber_len_t size, int stack, void *ctx, int flag);
LDAP_SLAPD_F(void) slap_sl_mem_setctx(void *ctx, void *memctx);
LDAP_SLAPD_F(void) slap_sl_mem_destroy(void *key, void *data);
LDAP_SLAPD_F(void *) slap_sl_context(void *ptr);

/*
 * starttls.c
 */
LDAP_SLAPD_F(SLAP_EXTOP_MAIN_FN) starttls_extop;

/*
 * str2filter.c
 */
LDAP_SLAPD_F(Filter *) str2filter(const char *str);
LDAP_SLAPD_F(Filter *) str2filter_x(Operation *op, const char *str);

/*
 * syncrepl.c
 */

LDAP_SLAPD_F(int) syncrepl_add_glue(Operation *, Entry *);
LDAP_SLAPD_F(void)
syncrepl_diff_entry(Operation *op, Attribute *old, Attribute *anew,
                    Modifications **mods, Modifications **ml, int is_ctx);
LDAP_SLAPD_F(void) syncinfo_free(struct syncinfo_s *, int all);

/* syntax.c */
LDAP_SLAPD_F(int) syn_is_sup(Syntax *syn, Syntax *sup);
LDAP_SLAPD_F(Syntax *) syn_find(const char *synname);
LDAP_SLAPD_F(Syntax *) syn_find_desc(const char *syndesc, int *slen);
LDAP_SLAPD_F(int)
syn_add(LDAPSyntax *syn, int user, slap_syntax_defs_rec *def, Syntax **ssyn,
        Syntax *prev, const char **err);
LDAP_SLAPD_F(void) syn_destroy(void);

LDAP_SLAPD_F(int) register_syntax(slap_syntax_defs_rec *def);

LDAP_SLAPD_F(int) syn_schema_info(Entry *e);

LDAP_SLAPD_F(int) syn_start(Syntax **at);
LDAP_SLAPD_F(int) syn_next(Syntax **at);
LDAP_SLAPD_F(void) syn_delete(Syntax *at);

LDAP_SLAPD_F(void)
syn_unparse(BerVarray *bva, Syntax *start, Syntax *end, int system);

/*
 * user.c
 */
#if defined(HAVE_PWD_H) && defined(HAVE_GRP_H)
LDAP_SLAPD_F(void) slap_init_user(char *username, char *groupname);
#endif

/*
 * value.c
 */
LDAP_SLAPD_F(int)
asserted_value_validate_normalize(AttributeDescription *ad, MatchingRule *mr,
                                  unsigned usage, struct berval *in,
                                  struct berval *out, const char **text,
                                  void *ctx);

LDAP_SLAPD_F(int)
value_match(int *match, AttributeDescription *ad, MatchingRule *mr,
            unsigned flags, struct berval *v1, void *v2, const char **text);
LDAP_SLAPD_F(int)
value_find_ex(AttributeDescription *ad, unsigned flags, BerVarray values,
              struct berval *value, void *ctx);

LDAP_SLAPD_F(int)
ordered_value_add(Entry *e, AttributeDescription *ad, Attribute *a,
                  BerVarray vals, BerVarray nvals);

LDAP_SLAPD_F(int)
ordered_value_validate(AttributeDescription *ad, struct berval *in, int mop);

LDAP_SLAPD_F(int)
ordered_value_pretty(AttributeDescription *ad, struct berval *val,
                     struct berval *out, void *ctx);

LDAP_SLAPD_F(int)
ordered_value_normalize(slap_mask_t usage, AttributeDescription *ad,
                        MatchingRule *mr, struct berval *val,
                        struct berval *normalized, void *ctx);

LDAP_SLAPD_F(int)
ordered_value_match(int *match, AttributeDescription *ad, MatchingRule *mr,
                    unsigned flags, struct berval *v1, struct berval *v2,
                    const char **text);

LDAP_SLAPD_F(void) ordered_value_renumber(Attribute *a);

LDAP_SLAPD_F(int) ordered_value_sort(Attribute *a, int do_renumber);

LDAP_SLAPD_F(int) value_add(BerVarray *vals, BerVarray addvals);
LDAP_SLAPD_F(int) value_add_one(BerVarray *vals, const struct berval *addval);
LDAP_SLAPD_F(int) value_add_one_str(BerVarray *vals, const char *str);
LDAP_SLAPD_F(int) value_add_one_int(BerVarray *vals, int x);
LDAP_SLAPD_F(int)
value_join_str(BerVarray vals, const char *comma, BerValue *dest);

/* assumes (x) > (y) returns 1 if true, 0 otherwise */
#define SLAP_PTRCMP(x, y) ((x) < (y) ? -1 : (x) > (y))

/*
 * Other...
 */
LDAP_SLAPD_V(unsigned int) index_substr_if_minlen;
LDAP_SLAPD_V(unsigned int) index_substr_if_maxlen;
LDAP_SLAPD_V(unsigned int) index_substr_any_len;
LDAP_SLAPD_V(unsigned int) index_substr_any_step;
LDAP_SLAPD_V(unsigned int) index_intlen;
/* all signed integers from strings of this size need more than intlen bytes */
/* i.e. log(10)*(index_intlen_strlen-2) > log(2)*(8*(index_intlen)-1) */
LDAP_SLAPD_V(unsigned int) index_intlen_strlen;
#define SLAP_INDEX_INTLEN_STRLEN(intlen) ((8 * (intlen)-1) * 146 / 485 + 3)

LDAP_SLAPD_V(ber_len_t) sockbuf_max_incoming;
LDAP_SLAPD_V(ber_len_t) sockbuf_max_incoming_auth;
LDAP_SLAPD_V(int) slap_conn_max_pending;
LDAP_SLAPD_V(int) slap_conn_max_pending_auth;

LDAP_SLAPD_V(slap_mask_t) global_allows;
LDAP_SLAPD_V(slap_mask_t) global_disallows;

LDAP_SLAPD_V(BerVarray) default_referral;
LDAP_SLAPD_V(const char) SlapdVersionStr[];

LDAP_SLAPD_V(int) global_gentlehup;
LDAP_SLAPD_V(int) global_idletimeout;
LDAP_SLAPD_V(int) global_writetimeout;
LDAP_SLAPD_V(char *) global_host;
LDAP_SLAPD_V(struct berval) global_host_bv;
LDAP_SLAPD_V(char *) global_realm;
LDAP_SLAPD_V(char *) sasl_host;
LDAP_SLAPD_V(char *) slap_sasl_auxprops;
#ifdef SLAP_AUXPROP_DONTUSECOPY
LDAP_SLAPD_V(int) slap_dontUseCopy_ignore;
LDAP_SLAPD_V(BerVarray) slap_dontUseCopy_propnames;
#endif /* SLAP_AUXPROP_DONTUSECOPY */
LDAP_SLAPD_V(char **) default_passwd_hash;
LDAP_SLAPD_V(struct berval) default_search_base;
LDAP_SLAPD_V(struct berval) default_search_nbase;

LDAP_SLAPD_V(slap_counters_t) slap_counters;

LDAP_SLAPD_V(char *) slapd_pid_file;
LDAP_SLAPD_V(char *) slapd_args_file;
LDAP_SLAPD_V(slap_time_t) starttime;

LDAP_SLAPD_V(ldap_pvt_thread_pool_t) connection_pool;
LDAP_SLAPD_V(int) connection_pool_max;
LDAP_SLAPD_V(int) connection_pool_queues;
LDAP_SLAPD_V(int) slap_tool_thread_max;

LDAP_SLAPD_V(ldap_pvt_thread_mutex_t) entry2str_mutex;

LDAP_SLAPD_V(ldap_pvt_thread_mutex_t) ad_index_mutex;
LDAP_SLAPD_V(ldap_pvt_thread_mutex_t) ad_undef_mutex;
LDAP_SLAPD_V(ldap_pvt_thread_mutex_t) oc_undef_mutex;

LDAP_SLAPD_V(ber_socket_t) dtblsize;

LDAP_SLAPD_V(int) use_reverse_lookup;

/*
 * operations
 */
LDAP_SLAPD_F(int) do_abandon(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_add(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_bind(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_compare(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_delete(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_modify(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_modrdn(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_search(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_unbind(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) do_extended(Operation *op, SlapReply *rs);

/*
 * frontend operations
 */
LDAP_SLAPD_F(int) fe_op_abandon(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_add(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_bind(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_bind_success(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_compare(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_delete(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_modify(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_modrdn(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_op_search(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int) fe_aux_operational(Operation *op, SlapReply *rs);
#if 0
LDAP_SLAPD_F (int) fe_op_unbind(Operation *op, SlapReply *rs);
#endif
LDAP_SLAPD_F(int) fe_extended(Operation *op, SlapReply *rs);
LDAP_SLAPD_F(int)
fe_acl_group(Operation *op, Entry *target, struct berval *gr_ndn,
             struct berval *op_ndn, ObjectClass *group_oc,
             AttributeDescription *group_at);
LDAP_SLAPD_F(int)
fe_acl_attribute(Operation *op, Entry *target, struct berval *edn,
                 AttributeDescription *entry_at, BerVarray *vals,
                 slap_access_t access);
LDAP_SLAPD_F(int)
fe_access_allowed(Operation *op, Entry *e, AttributeDescription *desc,
                  struct berval *val, slap_access_t access,
                  AccessControlState *state, slap_mask_t *maskp);

/* NOTE: this macro assumes that bv has been allocated
 * by ber_* malloc functions or is { 0L, NULL } */
#ifdef USE_MP_BIGNUM
#define UI2BVX(bv, ui, ctx)                                                    \
  do {                                                                         \
    char *val;                                                                 \
    ber_len_t len;                                                             \
    val = BN_bn2dec(ui);                                                       \
    if (val) {                                                                 \
      len = strlen(val);                                                       \
      if (len > (bv)->bv_len) {                                                \
        (bv)->bv_val = ber_memrealloc_x((bv)->bv_val, len + 1, (ctx));         \
      }                                                                        \
      memcpy((bv)->bv_val, val, len + 1);                                      \
      (bv)->bv_len = len;                                                      \
      OPENSSL_free(val);                                                       \
    } else {                                                                   \
      ber_memfree_x((bv)->bv_val, (ctx));                                      \
      BER_BVZERO((bv));                                                        \
    }                                                                          \
  } while (0)

#elif defined(USE_MP_GMP)
/* NOTE: according to the documentation, the result
 * of mpz_sizeinbase() can exceed the length of the
 * string representation of the number by 1
 */
#define UI2BVX(bv, ui, ctx)                                                    \
  do {                                                                         \
    ber_len_t len = mpz_sizeinbase((ui), 10);                                  \
    if (len > (bv)->bv_len) {                                                  \
      (bv)->bv_val = ber_memrealloc_x((bv)->bv_val, len + 1, (ctx));           \
    }                                                                          \
    (void)mpz_get_str((bv)->bv_val, 10, (ui));                                 \
    if ((bv)->bv_val[len - 1] == '\0') {                                       \
      len--;                                                                   \
    }                                                                          \
    (bv)->bv_len = len;                                                        \
  } while (0)

#else
#ifdef USE_MP_LONG_LONG
#define UI2BV_FORMAT "%llu"
#elif defined USE_MP_LONG
#define UI2BV_FORMAT "%lu"
#elif defined HAVE_LONG_LONG
#define UI2BV_FORMAT "%llu"
#else
#define UI2BV_FORMAT "%lu"
#endif

#define UI2BVX(bv, ui, ctx)                                                    \
  do {                                                                         \
    char buf[LDAP_PVT_INTTYPE_CHARS(long)];                                    \
    ber_len_t len;                                                             \
    len = snprintf(buf, sizeof(buf), UI2BV_FORMAT, (ui));                      \
    if (len > (bv)->bv_len) {                                                  \
      (bv)->bv_val = ber_memrealloc_x((bv)->bv_val, len + 1, (ctx));           \
    }                                                                          \
    (bv)->bv_len = len;                                                        \
    memcpy((bv)->bv_val, buf, len + 1);                                        \
  } while (0)
#endif

#define UI2BV(bv, ui) UI2BVX(bv, ui, NULL)

LDAP_END_DECL

#endif /* PROTO_SLAP_H */
