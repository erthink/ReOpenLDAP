/* avl.h - avl tree definitions */
/* $ReOpenLDAP$ */
/* Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
 * Copyright (c) 2015,2016 Peter-Service R&D LLC <http://billing.ru/>.
 *
 * This file is part of ReOpenLDAP.
 *
 * ReOpenLDAP is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReOpenLDAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1993 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */


#ifndef _AVL
#define _AVL

#include <ldap_cdefs.h>

/*
 * this structure represents a generic avl tree node.
 */

LDAP_BEGIN_DECL

typedef struct avlnode Avlnode;

struct avlnode {
	void*		avl_data;
	struct avlnode	*avl_link[2];
	char		avl_bits[2];
	signed char		avl_bf;
};

#define avl_left	avl_link[0]
#define avl_right	avl_link[1]
#define avl_lbit	avl_bits[0]
#define avl_rbit	avl_bits[1]

#ifdef AVL_INTERNAL

#define NULLAVL	((Avlnode *) NULL)

/* balance factor values */
#define LH 	(-1)
#define EH 	0
#define RH 	1

#define avl_bf2str(bf)	((bf) == -1 ? "LH" : (bf) == 0 ? "EH" : (bf) == 1 ? "RH" : "(unknown)" )

/* thread bits */
#define	AVL_THREAD	0
#define	AVL_CHILD	1

/* avl routines */
#define avl_getone(x)	((x) == 0 ? 0 : (x)->avl_data)
#define avl_onenode(x)	((x) == 0 || ((x)->avl_left == 0 && (x)->avl_right == 0))

#endif /* AVL_INTERNALS */

#define	avl_child(x,dir)	((x)->avl_bits[dir]) == AVL_CHILD ? \
	(x)->avl_link[dir] : NULL
#define	avl_lchild(x)	avl_child(x,0)
#define	avl_rchild(x)	avl_child(x,1)

typedef int		(*AVL_APPLY) LDAP_P((void *, void*));
typedef int		(*AVL_CMP) LDAP_P((const void*, const void*));
typedef int		(*AVL_DUP) LDAP_P((void*, void*));
typedef void	(*AVL_FREE) LDAP_P((void*));

LDAP_AVL_F( int )
avl_free LDAP_P(( Avlnode *root, AVL_FREE dfree ));

LDAP_AVL_F( int )
avl_insert LDAP_P((Avlnode **, void*, AVL_CMP, AVL_DUP));

LDAP_AVL_F( void* )
avl_delete LDAP_P((Avlnode **, void*, AVL_CMP));

LDAP_AVL_F( void* )
avl_find LDAP_P((Avlnode *, const void*, AVL_CMP));

LDAP_AVL_F( Avlnode* )
avl_find2 LDAP_P((Avlnode *, const void*, AVL_CMP));

LDAP_AVL_F( void* )
avl_find_lin LDAP_P((Avlnode *, const void*, AVL_CMP));

#ifdef AVL_NONREENTRANT
LDAP_AVL_F( void* )
avl_getfirst LDAP_P((Avlnode *));

LDAP_AVL_F( void* )
avl_getnext LDAP_P((void));
#endif

LDAP_AVL_F( int )
avl_dup_error LDAP_P((void*, void*));

LDAP_AVL_F( int )
avl_dup_ok LDAP_P((void*, void*));

LDAP_AVL_F( int )
avl_apply LDAP_P((Avlnode *, AVL_APPLY, void*, int, int));

LDAP_AVL_F( int )
avl_prefixapply LDAP_P((Avlnode *, void*, AVL_CMP, void*, AVL_CMP, void*, int));

LDAP_AVL_F( int )
tavl_free LDAP_P(( Avlnode *root, AVL_FREE dfree ));

LDAP_AVL_F( int )
tavl_insert LDAP_P((Avlnode **, void*, AVL_CMP, AVL_DUP));

LDAP_AVL_F( void* )
tavl_delete LDAP_P((Avlnode **, void*, AVL_CMP));

LDAP_AVL_F( void* )
tavl_find LDAP_P((Avlnode *, const void*, AVL_CMP));

LDAP_AVL_F( Avlnode* )
tavl_find2 LDAP_P((Avlnode *, const void*, AVL_CMP));

LDAP_AVL_F( Avlnode* )
tavl_find3 LDAP_P((Avlnode *, const void*, AVL_CMP, int *ret));

#define	TAVL_DIR_LEFT	0
#define	TAVL_DIR_RIGHT	1

LDAP_AVL_F( Avlnode* )
tavl_end LDAP_P((Avlnode *, int direction ));

LDAP_AVL_F( Avlnode* )
tavl_next LDAP_P((Avlnode *, int direction ));

/* apply traversal types */
#define AVL_PREORDER	1
#define AVL_INORDER	2
#define AVL_POSTORDER	3
/* what apply returns if it ran out of nodes */
#define AVL_NOMORE	(-6)

LDAP_END_DECL

#endif /* _AVL */
