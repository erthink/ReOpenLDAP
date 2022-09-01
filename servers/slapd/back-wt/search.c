/* $ReOpenLDAP$ */
/* Copyright 2002-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* ACKNOWLEDGEMENTS:
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#include "reldap.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "idl.h"

static int search_aliases(Operation *op, SlapReply *rs, Entry *e,
                          WT_SESSION *session, ID *ids, ID *scopes, ID *stack) {
  /* TODO: search_aliases does not implement yet. */
  WT_IDL_ZERO(ids);
  return 0;
}

static int base_candidate(BackendDB *be, Entry *e, ID *ids) {
  Debug(LDAP_DEBUG_ARGS, "base_candidate: base: \"%s\" (0x%08lx)\n",
        e->e_nname.bv_val, (long)e->e_id);

  ids[0] = 1;
  ids[1] = e->e_id;
  return 0;
}

/* Look for "objectClass Present" in this filter.
 * Also count depth of filter tree while we're at it.
 */
static int oc_filter(Filter *f, int cur, int *max) {
  int rc = 0;

  assert(f != NULL);

  if (cur > *max)
    *max = cur;

  switch (f->f_choice) {
  case LDAP_FILTER_PRESENT:
    if (f->f_desc == slap_schema.si_ad_objectClass) {
      rc = 1;
    }
    break;

  case LDAP_FILTER_AND:
  case LDAP_FILTER_OR:
    cur++;
    for (f = f->f_and; f; f = f->f_next) {
      (void)oc_filter(f, cur, max);
    }
    break;

  default:
    break;
  }
  return rc;
}

static void search_stack_free(void *key, void *data) {
  ber_memfree_x(data, NULL);
}

static void *search_stack(Operation *op) {
  struct wt_info *wi = (struct wt_info *)op->o_bd->be_private;
  void *ret = NULL;

  if (op->o_threadctx) {
    ldap_pvt_thread_pool_getkey(op->o_threadctx, (void *)search_stack, &ret,
                                NULL);
  } else {
    ret = wi->wi_search_stack;
  }

  if (!ret) {
    ret = ch_malloc(wi->wi_search_stack_depth * WT_IDL_UM_SIZE * sizeof(ID));
    if (op->o_threadctx) {
      ldap_pvt_thread_pool_setkey(op->o_threadctx, (void *)search_stack, ret,
                                  search_stack_free, NULL, NULL);
    } else {
      wi->wi_search_stack = ret;
    }
  }
  return ret;
}

static int search_candidates(Operation *op, SlapReply *rs, Entry *e, wt_ctx *wc,
                             ID *ids, ID *scopes) {
  struct wt_info *wi = (struct wt_info *)op->o_bd->be_private;
  int rc, depth = 1;
  Filter f, rf, xf, nf;
  ID *stack;
  AttributeAssertion aa_ref = ATTRIBUTEASSERTION_INIT;
  Filter sf;
  AttributeAssertion aa_subentry = ATTRIBUTEASSERTION_INIT;

  Debug(LDAP_DEBUG_TRACE,
        "wt_search_candidates: base=\"%s\" (0x%08lx) scope=%d\n",
        e->e_nname.bv_val, (long)e->e_id, op->oq_search.rs_scope);

  xf.f_or = op->oq_search.rs_filter;
  xf.f_choice = LDAP_FILTER_OR;
  xf.f_next = NULL;

  /* If the user's filter uses objectClass=*,
   * these clauses are redundant.
   */
  if (!oc_filter(op->oq_search.rs_filter, 1, &depth) &&
      !get_subentries_visibility(op)) {
    if (!get_manageDSAit(op) && !get_domainScope(op)) {
      /* match referral objects */
      struct berval bv_ref = BER_BVC("referral");
      rf.f_choice = LDAP_FILTER_EQUALITY;
      rf.f_ava = &aa_ref;
      rf.f_av_desc = slap_schema.si_ad_objectClass;
      rf.f_av_value = bv_ref;
      rf.f_next = xf.f_or;
      xf.f_or = &rf;
      depth++;
    }
  }

  f.f_next = NULL;
  f.f_choice = LDAP_FILTER_AND;
  f.f_and = &nf;
  /* Dummy; we compute scope separately now */
  nf.f_choice = SLAPD_FILTER_COMPUTED;
  nf.f_result = LDAP_SUCCESS;
  nf.f_next =
      (xf.f_or == op->oq_search.rs_filter) ? op->oq_search.rs_filter : &xf;
  /* Filter depth increased again, adding dummy clause */
  depth++;

  if (get_subentries_visibility(op)) {
    struct berval bv_subentry = BER_BVC("subentry");
    sf.f_choice = LDAP_FILTER_EQUALITY;
    sf.f_ava = &aa_subentry;
    sf.f_av_desc = slap_schema.si_ad_objectClass;
    sf.f_av_value = bv_subentry;
    sf.f_next = nf.f_next;
    nf.f_next = &sf;
  }

  /* Allocate IDL stack, plus 1 more for former tmp */
  if (depth + 1 > wi->wi_search_stack_depth) {
    stack = ch_malloc((depth + 1) * WT_IDL_UM_SIZE * sizeof(ID));
  } else {
    stack = search_stack(op);
  }

  if (op->ors_deref & LDAP_DEREF_SEARCHING) {
    rc = search_aliases(op, rs, e, wc->session, ids, scopes, stack);
    if (WT_IDL_IS_ZERO(ids) && rc == LDAP_SUCCESS)
      rc = wt_dn2idl(op, wc, &e->e_nname, e, ids, stack);
  } else {
    rc = wt_dn2idl(op, wc, &e->e_nname, e, ids, stack);
  }

  if (rc == LDAP_SUCCESS) {
    rc = wt_filter_candidates(op, wc, &f, ids, stack, stack + WT_IDL_UM_SIZE);
  }

  if (depth + 1 > wi->wi_search_stack_depth) {
    ch_free(stack);
  }

  if (rc) {
    Debug(LDAP_DEBUG_TRACE, "wt_search_candidates: failed (rc=%d)\n", rc);

  } else {
    Debug(LDAP_DEBUG_TRACE, "wt_search_candidates: id=%ld first=%ld last=%ld\n",
          (long)ids[0], (long)WT_IDL_FIRST(ids), (long)WT_IDL_LAST(ids));
  }
  return 0;
}

static int parse_paged_cookie(Operation *op, SlapReply *rs) {
  int rc = LDAP_SUCCESS;
  PagedResultsState *ps = op->o_pagedresults_state;

  /* this function must be invoked only if the pagedResults
   * control has been detected, parsed and partially checked
   * by the frontend */
  assert(get_pagedresults(op) > SLAP_CONTROL_IGNORED);

  /* cookie decoding/checks deferred to backend... */
  if (ps->ps_cookieval.bv_len) {
    PagedResultsCookie reqcookie;
    if (ps->ps_cookieval.bv_len != sizeof(reqcookie)) {
      /* bad cookie */
      rs->sr_text = "paged results cookie is invalid";
      rc = LDAP_PROTOCOL_ERROR;
      goto done;
    }

    memcpy(&reqcookie, ps->ps_cookieval.bv_val, sizeof(reqcookie));

    if (reqcookie > ps->ps_cookie) {
      /* bad cookie */
      rs->sr_text = "paged results cookie is invalid";
      rc = LDAP_PROTOCOL_ERROR;
      goto done;

    } else if (reqcookie < ps->ps_cookie) {
      rs->sr_text = "paged results cookie is invalid or old";
      rc = LDAP_UNWILLING_TO_PERFORM;
      goto done;
    }

  } else {
    /* we're going to use ps_cookie */
    op->o_conn->c_pagedresults_state.ps_cookie = 0;
  }

done:;

  return rc;
}

static void send_paged_response(Operation *op, SlapReply *rs, ID *lastid,
                                int tentries) {
  LDAPControl *ctrls[2];
  BerElementBuffer berbuf;
  BerElement *ber = (BerElement *)&berbuf;
  PagedResultsCookie respcookie;
  struct berval cookie;

  Debug(LDAP_DEBUG_ARGS, "send_paged_response: lastid=0x%08lx nentries=%d\n",
        lastid ? *lastid : 0, rs->sr_nentries);

  ctrls[1] = NULL;

  ber_init2(ber, NULL, LBER_USE_DER);

  if (lastid) {
    respcookie = (PagedResultsCookie)(*lastid);
    cookie.bv_len = sizeof(respcookie);
    cookie.bv_val = (char *)&respcookie;

  } else {
    respcookie = (PagedResultsCookie)0;
    BER_BVSTR(&cookie, "");
  }

  op->o_conn->c_pagedresults_state.ps_cookie = respcookie;
  op->o_conn->c_pagedresults_state.ps_count =
      ((PagedResultsState *)op->o_pagedresults_state)->ps_count +
      rs->sr_nentries;

  /* return size of 0 -- no estimate */
  ber_printf(ber, "{iO}", 0, &cookie);

  ctrls[0] = op->o_tmpalloc(sizeof(LDAPControl), op->o_tmpmemctx);
  if (ber_flatten2(ber, &ctrls[0]->ldctl_value, 0) == -1) {
    goto done;
  }

  ctrls[0]->ldctl_oid = LDAP_CONTROL_PAGEDRESULTS;
  ctrls[0]->ldctl_iscritical = 0;

  slap_add_ctrls(op, rs, ctrls);
  rs->sr_err = LDAP_SUCCESS;
  send_ldap_result(op, rs);

done:
  (void)ber_free_buf(ber);
}

int wt_search(Operation *op, SlapReply *rs) {
  struct wt_info *wi = (struct wt_info *)op->o_bd->be_private;
  ID id, cursor;
  ID lastid = NOID;
  int manageDSAit;
  wt_ctx *wc;
  int rc = LDAP_OTHER;
  Entry *e = NULL;
  Entry *ae = NULL;
  Entry *base = NULL;
  slap_mask_t mask;
  time_t stoptime;

  ID candidates[WT_IDL_UM_SIZE];
  ID scopes[WT_IDL_DB_SIZE];
  int tentries = 0;
  unsigned nentries = 0;

  Debug(LDAP_DEBUG_ARGS, "==> wt_search: %s\n", op->o_req_dn.bv_val);

  manageDSAit = get_manageDSAit(op);

  wc = wt_ctx_get(op, wi);
  if (!wc) {
    Debug(LDAP_DEBUG_ANY, "wt_search: wt_ctx_get failed: %d\n", rc);
    send_ldap_error(op, rs, LDAP_OTHER, "internal error");
    return rc;
  }

  /* get entry */
  rc = wt_dn2entry(op->o_bd, wc, &op->o_req_ndn, &e);
  switch (rc) {
  case 0:
    break;
  case WT_NOTFOUND:
    rc = wt_dn2aentry(op->o_bd, wc, &op->o_req_ndn, &ae);
    break;
  default:
    /* TODO: error handling */
    Debug(LDAP_DEBUG_ANY, "wt_search: error at wt_dn2entry() rc=%d\n", rc);
    send_ldap_error(op, rs, LDAP_OTHER, "internal error");
    goto done;
  }

  if (op->ors_deref & LDAP_DEREF_FINDING) {
    /* not implement yet */
  }

  if (e == NULL) {
    if (ae) {
      struct berval matched_dn = BER_BVNULL;
      /* found ancestor entry */
      if (access_allowed(op, ae, slap_schema.si_ad_entry, NULL, ACL_DISCLOSE,
                         NULL)) {
        BerVarray erefs = NULL;
        ber_dupbv(&matched_dn, &ae->e_name);
        erefs = is_entry_referral(ae) ? get_entry_referrals(op, ae) : NULL;
        rs->sr_err = LDAP_REFERRAL;
        rs->sr_matched = matched_dn.bv_val;
        if (erefs) {
          rs->sr_ref = referral_rewrite(erefs, &matched_dn, &op->o_req_dn,
                                        op->oq_search.rs_scope);
          ber_bvarray_free(erefs);
        }
        Debug(LDAP_DEBUG_ARGS, "wt_search: ancestor is referral\n");
        rs->sr_flags = REP_MATCHED_MUSTBEFREED | REP_REF_MUSTBEFREED;
        send_ldap_result(op, rs);
        goto done;
      }
    }
    Debug(LDAP_DEBUG_ARGS, "wt_search: no such object %s\n",
          op->o_req_dn.bv_val);
    rs->sr_err = LDAP_NO_SUCH_OBJECT;
    send_ldap_result(op, rs);
    goto done;
  }

  /* NOTE: __NEW__ "search" access is required
   * on searchBase object */
  if (!access_allowed_mask(op, e, slap_schema.si_ad_entry, NULL, ACL_SEARCH,
                           NULL, &mask)) {
    if (!ACL_GRANT(mask, ACL_DISCLOSE)) {
      rs->sr_err = LDAP_NO_SUCH_OBJECT;
    } else {
      rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
    }

    send_ldap_result(op, rs);
    goto done;
  }

  if (!manageDSAit && is_entry_referral(e)) {
    struct berval matched_dn = BER_BVNULL;
    BerVarray erefs = NULL;
    ber_dupbv(&matched_dn, &e->e_name);
    erefs = get_entry_referrals(op, e);
    rs->sr_err = LDAP_REFERRAL;
    if (erefs) {
      rs->sr_ref = referral_rewrite(erefs, &matched_dn, &op->o_req_dn,
                                    op->oq_search.rs_scope);
      ber_bvarray_free(erefs);
      if (!rs->sr_ref) {
        rs->sr_text = "bad_referral object";
      }
    }
    Debug(LDAP_DEBUG_ARGS, "wt_search: entry is referral\n");
    rs->sr_matched = matched_dn.bv_val;
    send_ldap_result(op, rs);
    ber_bvarray_free(rs->sr_ref);
    rs->sr_ref = NULL;
    ber_memfree(matched_dn.bv_val);
    rs->sr_matched = NULL;
    goto done;
  }

  if (get_assert(op) &&
      (test_filter(op, e, get_assertion(op)) != LDAP_COMPARE_TRUE)) {
    rs->sr_err = LDAP_ASSERTION_FAILED;
    send_ldap_result(op, rs);
    goto done;
  }

  /* compute it anyway; root does not use it */
  stoptime = op->o_time + op->ors_tlimit;

  base = e;

  e = NULL;

  /* select candidates */
  if (op->oq_search.rs_scope == LDAP_SCOPE_BASE) {
    rs->sr_err = base_candidate(op->o_bd, base, candidates);
  } else {
    WT_IDL_ZERO(candidates);
    WT_IDL_ZERO(scopes);
    rc = search_candidates(op, rs, base, wc, candidates, scopes);
    switch (rc) {
    case 0:
    case WT_NOTFOUND:
      break;
    default:
      Debug(LDAP_DEBUG_ANY, "wt_search: error search_candidates\n");
      send_ldap_error(op, rs, LDAP_OTHER, "internal error");
      goto done;
    }
  }

  /* start cursor at beginning of candidates.
   */
  cursor = 0;

  if (candidates[0] == 0) {
    Debug(LDAP_DEBUG_TRACE, "wt_search: no candidates\n");
    goto nochange;
  }

  if (op->ors_limit && op->ors_limit->lms_s_unchecked != -1 &&
      WT_IDL_N(candidates) > (unsigned)op->ors_limit->lms_s_unchecked) {
    rs->sr_err = LDAP_ADMINLIMIT_EXCEEDED;
    send_ldap_result(op, rs);
    rs->sr_err = LDAP_SUCCESS;
    goto done;
  }

  if (op->ors_limit == NULL /* isroot == TRUE */ ||
      !op->ors_limit->lms_s_pr_hide) {
    tentries = WT_IDL_N(candidates);
  }

  if (get_pagedresults(op) > SLAP_CONTROL_IGNORED) {
    /* TODO: pageresult */
    PagedResultsState *ps = op->o_pagedresults_state;
    /* deferred cookie parsing */
    rs->sr_err = parse_paged_cookie(op, rs);
    if (rs->sr_err != LDAP_SUCCESS) {
      send_ldap_result(op, rs);
      goto done;
    }

    cursor = (ID)ps->ps_cookie;
    if (cursor && ps->ps_size == 0) {
      rs->sr_err = LDAP_SUCCESS;
      rs->sr_text = "search abandoned by pagedResult size=0";
      send_ldap_result(op, rs);
      goto done;
    }
    id = wt_idl_first(candidates, &cursor);
    if (id == NOID) {
      Debug(LDAP_DEBUG_TRACE, "wt_search: no paged results candidates\n");
      send_paged_response(op, rs, &lastid, 0);

      rs->sr_err = LDAP_OTHER;
      goto done;
    }
    nentries = ps->ps_count;
    if (id == (ID)ps->ps_cookie)
      id = wt_idl_next(candidates, &cursor);
    goto loop_begin;
  }

  for (id = wt_idl_first(candidates, &cursor); id != NOID;
       id = wt_idl_next(candidates, &cursor)) {
    int scopeok;

  loop_begin:

    /* check for abandon */
    if (slap_get_op_abandon(op)) {
      rs->sr_err = SLAPD_ABANDON;
      send_ldap_result(op, rs);
      goto done;
    }

    /* mostly needed by internal searches,
     * e.g. related to syncrepl, for whom
     * abandon does not get set... */
    if (slapd_shutdown) {
      rs->sr_err = LDAP_UNAVAILABLE;
      send_ldap_disconnect(op, rs);
      goto done;
    }

    /* check time limit */
    if (op->ors_tlimit != SLAP_NO_LIMIT && ldap_time_steady() > stoptime) {
      rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
      rs->sr_ref = rs->sr_v2ref;
      send_ldap_result(op, rs);
      rs->sr_err = LDAP_SUCCESS;
      goto done;
    }

    nentries++;

    /*	fetch_entry_retry: */

    rc = wt_id2entry(op->o_bd, wc, id, &e);
    /* TODO: error handling */
    if (e == NULL) {
      /* TODO: */
      goto loop_continue;
    }
    if (is_entry_subentry(e)) {
      if (op->oq_search.rs_scope != LDAP_SCOPE_BASE) {
        if (!get_subentries_visibility(op)) {
          /* only subentries are visible */
          goto loop_continue;
        }

      } else if (get_subentries(op) && !get_subentries_visibility(op)) {
        /* only subentries are visible */
        goto loop_continue;
      }

    } else if (get_subentries_visibility(op)) {
      /* only subentries are visible */
      goto loop_continue;
    }

    scopeok = 0;
    switch (op->ors_scope) {
    case LDAP_SCOPE_BASE:
      /* This is always true, yes? */
      if (id == base->e_id)
        scopeok = 1;
      break;
    case LDAP_SCOPE_ONELEVEL:
      scopeok = 1;
      break;
    case LDAP_SCOPE_CHILDREN:
      if (id == base->e_id)
        break;
      /* Fall-thru */
    case LDAP_SCOPE_SUBTREE:
      /* TODO: check for range ids */
      scopeok = 1;
      break;
    }

    /* aliases were already dereferenced in candidate list */
    if (op->ors_deref & LDAP_DEREF_SEARCHING) {
      /* but if the search base is an alias, and we didn't
       * deref it when finding, return it.
       */
      if (is_entry_alias(e) && ((op->ors_deref & LDAP_DEREF_FINDING) ||
                                !bvmatch(&e->e_nname, &op->o_req_ndn))) {
        goto loop_continue;
      }
      /* TODO: alias handling */
    }

    /* Not in scope, ignore it */
    if (!scopeok) {
      Debug(LDAP_DEBUG_TRACE, "wt_search: %ld scope not okay\n", (long)id);
      goto loop_continue;
    }

    /*
     * if it's a referral, add it to the list of referrals. only do
     * this for non-base searches, and don't check the filter
     * explicitly here since it's only a candidate anyway.
     */
    if (!manageDSAit && op->oq_search.rs_scope != LDAP_SCOPE_BASE &&
        is_entry_referral(e)) {
      BerVarray erefs = get_entry_referrals(op, e);
      rs->sr_ref = referral_rewrite(
          erefs, &e->e_name, NULL,
          op->oq_search.rs_scope == LDAP_SCOPE_ONELEVEL ? LDAP_SCOPE_BASE
                                                        : LDAP_SCOPE_SUBTREE);
      rs->sr_entry = e;
      send_search_reference(op, rs);
      rs->sr_entry = NULL;
      ber_bvarray_free(rs->sr_ref);
      ber_bvarray_free(erefs);
      goto loop_continue;
    }

    if (!manageDSAit && is_entry_glue(e)) {
      goto loop_continue;
    }

    /* if it matches the filter and scope, send it */
    rs->sr_err = test_filter(op, e, op->oq_search.rs_filter);
    if (rs->sr_err == LDAP_COMPARE_TRUE) {
      /* check size limit */
      if (get_pagedresults(op) > SLAP_CONTROL_IGNORED) {
        if (rs->sr_nentries >=
            ((PagedResultsState *)op->o_pagedresults_state)->ps_size) {
          wt_entry_return(e);
          e = NULL;
          send_paged_response(op, rs, &lastid, tentries);
          goto done;
        }
        lastid = id;
      }

      if (e) {
        /* safe default */
        rs->sr_attrs = op->oq_search.rs_attrs;
        rs->sr_operational_attrs = NULL;
        rs->sr_ctrls = NULL;
        rs->sr_entry = e;
        RS_ASSERT(e->e_private != NULL);
        rs->sr_flags = REP_ENTRY_MUSTRELEASE;
        rs->sr_err = LDAP_SUCCESS;
        rs->sr_err = send_search_entry(op, rs);
        rs->sr_attrs = NULL;
        rs->sr_entry = NULL;
        e = NULL;

        switch (rs->sr_err) {
        case LDAP_SUCCESS: /* entry sent ok */
          break;
        default: /* entry not sent */
          break;
        case LDAP_BUSY:
          send_ldap_result(op, rs);
          goto done;
        case LDAP_UNAVAILABLE:
          rs->sr_err = LDAP_OTHER;
          goto done;
        case LDAP_SIZELIMIT_EXCEEDED:
          rs->sr_ref = rs->sr_v2ref;
          send_ldap_result(op, rs);
          rs->sr_err = LDAP_SUCCESS;
          goto done;
        }
      }
    } else {
      Debug(LDAP_DEBUG_TRACE, "wt_search: %ld does not match filter\n",
            (long)id);
    }

  loop_continue:
    if (e) {
      wt_entry_return(e);
      e = NULL;
    }
  }

nochange:
  rs->sr_ctrls = NULL;
  rs->sr_ref = rs->sr_v2ref;
  rs->sr_err = (rs->sr_v2ref == NULL) ? LDAP_SUCCESS : LDAP_REFERRAL;
  rs->sr_rspoid = NULL;
  if (get_pagedresults(op) > SLAP_CONTROL_IGNORED) {
    send_paged_response(op, rs, NULL, 0);
  } else {
    send_ldap_result(op, rs);
  }

  rs->sr_err = LDAP_SUCCESS;

done:

  if (base) {
    wt_entry_return(base);
  }

  if (e) {
    wt_entry_return(e);
  }

  if (ae) {
    wt_entry_return(ae);
  }

  return rs->sr_err;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
