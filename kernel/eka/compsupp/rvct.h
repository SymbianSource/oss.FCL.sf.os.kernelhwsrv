// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is the preinclude file for the RVCT compiler. It contains all the compiler
// specific definitions required by the Symbian OS source code.
// 
//

/**
 @file
 @publishedAll
 @released
*/

#if defined(__PRODUCT_INCLUDE__)
#include __PRODUCT_INCLUDE__
#endif

// stuff from e32def.h
#define __NO_CLASS_CONSTS__
#define __NORETURN__ __declspec(noreturn)
#define __NORETURN_TERMINATOR()

#ifndef IMPORT_C
#define IMPORT_C __declspec(dllimport)
#endif

#ifndef EXPORT_C
#define EXPORT_C __declspec(dllexport)
#endif

#ifndef IMPORT_D
#if __ARMCC_VERSION < 310000
    #define IMPORT_D __declspec(dllexport)
#else
    #define IMPORT_D __declspec(dllimport)
#endif
#endif

#ifndef EXPORT_D
#define EXPORT_D __declspec(dllexport)
#endif

// Used by some of RVCT's header files. For more information, see "C Library ABI for
// the ARM Architecture."
#define _AEABI_PORTABILITY_LEVEL 0

/**
Declares a class as being non-sharable.

If a class is non-sharable, then a class implemented in another DLL cannot
derive (inherit) from that class.

Declaring a class as non-sharable prevents the compiler from exporting compiler
implementation-specific symbols, i.e. run-time type-information and virtual
tables. This prevents classes in other DLLs from being able to derive from it.

Note :
- if a class is marked as non-sharable, then Symbian OS requires all
classes that are derived from that class, and which are also implemented in the same DLL, 
to be declared as non-sharable.
- by default, a class is sharable.

The following code fragment shows how a non-sharable class is declared.

@code
NONSHARABLE_CLASS(CMyClass) : public CBase
{
public :
...
private :
...
}
@endcode

@param x  The name of the class to be declared as non-sharable.
*/
#define NONSHARABLE_CLASS(x)    class __declspec(notshared) x
#define NONSHARABLE_STRUCT(x)   struct __declspec(notshared) x

#define __NO_THROW throw ()
#define __THROW(t) throw (t)

#define TEMPLATE_SPECIALIZATION template<>

#ifndef __int64
#define __int64  long long
#endif

#define __VALUE_IN_REGS__ __value_in_regs

#define	I64LIT(x)	x##LL
#define	UI64LIT(x)	x##ULL

#define __SOFTFP	__softfp

// __TText from e32cmn.h also e32des16.h
#ifdef __cplusplus
typedef wchar_t __TText;	// Only ISO C++ has wchar_t as a primitive type
#define __wchar_t_defined
#else
typedef unsigned short __TText;	
#endif
#define __TText_defined

// __NAKED__ from cpudefs.h
#define __NAKED__ __asm
#define ____ONLY_USE_NAKED_IN_CIA____ __asm
#define __WEAK__  __attribute__((weak))

// Int64 and Uint64 from nkern\nklib.h
typedef long long Int64;
typedef unsigned long long Uint64;

// Here are RVCT's definitions for stdarg.h
// These should be used by e.g. stdlib

#ifdef __cplusplus
namespace std { extern "C" {
#endif

    typedef struct __va_list { void *__ap; } va_list;

#ifdef __cplusplus
} } 
using ::std::va_list;
#endif

#define va_start(ap, parmN) __va_start(ap, parmN)
#define va_arg(ap, type)    __va_arg(ap, type)
#define va_end(ap)          ((void)0)

// These are for Symbian OS C++ code
#define VA_START(ap,pn) va_start(ap, pn)
#define VA_ARG(ap,type) va_arg(ap,type)
#define VA_END(ap)      va_end(ap)
#define VA_LIST va_list
#define __VA_LIST_defined
// This should prevent /stdlib/linc/stdarg.h from doing damage.
#define _STDARG_H

// now deal with stdarg_e.h
typedef va_list __e32_va_list;
#define _STDARG_E_H

// This is an EABI compliant compiler
#ifndef __EABI__
#define __EABI__
#endif

// These are hopefully temporary.

// defining this means we don't get __NAKED__ ctors
#ifndef __EABI_CTORS__
#define __EABI_CTORS__
#endif

#ifndef __SYMBIAN_STDCPP_SUPPORT__
	#include <symcpp.h>
#endif

#ifdef __cplusplus
    // Support for throwing exceptions through embedded assembler Should only be
    // needed user side.
    
    #define	__EH_FRAME_ADDRESS(reg,offset)  FRAME ADDRESS reg, offset
    #define __EH_FRAME_PUSH2(reg1,reg2)     FRAME PUSH {reg1, reg2}
    #define __EH_FRAME_SAVE1(reg,offset)    FRAME SAVE {reg}, offset
#endif

