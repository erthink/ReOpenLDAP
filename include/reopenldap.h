/*
    Copyright (c) 2015,2016 Leonid Yuriev <leo@yuriev.ru>.
    Copyright (c) 2015,2016 Peter-Service R&D LLC.

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

/* LY: Please do not ask us for Windows support, just never!
 * But you can make a fork for Windows, or become maintainer for FreeBSD... */
#ifndef __gnu_linux__
#	error "ReOpenLDAP branch supports only GNU Linux"
#endif

#ifndef _LDAP_REOPEN_H
#define _LDAP_REOPEN_H

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#ifdef HAVE_ANSIDECL_H
#	include <ansidecl.h>
#endif

#ifndef __has_attribute
#	define __has_attribute(x) (0)
#endif

#if !defined(GCC_VERSION) && defined(__GNUC__)
#	define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#endif /* GCC_VERSION */

#ifndef ALLOW_UNUSED
#	ifdef ATTRIBUTE_UNUSED
#		define ALLOW_UNUSED ATTRIBUTE_UNUSED
#	elif defined(GCC_VERSION) && (GCC_VERSION >= 3004)
#		define ALLOW_UNUSED __attribute__((__unused__))
#	elif __has_attribute(__unused__)
#		define ALLOW_UNUSED __attribute__((__unused__))
#	elif __has_attribute(unused)
#		define ALLOW_UNUSED __attribute__((unused))
#	else
#		define ALLOW_UNUSED
#	endif
#endif /* ALLOW_UNUSED */

#if !defined(__thread) && (defined(_MSC_VER) || defined(__DMC__))
#	define __thread __declspec(thread)
#endif

#ifndef __forceinline
#	if defined(__GNUC__) || defined(__clang__)
#		define __forceinline __inline __attribute__((always_inline))
#	elif ! defined(_MSC_VER)
#		define __forceinline
#	endif
#endif /* __forceinline */

#ifndef __noinline
#	if defined(__GNUC__) || defined(__clang__)
#		define __noinline __attribute__((noinline))
#	elif defined(_MSC_VER)
#		define __noinline __declspec(noinline)
#	endif
#endif /* __noinline */

#ifndef __must_check_result
#	if defined(__GNUC__) || defined(__clang__)
#		define __must_check_result __attribute__((warn_unused_result))
#	else
#		define __must_check_result
#	endif
#endif /* __must_check_result */

#ifndef __hot
#	if defined(__GNUC__) && !defined(__clang__)
#		define __hot __attribute__((hot, optimize("O3")))
#	else
#		define __hot
#	endif
#endif /* __hot */

#ifndef __cold
#	if defined(__GNUC__) && !defined(__clang__)
#		define __cold __attribute__((cold, optimize("Os")))
#	else
#		define __cold
#	endif
#endif /* __cold */

#ifndef __flatten
#	if defined(__GNUC__) || defined(__clang__)
#		define __flatten __attribute__((flatten))
#	else
#		define __flatten
#	endif
#endif /* __flatten */

#ifndef __aligned
#	if defined(__GNUC__) || defined(__clang__)
#		define __aligned(N) __attribute__((aligned(N)))
#	elif defined(__MSC_VER)
#		define __aligned(N) __declspec(align(N))
#	else
#		define __aligned(N)
#	endif
#endif /* __align */

#ifndef __noreturn
#	if defined(__GNUC__) || defined(__clang__)
#		define __noreturn __attribute__((noreturn))
#	elif defined(__MSC_VER)
#		define __noreturn __declspec(noreturn)
#	else
#		define __noreturn
#	endif
#endif

#ifndef __nothrow
#	if defined(__GNUC__) || defined(__clang__)
#		define __nothrow __attribute__((nothrow))
#	elif defined(__MSC_VER)
#		define __nothrow __declspec(nothrow)
#	else
#		define __nothrow
#	endif
#endif

#ifndef CACHELINE_SIZE
#	if defined(__ia64__) || defined(__ia64) || defined(_M_IA64)
#		define CACHELINE_SIZE 128
#	else
#		define CACHELINE_SIZE 64
#	endif
#endif

#ifndef __cache_aligned
#	define __cache_aligned __aligned(CACHELINE_SIZE)
#endif

#ifndef likely
#	if defined(__GNUC__) || defined(__clang__)
#		ifdef __cplusplus
			/* LY: workaround for "pretty" boost */
			static __inline __attribute__((always_inline))
				bool likely(bool cond) { return __builtin_expect(cond, 1); }
#		else
#			define likely(cond) __builtin_expect(!!(cond), 1)
#		endif
#	else
#		define likely(x) (x)
#	endif
#endif /* likely */

#ifndef unlikely
#	if defined(__GNUC__) || defined(__clang__)
#		ifdef __cplusplus
			/* LY: workaround for "pretty" boost */
			static __inline __attribute__((always_inline))
				bool unlikely(bool cond) { return __builtin_expect(cond, 0); }
#		else
#			define unlikely(cond) __builtin_expect(!!(cond), 0)
#		endif
#	else
#		define unlikely(x) (x)
#	endif
#endif /* unlikely */

#ifndef __extern_C
#    ifdef __cplusplus
#        define __extern_C extern "C"
#    else
#        define __extern_C
#    endif
#endif

#ifndef __noop
#    define __noop() do {} while (0)
#endif

#ifndef compiler_barrier
#	define compiler_barrier() do { \
		__asm__ __volatile__ ("" ::: "memory"); \
	} while(0)
#endif /* compiler_barrier */

/* -------------------------------------------------------------------------- */

__extern_C void __ldap_assert_fail(
		const char* assertion,
		const char* file,
		unsigned line,
		const char* function) __nothrow __noreturn;

#ifndef LDAP_ASSERT_CHECK
#	if defined(LDAP_DEBUG)
#		define LDAP_ASSERT_CHECK LDAP_DEBUG
#	elif defined(NDEBUG)
#		define LDAP_ASSERT_CHECK 0
#	else
#		define LDAP_ASSERT_CHECK 1
#	endif
#endif

/* LY: ReOpenLDAP operation mode global flags */
#define REOPENLDAP_FLAG_IDDQD	1
#define REOPENLDAP_FLAG_IDKFA	2
#define REOPENLDAP_FLAG_IDCLIP	4
#define REOPENLDAP_FLAG_JITTER	8

__extern_C void reopenldap_flags_setup (int flags);
__extern_C int reopenldap_flags;

#define reopenldap_mode_iddqd() \
	likely((reopenldap_flags & REOPENLDAP_FLAG_IDDQD) != 0)
#define reopenldap_mode_idkfa() \
	unlikely((reopenldap_flags & REOPENLDAP_FLAG_IDKFA) != 0)
#define reopenldap_mode_idclip() \
	likely((reopenldap_flags & REOPENLDAP_FLAG_IDCLIP) != 0)
#define reopenldap_mode_jitter() \
	unlikely((reopenldap_flags & REOPENLDAP_FLAG_JITTER) != 0)

__extern_C void reopenldap_jitter(int probability_percent);

#define LDAP_JITTER(prob) do \
		if (reopenldap_mode_jitter()) \
			reopenldap_jitter(prob); \
	while (0)

#define LDAP_ENSURE(condition) do \
		if (unlikely(!(condition))) \
			__ldap_assert_fail("ldap: " #condition, __FILE__, __LINE__, __FUNCTION__); \
	while (0)

#if ! LDAP_ASSERT_CHECK
#	define LDAP_ASSERT(condition) __noop()
#elif LDAP_ASSERT_CHECK > 1
#	define LDAP_ASSERT(condition) LDAP_ENSURE(condition)
#else
#	define LDAP_ASSERT(condition) do \
		if (reopenldap_mode_idkfa()) \
			LDAP_ENSURE(condition); \
	while (0)
#endif /* LDAP_ASSERT_CHECK */

#define LDAP_BUG() __ldap_assert_fail("ldap-BUG", __FILE__, __LINE__, __FUNCTION__)

#undef assert
#define assert(expr) LDAP_ASSERT(expr)

/* -------------------------------------------------------------------------- */

/* LY: engaging overlap checking for memcpy */
#ifndef LDAP_SAFEMEMCPY
#	if LDAP_ASSERT_CHECK || LDAP_DEBUG || ! defined(NDEBUG)
#		define LDAP_SAFEMEMCPY 1
#	else
#		define LDAP_SAFEMEMCPY 0
#	endif
#endif

/* -------------------------------------------------------------------------- */

#include <stdint.h>
#include <time.h>

/* LY: to avoid include <sys/time.h> here */
struct timeval;

__extern_C time_t ldap_time_steady(void);
__extern_C time_t ldap_time_unsteady(void);
__extern_C void ldap_timespec(struct timespec *);
__extern_C unsigned ldap_timeval(struct timeval *);
__extern_C uint64_t ldap_now_ns(void);

typedef struct {
	uint64_t ns;
} slap_time_t;

#define ldap_now(void) ({ \
	slap_time_t __t = { ldap_now_ns() }; \
	__t; \
})

#define ldap_from_seconds(S) ({ \
	slap_time_t __t = { (S) * (uint64_t) 1000ul * 1000ul * 1000ul }; \
	__t; \
})

#define ldap_from_timeval(T) ({ \
	slap_time_t __t = { \
		(T)->tv_sec * (uint64_t) 1000ul * 1000ul * 1000ul \
		+ (T)->tv_usec * (uint64_t) 1000ul }; \
	__t; \
})

#define ldap_from_timespec(T) ({ \
	slap_time_t __t = { \
		(T)->tv_sec * (uint64_t) 1000ul * 1000ul * 1000ul \
		+ (T)->tv_nsec }; \
	__t; \
})

#define ldap_to_milliseconds(TS) ((TS).ns / 1000000ul)
#define ldap_to_seconds(TS) ((TS).ns / 1000000000ull)
#define ldap_to_time(TS) ldap_to_seconds(TS)

/* -------------------------------------------------------------------------- */

#if defined(HAVE_VALGRIND) || defined(USE_VALGRIND)
	/* Get debugging help from Valgrind */
#	include <valgrind/memcheck.h>
#	ifndef VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE
		/* LY: available since Valgrind 3.10 */
#		define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#		define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	endif
#else
#	define VALGRIND_CREATE_MEMPOOL(h,r,z)
#	define VALGRIND_DESTROY_MEMPOOL(h)
#	define VALGRIND_MEMPOOL_TRIM(h,a,s)
#	define VALGRIND_MEMPOOL_ALLOC(h,a,s)
#	define VALGRIND_MEMPOOL_FREE(h,a)
#	define VALGRIND_MEMPOOL_CHANGE(h,a,b,s)
#	define VALGRIND_MAKE_MEM_NOACCESS(a,s)
#	define VALGRIND_MAKE_MEM_DEFINED(a,s)
#	define VALGRIND_MAKE_MEM_UNDEFINED(a,s)
#	define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#	define VALGRIND_CHECK_MEM_IS_ADDRESSABLE(a,s) (0)
#	define VALGRIND_CHECK_MEM_IS_DEFINED(a,s) (0)
#endif /* ! USE_VALGRIND */

#if defined(__has_feature)
#	if __has_feature(thread_sanitizer)
#		define __SANITIZE_THREAD__ 1
#	endif
#endif

#ifdef __SANITIZE_THREAD__
#	define ATTRIBUTE_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread, noinline))
#	define ATTRIBUTE_NO_SANITIZE_THREAD_INLINE ATTRIBUTE_NO_SANITIZE_THREAD
#else
#	define ATTRIBUTE_NO_SANITIZE_THREAD
#	define ATTRIBUTE_NO_SANITIZE_THREAD_INLINE __inline
#endif

#if defined(__has_feature)
#	if __has_feature(address_sanitizer)
#		define __SANITIZE_ADDRESS__ 1
#	endif
#endif

#ifdef __SANITIZE_ADDRESS__
#	include <sanitizer/asan_interface.h>
#	define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address, noinline))
#	define ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE ATTRIBUTE_NO_SANITIZE_ADDRESS
#else
#	define ASAN_POISON_MEMORY_REGION(addr, size) \
		((void)(addr), (void)(size))
#	define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
		((void)(addr), (void)(size))
#	define ATTRIBUTE_NO_SANITIZE_ADDRESS
#	define ATTRIBUTE_NO_SANITIZE_ADDRESS_INLINE __inline
#endif /* __SANITIZE_ADDRESS__ */

#endif /* _LDAP_REOPEN_H */
