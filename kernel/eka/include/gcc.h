// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\include\gcc.h
// This is a preinclude file for GCC compilers. Currently only tested with GCC 3.4.4.
// 
//

/**
 @file
 @internalAll
*/

// Check this header is being used as anticipated
#ifndef __GCC32__
#error Unexpected include of GCC header
#endif
#ifndef __X86__
#error GCC header untested on non-X86 platforms
#endif

// Do product include if not done already
#ifdef PRODUCT_INCLUDE
#include PRODUCT_INCLUDE
#endif

// Visibility
#define IMPORT_C
#define IMPORT_D __attribute__((dllimport))
#define EXPORT_C __attribute__((dllexport))
#define EXPORT_D 
#ifdef GCC4_OR_LATER // this would be nice... use in conjunction with -fvisibility=hidden
#undef  EXPORT_C
#define EXPORT_C __attribute__((visibility("default")))
#endif

// Varargs
typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_end(v) __builtin_va_end(v)
#define VA_LIST va_list
#define _VA_LIST_DEFINED //To deal with stdarg.h
#define __VA_LIST_defined //To deal with e32def.h
#define VA_START(ap,pn) va_start(ap, pn)
#define VA_ARG(ap,type) va_arg(ap,type)
#define VA_END(ap)      va_end(ap)

// Stdlib/OpenEnv support
// This should prevent /stdlib/linc/stdarg.h from doing damage.
#define _STDARG_H
// now deal with stdarg_e.h
typedef va_list __e32_va_list;
#define _STDARG_E_H
// wchar_t is a primtive type for C++ on GCC
#ifdef __cplusplus
#define __wchar_t_defined
#endif

// General
#define __NO_CLASS_CONSTS__
#define	I64LIT(x)	x##LL
#define	UI64LIT(x)	x##ULL
#define __SOFTFP
#define __NORETURN__  __attribute__ ((noreturn))
#define NONSHARABLE_CLASS(x) class /*__attribute__(notshared)*/ x
#define NONSHARABLE_STRUCT(x) struct /*__attribute__(notshared)*/ x
#define __NO_THROW //throw ()
#define __THROW(t) //throw (t)
#define TEMPLATE_SPECIALIZATION template<>

#ifndef __cdecl
#define __cdecl __attribute__((cdecl))
#endif
#ifndef __fastcall
#define __fastcall __attribute__((fastcall))
#endif
#ifndef __stdcall
#define __stdcall __attribute__((stdcall))
#endif

// These are built-in defines during compilation stage but are also required (and not defined) during makmake stage
#ifndef __GNUC__
#define __GNUC__ 3
#endif

#ifndef __i386
#define __i386
#endif

