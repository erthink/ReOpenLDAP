/* $ReOpenLDAP$ */
/* Copyright 1992-2018 ReOpenLDAP AUTHORS: please see AUTHORS file.
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

#ifndef _AVL
#define _AVL

#include <ldap_cdefs.h>

/*
 * this structure represents a generic avl tree node.
 */

LDAP_BEGIN_DECL

typedef struct avlnode Avlnode;

struct avlnode {
  void *avl_data;
  struct avlnode *avl_link[2];
  char avl_bits[2];
  signed char avl_bf;
};

#define avl_left avl_link[0]
#define avl_right avl_link[1]
#define avl_lbit avl_bits[0]
#define avl_rbit avl_bits[1]

typedef struct tavlnode TAvlnode;

struct tavlnode {
  void *avl_data;
  struct tavlnode *avl_link[2];
  char avl_bits[2];
  signed char avl_bf;
};

#ifdef AVL_INTERNAL

/* balance factor values */
#define LH (-1)
#define EH 0
#define RH 1

#define avl_bf2str(bf) ((bf) == -1 ? "LH" : (bf) == 0 ? "EH" : (bf) == 1 ? "RH" : "(unknown)")

/* thread bits */
#define AVL_CHILD 0
#define AVL_THREAD 1

/* avl routines */
#define avl_getone(x) ((x) == 0 ? 0 : (x)->avl_data)
#define avl_onenode(x) ((x) == 0 || ((x)->avl_left == 0 && (x)->avl_right == 0))

#endif /* AVL_INTERNALS */

#define avl_child(x, dir) ((x)->avl_bits[dir]) == AVL_CHILD ? (x)->avl_link[dir] : NULL
#define avl_lchild(x) avl_child(x, 0)
#define avl_rchild(x) avl_child(x, 1)

typedef int (*AVL_APPLY)(void *, void *);
typedef int (*AVL_CMP)(const void *, const void *);
typedef int (*AVL_DUP)(void *, void *);
typedef void (*AVL_FREE)(void *);

LDAP_AVL_F(int)
avl_free(Avlnode *root, AVL_FREE dfree);

LDAP_AVL_F(int)
avl_insert(Avlnode **, void *, AVL_CMP, AVL_DUP);

LDAP_AVL_F(void *)
avl_delete(Avlnode **, void *, AVL_CMP);

LDAP_AVL_F(void *)
avl_find(Avlnode *, const void *, AVL_CMP);

LDAP_AVL_F(Avlnode *)
avl_find2(Avlnode *, const void *, AVL_CMP);

LDAP_AVL_F(void *)
avl_find_lin(Avlnode *, const void *, AVL_CMP);

#ifdef AVL_NONREENTRANT
LDAP_AVL_F(void *)
avl_getfirst(Avlnode *);

LDAP_AVL_F(void *)
avl_getnext(void);
#endif

LDAP_AVL_F(int)
avl_dup_error(void *, void *);

LDAP_AVL_F(int)
avl_dup_ok(void *, void *);

LDAP_AVL_F(int)
avl_apply(Avlnode *, AVL_APPLY, void *, int, int);

LDAP_AVL_F(int)
avl_prefixapply(Avlnode *, void *, AVL_CMP, void *, AVL_CMP, void *, int);

LDAP_AVL_F(int)
tavl_free(TAvlnode *root, AVL_FREE dfree);

LDAP_AVL_F(int)
tavl_insert(TAvlnode **, void *, AVL_CMP, AVL_DUP);

LDAP_AVL_F(void *)
tavl_delete(TAvlnode **, void *, AVL_CMP);

LDAP_AVL_F(void *)
tavl_find(TAvlnode *, const void *, AVL_CMP);

LDAP_AVL_F(TAvlnode *)
tavl_find2(TAvlnode *, const void *, AVL_CMP);

LDAP_AVL_F(TAvlnode *)
tavl_find3(TAvlnode *, const void *, AVL_CMP, int *ret);

#define TAVL_DIR_LEFT 0
#define TAVL_DIR_RIGHT 1

LDAP_AVL_F(TAvlnode *)
tavl_end(TAvlnode *, int direction);

LDAP_AVL_F(TAvlnode *)
tavl_next(TAvlnode *, int direction);

/* apply traversal types */
#define AVL_PREORDER 1
#define AVL_INORDER 2
#define AVL_POSTORDER 3
/* what apply returns if it ran out of nodes */
#define AVL_NOMORE (-6)

LDAP_END_DECL

#endif /* _AVL */
