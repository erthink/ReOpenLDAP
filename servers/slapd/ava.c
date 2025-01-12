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

/* ava.c - routines for dealing with attribute value assertions */

#include "reldap.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

#ifdef LDAP_COMP_MATCH
#include "component.h"
#endif

void ava_free(Operation *op, AttributeAssertion *ava, int freeit) {
#ifdef LDAP_COMP_MATCH
  if (ava->aa_cf && ava->aa_cf->cf_ca->ca_comp_data.cd_mem_op)
    nibble_mem_free(ava->aa_cf->cf_ca->ca_comp_data.cd_mem_op);
#endif
  op->o_tmpfree(ava->aa_value.bv_val, op->o_tmpmemctx);
  if (ava->aa_desc->ad_flags & SLAP_DESC_TEMPORARY)
    op->o_tmpfree(ava->aa_desc, op->o_tmpmemctx);
  if (freeit)
    op->o_tmpfree((char *)ava, op->o_tmpmemctx);
}

int get_ava(Operation *op, BerElement *ber, Filter *f, unsigned usage, const char **text) {
  int rc;
  ber_tag_t rtag;
  struct berval type, value;
  AttributeAssertion *aa;
#ifdef LDAP_COMP_MATCH
  AttributeAliasing *a_alias = NULL;
#endif

  rtag = ber_scanf(ber, "{mm}", &type, &value);

  if (rtag == LBER_ERROR) {
    Debug(LDAP_DEBUG_ANY, "  get_ava ber_scanf\n");
    *text = "Error decoding attribute value assertion";
    return SLAPD_DISCONNECT;
  }

  aa = op->o_tmpalloc(sizeof(AttributeAssertion), op->o_tmpmemctx);
  aa->aa_desc = NULL;
  aa->aa_value.bv_val = NULL;
#ifdef LDAP_COMP_MATCH
  aa->aa_cf = NULL;
#endif

  rc = slap_bv2ad(&type, &aa->aa_desc, text);

  if (rc != LDAP_SUCCESS) {
    f->f_choice |= SLAPD_FILTER_UNDEFINED;
    *text = NULL;
    rc = slap_bv2undef_ad(&type, &aa->aa_desc, text, SLAP_AD_PROXIED | SLAP_AD_NOINSERT);

    if (rc != LDAP_SUCCESS) {
      Debug(LDAP_DEBUG_FILTER, "get_ava: unknown attributeType %s\n", type.bv_val);
      aa->aa_desc = slap_bv2tmp_ad(&type, op->o_tmpmemctx);
      ber_dupbv_x(&aa->aa_value, &value, op->o_tmpmemctx);
      f->f_ava = aa;
      return LDAP_SUCCESS;
    }
  }

  rc = asserted_value_validate_normalize(aa->aa_desc, ad_mr(aa->aa_desc, usage), usage, &value, &aa->aa_value, text,
                                         op->o_tmpmemctx);

  if (rc != LDAP_SUCCESS) {
    f->f_choice |= SLAPD_FILTER_UNDEFINED;
    Debug(LDAP_DEBUG_FILTER, "get_ava: illegal value for attributeType %s\n", type.bv_val);
    ber_dupbv_x(&aa->aa_value, &value, op->o_tmpmemctx);
    *text = NULL;
    rc = LDAP_SUCCESS;
  }

#ifdef LDAP_COMP_MATCH
  if (is_aliased_attribute) {
    a_alias = is_aliased_attribute(aa->aa_desc);
    if (a_alias) {
      rc = get_aliased_filter_aa(op, aa, a_alias, text);
      if (rc != LDAP_SUCCESS) {
        Debug(LDAP_DEBUG_FILTER, "get_ava: Invalid Attribute Aliasing\n");
        return rc;
      }
    }
  }
#endif
  f->f_ava = aa;
  return LDAP_SUCCESS;
}
