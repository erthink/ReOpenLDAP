#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "config.h"

typedef struct mobiuniquemember_private_s {
  struct berval *dn;
  int count;
} mobiuniquemember_private;

/* Get DN of entry from reply - store it into private structure */
static int mobiuniquemember_getdn_cb(Operation *op, SlapReply *rs) {

  mobiuniquemember_private *pd;

  /* because you never know */
  if (!op || !rs)
    return (0);

  /* Only search entries are interesting */
  if (rs->sr_type != REP_SEARCH)
    return (0);

  pd = op->o_callback->sc_private;
  pd->dn = ber_dupbv(pd->dn, &rs->sr_entry->e_nname);
  pd->count++;

  return (0);
}

/* get dn based on passed filter */
static int mobiuniquemember_getdn(Operation *op, struct berval *filterstr,
                                  struct berval *dn) {

  Operation op2 = *op;
  SlapReply rs2 = {REP_RESULT};
  slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
  slap_callback cb = {NULL, NULL, NULL, NULL};
  int rc = 0;
  struct berval base;

  BER_BVSTR(&base, "o=mobistar.be");

  mobiuniquemember_private pd = {NULL, 0};

  /* Set new filter */
  op2.ors_filter = str2filter_x(&op2, filterstr->bv_val);
  op2.ors_filterstr = *filterstr;

  /* Set callbacks & private info */
  cb.sc_response = (slap_response *)mobiuniquemember_getdn_cb;
  cb.sc_private = &pd;
  op2.o_callback = &cb;

  /* Set search type, scope, base dn */
  op2.o_tag = LDAP_REQ_SEARCH;
  op2.o_dn = op->o_bd->be_rootdn;
  op2.o_ndn = op->o_bd->be_rootndn;
  op2.ors_scope = LDAP_SCOPE_SUBTREE;
  op2.ors_deref = LDAP_DEREF_NEVER;
  op2.ors_limit = NULL;
  op2.ors_slimit = SLAP_NO_LIMIT;
  op2.ors_tlimit = SLAP_NO_LIMIT;
  op2.ors_attrs = slap_anlist_no_attrs;
  op2.ors_attrsonly = 0;

  op2.o_req_dn = base;
  op2.o_req_ndn = base;

  /* Send query */
  op2.o_bd = on->on_info->oi_origdb;
  rc = op2.o_bd->be_search(&op2, &rs2);
  filter_free_x(&op2, op2.ors_filter, 1);

  if (rc != LDAP_SUCCESS && rc != LDAP_NO_SUCH_OBJECT) {
    op->o_bd->bd_info = (BackendInfo *)on->on_info;
    send_ldap_error(op, &rs2, rc, "mobiuniquemember_getdn failed");
    return (0);
  }

  if (pd.count > 0)
    dn = ber_dupbv(dn, pd.dn);
  rc = pd.count;

  return (rc);
}

void bvtrim(struct berval *bv) {

  char *i, *j, *next, prev = 0;

  if (bv == NULL)
    return;

  j = bv->bv_val;
  i = bv->bv_val;

  Debug(LDAP_DEBUG_TRACE, "==> bvtrim: %s\n", bv->bv_val, 0, 0);

  /* skip leading ws */
  while ((*i) && (*i == 0x20)) {
    i++;
  };

  /* remove intermediate withespaces upon condition */
  next = i;
  while ((*i)) {
    next++;
    if ((*i == 0x20) && ((prev == ',') || (prev == '=') || (prev == 0x20) ||
                         (*next == ',') || (*next == '=') || (*next == 0x20))) {
      i++;
    } else {
      *j = *i;
      prev = *i;
      i++;
      j++;
    }
  }
  *j = *i;

  /* remove all after comma */
  i = bv->bv_val;
  while ((*i) && (*i != ',')) {
    i++;
  };
  if (*i == ',') {
    *i = 0;
  }

  bv->bv_len = strlen(bv->bv_val);
}

static int mobiuniquemember_op_search(Operation *op, SlapReply *rs) {
  // slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
  // mobiuniquemember_t *ao = (mobiuniquemember_t *)on->on_bi.bi_private;

  struct berval filterstr;

  filterstr = op->ors_filterstr;

  /* Check for objectclas attribute */
  Filter *filter, *newfilter, **fref;
  AttributeAssertion *aa;
  AttributeDescription *ad;
  struct berval at;
  struct berval av;
  struct berval bv_prefix, bv_um;
  struct berval *bvsub = NULL;
  struct berval userDN;
  struct berval newfilterstr;
  int savelen = 0;

  BER_BVSTR(&bv_um, "uniqueMember");
  BER_BVSTR(&bv_prefix, "uid=");

  /* parse all filter to get substring undefined on uniqueMember */
  fref = &op->ors_filter;
  for (filter = op->ors_filter; filter != NULL;) {

    switch (filter->f_choice) {
    case LDAP_FILTER_OR:
    case LDAP_FILTER_AND:
    case LDAP_FILTER_NOT:
      fref = &filter->f_un.f_un_complex;
      filter = filter->f_un.f_un_complex;
      break;

    case LDAP_FILTER_SUBSTRINGS | SLAPD_FILTER_UNDEFINED:
      aa = filter->f_un.f_un_ava;
      ad = aa->aa_desc;
      at = ad->ad_cname;

      /* next if not uniquemember */
      if (ber_bvstrcasecmp(&at, &bv_um) != 0) {
        fref = &filter->f_next;
        filter = filter->f_next;
        break;
      }

      /* check if first part of value is 'uid=' */
      bvsub = ber_bvdup(&filter->f_sub_initial);
      if (bvsub == NULL) {
        Debug(LDAP_DEBUG_TRACE, "mobiuniquemember_op_search - bvdup error\n", 0,
              0, 0);
        fref = &filter->f_next;
        filter = filter->f_next;
        break;
      }

      bvtrim(bvsub);
      savelen = bvsub->bv_len;
      if (bvsub->bv_len > 4) {
        bvsub->bv_len = 4;
      }

      /* next if not 'uid=' */
      if (ber_bvstrcasecmp(bvsub, &bv_prefix) != 0) {
        ber_bvfree(bvsub);
        fref = &filter->f_next;
        filter = filter->f_next;
        break;
      }

      bvsub->bv_len = savelen;

      av = filter->f_sub_initial;
      Debug(LDAP_DEBUG_TRACE, "==> substring (%s=%s)\n", at.bv_val, av.bv_val,
            0);

      /* replace with correct value */
      if (mobiuniquemember_getdn(op, bvsub, &userDN) < 1) {
        ber_bvfree(bvsub);
        fref = &filter->f_next;
        filter = filter->f_next;
        break;
      }

      Debug(LDAP_DEBUG_TRACE, "mobiuniquemember: userdn = %s)\n", userDN.bv_val,
            0, 0);

      newfilterstr.bv_val =
          malloc(sizeof(char) * (userDN.bv_len + bv_um.bv_len + 2));

      /* copy "uniqueMember=" in newfilterstr */
      memcpy(newfilterstr.bv_val, bv_um.bv_val, bv_um.bv_len);
      newfilterstr.bv_val[bv_um.bv_len] = '=';
      memcpy(&newfilterstr.bv_val[bv_um.bv_len + 1], userDN.bv_val,
             userDN.bv_len + 1);
      newfilterstr.bv_val[userDN.bv_len + bv_um.bv_len + 2] = 0x00;
      newfilterstr.bv_len = userDN.bv_len + bv_um.bv_len + 1;
      newfilter = str2filter_x(op, newfilterstr.bv_val);

      ber_memfree(newfilterstr.bv_val);

      newfilter->f_next = filter->f_next;
      *fref = newfilter;
      fref = &filter->f_next;
      filter_free(filter);
      filter = newfilter->f_next;
      break;

    default:
      fref = &filter->f_next;
      filter = filter->f_next;
    }
  }

  return SLAP_CB_CONTINUE;
}

/* Overlay removal function */
static int mobiuniquemember_db_destroy(BackendDB *be, ConfigReply *cr) {

  slap_overinst *on = (slap_overinst *)be->bd_info;
  mobiuniquemember_private *ao =
      (mobiuniquemember_private *)on->on_bi.bi_private;

  if (ao != NULL) {
    ch_free(ao);
    on->on_bi.bi_private = NULL;
  }
  return 0;
}

/* Overlay registration and initialization */
static slap_overinst mobiuniquemember;

int mobiuniquemember_init() {
  mobiuniquemember.on_bi.bi_type = "mobiuniquemember";
  mobiuniquemember.on_bi.bi_db_destroy = mobiuniquemember_db_destroy;
  mobiuniquemember.on_bi.bi_op_search = mobiuniquemember_op_search;

  return overlay_register(&mobiuniquemember);
}

int init_module(int argc, char *argv[]) { return mobiuniquemember_init(); }
