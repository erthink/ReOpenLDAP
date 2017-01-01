/* $ReOpenLDAP$ */
/* Copyright 1992-2017 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

/* File locking methods
 *
 * lutil_lockf() will block until an exclusive lock is acquired.
 */

#ifndef _LUTIL_LOCKF_H_
#define _LUTIL_LOCKF_H_

LDAP_BEGIN_DECL

LDAP_LUTIL_F( int )
lutil_lockf LDAP_P(( int fd ));

LDAP_LUTIL_F( int )
lutil_unlockf LDAP_P(( int fd ));

LDAP_END_DECL

#endif /* _LUTIL_LOCKF_H_ */
