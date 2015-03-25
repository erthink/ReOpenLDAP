/*
    Imported from 1Hippeus project at 2015-01-19.

    Copyright (c) 2015 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2015 Peter-Service R&D LLC.

    This file is part of ReOpenLDAP.

    ReOpenLDAP is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ReOpenLDAP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "portable.h"

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "lber_hipagut.h"

/* LY: only for self debugging
#include <stdio.h> */

/* -------------------------------------------------------------------------- */

static __forceinline uint64_t unaligned_load(const volatile void* ptr) {
#if defined(__x86_64__) || defined(__i386__)
	return *(const volatile uint64_t*) ptr;
#else
	uint64_t local;
#	if defined(__GNUC__) || defined(__clang__)
		__builtin_memcpy(&local, ptr, 8);
#	else
		memcpy(&local, ptr, 8);
#	endif /* __GNUC__ || __clang__ */
	return local;
#endif /* arch selector */
}

static __forceinline void unaligned_store(volatile void* ptr, uint64_t value) {
#if defined(__x86_64__) || defined(__i386__)
	*(volatile uint64_t*) ptr = value;
#else
#	if defined(__GNUC__) || defined(__clang__)
		__builtin_memcpy(ptr, &value, ptr, 8);
#	else
		memcpy(ptr, &value, 8);
#	endif /* __GNUC__ || __clang__ */
#endif /* arch selector */
}

struct _lber_hug_memchk_info lber_hug_memchk_info __cache_aligned;

#if defined(__GNUC__) || defined(__clang__)
	/* LY: noting needed */
#elif defined(_MSC_VER)
#	define __sync_fetch_and_add(p,v) _InterlockedExchangeAdd(p,v)
#	define __sync_fetch_and_sub(p,v) _InterlockedExchangeAdd(p,-(v))
#elif defined(__APPLE__)
#	include <libkern/OSAtomic.h>
	static size_t __sync_fetch_and_add(volatile size_t *p, size_t v) {
		switch(sizeof(size_t)) {
		case 4:
			return OSAtomicAdd32(v, (volatile int32_t*) p);
		case 8:
			return OSAtomicAdd64(v, (volatile int64_t*) p);
		default: {
			size_t snap = *p; *p += v; return snap;
		}
	}
	#	define __sync_fetch_and_sub(p,v) __sync_fetch_and_add(p,-(v))
#else
	static size_t __sync_fetch_and_add(volatile size_t *p, size_t v) {
		size_t snap = *p; *p += v; return snap;
	}
#	define __sync_fetch_and_sub(p,v) __sync_fetch_and_add(p,-(v))
#endif

static __forceinline uint64_t entropy_ticks() {
#if defined(__GNUC__) || defined(__clang__)
#if defined(__ia64__)
	uint64_t ticks;
	__asm("mov %0=ar.itc" : "=r" (ticks));
	return ticks;
#elif defined(__hppa__)
	uint64_t ticks;
	__asm("mfctl 16, %0" : "=r" (ticks));
	return ticks;
#elif defined(__s390__)
	uint64_t ticks;
	__asm("stck 0(%0)" : : "a" (&(ticks)) : "memory", "cc");
	return ticks;
#elif defined(__alpha__)
	uint64_t ticks;
	__asm("rpcc %0" : "=r" (ticks));
	return ticks;
#elif defined(__sparc_v9__)
	uint64_t ticks;
	__asm("rd %%tick, %0" : "=r" (ticks));
	return ticks;
#elif defined(__ppc__) || defined(__powerpc__)
	unsigned tbl, tbu0, tbu1;

	do {
		__asm("mftbu %0" : "=r" (tbu0));
		__asm("mftb %0" : "=r" (tbl));
		__asm("mftbu %0" : "=r" (tbu1));
	} while (unlikely(tbu0 != tbu1));

	return (((uin64_t) tbu0) << 32) | tbl;
#elif defined(__x86_64__) || defined(__i386__)
	unsigned lo, hi;

	/* LY: Using the "a" and "d" constraints is important for correct code. */
	__asm("rdtsc" : "=a" (lo), "=d" (hi));

	return (((uint64_t)hi) << 32) + lo;
#endif /* arch selector */
#endif /* __GNUC__ || __clang__ */

	static volatile size_t sequence;
	uint64_t salt = __sync_fetch_and_add(&sequence, 798021487u);
	return ((salt << 17) ^ (salt >> 5)) * 3681268039ul;
}

/* -------------------------------------------------------------------------- */

static __forceinline unsigned canary() {
	uint64_t ticks = entropy_ticks();
    return ticks ^ (ticks >> 32);
}

static __forceinline uint32_t linear_congruential(uint32_t value) {
	return value * 1664525u + 1013904223u;
}

/*! Calculate magic mesh from a variable chirp and the constant salt n42 */
static __forceinline uint32_t mixup(unsigned chirp, const unsigned n42) {
	uint64_t caramba;

	/* LY: just multiplication by a prime number. */
	if (sizeof(unsigned long) > 4)
		caramba = 1445174711lu * (unsigned long)(chirp + n42);
	else {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
		__asm("mull %2" : "=A" (caramba) : "a" (1445174711u), "r" (chirp + n42) : "cc");
#else
		caramba = ((uint64_t) chirp + n42) * 1445174711u;
#endif
	}

	/* LY: significally less collisions when only the few bits damaged. */
	return caramba ^ (caramba >> 32);
}

static __forceinline int fairly(uint32_t value) {
	if (unlikely((value >= 0xFFFF0000u) || (value <= 0x0000FFFFu)))
		return 0;

	return 1;
}

__hot __flatten void lber_hug_setup(lber_hug_t* self, const unsigned n42) {
	uint32_t chirp = canary();
	for (;;) {
		if (likely(fairly(chirp))) {
			uint32_t sign = mixup(chirp, n42);
			if (likely(fairly(sign))) {
				uint64_t tale = chirp + (((uint64_t)sign) << 32);
				/* fprintf(stderr, "hipagut_setup: ptr %p | n42 %08x, "
						"tale %016lx, chirp %08x, sign %08x ? %08x\n",
						self, n42, tale, chirp, sign, mixup(chirp, n42)); */
				unaligned_store(self->opacue, tale);
				break;
			}
		}
		chirp = linear_congruential(chirp);
	}
}

__hot __flatten int lber_hug_probe(const lber_hug_t* self, const unsigned n42) {
	if (unlikely(lber_hug_nasty_disabled == LBER_HUG_DISABLED))
		return 0;

	uint64_t tale = unaligned_load(self->opacue);
	uint32_t chirp = tale;
	uint32_t sign = tale >> 32;
	/* fprintf(stderr, "hipagut_probe: ptr %p | n42 %08x, "
			"tale %016lx, chirp %08x, sign %08x ? %08x\n",
			self, n42, tale, chirp, sign, mixup(chirp, n42)); */
	return likely(fairly(chirp) && fairly(sign)
				  && sign == mixup(chirp, n42)) ? 0 : -1;
}

__hot __flatten void lber_hug_drown(lber_hug_t* gizmo) {
	/* LY: This notable value would always bite,
	 * because (chirp == 0) != (sign == 0). */
	unaligned_store(gizmo->opacue, 0xDEADB0D1Ful);
}

void lber_hug_setup_link(lber_hug_t* slave, const lber_hug_t* master) {
	lber_hug_setup(slave, unaligned_load(master->opacue) >> 32);
}

void lber_hug_drown_link(lber_hug_t* slave, const lber_hug_t* master) {
	lber_hug_drown(slave);
}

int lber_hug_probe_link(const lber_hug_t* slave, const lber_hug_t* master) {
	return lber_hug_probe(slave, unaligned_load(master->opacue) >> 32);
}

/* -------------------------------------------------------------------------- */

#define MEMCHK_TAG_HEADER	header
#define MEMCHK_TAG_BOTTOM	bottom
#define MEMCHK_TAG_COVER	cover

#define MEMCHK_TAG_HEADER_REALLOC	_Header
#define MEMCHK_TAG_BOTTOM_REALLOC	_Bottom
#define MEMCHK_TAG_COVER_REALLOC	_Cover

static void lber_hug_memchk_throw(const void* payload, unsigned bits) {
	const char* trouble;
	switch (bits) {
	case 0:
		return;
	case 1:
		trouble = "hipagut: corrupted memory chunk"
				  " (likely overruned by other)";
		break;
	case 2:
		trouble = "hipagut: corrupted memory chunk"
				  " (likely underrun)";
		break;
	case 4:
		trouble = "hipagut: corrupted memory chunk"
				  " (likely overrun)";
		break;
	/* case 3: */
	default:
		trouble = "hipagut: corrupted memory chunk"
				  " (control header was destroyed)";
	}
	__assert_fail(trouble, __FILE__, __LINE__, __FUNCTION__);
}

__hot __flatten size_t lber_hug_memchk_size(const void* payload) {
	size_t size = 0;
	unsigned bits = lber_hug_memchk_probe(payload, &size, NULL);

	if (unlikely (bits != 0))
		lber_hug_memchk_throw(payload, bits);

	return size;
}

#define VALGRIND_CLOSE(memchunk) { \
		VALGRIND_MAKE_MEM_NOACCESS( \
			(char *) memchunk + sizeof(*memchunk) + memchunk->hm_length, \
			sizeof(struct lber_hipagut)); \
		VALGRIND_MAKE_MEM_NOACCESS(memchunk, sizeof(*memchunk)); \
	}

#define VALGRIND_OPEN(memchunk) { \
		VALGRIND_MAKE_MEM_DEFINED(memchunk, sizeof(*memchunk)); \
		VALGRIND_MAKE_MEM_DEFINED( \
			(char *) memchunk + sizeof(*memchunk) + memchunk->hm_length, \
			sizeof(struct lber_hipagut)); \
	}

__hot __flatten int lber_hug_memchk_probe(
		const void* payload,
		size_t *length,
		size_t *sequence ) {
	struct lber_hug_memchk* memchunk = LBER_HUG_CHUNK(payload);
	unsigned bits = 0;

	VALGRIND_OPEN(memchunk);
	if (likely(lber_hug_nasty_disabled != LBER_HUG_DISABLED)) {
		if (unlikely(LBER_HUG_PROBE(memchunk->hm_guard_head, MEMCHK_TAG_HEADER)))
			bits |= 1;
		if (unlikely(LBER_HUG_PROBE(memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM)))
			bits |= 2;
		if (likely(bits == 0)) {
			if (length)
				*length = memchunk->hm_length;
			if (sequence)
				*sequence = memchunk->hm_sequence;
			if (unlikely(LBER_HUG_PROBE_ASIDE(memchunk, MEMCHK_TAG_COVER,
					memchunk->hm_length + sizeof(struct lber_hug_memchk))))
				bits |= 4;
			else
				LDAP_ENSURE(VALGRIND_CHECK_MEM_IS_ADDRESSABLE(memchunk,
						memchunk->hm_length + LBER_HUG_MEMCHK_OVERHEAD) == 0);
		}
	}
	VALGRIND_CLOSE(memchunk);
	return bits;
}

__hot __flatten void lber_hug_memchk_ensure(const void* payload) {
	unsigned bits = lber_hug_memchk_probe(payload, NULL, NULL);

	if (unlikely (bits != 0))
		lber_hug_memchk_throw(payload, bits);
}

unsigned lber_hug_nasty_disabled;
unsigned lber_hug_memchk_poison_alloc;
unsigned lber_hug_memchk_poison_free;
unsigned lber_hug_memchk_trace_disabled
#ifndef LDAP_MEMORY_TRACE
	= LBER_HUG_DISABLED
#endif
	;

__hot __flatten void* lber_hug_memchk_setup(
		struct lber_hug_memchk* memchunk,
		size_t payload_size,
		unsigned poison_mode) {
	void* payload = LBER_HUG_PAYLOAD(memchunk);

	size_t sequence = LBER_HUG_DISABLED;
	if (unlikely(lber_hug_memchk_trace_disabled != LBER_HUG_DISABLED)) {
		sequence = __sync_fetch_and_add(&lber_hug_memchk_info.mi_sequence, 1);
		__sync_fetch_and_add(&lber_hug_memchk_info.mi_inuse_chunks, 1);
		__sync_fetch_and_add(&lber_hug_memchk_info.mi_inuse_bytes, payload_size);
	}

	LBER_HUG_SETUP(memchunk->hm_guard_head, MEMCHK_TAG_HEADER);
	memchunk->hm_length = payload_size;
	memchunk->hm_sequence = sequence;
	LBER_HUG_SETUP(memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM);
	LBER_HUG_SETUP_ASIDE(memchunk, MEMCHK_TAG_COVER,
		payload_size + sizeof(struct lber_hug_memchk));
	VALGRIND_CLOSE(memchunk);

	if (poison_mode == LBER_HUG_POISON_DEFAULT)
		poison_mode = lber_hug_memchk_poison_alloc;
	switch(poison_mode) {
	default:
		memset(payload, (char) poison_mode, payload_size);
	case LBER_HUG_POISON_DISABLED:
		VALGRIND_MAKE_MEM_UNDEFINED(payload, payload_size);
		break;
	case LBER_HUG_POISON_CALLOC_SETUP:
		memset(payload, 0, payload_size);
	case LBER_HUG_POISON_CALLOC_ALREADY:
		VALGRIND_MAKE_MEM_DEFINED(payload, payload_size);
		break;
	}
	return payload;
}

__hot __flatten void* lber_hug_memchk_drown(void* payload) {
	size_t payload_size;
	struct lber_hug_memchk* memchunk;

	lber_hug_memchk_ensure(payload);
	memchunk = LBER_HUG_CHUNK(payload);
	VALGRIND_OPEN(memchunk);
	payload_size = memchunk->hm_length;
	LBER_HUG_DROWN(memchunk->hm_guard_head);
	/* memchunk->hm_length = ~memchunk->hm_length; */
	LBER_HUG_DROWN(memchunk->hm_guard_bottom);
	LBER_HUG_DROWN_ASIDE(memchunk,
		payload_size + sizeof(struct lber_hug_memchk));

	if (unlikely(lber_hug_memchk_trace_disabled != LBER_HUG_DISABLED)) {
		__sync_fetch_and_sub(&lber_hug_memchk_info.mi_inuse_chunks, 1);
		__sync_fetch_and_sub(&lber_hug_memchk_info.mi_inuse_bytes, payload_size);
	}

	if (lber_hug_memchk_poison_free != LBER_HUG_POISON_DISABLED)
		memset(payload, (char) lber_hug_memchk_poison_free, payload_size);

	VALGRIND_MAKE_MEM_NOACCESS(memchunk, LBER_HUG_MEMCHK_OVERHEAD + payload_size);
	return memchunk;
}

static int lber_hug_memchk_probe_realloc(
		struct lber_hug_memchk* memchunk, unsigned key) {
	unsigned bits = 0;

	if (likely(lber_hug_nasty_disabled != LBER_HUG_DISABLED)) {
		if (unlikely(lber_hug_probe(&memchunk->hm_guard_head, key)))
			bits |= 1;
		if (unlikely(lber_hug_probe(&memchunk->hm_guard_bottom, key + 1)))
			bits |= 2;
		if (likely(bits == 0)) {
			if (unlikely(
				lber_hug_probe(LBER_HUG_ASIDE(memchunk,
					memchunk->hm_length + sizeof(struct lber_hug_memchk)),
					key + 2)))
				bits |= 4;
			else
				LDAP_ENSURE(VALGRIND_CHECK_MEM_IS_ADDRESSABLE(memchunk,
						memchunk->hm_length + LBER_HUG_MEMCHK_OVERHEAD) == 0);
		}
	}
	return bits;
}

unsigned lber_hug_realloc_begin ( const void* payload, size_t* old_size ) {
	struct lber_hug_memchk* memchunk;
	unsigned key = canary();

	lber_hug_memchk_ensure(payload);
	memchunk = LBER_HUG_CHUNK(payload);
	VALGRIND_OPEN(memchunk);
	*old_size = memchunk->hm_length;
	LBER_HUG_SETUP(memchunk->hm_guard_head, key);
	LBER_HUG_SETUP(memchunk->hm_guard_bottom, key + 1);
	LBER_HUG_SETUP_ASIDE(memchunk, key + 2,
		memchunk->hm_length + sizeof(struct lber_hug_memchk));

	return key;
}

void* lber_hug_realloc_undo ( struct lber_hug_memchk* memchunk,
		unsigned key) {
	unsigned bits = lber_hug_memchk_probe_realloc(memchunk, key);

	if (unlikely (bits != 0)) {
		void* payload = LBER_HUG_PAYLOAD(memchunk);
		lber_hug_memchk_throw(payload, bits);
	}

	LBER_HUG_SETUP(memchunk->hm_guard_head, MEMCHK_TAG_HEADER);
	LBER_HUG_SETUP(memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM);
	LBER_HUG_SETUP_ASIDE(memchunk, MEMCHK_TAG_COVER,
		memchunk->hm_length + sizeof(struct lber_hug_memchk));
	VALGRIND_CLOSE(memchunk);

	return LBER_HUG_PAYLOAD(memchunk);
}

void* lber_hug_realloc_commit ( size_t old_size,
		struct lber_hug_memchk* new_memchunk,
		size_t new_size ) {
	void* new_payload = LBER_HUG_PAYLOAD(new_memchunk);
	size_t sequence = LBER_HUG_DISABLED;

	LDAP_ENSURE(VALGRIND_CHECK_MEM_IS_ADDRESSABLE(new_memchunk,
			new_size + LBER_HUG_MEMCHK_OVERHEAD) == 0);

	if (unlikely(lber_hug_memchk_trace_disabled != LBER_HUG_DISABLED)) {
		sequence = __sync_fetch_and_add(&lber_hug_memchk_info.mi_sequence, 1);
		__sync_fetch_and_add(&lber_hug_memchk_info.mi_inuse_bytes, new_size - old_size);
	}

	LBER_HUG_SETUP(new_memchunk->hm_guard_head, MEMCHK_TAG_HEADER);
	new_memchunk->hm_length = new_size;
	new_memchunk->hm_sequence = sequence;
	LBER_HUG_SETUP(new_memchunk->hm_guard_bottom, MEMCHK_TAG_BOTTOM);
	LBER_HUG_SETUP_ASIDE(new_memchunk, MEMCHK_TAG_COVER,
		new_size + sizeof(struct lber_hug_memchk));
	VALGRIND_CLOSE(new_memchunk);

	if (new_size > old_size) {
		if (lber_hug_memchk_poison_alloc != LBER_HUG_POISON_DISABLED)
			memset((char *) new_payload + old_size,
			   (char) lber_hug_memchk_poison_alloc, new_size - old_size);
		VALGRIND_MAKE_MEM_UNDEFINED(
			(char *) new_payload + old_size, new_size - old_size);
	}

	return new_payload;
}
