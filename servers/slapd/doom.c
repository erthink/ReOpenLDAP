/*
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

#include "slap.h"
#include "proto-slap.h"
#include "lber_hipagut.h"
#include <time.h>

#if SLAPD_MDB == SLAPD_MOD_STATIC
#	include "../../libraries/liblmdb/lmdb.h"
#endif /* SLAPD_MDB */

int reopenldap_flags
#if (LDAP_MEMORY_DEBUG > 1) || (LDAP_DEBUG > 1)
		= REOPENLDAP_FLAG_IDKFA
#endif
		;

void __attribute__((constructor)) reopenldap_flags_init() {
	int flags = reopenldap_flags;
	if (getenv("REOPENLDAP_FORCE_IDKFA"))
		flags |= REOPENLDAP_FLAG_IDKFA;
	if (getenv("REOPENLDAP_FORCE_IDDQD"))
		flags |= REOPENLDAP_FLAG_IDDQD;
	reopenldap_flags_setup(flags);
}

void reopenldap_flags_setup(int flags) {
	reopenldap_flags = flags & (REOPENLDAP_FLAG_IDDQD
								| REOPENLDAP_FLAG_IDKFA
								| REOPENLDAP_FLAG_JITTER);

#if LDAP_MEMORY_DEBUG > 0
	if (reopenldap_mode_idkfa()) {
		lber_hug_nasty_disabled = 0;
#ifdef LDAP_MEMORY_TRACE
		lber_hug_memchk_trace_disabled = 0;
#endif /* LDAP_MEMORY_TRACE */
		lber_hug_memchk_poison_alloc = 0xCC;
		lber_hug_memchk_poison_free = 0xDD;
	} else {
		lber_hug_nasty_disabled = LBER_HUG_DISABLED;
		lber_hug_memchk_trace_disabled = LBER_HUG_DISABLED;
		lber_hug_memchk_poison_alloc = 0;
		lber_hug_memchk_poison_free = 0;
	}
#endif /* LDAP_MEMORY_DEBUG */

#if SLAPD_MDB == SLAPD_MOD_STATIC
	flags = mdb_setup_debug(MDB_DBG_DNT, (MDB_debug_func*) MDB_DBG_DNT, MDB_DBG_DNT);
	flags &= ~(MDB_DBG_TRACE | MDB_DBG_EXTRA | MDB_DBG_ASSERT);
	if (reopenldap_mode_idkfa())
		flags |=
#	if LDAP_DEBUG > 2
				MDB_DBG_TRACE | MDB_DBG_EXTRA |
#	endif /* LDAP_DEBUG > 2 */
				MDB_DBG_ASSERT;

	mdb_setup_debug(flags, (MDB_debug_func*) MDB_DBG_DNT, MDB_DBG_DNT);
#endif /* SLAPD_MDB */
}

static __inline unsigned cpu_ticks() {
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
	return (uint32_t) ticks;
#elif defined(__sparc_v9__)
	uint64_t ticks;
	__asm("rd %%tick, %0" : "=r" (ticks));
	return ticks;
#elif defined(__powerpc64__) || defined(__ppc64__)
	uint64_t ticks;
	__asm("mfspr %0, 268" : "=r" (ticks));
	return ticks;
#elif defined(__ppc__) || defined(__powerpc__)
	unsigned ticks;
	__asm("mftb %0" : "=r" (ticks));
	return ticks;
#elif defined(__mips__)
	return hippeus_mips_rdtsc();
#elif defined(__x86_64__) || defined(__i386__)
	unsigned low, high;
	__asm("rdtsc" : "=a" (low), "=d" (high));
	return low;
#else
	struct timespec ts;
#	if defined(CLOCK_MONOTONIC_COARSE)
		clockid_t clock = CLOCK_MONOTONIC_COARSE;
#	elif defined(CLOCK_MONOTONIC_RAW)
		clockid_t clock = CLOCK_MONOTONIC_RAW;
#	else
		clockid_t clock = CLOCK_MONOTONIC;
#	endif
	LDAP_ENSURE(clock_gettime(clock, &ts) == 0);
	return ts.tv_nsec;
#endif
}

void reopenldap_jitter(int probability) {
#if defined(__x86_64__) || defined(__i386__)
	__asm __volatile("pause");
#endif
	for (;;) {
		unsigned counter = cpu_ticks();
		unsigned salt = (counter << 12) | (counter >> 19);
		unsigned twister = (3023231633 * counter) ^ salt;
		if ((int)(twister % 101) > --probability)
			break;
		sched_yield();
	}
}
