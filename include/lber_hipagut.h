/*
	Imported from 1Hippeus project at 2015-01-19.

	Copyright (c) 2013 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2013 Peter-Service LLC.
    Copyright (c) 2013 Other contributors as noted in the AUTHORS file.

    This file is part of 1Hippeus.

    1Hippeus is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    1Hippeus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LBER_HUG_H
#define _LBER_HUG_H

#include <stddef.h>
#include <ldap_cdefs.h>

#if defined(LDAP_MEMORY_TRACE) && !defined(LDAP_MEMORY_DEBUG)
#	define LDAP_MEMORY_DEBUG 1
#endif

#ifndef LDAP_MEMORY_DEBUG
#	ifdef LDAP_DEBUG
#		define LDAP_MEMORY_DEBUG 1
#	else
#		define LDAP_MEMORY_DEBUG 0
#	endif
#endif

/* ----------------------------------------------------------------------------
 * LY: Originally calles "Hipagut" = sister in law, nurse
 * (Tagalog language, Philippines). */

LDAP_BEGIN_DECL

struct lber_hipagut {
	char opacue[8];
};
typedef struct lber_hipagut lber_hug_t;

LBER_F(void) lber_hug_setup LDAP_P((
	lber_hug_t*,
	const unsigned n42 ));

LBER_F(void) lber_hug_drown LDAP_P((
	lber_hug_t* ));

/* Return zero when no corruption detected, otherwise returns -1. */
LBER_F(int) lber_hug_probe LDAP_P((
	const lber_hug_t*,
	const unsigned n42 ));

LBER_F(void) lber_hug_setup_link LDAP_P((
	lber_hug_t* slave,
	const lber_hug_t* master ));

LBER_F(void) lber_hug_drown_link LDAP_P((
	lber_hug_t* slave,
	const lber_hug_t* master ));

/* Return zero when no corruption detected, otherwise returns -1. */
LBER_F(int) lber_hug_probe_link LDAP_P((
	const lber_hug_t* slave,
	const lber_hug_t* master ));

#define LBER_HUG_TETRAD(a, b, c, d) \
	((a) << 24 | (b) << 16 | (c) << 8 | (d))

#define LBER_HUG_N42(label) \
	LBER_HUG_TETRAD(# label[0], # label[1], # label[2], # label[3])

#define LBER_HUG_DECLARE(gizmo) \
	struct lber_hipagut gizmo

/* -------------------------------------------------------------------------- */

#define LBER_HUG_SETUP(gizmo, label) \
	lber_hug_setup(&(gizmo), LBER_HUG_N42(label))

#define LBER_HUG_DROWN(gizmo) \
	lber_hug_drown(&(gizmo))

#define LBER_HUG_PROBE(gizmo, label) \
	lber_hug_probe(&(gizmo), LBER_HUG_N42(label))

#define LBER_HUG_SETUP_LINK(slave, master) \
	lber_hug_setup_link(&(slave), &(master))

#define LBER_HUG_PROBE_LINK(slave, master) \
	lber_hug_probe_link(&(slave), &(master))

/* -------------------------------------------------------------------------- */

#define LBER_HUG_SPACE \
	((ptrdiff_t)sizeof(lber_hug_t))

#define LBER_HUG_ASIDE(base, offset) \
	((lber_hug_t*)(((char*)(base)) + (offset)))

#define LBER_HUG_BEFORE(address) \
	LBER_HUG_ASIDE(address, -LBER_HUG_SPACE)

/* -------------------------------------------------------------------------- */

#define LBER_HUG_SETUP_ASIDE(base, label, offset) \
	lber_hug_setup(LBER_HUG_ASIDE(base, offset), LBER_HUG_N42(label))

#define LBER_HUG_DROWN_ASIDE(base, offset) \
	lber_hug_drown(LBER_HUG_ASIDE(base, offset))

#define LBER_HUG_PROBE_ASIDE(base, label, offset) \
	lber_hug_probe(LBER_HUG_ASIDE(base, offset), LBER_HUG_N42(label))

#define LBER_HUG_SETUP_LINK_ASIDE(base, master, offset) \
	lber_hug_setup_link(LBER_HUG_ASIDE(base, offset), &(master))

#define LBER_HUG_PROBE_LINK_ASIDE(base, master, offset) \
	lber_hug_probe_link(LBER_HUG_ASIDE(base, offset), &(master))

/* -------------------------------------------------------------------------- */

#define lber_hug_throw(info) \
	__assert_fail("hipagut: guard " info, __FILE__, __LINE__, __FUNCTION__)

#define LBER_HUG_ENSURE(gizmo, label) do \
	if (unlikely(! LBER_HUG_PROBE(gizmo, label))) \
			lber_hug_throw(#label "@" #gizmo); \
    while (0)

#define LBER_HUG_ENSURE_ASIDE(base, label, offset) \
	if (unlikely(! LBER_HUG_PROBE_ASIDE(base, label, offset))) \
		lber_hug_throw(#label "@" #base "[" #offset "]"); \
    while (0)

#define LBER_HUG_ENSURE_LINK(slave, master) do \
	if (unlikely(! LBER_HUG_PROBE_LINK(slave, master))) \
			lber_hug_throw(#master "~" #slave); \
    while (0)

#define LBER_HUG_ENSURE_LINK_ASIDE(base, master, offset) \
	if (unlikely(! LBER_HUG_PROBE_LINK_ASIDE(base, master, offset))) \
		lber_hug_throw(#master "~" #base "[" #offset "]"); \
    while (0)

#if HIPAGUT_ASSERT_CHECK
#   define LBER_HUG_ASSERT(gizmo, label)  \
		LBER_HUG_ENSURE(gizmo, label)
#   define LBER_HUG_ASSERT_ASIDE(base, label, offset) \
		LBER_HUG_ENSURE_ASIDE(base, label, offset)
#   define LBER_HUG_ASSERT_LINK(slave, maser) \
		LBER_HUG_ENSURE_LINK(slave, master)
#   define LBER_HUG_ASSERT_LINK_ASIDE(base, master, offset) \
		LBER_HUG_ENSURE_LINK_ASIDE(base, master, offset)
#else
#   define LBER_HUG_ASSERT(gizmo, label) \
		do {} while (0)
#   define LBER_HUG_ASSERT_ASIDE(base, label, offset) \
		do {} while (0)
#   define LBER_HUG_ASSERT_LINK(slave, master) \
		do {} while (0)
#   define LBER_HUG_ASSERT_LINK_ASIDE(base, master, offset) \
		do {} while (0)
#endif /* HIPAGUT_ASSERT_CHECK */

#define LBER_HUG_DISABLED 0xfea51b1eu /* feasible */
LBER_V(unsigned) lber_hug_nasty_disabled;

/* -------------------------------------------------------------------------- */

#if LDAP_MEMORY_DEBUG > 0

LBER_V(unsigned) lber_hug_memchk_poison_alloc;
LBER_V(unsigned) lber_hug_memchk_poison_free;
LBER_V(unsigned) lber_hug_memchk_trace_disabled;

struct _lber_hug_memchk_info {
	volatile size_t mi_inuse_bytes;
	volatile size_t mi_inuse_chunks;
	volatile size_t mi_sequence;
};

LBER_V(struct _lber_hug_memchk_info) lber_hug_memchk_info;

struct lber_hug_memchk {
	LBER_HUG_DECLARE(hm_guard_head);
	size_t hm_length;
	size_t hm_sequence; /* Allocation sequence number */
	LBER_HUG_DECLARE(hm_guard_bottom);
};

#define LBER_HUG_MEMCHK_OVERHEAD \
	(sizeof(struct lber_hug_memchk) + sizeof(struct lber_hipagut))

#define LBER_HUG_CHUNK(ptr) \
	((struct lber_hug_memchk*) \
		(((char*) (ptr)) - sizeof(struct lber_hug_memchk)))

#define LBER_HUG_PAYLOAD(memchunk) \
	((void*) \
		(((char*) (memchunk)) + sizeof(struct lber_hug_memchk)))

/* Defines for 'poison_mode' of lber_hug_memchk_setup() */
#define LBER_HUG_POISON_DEFAULT		0xDEFA0178u
#define LBER_HUG_POISON_CALLOC_SETUP	0xCA110C00u
#define LBER_HUG_POISON_CALLOC_ALREADY	(LBER_HUG_POISON_CALLOC_SETUP | 0xFFu)
#define LBER_HUG_POISON_DISABLED	LBER_HUG_DISABLED

LBER_F(void*) lber_hug_memchk_setup LDAP_P((
	struct lber_hug_memchk* memchunk,
	size_t payload_size,
	unsigned poison_mode ));

LBER_F(void*) lber_hug_memchk_drown LDAP_P(( void* payload ));
LBER_F(void) lber_hug_memchk_ensure LDAP_P(( const void* payload ));
LBER_F(size_t) lber_hug_memchk_size LDAP_P(( const void* payload ));

LBER_F(unsigned) lber_hug_realloc_begin LDAP_P((
	const void* payload,
	size_t* old_size ));
LBER_F(void*) lber_hug_realloc_undo LDAP_P((
	struct lber_hug_memchk* old_memchunk,
	unsigned undo_key ));
LBER_F(void*) lber_hug_realloc_commit LDAP_P((
	size_t old_size,
	struct lber_hug_memchk* new_memchunk,
	size_t new_size ));

/* Return zero when no corruption detected,
 * otherwise returns combination of OR'ed bits: 1, 2, 3. */
LBER_F(int) lber_hug_memchk_probe LDAP_P((
	const void* payload,
	size_t *length,
	size_t *sequence ));

#endif /* LDAP_MEMORY_DEBUG > 0 */

/* LY: memcpy with checking for overlap */
LBER_F(void*) ber_memcpy_safe LDAP_P((
	void* dest,
	const void* src,
	size_t n ));

LDAP_END_DECL

#endif /* _LBER_HUG_H */
