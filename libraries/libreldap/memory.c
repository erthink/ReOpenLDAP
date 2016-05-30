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
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <ac/stdlib.h>
#include <ac/string.h>

#include "lber-int.h"

#ifdef LDAP_MEMORY_TRACE
#	include <stdio.h>
#endif

/* LY: With respect to http://en.wikipedia.org/wiki/Fail-fast */
#if LDAP_MEMORY_DEBUG
/*
 * LDAP_MEMORY_DEBUG should be enabled for properly memory handling,
 * durability and data consistency.
 *
 * It can't trigger assertion failure in a prefectly valid program!
 *
 * But it should be enabled by an experienced developer as it causes
 * the inclusion of numerous assert()'s due overall poor code quality
 * in the LDAP's world.
 *
 * If LDAP_MEMORY_DEBUG > 1, that includes
 * asserts known to break both slapd and current clients.
 *
 */
#	include <lber_hipagut.h>
#	define BER_MEM_VALID(p, s) lber_hug_memchk_ensure(p, 0)
#	define LIBC_MEM_TAG 0x71BC
#elif defined(HAVE_VALGRIND) || defined(USE_VALGRIND) || defined(__SANITIZE_ADDRESS__)
#	define BER_MEM_VALID(p, s) do { \
		if((s) > 0) { \
			LDAP_ENSURE(VALGRIND_CHECK_MEM_IS_ADDRESSABLE(p, s) == 0); \
			LDAP_ENSURE(ASAN_REGION_IS_POISONED(p, s) == 0); \
		} \
	} while(0)
#else
#	define BER_MEM_VALID(p, s) __noop()
#endif /* LDAP_MEMORY_DEBUG */

BerMemoryFunctions *ber_int_memory_fns = NULL;

void
ber_memfree_x( void *p, void *ctx )
{
	if( p == NULL ) {
		return;
	}

	if( ber_int_memory_fns == NULL || ctx == NULL ) {
#if LDAP_MEMORY_DEBUG > 0
		p = lber_hug_memchk_drown(p, LIBC_MEM_TAG);
#ifdef LDAP_MEMORY_TRACE
		struct lber_hug_memchk *mh = p;
		fprintf(stderr, "%p.%zu -f- %zu ber_memfree %zu\n",
			mh, mh->hm_sequence, mh->hm_length,
			lber_hug_memchk_info.mi_inuse_bytes);
#endif
#endif /* LDAP_MEMORY_DEBUG */

		free( p );
	} else {
		LDAP_MEMORY_ASSERT( ber_int_memory_fns->bmf_free != 0 );
		(*ber_int_memory_fns->bmf_free)( p, ctx );
	}
}

void
ber_memfree( void *p )
{
	ber_memfree_x(p, NULL);
}

void
ber_memvfree_x( void **vec, void *ctx )
{
	int	i;

	if( vec == NULL ) {
		return;
	}

	for ( i = 0; vec[i] != NULL; i++ ) {
		ber_memfree_x( vec[i], ctx );
	}

	ber_memfree_x( vec, ctx );
}

void
ber_memvfree( void **vec )
{
	ber_memvfree_x( vec, NULL );
}

void *
ber_memalloc_x( ber_len_t s, void *ctx )
{
	void *p;

	if( s == 0 ) {
		LDAP_MEMORY_ASSERT( s != 0 );
		return NULL;
	}

	if( ber_int_memory_fns == NULL || ctx == NULL ) {
		size_t bytes = s;
#if LDAP_MEMORY_DEBUG > 0
		bytes += LBER_HUG_MEMCHK_OVERHEAD;
		if (unlikely(bytes < s)) {
			ber_errno = LBER_ERROR_MEMORY;
			return NULL;
		}
#endif /* LDAP_MEMORY_DEBUG */

		p = malloc( bytes );

#if LDAP_MEMORY_DEBUG > 0
		if (p) {
			p = lber_hug_memchk_setup(p, s,
				LIBC_MEM_TAG, LBER_HUG_POISON_DEFAULT);
#ifdef LDAP_MEMORY_TRACE
			struct lber_hug_memchk *mh = LBER_HUG_CHUNK(p);
			fprintf(stderr, "%p.%zu -a- %zu ber_memalloc %zu\n",
				mh, mh->hm_sequence, mh->hm_length,
				lber_hug_memchk_info.mi_inuse_bytes);
#endif
		}
#endif /* LDAP_MEMORY_DEBUG */

	} else {
		p = (*ber_int_memory_fns->bmf_malloc)( s, ctx );
	}

	if( p == NULL ) {
		ber_errno = LBER_ERROR_MEMORY;
		return p;
	}

	BER_MEM_VALID( p, s );
	return p;
}

void *
ber_memalloc( ber_len_t s )
{
	return ber_memalloc_x( s, NULL );
}

#define LIM_SQRT(t) /* some value < sqrt(max value of unsigned type t) */ \
	((0UL|(t)-1) >>31>>31 > 1 ? ((t)1 <<32) - 1 : \
	 (0UL|(t)-1) >>31 ? 65535U : (0UL|(t)-1) >>15 ? 255U : 15U)

void *
ber_memcalloc_x( ber_len_t n, ber_len_t s, void *ctx )
{
	void *p;

	if( n == 0 || s == 0 ) {
		LDAP_MEMORY_ASSERT( n != 0 && s != 0);
		return NULL;
	}

	if( ber_int_memory_fns == NULL || ctx == NULL ) {
#if LDAP_MEMORY_DEBUG > 0
		/* The sqrt test is a slight optimization: often avoids the division */
		if (unlikely((n | s) > LIM_SQRT(size_t) && (size_t)-1/n > s)) {
			ber_errno = LBER_ERROR_MEMORY;
			return NULL;
		}

		size_t payload_bytes = (size_t) s * (size_t) n;
		size_t total_bytes = payload_bytes + LBER_HUG_MEMCHK_OVERHEAD;
		if (unlikely(total_bytes < payload_bytes)) {
			ber_errno = LBER_ERROR_MEMORY;
			return NULL;
		}

		n = total_bytes;
		s = 1;
#endif /* LDAP_MEMORY_DEBUG */

		p = calloc( n, s );

#if LDAP_MEMORY_DEBUG > 0
		if (p) {
			p = lber_hug_memchk_setup(p, payload_bytes,
				LIBC_MEM_TAG, LBER_HUG_POISON_CALLOC_ALREADY);
#ifdef LDAP_MEMORY_TRACE
			struct lber_hug_memchk *mh = LBER_HUG_CHUNK(p);
			fprintf(stderr, "%p.%zu -a- %zu ber_memcalloc %zu\n",
				mh, mh->hm_sequence, mh->hm_length,
				lber_hug_memchk_info.mi_inuse_bytes);
#endif
		}
#endif /* LDAP_MEMORY_DEBUG */

	} else {
		p = (*ber_int_memory_fns->bmf_calloc)( n, s, ctx );
	}

	if( p == NULL ) {
		ber_errno = LBER_ERROR_MEMORY;
		return p;
	}

	BER_MEM_VALID( p, (size_t) n * (size_t) s );
	return p;
}

void *
ber_memcalloc( ber_len_t n, ber_len_t s )
{
	return ber_memcalloc_x( n, s, NULL );
}

void *
ber_memrealloc_x( void* p, ber_len_t s, void *ctx )
{
	/* realloc(NULL,s) -> malloc(s) */
	if( p == NULL ) {
		return ber_memalloc_x( s, ctx );
	}

	/* realloc(p,0) -> free(p) */
	if( s == 0 ) {
		ber_memfree_x( p, ctx );
		return NULL;
	}

	if( ber_int_memory_fns == NULL || ctx == NULL ) {
		size_t bytes = s;

#if LDAP_MEMORY_DEBUG > 0
		size_t old_size;
		unsigned undo_key;

		bytes += LBER_HUG_MEMCHK_OVERHEAD;
		if(unlikely(bytes < s)) {
			ber_errno = LBER_ERROR_MEMORY;
			return NULL;
		}
		undo_key = lber_hug_realloc_begin(p, LIBC_MEM_TAG, &old_size);
		p = LBER_HUG_CHUNK(p);
#endif /* LDAP_MEMORY_DEBUG */

		p = realloc( p, bytes );

#if LDAP_MEMORY_DEBUG > 0
		if (p) {
			p = lber_hug_realloc_commit(old_size, p, LIBC_MEM_TAG, s);
#ifdef LDAP_MEMORY_TRACE
			struct lber_hug_memchk *mh = LBER_HUG_CHUNK(p);
			fprintf(stderr, "%p.%zu -a- %zu ber_memrealloc %zu\n",
				mh, mh->hm_sequence, mh->hm_length,
				lber_hug_memchk_info.mi_inuse_bytes);
#endif
		} else {
			lber_hug_realloc_undo(p, LIBC_MEM_TAG, undo_key);
		}
#endif /* LDAP_MEMORY_DEBUG */

	} else {
		p = (*ber_int_memory_fns->bmf_realloc)( p, s, ctx );
	}

	if( p == NULL ) {
		ber_errno = LBER_ERROR_MEMORY;
		return p;
	}

	BER_MEM_VALID( p, s );
	return p;
}

void *
ber_memrealloc( void* p, ber_len_t s )
{
	return ber_memrealloc_x( p, s, NULL );
}

void
ber_bvfree_x( struct berval *bv, void *ctx )
{
	if( bv == NULL ) {
		return;
	}

	BER_MEM_VALID( bv, 0 );

	if ( bv->bv_val != NULL ) {
		ber_memfree_x( bv->bv_val, ctx );
	}

	ber_memfree_x( (char *) bv, ctx );
}

void
ber_bvfree( struct berval *bv )
{
	ber_bvfree_x( bv, NULL );
}

void
ber_bvecfree_x( struct berval **bv, void *ctx )
{
	int	i;

	if( bv == NULL ) {
		return;
	}

	BER_MEM_VALID( bv, 0 );

	/* count elements */
	for ( i = 0; bv[i] != NULL; i++ ) ;

	/* free in reverse order */
	for ( i--; i >= 0; i-- ) {
		ber_bvfree_x( bv[i], ctx );
	}

	ber_memfree_x( (char *) bv, ctx );
}

void
ber_bvecfree( struct berval **bv )
{
	ber_bvecfree_x( bv, NULL );
}

int
ber_bvecadd_x( struct berval ***bvec, struct berval *bv, void *ctx )
{
	ber_len_t i;
	struct berval **new;

	if( *bvec == NULL ) {
		if( bv == NULL ) {
			/* nothing to add */
			return 0;
		}

		*bvec = ber_memalloc_x( 2 * sizeof(struct berval *), ctx );

		if( *bvec == NULL ) {
			return -1;
		}

		(*bvec)[0] = bv;
		(*bvec)[1] = NULL;

		return 1;
	}

	BER_MEM_VALID( bvec, 0 );

	/* count entries */
	for ( i = 0; (*bvec)[i] != NULL; i++ ) {
		/* EMPTY */;
	}

	if( bv == NULL ) {
		return i;
	}

	new = ber_memrealloc_x( *bvec, (i+2) * sizeof(struct berval *), ctx);

	if( new == NULL ) {
		return -1;
	}

	*bvec = new;

	(*bvec)[i++] = bv;
	(*bvec)[i] = NULL;

	return i;
}

int
ber_bvecadd( struct berval ***bvec, struct berval *bv )
{
	return ber_bvecadd_x( bvec, bv, NULL );
}

struct berval *
ber_dupbv_x(
	struct berval *dst, const struct berval *src, void *ctx )
{
	struct berval *new;

	if( src == NULL ) {
		ber_errno = LBER_ERROR_PARAM;
		return NULL;
	}

	if ( dst ) {
		new = dst;
	} else {
		if(( new = ber_memalloc_x( sizeof(struct berval), ctx )) == NULL ) {
			return NULL;
		}
	}

	if ( src->bv_val == NULL ) {
		new->bv_val = NULL;
		new->bv_len = 0;
		return new;
	}

	if(( new->bv_val = ber_memalloc_x( src->bv_len + 1, ctx )) == NULL ) {
		if ( !dst )
			ber_memfree_x( new, ctx );
		return NULL;
	}

	memcpy( new->bv_val, src->bv_val, src->bv_len );
	new->bv_val[src->bv_len] = '\0';
	new->bv_len = src->bv_len;

	return new;
}

struct berval *
ber_dupbv(
	struct berval *dst, const struct berval *src )
{
	return ber_dupbv_x( dst, src, NULL );
}

struct berval *
ber_bvdup(
	struct berval *src )
{
	return ber_dupbv_x( NULL, src, NULL );
}

struct berval *
ber_str2bv_x(
	LDAP_CONST char *s, ber_len_t len, int dup, struct berval *bv,
	void *ctx)
{
	struct berval *new;

	if( s == NULL ) {
		ber_errno = LBER_ERROR_PARAM;
		return NULL;
	}

	if( bv ) {
		new = bv;
	} else {
		if(( new = ber_memalloc_x( sizeof(struct berval), ctx )) == NULL ) {
			return NULL;
		}
	}

	new->bv_len = len ? len : strlen( s );
	if ( dup ) {
		if ( (new->bv_val = ber_memalloc_x( new->bv_len+1, ctx )) == NULL ) {
			if ( !bv )
				ber_memfree_x( new, ctx );
			return NULL;
		}

		memcpy( new->bv_val, s, new->bv_len );
		new->bv_val[new->bv_len] = '\0';
	} else {
		new->bv_val = (char *) s;
	}

	return( new );
}

struct berval *
ber_str2bv(
	LDAP_CONST char *s, ber_len_t len, int dup, struct berval *bv)
{
	return ber_str2bv_x( s, len, dup, bv, NULL );
}

struct berval *
ber_mem2bv_x(
	LDAP_CONST char *s, ber_len_t len, int dup, struct berval *bv,
	void *ctx)
{
	struct berval *new;

	if( s == NULL ) {
		ber_errno = LBER_ERROR_PARAM;
		return NULL;
	}

	if( bv ) {
		new = bv;
	} else {
		if(( new = ber_memalloc_x( sizeof(struct berval), ctx )) == NULL ) {
			return NULL;
		}
	}

	new->bv_len = len;
	if ( dup ) {
		if ( (new->bv_val = ber_memalloc_x( new->bv_len+1, ctx )) == NULL ) {
			if ( !bv ) {
				ber_memfree_x( new, ctx );
			}
			return NULL;
		}

		memcpy( new->bv_val, s, new->bv_len );
		new->bv_val[new->bv_len] = '\0';
	} else {
		new->bv_val = (char *) s;
	}

	return( new );
}

struct berval *
ber_mem2bv(
	LDAP_CONST char *s, ber_len_t len, int dup, struct berval *bv)
{
	return ber_mem2bv_x( s, len, dup, bv, NULL );
}

char *
ber_strdup_x( LDAP_CONST char *s, void *ctx )
{
	char    *p;
	size_t	len;

	LDAP_MEMORY_ASSERT(s != NULL);			/* bv damn better point to something */

	if( s == NULL ) {
		ber_errno = LBER_ERROR_PARAM;
		return NULL;
	}

	len = strlen( s ) + 1;
	if ( (p = ber_memalloc_x( len, ctx )) != NULL ) {
		memcpy( p, s, len );
	}

	return p;
}

char *
ber_strdup( LDAP_CONST char *s )
{
	return ber_strdup_x( s, NULL );
}

ber_len_t
ber_strnlen( LDAP_CONST char *s, ber_len_t len )
{
	ber_len_t l;

	for ( l = 0; l < len && s[l] != '\0'; l++ ) ;

	return l;
}

char *
ber_strndup_x( LDAP_CONST char *s, ber_len_t l, void *ctx )
{
	char    *p;
	size_t	len;

	LDAP_MEMORY_ASSERT(s != NULL);			/* bv damn better point to something */

	if( s == NULL ) {
		ber_errno = LBER_ERROR_PARAM;
		return NULL;
	}

	len = ber_strnlen( s, l );
	if ( (p = ber_memalloc_x( len + 1, ctx )) != NULL ) {
		memcpy( p, s, len );
		p[len] = '\0';
	}

	return p;
}

char *
ber_strndup( LDAP_CONST char *s, ber_len_t l )
{
	return ber_strndup_x( s, l, NULL );
}

/*
 * dst is resized as required by src and the value of src is copied into dst
 * dst->bv_val must be NULL (and dst->bv_len must be 0), or it must be
 * alloc'ed with the context ctx
 */
struct berval *
ber_bvreplace_x( struct berval *dst, LDAP_CONST struct berval *src, void *ctx )
{
	LDAP_MEMORY_ASSERT( dst != NULL );
	LDAP_MEMORY_ASSERT( !BER_BVISNULL( src ) );

	if ( BER_BVISNULL( dst ) || dst->bv_len < src->bv_len ) {
		dst->bv_val = ber_memrealloc_x( dst->bv_val, src->bv_len + 1, ctx );
	}

	memcpy( dst->bv_val, src->bv_val, src->bv_len + 1 );
	dst->bv_len = src->bv_len;

	return dst;
}

struct berval *
ber_bvreplace( struct berval *dst, LDAP_CONST struct berval *src )
{
	return ber_bvreplace_x( dst, src, NULL );
}

void
ber_bvarray_free_x( BerVarray a, void *ctx )
{
	int i;

	if (a) {
		BER_MEM_VALID( a, 0 );

		/* count elements */
		for (i=0; a[i].bv_val; i++) ;

		/* free in reverse order */
		for (i--; i>=0; i--) {
			ber_memfree_x(a[i].bv_val, ctx);
		}

		ber_memfree_x(a, ctx);
	}
}

void
ber_bvarray_free( BerVarray a )
{
	ber_bvarray_free_x(a, NULL);
}

int
ber_bvarray_dup_x( BerVarray *dst, BerVarray src, void *ctx )
{
	int i, j;
	BerVarray new;

	if ( !src ) {
		*dst = NULL;
		return 0;
	}

	for (i=0; !BER_BVISNULL( &src[i] ); i++) ;
	new = ber_memalloc_x(( i+1 ) * sizeof(BerValue), ctx );
	if ( !new )
		return -1;
	for (j=0; j<i; j++) {
		ber_dupbv_x( &new[j], &src[j], ctx );
		if ( BER_BVISNULL( &new[j] )) {
			ber_bvarray_free_x( new, ctx );
			return -1;
		}
	}
	BER_BVZERO( &new[j] );
	*dst = new;
	return 0;
}

int
ber_bvarray_add_x( BerVarray *a, BerValue *bv, void *ctx )
{
	int	n;

	if ( *a == NULL ) {
		if (bv == NULL) {
			return 0;
		}
		n = 0;

		*a = (BerValue *) ber_memalloc_x( 2 * sizeof(BerValue), ctx );
		if ( *a == NULL ) {
			return -1;
		}

	} else {
		BerVarray atmp;

		for ( n = 0; *a != NULL && (*a)[n].bv_val != NULL; n++ ) {
			;	/* just count them */
		}

		if (bv == NULL) {
			return n;
		}

		atmp = (BerValue *) ber_memrealloc_x( (char *) *a,
		    (n + 2) * sizeof(BerValue), ctx );

		if( atmp == NULL ) {
			return -1;
		}

		*a = atmp;
	}

	(*a)[n++] = *bv;
	(*a)[n].bv_val = NULL;
	(*a)[n].bv_len = 0;

	return n;
}

int
ber_bvarray_add( BerVarray *a, BerValue *bv )
{
	return ber_bvarray_add_x( a, bv, NULL );
}
