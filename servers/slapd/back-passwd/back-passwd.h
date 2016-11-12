/* $ReOpenLDAP$ */
/* Copyright 1990-2016 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#ifndef _BACK_PASSWD_H
#define _BACK_PASSWD_H

#include "proto-passwd.h"

LDAP_BEGIN_DECL

extern ldap_pvt_thread_mutex_t passwd_mutex;

extern BI_destroy	passwd_back_destroy;

extern BI_op_search	passwd_back_search;

LDAP_END_DECL

#endif /* _BACK_PASSWD_H */
