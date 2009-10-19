// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\compsupp\rvct2_0\rvct2_0.h
// This is the preinclude file for the rvct 2.0 compiler
// It contains all the compiler specific definitions required by the SOS source
// 
//


#if defined(__PRODUCT_INCLUDE__)
#include __PRODUCT_INCLUDE__
#endif


// stuff from e32def.h
/**
@publishedAll
@released
*/
#define __NO_CLASS_CONSTS__

/**
@publishedAll
@released
*/
#define __NORETURN__

/**
@publishedAll
@released
*/
#define __NORETURN_TERMINATOR()

/**
@publishedAll
@released
*/
#define IMPORT_C __declspec(dllimport) 

/**
@publishedAll
@released
*/
#define EXPORT_C __declspec(dllexport)

/**
@publishedAll
@released
*/
#define IMPORT_VT __declspec(dllimport) 

/**
@publishedAll
@released
*/
#define __NO_THROW throw ()

/**
@publishedAll
@released
*/
#define __THROW(t) throw (t)

/**
@publishedAll
@released
*/
#define TEMPLATE_SPECIALIZATION template<>

#ifndef __int64
/**
@internalComponent
*/
#define __int64  long long
#endif

/**
@internalComponent
*/
#define __VALUE_IN_REGS__ __value_in_regs

/**
@publishedAll
@released
*/
#define	I64LIT(x)	x##LL

/**
@publishedAll
@released
*/
#define	UI64LIT(x)	x##ULL

// __TText from e32cmn.h also e32des16.h
#ifdef __cplusplus
/**
@internalComponent
*/
typedef wchar_t __TText;	// Only ISO C++ has wchar_t as a primitive type

/**
@internalComponent
*/
#define __wchar_t_defined
#else
/**
@internalComponent
*/
typedef unsigned short __TText;	
#endif

/**
@internalComponent
*/
#define __TText_defined

// __NAKED__ from cpudefs.h
/**
@publishedAll
@released
*/
#define __NAKED__ __asm

/**
@internalComponent
*/
#define ____ONLY_USE_NAKED_IN_CIA____ __asm

// Int64 and Uint64 from nkern\nklib.h
/**
@publishedAll
@released
*/
typedef long long Int64;

/**
@publishedAll
@released
*/
typedef unsigned long long Uint64;

// Here are RVCT 2.0's definitions for stdarg.h
// These should be used by e.g. stdlib

// see if we're using the BETA B compiler
#if (__ARMCC_VERSION == 200022)
#define RVCTBETA
#endif

#ifdef __cplusplus
    namespace std {
        extern "C" {
#endif  /* __cplusplus */

#ifdef RVCTBETA
/**
@internalComponent
*/
	  typedef int *va_list[1];
#else
/**
@internalComponent
*/
	  typedef struct __va_list { void *__ap; } va_list;
#endif

#ifdef __cplusplus
	}  /* extern "C" */
    }  /* namespace std */

    using ::std::va_list;
#endif

/**
@internalComponent
*/
#define va_start(ap, parmN) __va_start(ap, parmN)

/**
@internalComponent
*/
#define va_arg(ap, type) __va_arg(ap, type)

/**
@internalComponent
*/
#define va_end(ap) ((void)0)

// These are for Symbian OS C++ code
/**
@publishedAll
@released
*/
#define VA_START(ap,pn) va_start(ap, pn)

/**
@publishedAll
@released
*/
#define VA_ARG(ap,type) va_arg(ap,type)

/**
@publishedAll
@released
*/
#define VA_END(ap)      va_end(ap)

/**
@publishedAll
@released
*/
#define VA_LIST va_list

/**
@internalComponent
*/
#define __VA_LIST_defined
// This should prevent /stdlib/linc/stdarg.h from doing damage.
#define _STDARG_H

// now deal with stdarg_e.h
/**
@internalComponent
*/
typedef va_list __e32_va_list;

/**
@internalComponent
*/
#define _STDARG_E_H

// This is an EABI compliant compiler
#ifndef __EABI__
/**
@publishedAll
@released
*/
#define __EABI__
#endif

// these are hopefully temporary

// defining this means we don't get __NAKED__ ctors
#ifndef __EABI_CTORS__
/**
@publishedAll
@released
*/
#define __EABI_CTORS__
#endif

//#define __EARLY_DEBUG__

