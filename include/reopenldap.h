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
#	if defined(__GNU_C) || defined(__clang__)
#		define __forceinline __inline __attribute__((always_inline))
#	elif ! defined(_MSC_VER)
#		define __forceinline
#	endif
#endif /* __forceinline */

#ifndef __must_check_result
#	if defined(__GNU_C) || defined(__clang__)
#		define __must_check_result __attribute__((warn_unused_result))
#	else
#		define __must_check_result
#	endif
#endif /* __must_check_result */

#ifndef __hot
#	if defined(__GNU_C) || defined(__clang__)
#		define __hot __attribute__((hot, optimize("O3")))
#	else
#		define __hot
#	endif
#endif /* __hot */

#ifndef __flatten
#	if defined(__GNU_C) || defined(__clang__)
#		define __flatten __attribute__((flatten))
#	else
#		define __flatten
#	endif
#endif /* __flatten */

#ifndef __aligned
#	if defined(__GNU_C) || defined(__clang__)
#		define __aligned(N) __attribute__((aligned(N)))
#	elif defined(__MSC_VER)
#		define __aligned(N) __declspec(align(N))
#	else
#		define __aligned(N)
#	endif
#endif /* __align */

#ifndef CACHELINE_SIZE
#	define CACHELINE_SIZE 64
#endif

#ifndef __cache_aligned
#	define __cache_aligned __aligned(CACHELINE_SIZE)
#endif

/* -------------------------------------------------------------------------- */

#ifdef USE_VALGRIND
	/* Get debugging help from Valgrind */
#       include <valgrind/memcheck.h>
#else
#       define VALGRIND_CREATE_MEMPOOL(h,r,z)
#       define VALGRIND_DESTROY_MEMPOOL(h)
#       define VALGRIND_MEMPOOL_TRIM(h,a,s)
#       define VALGRIND_MEMPOOL_ALLOC(h,a,s)
#       define VALGRIND_MEMPOOL_FREE(h,a)
#       define VALGRIND_MEMPOOL_CHANGE(h,a,b,s)
#       define VALGRIND_MAKE_MEM_NOACCESS(a,s)
#       define VALGRIND_MAKE_MEM_DEFINED(a,s)
#       define VALGRIND_MAKE_MEM_UNDEFINED(a,s)
#       define VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#       define VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(a,s)
#endif /* ! USE_VALGRIND */

#endif /* _LDAP_REOPEN_H */
