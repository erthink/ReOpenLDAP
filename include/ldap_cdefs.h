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

#ifndef _LDAP_CDEFS_H
#define _LDAP_CDEFS_H

/* LDAP C Defines */

#if defined(__cplusplus) || defined(c_plusplus)
#define LDAP_BEGIN_DECL extern "C" {
#define LDAP_END_DECL }
#else
#define LDAP_BEGIN_DECL /* begin declarations */
#define LDAP_END_DECL   /* end declarations */
#endif

/* ANSI C or C++ */
#define LDAP_CONCAT1(x, y) x##y
#define LDAP_CONCAT(x, y) LDAP_CONCAT1(x, y)
#define LDAP_STRING(x) #x              /* stringify without expanding x */
#define LDAP_XSTRING(x) LDAP_STRING(x) /* expand x, then stringify */

#ifndef __has_attribute
#define __has_attribute(x) (0)
#endif

#ifndef __has_feature
#define __has_feature(x) (0)
#endif

#ifndef __has_extension
#define __has_extension(x) (0)
#endif

#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

#ifndef __has_warning
#define __has_warning(x) (0)
#endif

#ifndef __has_include
#define __has_include(x) (0)
#endif

#ifndef __GNUC_PREREQ
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define __GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define __GNUC_PREREQ(maj, min) 0
#endif
#endif /* __GNUC_PREREQ */

#if !__GNUC_PREREQ(4, 2)
/* LY: Actualy ReOpenLDAP was not tested with compilers
 *     older than GCC 4.4 from RHEL6.
 * But you could remove this #error and try to continue at your own risk.
 * In such case please don't rise up an issues related ONLY to old compilers.
 */
#error "ReOpenLDAP required at least GCC 4.2 compatible C/C++ compiler."
#endif

#ifndef __CLANG_PREREQ
#ifdef __clang__
#define __CLANG_PREREQ(maj, min) ((__clang_major__ << 16) + __clang_minor__ >= ((maj) << 16) + (min))
#else
#define __CLANG_PREREQ(maj, min) (0)
#endif
#endif /* __CLANG_PREREQ */

#ifndef __reldap_exportable
#if defined(__GNUC__)
#if !defined(__clang__)
#define __reldap_exportable __attribute__((visibility("default"), externally_visible))
#else
#define __reldap_exportable __attribute__((visibility("default")))
#endif
#else
#define __reldap_exportable
#endif
#endif /* __reldap_exportable */

#ifndef __reldap_deprecated
#if defined(__attribute_deprecated__)
#define __reldap_deprecated __attribute_deprecated__
#elif defined(__deprecated)
#define __reldap_deprecated __deprecated
#elif defined(__GNUC__)
#define __reldap_deprecated __attribute__((__deprecated__))
#else
#define __reldap_deprecated
#endif
#endif /* __reldap_deprecated */

#ifndef __reldap_deprecated_msg
#if __has_feature(attribute_deprecated_with_message)
#define __reldap_deprecated_msg(msg) __attribute__((__deprecated__(msg)))
#elif defined(__GNUC__) && __GNUC_PREREQ(4, 5)
#define __reldap_deprecated_msg(msg) __attribute__((__deprecated__(msg)))
#else
#define __reldap_deprecated_msg(msg) __reldap_deprecated
#endif
#endif /* __reldap_deprecated_msg */

#define LDAP_GCCATTR(attrs) __attribute__(attrs)

/* -------------------------------------------------------------------------- */

/*
 * Support for dynamic SO-libraries.
 *
 * When external source code includes header files for dynamic libraries,
 * the external source code is "importing" DLL symbols into its resulting
 * object code.
 * This is not totally necessary for functions because the compiler
 * (gcc or MSVC) will generate stubs when this directive is absent.
 * However, this is required for imported variables.
 *
 * The LDAP library, i.e. libreldap, can be built as
 * static or shared, based on configuration. Just about all other source
 * code in ReOpenLDAP use these libraries. If the LDAP library
 * is configured as shared, 'configure' defines the RELDAP_LIBS_SHARED
 * macro. When other source files include LDAP library headers, the
 * LDAP library symbols will automatically be marked as imported. When
 * the actual LDAP library is being built, the symbols will not
 * be marked as imported because the RELDAP_LIBRARY macro
 * will be respectively defined.
 *
 * Any project outside of ReOpenLDAP with source code wanting to use
 * LDAP dynamic libraries should explicitly define RELDAP_LIBS_SHARED.
 * This will ensure that external source code appropriately marks symbols
 * that will be imported.
 *
 * The slapd executable, itself, can be used as a dynamic library.
 * For example, if a backend module is compiled as shared, it will
 * import symbols from slapd. When this happens, the slapd symbols
 * must be marked as imported in header files that the backend module
 * includes. Remember that slapd links with various static libraries.
 * If the LDAP libraries were configured as static, their object
 * code is also part of the monolithic slapd executable. Thus, when
 * a backend module imports symbols from slapd, it imports symbols from
 * all of the static libraries in slapd as well. Thus, the SLAP_IMPORT
 * macro, when defined, will appropriately mark symbols as imported.
 * This macro should be used by shared backend modules as well as any
 * other external source code that imports symbols from the slapd
 * executable as if it were a DLL.
 *
 * NOTE ABOUT BACKENDS: Backends can be configured as static or dynamic.
 * When a backend is configured as dynamic, slapd will load the backend
 * explicitly and populate function pointer structures by calling
 * the backend's well-known initialization function. Because of this
 * procedure, slapd never implicitly imports symbols from dynamic backends.
 */

/* RELDAP library */
#if defined(RELDAP_LIBRARY) && (defined(RELDAP_LIBS_SHARED) || defined(SLAPD_DYNAMIC))
#define LDAP_F(type) __reldap_exportable type
#define LDAP_V(type) extern __reldap_exportable type
#define LDAP_LDIF_F(type) __reldap_exportable type
#define LDAP_LDIF_V(type) extern __reldap_exportable type
#define LBER_F(type) __reldap_exportable type
#define LBER_V(type) extern __reldap_exportable type
#else
#define LDAP_F(type) type
#define LDAP_V(type) extern type
#define LDAP_LDIF_F(type) type
#define LDAP_LDIF_V(type) extern type
#define LBER_F(type) type
#define LBER_V(type) extern type
#endif

/* LUNICODE library */
#if defined(SLAPD_DYNAMIC) && !defined(SLAPD_IMPORT)
#define LDAP_LUNICODE_F(type) __reldap_exportable type
#define LDAP_LUNICODE_V(type) extern __reldap_exportable type
#else
#define LDAP_LUNICODE_F(type) type
#define LDAP_LUNICODE_V(type) extern type
#endif

/* LUTIL libraries */
#if defined(SLAPD_DYNAMIC) && !defined(SLAPD_IMPORT)
#define LDAP_LUTIL_F(type) __reldap_exportable type
#define LDAP_LUTIL_V(type) extern __reldap_exportable type
#define LDAP_AVL_F(type) __reldap_exportable type
#define LDAP_AVL_V(type) extern __reldap_exportable type
#else
#define LDAP_LUTIL_F(type) type
#define LDAP_LUTIL_V(type) extern type
#define LDAP_AVL_F(type) type
#define LDAP_AVL_V(type) extern type
#endif

/* REWRITE library */
#if defined(SLAPD_DYNAMIC) && !defined(SLAPD_IMPORT)
#define LDAP_REWRITE_F(type) __reldap_exportable type
#define LDAP_REWRITE_V(type) extern __reldap_exportable type
#else
#define LDAP_REWRITE_F(type) type
#define LDAP_REWRITE_V(type) extern type
#endif

/* SLAPD (as a dynamic library exporting symbols) */
#if defined(SLAPD_DYNAMIC) && !defined(SLAPD_IMPORT)
#define LDAP_SLAPD_F(type) __reldap_exportable type
#define LDAP_SLAPD_V(type) extern __reldap_exportable type
#else
#define LDAP_SLAPD_F(type) type
#define LDAP_SLAPD_V(type) extern type
#endif

/* RESLAPI library */
#if defined(SLAPI_LIBRARY)
#define LDAP_SLAPI_F(type) __reldap_exportable type
#define LDAP_SLAPI_V(type) extern __reldap_exportable type
#else
#define LDAP_SLAPI_F(type) type
#define LDAP_SLAPI_V(type) extern type
#endif

/* C library. */
#define LDAP_LIBC_F(type) type
#define LDAP_LIBC_V(type) extern type

#endif /* _LDAP_CDEFS_H */
