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

#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/localize.h>
#include <ac/errno.h>

#include "ldap-int.h"
#include "ldap_log.h"

/*
 *	BindRequest ::= SEQUENCE {
 *		version		INTEGER,
 *		name		DistinguishedName,	 -- who
 *		authentication	CHOICE {
 *			simple		[0] OCTET STRING -- passwd
 *			krbv42ldap	[1] OCTET STRING -- OBSOLETE
 *			krbv42dsa	[2] OCTET STRING -- OBSOLETE
 *			sasl		[3] SaslCredentials	-- LDAPv3
 *		}
 *	}
 *
 *	BindResponse ::= SEQUENCE {
 *		COMPONENTS OF LDAPResult,
 *		serverSaslCreds		OCTET STRING OPTIONAL -- LDAPv3
 *	}
 *
 * (Source: RFC 2251)
 */

/*
 * ldap_bind - bind to the ldap server (and X.500).  The dn and password
 * of the entry to which to bind are supplied, along with the authentication
 * method to use.  The msgid of the bind request is returned on success,
 * -1 if there's trouble.  ldap_result() should be called to find out the
 * outcome of the bind request.
 *
 * Example:
 *	ldap_bind( ld, "cn=manager, o=university of michigan, c=us", "secret",
 *	    LDAP_AUTH_SIMPLE )
 */

int ldap_bind(LDAP *ld, const char *dn, const char *passwd, int authmethod) {
  Debug(LDAP_DEBUG_TRACE, "ldap_bind\n");

  switch (authmethod) {
  case LDAP_AUTH_SIMPLE:
    return (ldap_simple_bind(ld, dn, passwd));

#ifdef HAVE_GSSAPI
  case LDAP_AUTH_NEGOTIATE:
    return (ldap_gssapi_bind_s(ld, dn, passwd));
#endif

  case LDAP_AUTH_SASL:
    /* user must use ldap_sasl_bind */
    /* FALL-THRU */

  default:
    ld->ld_errno = LDAP_AUTH_UNKNOWN;
    return (-1);
  }
}

/*
 * ldap_bind_s - bind to the ldap server (and X.500).  The dn and password
 * of the entry to which to bind are supplied, along with the authentication
 * method to use.  This routine just calls whichever bind routine is
 * appropriate and returns the result of the bind (e.g. LDAP_SUCCESS or
 * some other error indication).
 *
 * Examples:
 *	ldap_bind_s( ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret", LDAP_AUTH_SIMPLE )
 *	ldap_bind_s( ld, "cn=manager, o=university of michigan, c=us",
 *	    NULL, LDAP_AUTH_KRBV4 )
 */
int ldap_bind_s(LDAP *ld, const char *dn, const char *passwd, int authmethod) {
  Debug(LDAP_DEBUG_TRACE, "ldap_bind_s\n");

  switch (authmethod) {
  case LDAP_AUTH_SIMPLE:
    return (ldap_simple_bind_s(ld, dn, passwd));

#ifdef HAVE_GSSAPI
  case LDAP_AUTH_NEGOTIATE:
    return (ldap_gssapi_bind_s(ld, dn, passwd));
#endif

  case LDAP_AUTH_SASL:
    /* user must use ldap_sasl_bind */
    /* FALL-THRU */

  default:
    return (ld->ld_errno = LDAP_AUTH_UNKNOWN);
  }
}

/*
 * ldap_open - initialize and connect to an ldap server.  A magic cookie to
 * be used for future communication is returned on success, NULL on failure.
 * "host" may be a space-separated list of hosts or IP addresses
 *
 * Example:
 *	LDAP	*ld;
 *	ld = ldap_open( hostname, port );
 */

LDAP *ldap_open(const char *host, int port) {
  int rc;
  LDAP *ld;

  Debug(LDAP_DEBUG_TRACE, "ldap_open(%s, %d)\n", host, port);

  ld = ldap_init(host, port);
  if (ld == NULL) {
    return (NULL);
  }

  LDAP_MUTEX_LOCK(&ld->ld_conn_mutex);
  rc = ldap_open_defconn(ld);
  LDAP_MUTEX_UNLOCK(&ld->ld_conn_mutex);

  if (rc < 0) {
    ldap_ld_free(ld, 0, NULL, NULL);
    ld = NULL;
  }

  Debug(LDAP_DEBUG_TRACE, "ldap_open: %s\n",
        ld != NULL ? "succeeded" : "failed");

  return ld;
}

/* OLD U-Mich ber_init() */
void ber_init_w_nullc(BerElement *ber, int options) {
  ber_init2(ber, NULL, options);
}

int ber_flush(Sockbuf *sb, BerElement *ber, int freeit) {
  return ber_flush2(
      sb, ber, freeit ? LBER_FLUSH_FREE_ON_SUCCESS : LBER_FLUSH_FREE_NEVER);
}

BerElement *ber_alloc(void) /* deprecated */
{
  return ber_alloc_t(0);
}

BerElement *der_alloc(void) /* deprecated */
{
  return ber_alloc_t(LBER_USE_DER);
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_search - initiate an ldap search operation.
 *
 * Parameters:
 *
 *	ld		LDAP descriptor
 *	base		DN of the base object
 *	scope		the search scope - one of
 *				LDAP_SCOPE_BASE (baseObject),
 *			    LDAP_SCOPE_ONELEVEL (oneLevel),
 *				LDAP_SCOPE_SUBTREE (subtree), or
 *				LDAP_SCOPE_SUBORDINATE (children) -- OpenLDAP
 *extension filter		a string containing the search filter (e.g.,
 *"(|(cn=bob)(sn=bob))") attrs		list of attribute types to return for
 *matches attrsonly	1 => attributes only 0 => attributes and values
 *
 * Example:
 *	char	*attrs[] = { "mail", "title", 0 };
 *	msgid = ldap_search( ld, "dc=example,dc=com", LDAP_SCOPE_SUBTREE,
 *"cn~=bob", attrs, attrsonly );
 */
int ldap_search(LDAP *ld, const char *base, int scope, const char *filter,
                char **attrs, int attrsonly) {
  BerElement *ber;
  ber_int_t id;

  Debug(LDAP_DEBUG_TRACE, "ldap_search\n");

  assert(ld != NULL);
  assert(LDAP_VALID(ld));

  ber = ldap_build_search_req(ld, base, scope, filter, attrs, attrsonly, NULL,
                              NULL, -1, -1, -1, &id);

  if (ber == NULL) {
    return (-1);
  }

  /* send the message */
  return (ldap_send_initial_request(ld, LDAP_REQ_SEARCH, base, ber, id));
}

int ldap_search_st(LDAP *ld, const char *base, int scope, const char *filter,
                   char **attrs, int attrsonly, struct timeval *timeout,
                   LDAPMessage **res) {
  int msgid;

  *res = NULL;

  if ((msgid = ldap_search(ld, base, scope, filter, attrs, attrsonly)) == -1)
    return (ld->ld_errno);

  if (ldap_result(ld, msgid, LDAP_MSG_ALL, timeout, res) == -1 || !*res)
    return (ld->ld_errno);

  if (ld->ld_errno == LDAP_TIMEOUT) {
    (void)ldap_abandon_ext(ld, msgid, NULL, NULL);
    ld->ld_errno = LDAP_TIMEOUT;
    return (ld->ld_errno);
  }

  return (ldap_result2error(ld, *res, 0));
}

int ldap_search_s(LDAP *ld, const char *base, int scope, const char *filter,
                  char **attrs, int attrsonly, LDAPMessage **res) {
  int msgid;

  *res = NULL;

  if ((msgid = ldap_search(ld, base, scope, filter, attrs, attrsonly)) == -1)
    return (ld->ld_errno);

  if (ldap_result(ld, msgid, LDAP_MSG_ALL, (struct timeval *)NULL, res) == -1 ||
      !*res)
    return (ld->ld_errno);

  return (ldap_result2error(ld, *res, 0));
}

/*----------------------------------------------------------------------------*/

/*
 * Create a LDAPControl, optionally from ber - deprecated
 */
int ldap_create_control(const char *requestOID, BerElement *ber, int iscritical,
                        LDAPControl **ctrlp) {
  LDAPControl *ctrl;

  assert(requestOID != NULL);
  assert(ctrlp != NULL);

  ctrl = (LDAPControl *)LDAP_MALLOC(sizeof(LDAPControl));
  if (ctrl == NULL) {
    return LDAP_NO_MEMORY;
  }

  BER_BVZERO(&ctrl->ldctl_value);
  if (ber && (ber_flatten2(ber, &ctrl->ldctl_value, 1) == -1)) {
    LDAP_FREE(ctrl);
    return LDAP_NO_MEMORY;
  }

  ctrl->ldctl_oid = LDAP_STRDUP(requestOID);
  ctrl->ldctl_iscritical = iscritical;

  if (requestOID != NULL && ctrl->ldctl_oid == NULL) {
    ldap_control_free(ctrl);
    return LDAP_NO_MEMORY;
  }

  *ctrlp = ctrl;
  return LDAP_SUCCESS;
}

/*
 * Find a LDAPControl - deprecated
 */
LDAPControl *ldap_find_control(const char *oid, LDAPControl **ctrls) {
  if (ctrls == NULL || *ctrls == NULL) {
    return NULL;
  }

  for (; *ctrls != NULL; ctrls++) {
    if (strcmp((*ctrls)->ldctl_oid, oid) == 0) {
      return *ctrls;
    }
  }

  return NULL;
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_abandon - perform an ldap abandon operation. Parameters:
 *
 *	ld		LDAP descriptor
 *	msgid		The message id of the operation to abandon
 *
 * ldap_abandon returns 0 if everything went ok, -1 otherwise.
 *
 * Example:
 *	ldap_abandon( ld, msgid );
 */
int ldap_abandon(LDAP *ld, int msgid) {
  Debug(LDAP_DEBUG_TRACE, "ldap_abandon %d\n", msgid);
  return ldap_abandon_ext(ld, msgid, NULL, NULL) == LDAP_SUCCESS ? 0 : -1;
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_add - initiate an ldap add operation.  Parameters:
 *
 *	ld		LDAP descriptor
 *	dn		DN of the entry to add
 *	mods		List of attributes for the entry.  This is a null-
 *			terminated array of pointers to LDAPMod structures.
 *			only the type and values in the structures need be
 *			filled in.
 *
 * Example:
 *	LDAPMod	*attrs[] = {
 *			{ 0, "cn", { "babs jensen", "babs", 0 } },
 *			{ 0, "sn", { "jensen", 0 } },
 *			{ 0, "objectClass", { "person", 0 } },
 *			0
 *		}
 *	msgid = ldap_add( ld, dn, attrs );
 */

int ldap_add(LDAP *ld, const char *dn, LDAPMod **attrs) {
  int rc;
  int msgid = 0;

  rc = ldap_add_ext(ld, dn, attrs, NULL, NULL, &msgid);

  if (rc != LDAP_SUCCESS)
    return -1;

  return msgid;
}

int ldap_add_s(LDAP *ld, const char *dn, LDAPMod **attrs) {
  return ldap_add_ext_s(ld, dn, attrs, NULL, NULL);
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_simple_bind - bind to the ldap server (and X.500).  The dn and
 * password of the entry to which to bind are supplied.  The message id
 * of the request initiated is returned.
 *
 * Example:
 *	ldap_simple_bind( ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret" )
 */

int ldap_simple_bind(LDAP *ld, const char *dn, const char *passwd) {
  int rc;
  int msgid;
  struct berval cred;

  Debug(LDAP_DEBUG_TRACE, "ldap_simple_bind\n");

  assert(ld != NULL);
  assert(LDAP_VALID(ld));

  if (passwd != NULL) {
    cred.bv_val = (char *)passwd;
    cred.bv_len = strlen(passwd);
  } else {
    cred.bv_val = "";
    cred.bv_len = 0;
  }

  rc = ldap_sasl_bind(ld, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &msgid);

  return rc == LDAP_SUCCESS ? msgid : -1;
}

/*
 * ldap_simple_bind - bind to the ldap server (and X.500) using simple
 * authentication.  The dn and password of the entry to which to bind are
 * supplied.  LDAP_SUCCESS is returned upon success, the ldap error code
 * otherwise.
 *
 * Example:
 *	ldap_simple_bind_s( ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret" )
 */

int ldap_simple_bind_s(LDAP *ld, const char *dn, const char *passwd) {
  struct berval cred;

  Debug(LDAP_DEBUG_TRACE, "ldap_simple_bind_s\n");

  if (passwd != NULL) {
    cred.bv_val = (char *)passwd;
    cred.bv_len = strlen(passwd);
  } else {
    cred.bv_val = "";
    cred.bv_len = 0;
  }

  return ldap_sasl_bind_s(ld, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_compare_ext - perform an ldap extended compare operation.  The dn
 * of the entry to compare to and the attribute and value to compare (in
 * attr and value) are supplied.  The msgid of the response is returned.
 *
 * Example:
 *	msgid = ldap_compare( ld, "c=us@cn=bob", "userPassword", "secret" )
 */
int ldap_compare(LDAP *ld, const char *dn, const char *attr,
                 const char *value) {
  int msgid = 0;
  struct berval bvalue;

  assert(value != NULL);

  bvalue.bv_val = (char *)value;
  bvalue.bv_len = (value == NULL) ? 0 : strlen(value);

  return ldap_compare_ext(ld, dn, attr, &bvalue, NULL, NULL, &msgid) ==
                 LDAP_SUCCESS
             ? msgid
             : -1;
}

int ldap_compare_s(LDAP *ld, const char *dn, const char *attr,
                   const char *value) {
  struct berval bvalue;

  assert(value != NULL);

  bvalue.bv_val = (char *)value;
  bvalue.bv_len = (value == NULL) ? 0 : strlen(value);

  return ldap_compare_ext_s(ld, dn, attr, &bvalue, NULL, NULL);
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_delete - initiate an ldap (and X.500) delete operation. Parameters:
 *
 *	ld		LDAP descriptor
 *	dn		DN of the object to delete
 *
 * Example:
 *	msgid = ldap_delete( ld, dn );
 */
int ldap_delete(LDAP *ld, const char *dn) {
  int msgid = 0;

  /*
   * A delete request looks like this:
   *	DelRequet ::= DistinguishedName,
   */

  Debug(LDAP_DEBUG_TRACE, "ldap_delete\n");

  return ldap_delete_ext(ld, dn, NULL, NULL, &msgid) == LDAP_SUCCESS ? msgid
                                                                     : -1;
}

int ldap_delete_s(LDAP *ld, const char *dn) {
  return ldap_delete_ext_s(ld, dn, NULL, NULL);
}

/*----------------------------------------------------------------------------*/

/* deprecated */
void ldap_perror(LDAP *ld, const char *str) {
  int i;

  assert(ld != NULL);
  assert(LDAP_VALID(ld));
  assert(str != NULL);

  fprintf(stderr, "%s: %s (%d)\n", str ? str : "ldap_perror",
          ldap_err2string(ld->ld_errno), ld->ld_errno);

  if (ld->ld_matched != NULL && ld->ld_matched[0] != '\0') {
    fprintf(stderr, _("\tmatched DN: %s\n"), ld->ld_matched);
  }

  if (ld->ld_error != NULL && ld->ld_error[0] != '\0') {
    fprintf(stderr, _("\tadditional info: %s\n"), ld->ld_error);
  }

  if (ld->ld_referrals != NULL && ld->ld_referrals[0] != NULL) {
    fprintf(stderr, _("\treferrals:\n"));
    for (i = 0; ld->ld_referrals[i]; i++) {
      fprintf(stderr, _("\t\t%s\n"), ld->ld_referrals[i]);
    }
  }

  fflush(stderr);
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_modify - initiate an ldap modify operation.
 *
 * Parameters:
 *
 *	ld		LDAP descriptor
 *	dn		DN of the object to modify
 *	mods		List of modifications to make.  This is null-terminated
 *			array of struct ldapmod's, specifying the modifications
 *			to perform.
 *
 * Example:
 *	LDAPMod	*mods[] = {
 *			{ LDAP_MOD_ADD, "cn", { "babs jensen", "babs", 0 } },
 *			{ LDAP_MOD_REPLACE, "sn", { "babs jensen", "babs", 0 }
 *}, { LDAP_MOD_DELETE, "ou", 0 }, { LDAP_MOD_INCREMENT, "uidNumber, { "1", 0 }
 *}
 *			0
 *		}
 *	msgid = ldap_modify( ld, dn, mods );
 */
int ldap_modify(LDAP *ld, const char *dn, LDAPMod **mods) {
  int rc, msgid;

  Debug(LDAP_DEBUG_TRACE, "ldap_modify\n");

  rc = ldap_modify_ext(ld, dn, mods, NULL, NULL, &msgid);

  if (rc != LDAP_SUCCESS)
    return -1;

  return msgid;
}

int ldap_modify_s(LDAP *ld, const char *dn, LDAPMod **mods) {
  return ldap_modify_ext_s(ld, dn, mods, NULL, NULL);
}

/*----------------------------------------------------------------------------*/

/*
 * ldap_rename2 - initiate an ldap (and X.500) modifyDN operation. Parameters:
 *	(LDAP V3 MODIFYDN REQUEST)
 *	ld		LDAP descriptor
 *	dn		DN of the object to modify
 *	newrdn		RDN to give the object
 *	deleteoldrdn	nonzero means to delete old rdn values from the entry
 *	newSuperior	DN of the new parent if applicable
 *
 * ldap_rename2 uses a U-Mich Style API.  It returns the msgid.
 */

int ldap_rename2(LDAP *ld, const char *dn, const char *newrdn,
                 const char *newSuperior, int deleteoldrdn) {
  int msgid = -1;
  int rc;

  Debug(LDAP_DEBUG_TRACE, "ldap_rename2\n");

  rc = ldap_rename(ld, dn, newrdn, newSuperior, deleteoldrdn, NULL, NULL,
                   &msgid);

  return rc == LDAP_SUCCESS ? msgid : -1;
}

int ldap_rename2_s(LDAP *ld, const char *dn, const char *newrdn,
                   const char *newSuperior, int deleteoldrdn) {
  return ldap_rename_s(ld, dn, newrdn, newSuperior, deleteoldrdn, NULL, NULL);
}

int ldap_modrdn(LDAP *ld, const char *dn, const char *newrdn) {
  int msgid = -1;
  int rc;

  Debug(LDAP_DEBUG_TRACE, "ldap_modrdn\n");

  /* ldap_rename2( ld, dn, newrdn, NULL, 1 ) */
  rc = ldap_rename(ld, dn, newrdn, NULL, 1, NULL, NULL, &msgid);

  return rc == LDAP_SUCCESS ? msgid : -1;
}

/*
 * ldap_modrdn2 - initiate an ldap modifyRDN operation. Parameters:
 *
 *	ld		LDAP descriptor
 *	dn		DN of the object to modify
 *	newrdn		RDN to give the object
 *	deleteoldrdn	nonzero means to delete old rdn values from the entry
 *
 * Example:
 *	msgid = ldap_modrdn( ld, dn, newrdn );
 */
int ldap_modrdn2(LDAP *ld, const char *dn, const char *newrdn,
                 int deleteoldrdn) {
  int msgid = -1;
  int rc;

  Debug(LDAP_DEBUG_TRACE, "ldap_modrdn2\n");

  /* ldap_rename2( ld, dn, newrdn, NULL, deleteoldrdn ); */
  rc = ldap_rename(ld, dn, newrdn, NULL, deleteoldrdn, NULL, NULL, &msgid);

  return rc == LDAP_SUCCESS ? msgid : -1;
}

int ldap_modrdn2_s(LDAP *ld, const char *dn, const char *newrdn,
                   int deleteoldrdn) {
  return ldap_rename_s(ld, dn, newrdn, NULL, deleteoldrdn, NULL, NULL);
}

int ldap_modrdn_s(LDAP *ld, const char *dn, const char *newrdn) {
  return ldap_rename_s(ld, dn, newrdn, NULL, 1, NULL, NULL);
}

/*----------------------------------------------------------------------------*/

/*
 * RFC 1823 ldap_dn2ufn
 */
char *ldap_dn2ufn(const char *dn) {
  char *out = NULL;

  Debug(LDAP_DEBUG_TRACE, "ldap_dn2ufn\n");

  (void)ldap_dn_normalize(dn, LDAP_DN_FORMAT_LDAP, &out, LDAP_DN_FORMAT_UFN);

  return (out);
}

char *ldap_dn2dcedn(const char *dn) {
  char *out = NULL;

  Debug(LDAP_DEBUG_TRACE, "ldap_dn2dcedn\n");

  (void)ldap_dn_normalize(dn, LDAP_DN_FORMAT_LDAP, &out, LDAP_DN_FORMAT_DCE);

  return (out);
}

char *ldap_dcedn2dn(const char *dce) {
  char *out = NULL;

  Debug(LDAP_DEBUG_TRACE, "ldap_dcedn2dn\n");

  (void)ldap_dn_normalize(dce, LDAP_DN_FORMAT_DCE, &out, LDAP_DN_FORMAT_LDAPV3);

  return (out);
}

char *ldap_dn2ad_canonical(const char *dn) {
  char *out = NULL;

  Debug(LDAP_DEBUG_TRACE, "ldap_dn2ad_canonical\n");

  (void)ldap_dn_normalize(dn, LDAP_DN_FORMAT_LDAP, &out,
                          LDAP_DN_FORMAT_AD_CANONICAL);

  return (out);
}

/*----------------------------------------------------------------------------*/

int ldap_unbind(LDAP *ld) {
  Debug(LDAP_DEBUG_TRACE, "ldap_unbind\n");

  return (ldap_unbind_ext(ld, NULL, NULL));
}

int ldap_unbind_s(LDAP *ld) { return (ldap_unbind_ext(ld, NULL, NULL)); }

/* ---------------------------------------------------------------------------
        ldap_parse_page_control

        Decode a page control.

        ld          (IN) An LDAP session handle
        ctrls       (IN) Response controls
        count      (OUT) The number of entries in the page.
        cookie     (OUT) Opaque cookie.  Use ldap_memfree() to
                                         free the bv_val member of this
   structure.

   ---------------------------------------------------------------------------*/

int ldap_parse_page_control(LDAP *ld, LDAPControl **ctrls, ber_int_t *countp,
                            struct berval **cookiep) {
  LDAPControl *c;
  struct berval cookie;

  if (cookiep == NULL) {
    ld->ld_errno = LDAP_PARAM_ERROR;
    return ld->ld_errno;
  }

  if (ctrls == NULL) {
    ld->ld_errno = LDAP_CONTROL_NOT_FOUND;
    return ld->ld_errno;
  }

  c = ldap_control_find(LDAP_CONTROL_PAGEDRESULTS, ctrls, NULL);
  if (c == NULL) {
    /* No page control was found. */
    ld->ld_errno = LDAP_CONTROL_NOT_FOUND;
    return ld->ld_errno;
  }

  ld->ld_errno = ldap_parse_pageresponse_control(ld, c, countp, &cookie);
  if (ld->ld_errno == LDAP_SUCCESS) {
    *cookiep = LDAP_MALLOC(sizeof(struct berval));
    if (*cookiep == NULL) {
      ld->ld_errno = LDAP_NO_MEMORY;
    } else {
      **cookiep = cookie;
    }
  }

  return ld->ld_errno;
}
