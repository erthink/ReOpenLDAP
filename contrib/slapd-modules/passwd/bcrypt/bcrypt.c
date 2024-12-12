/*
 * Written by Wouter Clarie <wclarie at gmail.com>
 *
 * No copyright is claimed, and the software is hereby placed in the public
 * domain.  In case this attempt to disclaim copyright and place the software
 * in the public domain is deemed null and void, then the software is
 * Copyright (c) 2014 Wouter Clarie and it is hereby released to the general
 * public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * It is my intent that you should be able to use this on your system, as part
 * of a software package, or anywhere else to improve security, ensure
 * compatibility, or for any other purpose. I would appreciate it if you give
 * credit where it is due and keep your modifications in the public domain as
 * well, but I don't require that in order to let you place this code and any
 * modifications you make under a license of your choice.
 *
 * Please read the README file in this directory.
 */

#include "reldap.h"

#include "slap.h"
#include "lutil.h"
#include "ac/string.h"
#include "lber_pvt.h"
#include "lutil.h"

#include <stdlib.h>

#include "crypt_blowfish.h"

#ifdef SLAPD_BCRYPT_DEBUG
#include <stdio.h>
#define _DEBUG(args...) printf(args)
#else
#define _DEBUG(args...)
#endif

/* Always generate 'b' type hashes for new passwords to match
 * OpenBSD 5.5+ behaviour.
 * See first http://www.openwall.com/lists/announce/2011/07/17/1 and then
 * http://www.openwall.com/lists/announce/2014/08/31/1
 */
#define BCRYPT_DEFAULT_PREFIX "$2b"

/* Default work factor as currently used by the OpenBSD project for normal
 * accounts. Only used when no work factor is supplied in the slapd.conf
 * when loading the module. See README for more information.
 */
#define BCRYPT_DEFAULT_WORKFACTOR 8
#define BCRYPT_MIN_WORKFACTOR 4
#define BCRYPT_MAX_WORKFACTOR 32

#define BCRYPT_SALT_SIZE 16
#define BCRYPT_OUTPUT_SIZE 61

static int bcrypt_workfactor;
struct berval bcryptscheme = BER_BVC("{BCRYPT}");

static int hash_bcrypt(const struct berval *scheme, /* Scheme name to construct output */
                       const struct berval *passwd, /* Plaintext password to hash */
                       struct berval *hash,         /* Return value: schema + bcrypt hash */
                       const char **text)           /* Unused */
{
  _DEBUG("Entering hash_bcrypt\n");

  char bcrypthash[BCRYPT_OUTPUT_SIZE];
  char saltinput[BCRYPT_SALT_SIZE];
  char settingstring[sizeof(BCRYPT_DEFAULT_PREFIX) + 1 + BCRYPT_SALT_SIZE + 1];

  struct berval salt;
  struct berval digest;

  salt.bv_val = saltinput;
  salt.bv_len = sizeof(saltinput);

  _DEBUG("Obtaining entropy for bcrypt: %d bytes\n", (int)salt.bv_len);
  if (lutil_entropy((unsigned char *)salt.bv_val, salt.bv_len) < 0) {
    _DEBUG("Error: cannot get entropy\n");
    return LUTIL_PASSWD_ERR;
  }

  _DEBUG("Generating setting string and salt\n");
  if (_crypt_gensalt_blowfish_rn(BCRYPT_DEFAULT_PREFIX, bcrypt_workfactor, saltinput, BCRYPT_SALT_SIZE, settingstring,
                                 BCRYPT_OUTPUT_SIZE) == NULL) {
    _DEBUG("Error: _crypt_gensalt_blowfish_rn returned NULL\n");
    return LUTIL_PASSWD_ERR;
  }
  _DEBUG("Setting string: \"%s\"\n", settingstring);

  char *userpassword = passwd->bv_val;
  _DEBUG("Hashing password \"%s\" with settingstring \"%s\"\n", userpassword, settingstring);
  if (_crypt_blowfish_rn(userpassword, settingstring, bcrypthash, BCRYPT_OUTPUT_SIZE) == NULL)
    return LUTIL_PASSWD_ERR;

  _DEBUG("bcrypt hash created: \"%s\"\n", bcrypthash);

  digest.bv_len = scheme->bv_len + sizeof(bcrypthash);
  digest.bv_val = (char *)ber_memalloc(digest.bv_len + 1);

  if (digest.bv_val == NULL) {
    return LUTIL_PASSWD_ERR;
  }

  /* No need to base64 encode, as crypt_blowfish already does that */
  memcpy(digest.bv_val, scheme->bv_val, scheme->bv_len);
  memcpy(&digest.bv_val[scheme->bv_len], bcrypthash, sizeof(bcrypthash));

  digest.bv_val[digest.bv_len] = '\0';
  *hash = digest;

  return LUTIL_PASSWD_OK;
}

static int chk_bcrypt(const struct berval *scheme, /* Scheme of hashed reference password */
                      const struct berval *passwd, /* Hashed password to check against */
                      const struct berval *cred,   /* User-supplied password to check */
                      const char **text)           /* Unused */
{
  _DEBUG("Entering chk_bcrypt\n");
  char computedhash[BCRYPT_OUTPUT_SIZE];
  int rc;

  if (passwd->bv_val == NULL) {
    _DEBUG("Error: Stored hash is NULL\n");
    return LUTIL_PASSWD_ERR;
  }

  _DEBUG("Supplied hash: \"%s\"\n", (char *)passwd->bv_val);

  if (passwd->bv_len > BCRYPT_OUTPUT_SIZE) {
    _DEBUG("Error: Stored hash is too large. Size = %d\n", (int)passwd->bv_len);
    return LUTIL_PASSWD_ERR;
  }

  _DEBUG("Hashing provided credentials: \"%s\"\n", (char *)cred->bv_val);
  /* No need to base64 decode, as crypt_blowfish already does that */
  if (_crypt_blowfish_rn((char *)cred->bv_val, (char *)passwd->bv_val, computedhash, BCRYPT_OUTPUT_SIZE) == NULL) {
    _DEBUG("Error: _crypt_blowfish_rn returned NULL\n");
    return LUTIL_PASSWD_ERR;
  }
  _DEBUG("Resulting hash: \"%s\"\n", computedhash);

  _DEBUG("Comparing newly created hash with supplied hash: ");
  rc = memcmp((char *)passwd->bv_val, computedhash, BCRYPT_OUTPUT_SIZE);
  if (!rc) {
    _DEBUG("match\n");
    return LUTIL_PASSWD_OK;
  }

  _DEBUG("no match\n");
  return LUTIL_PASSWD_ERR;
}

SLAP_MODULE_ENTRY(pwbcrypt, modinit)(int argc, char *argv[]) {
  _DEBUG("Loading bcrypt password module\n");

  int result = 0;

  /* Work factor can be provided in the moduleload statement in slapd.conf. */
  if (argc > 0) {
    _DEBUG("Work factor argument provided, trying to use that\n");
    int work = atoi(argv[0]);
    if (work && work >= BCRYPT_MIN_WORKFACTOR && work <= BCRYPT_MAX_WORKFACTOR) {
      _DEBUG("Using configuration-supplied work factor %d\n", work);
      bcrypt_workfactor = work;

    } else {
      _DEBUG("Invalid work factor. Using default work factor %d\n", BCRYPT_DEFAULT_WORKFACTOR);
      bcrypt_workfactor = BCRYPT_DEFAULT_WORKFACTOR;
    }
  } else {
    _DEBUG("No arguments provided. Using default work factor %d\n", BCRYPT_DEFAULT_WORKFACTOR);
    bcrypt_workfactor = BCRYPT_DEFAULT_WORKFACTOR;
  }

  result = lutil_passwd_add(&bcryptscheme, chk_bcrypt, hash_bcrypt);
  _DEBUG("pw-bcrypt: Initialized with work factor %d\n", bcrypt_workfactor);

  return result;
}
