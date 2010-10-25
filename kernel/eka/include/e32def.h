/*
* Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* e32\include\e32def.h
* NOTE: THIS FILE SHOULD BE ACCEPTABLE TO A C COMPILER
* 
*
*/



#ifndef __E32DEF_H__
#define __E32DEF_H__

/*
 * __LEAVE_EQUALS_THROW__ requires the compiler to support C++ exceptions
 */
#ifndef __SUPPORT_CPP_EXCEPTIONS__
#undef __LEAVE_EQUALS_THROW__
#endif


#if defined(__VC32__)
/**
@publishedAll
@released
*/
#define __NO_CLASS_CONSTS__
#if (_MSC_VER >= 1200)
/**
@publishedAll
@released
*/
#define __NORETURN__ __declspec(noreturn)
#else
#define __NORETURN__
#endif
/**
@publishedAll
@released
*/
#define __NORETURN_TERMINATOR()
/**
@publishedAll
@released
*/
#define IMPORT_C __declspec(dllexport)
/**
@publishedAll
@released
*/
#define EXPORT_C __declspec(dllexport)
/**
@publishedAll
@released
*/
#define IMPORT_D __declspec(dllexport)
/**
@publishedAll
@released
*/
#define EXPORT_D __declspec(dllexport)
/**
@publishedAll
@released
*/
#define NONSHARABLE_CLASS(x) class x
/**
@publishedAll
@released
*/
#define NONSHARABLE_STRUCT(x) struct x
/**
@publishedAll
@released
*/
#define __NO_THROW throw()
/**
@publishedAll
@released
*/
#define __THROW(t) throw(t)
#pragma warning( disable : 4355 )	/* 'this' used in base member initializer list */
#pragma warning( disable : 4511 )	/* copy constructor could not be generated */
#pragma warning( disable : 4512 )	/* assignment operator could not be generated */
#pragma warning( disable : 4514 )	/* unreferenced inline function has been removed */
#pragma warning( disable : 4699 )	/* Note: Using precompiled header %s */
#pragma warning( disable : 4710 )	/* function not inlined */
#pragma warning( disable : 4121 )	/* alignment sensitive to packing */
#pragma warning( disable : 4273 )
#pragma warning( disable : 4097 )	/* typedef-name 'identifier1' used as synonym for class-name 'identifier2' */
#pragma warning( disable : 4291 )	/* 'TAny *CBase::operator new(TUint,TLeave)' : no matching operator delete found; memory will not be freed if initialization throws an exception */

#if _MSC_VER  >= 1100
/**
@publishedAll
@released
*/
#define TEMPLATE_SPECIALIZATION template<>
#else
#define TEMPLATE_SPECIALIZATION
#endif
#endif



#if defined(__CW32__)
#undef __embedded_cplusplus
/** @internalTechnology */
#define __embedded_cplusplus	1
#define __NO_CLASS_CONSTS__
#define __NORETURN__
#define __NORETURN_TERMINATOR()
#define IMPORT_C __declspec(dllexport)
#define EXPORT_C __declspec(dllexport)
#define IMPORT_D __declspec(dllexport)
#define EXPORT_D __declspec(dllexport)
#define NONSHARABLE_CLASS(x) class x
#define NONSHARABLE_STRUCT(x) struct x
#define __NO_THROW throw()
#define __THROW(t) throw(t)

#define TEMPLATE_SPECIALIZATION template<>
/**
@publishedAll
@released
*/
#define _asm	asm
#ifndef __int64
#pragma longlong on
/** @internalTechnology */
#define __int64  long long
#endif
#ifndef __SUPPORT_CPP_EXCEPTIONS__
#pragma exceptions off    /* no support for C++ exception handling */
#pragma RTTI off          /* no support for C++ runtime type information */
#endif
#if __MWERKS__ >= 0x3200
#pragma warning off (10480)	/* deleteing void pointer is undefined */
#pragma warning off (10350) /* N pad byte(s) inserted after data member */
#endif
#endif


//
// GCC (ARM) compiler
//
#if defined(__GCC32__) && defined(__MARM__)
#ifndef __GNUC__		/* GCC98r2 doesn't define this for some reason */
#define __GNUC__	2
#endif
#define __NO_CLASS_CONSTS__
#define __NORETURN__  __attribute__ ((noreturn))
#ifdef __GCCV3__
#define __NORETURN_TERMINATOR()
#else
#define __NORETURN_TERMINATOR()		abort()
#endif
#define IMPORT_C
#define IMPORT_D
#if !defined __WINS__ && defined _WIN32 /* VC++ Browser Hack */
#define EXPORT_C
#define EXPORT_D
/** @internalTechnology */
#define asm(x)
#else
#define EXPORT_C __declspec(dllexport)
#define EXPORT_D __declspec(dllexport)
#endif
#define NONSHARABLE_CLASS(x) class x
#define NONSHARABLE_STRUCT(x) struct x
#define __NO_THROW
#define __THROW(t)
#ifdef __EABI__
#define TEMPLATE_SPECIALIZATION template<>
#else
#define TEMPLATE_SPECIALIZATION
#endif
/**
@publishedAll
@released
*/
#define __DOUBLE_WORDS_SWAPPED__
#endif


/** @internalTechnology */
#define __NO_MUTABLE_KEYWORD
#if defined(__NO_MUTABLE_KEYWORD)
/**
@publishedAll
@deprecated
*/
#define __MUTABLE
#else
#define __MUTABLE mutable
#endif



/**
@publishedAll
@deprecated
*/
#define CONST_CAST(type,exp) (const_cast<type>(exp))

/**
@publishedAll
@deprecated
*/
#define STATIC_CAST(type,exp) (static_cast<type>(exp))

/**
@publishedAll
@deprecated
*/
#define REINTERPRET_CAST(type,exp) (reinterpret_cast<type>(exp))

#if defined(__NO_MUTABLE_KEYWORD)
/**
@publishedAll
@deprecated
*/
#define MUTABLE_CAST(type,exp) (const_cast<type>(exp))
#else
#define MUTABLE_CAST(type,exp) (exp)
#endif

/**
@publishedAll
@deprecated
*/
#define GLREF_D extern
/**
@publishedAll
@deprecated
*/
#define GLDEF_D
/**
@publishedAll
@deprecated
*/
#define LOCAL_D static
/**
@publishedAll
@deprecated
*/
#define GLREF_C extern
/**
@publishedAll
@deprecated
*/
#define GLDEF_C
/**
@publishedAll
@deprecated
*/
#define LOCAL_C static
/**
@internalAll
@prototype
*/
#ifndef IMPORT_D
#define IMPORT_D IMPORT_C 
#endif

/**
@publishedAll
@deprecated
*/
#define FOREVER for(;;)




/**
@publishedAll
@released

Symbolic definition for a true value.
*/
#define TRUE 1




/**
@publishedAll
@released

Symbolic definition for a false value.
*/
#define FALSE 0
#ifndef NULL




/**
@publishedAll
@released

Symbolic definition for a NULL value.
*/
#define NULL 0
#endif




#ifndef VA_START
/**
@publishedAll
@released

A macro used by Symbian OS code for handling a variable argument list
in a function call.

Sets a pointer to point to the first of the variable arguments.

Typical usage:

@code
Foo(CAbcdef aAbcdef,...)
   {
   VA_LIST list;
   VA_START(list, aAbcdef);
   // other code
   } 
@endcode

@param ap   A pointer used to hold the address of an argument in
            the variable argument list. After execution of the code generated 
            by this macro, the pointer points to the first argument in
            the variable argument list.
            This symbol is usually declared as a VA_LIST type. 

@param pn   The argument that immediately precedes the variable argument list.

@see VA_LIST
@see VA_ARG
*/
#define VA_START(ap,pn) ((ap)[0]=(TInt8 *)&pn+((sizeof(pn)+sizeof(TInt)-1)&~(sizeof(TInt)-1)),(void)0)
#endif




#ifndef VA_ARG
/**
@publishedAll
@released

A macro used by Symbian OS code for handling a variable argument list
in a function call.

Increments a pointer to a variable argument list to point to the next argument
in the list. The current argument is assumed to be of a type defined by
the second parameter to this macro.

Typical usage:

@code
Foo(CAbcdef aAbcdef,...)
   {
   VA_LIST list;
   VA_START(list, aAbcdef);
   ...
   TInt x = VA_ARG(list,TInt);
   ...
   const TDesC *pS=VA_ARG(aList,const TDesC*);
   ... 
   etc
   } 
@endcode

@param ap   A pointer used to hold the address of an argument in
            the variable argument list. It is assumed to point to the current
            argument in the variable argument list. After execution of the code
            generated by this macro, the pointer points to the next argument in
            the list. This symbol is usually declared as a VA_LIST type. 

@param type The type of the current argument.
            This can be any valid type, for example, TInt, const TDesC*, etc.
            
@see VA_LIST
@see VA_START            
*/
#define VA_ARG(ap,type) ((ap)[0]+=((sizeof(type)+sizeof(TInt)-1)&~(sizeof(TInt)-1)),(*(type *)((ap)[0]-((sizeof(type)+sizeof(TInt)-1)&~(sizeof(TInt)-1)))))
#endif




#ifndef VA_END
/**
@publishedAll
@released

A macro used by Symbian OS code for handling a variable argument list
in a function call.

Sets a pointer to zero.

@param ap   A pointer used to hold the address of an argument in
            the variable argument list. After execution of the code generated 
            by this macro, the pointer is reset to 0.
            This symbol is usually declared as a VA_LIST type. 
            
@see VA_LIST
@see VA_START
@see VA_ARG            
*/
#define VA_END(ap) ((ap)[0]=0,(void)0)
#endif
	


/**
@publishedAll
@released

Calculates the offset of member f within class c.

This is used in the TSglQue and TDblQue constructors to set the offset of
the link object from the start of a list element.

@param c The name of the class.
@param f The name of the member within the specified class.

@see TSglQue
@see TDblQue
*/
#ifndef _FOFF
#if __GNUC__ < 4
#define _FOFF(c,f)			(((TInt)&(((c *)0x1000)->f))-0x1000)
#else
#define _FOFF(c,f)			(__builtin_offsetof(c,f))
#endif
#endif



/**
@internalTechnology
@released
*/
#define _ALIGN_DOWN(x,a)	((x)&~((a)-1))
/**
@internalTechnology
@released
*/
#define _ALIGN_UP(x,a)		_ALIGN_DOWN((x)+(a)-1, a)




/** 
@publishedAll
@released

Pointer to any type.

TAny* is equivalent to void* in standard C or C++. TAny* is used in preference 
to void* because it is more suggestive of the actual meaning,
e.g. TAny* foo();.

TAny is not used where it really means "nothing", as in the declaration of 
functions which do not return a value; void is used instead, e.g. void Foo();.
*/
typedef void TAny;




/**
@publishedAll
@released

8-bit signed integer type, used in Symbian OS to mean an 8-bit
signed integer, independent of the implementation.
*/
typedef signed char TInt8;




/**
@publishedAll
@released

8-bit unsigned integer type; used in Symbian OS to mean an 8-bit
unsigned integer, independent of the implementation.
*/
typedef unsigned char TUint8;




/**
@publishedAll
@released

16-bit signed integer type, used in Symbian OS to mean a 16-bit
signed integer, independent of the implementation.
*/
typedef short int TInt16;




/**
@publishedAll
@released

16-bit unsigned integer type. used in Symbian OS to mean a 16-bit
unsigned integer, independent of the implementation.
*/
typedef unsigned short int TUint16;




/**
@publishedAll
@released

32-bit signed integer type, used in Symbian OS to mean a 32-bit
signed integer, independent of the implementation.
*/
typedef long int TInt32;




/**
@publishedAll
@released

A signed integer type of the same size as a pointer.
*/
typedef TInt32 T_IntPtr;
typedef TInt32 TIntPtr;




/**
@publishedAll
@released

32-bit unsigned integer type; used in Symbian OS to mean a 32-bit
unsigned integer, independent of the implementation.
*/
typedef unsigned long int TUint32;




/**
@publishedAll
@released

An unsigned integer type of the same size as a pointer.
*/
typedef TUint32 T_UintPtr;
typedef TUint32 TUintPtr;




/**
@publishedAll
@released

Signed integer type of the natural machine word length.

This is as defined by the C++ implementation's int type. In all
implementations, this is guaranteed to be at least 32 bits.

A TInt should be used in preference to a sized integer (TInt32, TInt16) for 
all general use. Sized integers should only be used when packing is essential. 
C++'s type conversion rules imply that all sized integers smaller than the 
natural machine word are in any case broadened to the natural machine word 
size when passed as function parameters.

A TInt should be used in preference to an unsigned integer (TUint) for all 
general use. Unsigned integers should only be used for flags (which use Boolean 
operations but not arithmetic) and, in very rare cases, for numbers whose 
range exceeds that available from signed integers. Although it is natural 
to attempt to use unsigned integers for quantities which cannot by nature 
be negative, the C++ language does not provide the support necessary to enforce 
the "expected" behaviour in these circumstances, and experience has shown 
that it is better to use signed integers unless there is good reason not to.

@see TUint
@see TInt32
@see TInt16
*/
typedef signed int TInt;




/**
@publishedAll
@released

Unsigned integer type of the natural machine word length. 

This is guaranteed to be at least 32 bits in all implementations.

In almost all circumstances, a TInt should be used in preference to a TUint. 
The main exception is in flags bytes.

@see TInt
*/
typedef unsigned int TUint;




/**
@publishedAll
@released

32-bit floating point number, providing IEEE754 single precision on all Symbian 
OS implementations.

TReal should normally be used in preference to TReal32.

Use of floating-point numbers should generally be avoided unless a natural 
part of the problem specification. Most Symbian OS implementations do not 
have a hardware floating point unit: as a result, their floating-point performance 
is hundreds of times slower than integer performance.
*/
typedef float TReal32;




/**
@publishedAll
@released

64-bit floating point number, providing IEEE754 double precision on all Symbian 
OS implementations.

Use of floating-point numbers should generally be avoided unless a natural 
part of the problem specification. Most Symbian OS implementations do not 
have a hardware floating point unit: as a result, their floating-point performance 
is hundreds of times slower than integer performance.

This type is identical to TReal.

@see TReal
*/
typedef double TReal64;




/**
@publishedAll
@released

64-bit floating point number; identical to TReal64.

Use of floating-point numbers should generally be avoided unless a natural 
part of the problem specification. Most Symbian OS implementations do not 
have a hardware floating point unit: as a result, their floating-point performance 
is hundreds of times slower than integer performance.

Most serious floating-point calculations require double-precision. All standard 
math functions (see Math class) take double-precision arguments. Single-precision 
should only be used where space and performance are at a premium, and when 
their limited precision is acceptable.

@see TReal64
@see Math
*/
typedef double TReal;




/**
@publishedAll
@released

8-bit unsigned character.

Use instead of C++ built-in char type because it is guaranteed to be unsigned. 
Use instead of TInt8 where the application is really for text rather than 
8-bit arithmetic or binary quantities.

For most purposes, you should use TText rather than TText8. TText is mapped 
onto either TText8 or TText16 depending on whether a non-Unicode or Unicode 
variant is being built. Use TText8 only when you are dealing explicitly with 
8-bit text, regardless of build.

@see TText */
typedef unsigned char TText8;




/**
@publishedAll
@released

16-bit unsigned character.

Use instead of C++ wchar_t type because it is guaranteed to be unsigned. Use 
instead of TInt16 where the application is really for text rather than 8-bit 
arithmetic or binary quantities.

For most purposes, you should use TText rather than TText16. TText is mapped 
onto either TText8 or TText16 depending on whether a non-Unicode or Unicode 
variant is being built. Use TText16 only when you are dealing explicitly with 
16-bit text, regardless of build.

@see TText
*/
typedef unsigned short int TText16;




/**
@publishedAll
@released

Boolean type which takes the value either ETrue or EFalse.

Although only a single bit would theoretically be necessary to represent a 
Boolean, a machine word is used instead, so that these quantities can be easily 
passed. Also, TBool must map onto int because of C++'s interpretation of 
operands in conditional expressions.
*/
typedef int TBool;




/**
@publishedPartner
@released

Defines a linear (virtual) address type.
*/
typedef T_UintPtr TLinAddr;



#if defined(__GCC32__)




/**
@publishedAll
@released

Defines a 64-bit signed integer type.
*/
typedef long long Int64;




/**
@publishedAll
@released

Defines a 64-bit unsigned integer type.
*/
typedef unsigned long long Uint64;




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

#elif defined(__VC32__)
typedef __int64 Int64;
typedef unsigned __int64 Uint64;
#define	I64LIT(x)	(__int64)##x
#define	UI64LIT(x)	(unsigned __int64)##x

#elif defined(__CW32__)
#pragma longlong on
typedef long long Int64;
typedef unsigned long long Uint64;
#define	I64LIT(x)	x##LL
#define	UI64LIT(x)	x##ULL
#endif




/**
@publishedAll
@released

Defines a 64-bit signed integer type.

NOTE: For those migrating from versions of Symbian OS before 8.1b (i.e. 8.1a, 7.0s etc)
TInt64 is now defined as a built-in type instead of as a class type. This means
that the member functions of the old TInt64 class are no longer exported
from EUSER.LIB, and represents a compatibility break.

To ease migration of source code, a number of macros are provided. Similar
macros have also been defined in Symbian OS versions 7.0s and 8.1a, but
implemented in terms of the old TInt64 class. This is important for code that
is common to : one or both of these Symbian OS versions, and to 8.1b and
subsequent versions.

The following list shows the new macros and the functions that they replace.
It also shows some alternative techniques.
In this list: x, v and r are declared as TInt64, c is declared as TInt, High
and Low are declared as TUint.

@code
OLD USAGE						REPLACEMENT

TInt64(High,Low);				MAKE_TINT64(High,Low);
x.Set(High,Low);				MAKE_TINT64(High,Low);
x.Low();						I64LOW(x);
x.High();						I64HIGH(x); 
x.GetTInt();					I64INT(x); 
x.GetTReal();					I64REAL(x); 
x.Lsr(c);						I64LSR(x,c); 
x.Mul10();						x*=10; 
x.MulTop(a);					I64MULTOP(x,a); 
x.DivMod(v,r);					r=x%v; x/=v;
@endcode 
*/
typedef	Int64	TInt64;




/**
@publishedAll
@released
 
Defines a 64-bit unsigned integer type.
*/
typedef	Uint64	TUint64;




/** @internalComponent */
#define _MAKE_TINT64_ZX(x)	((TInt64)((TUint32)(x)))

/** @internalComponent */
#define _MAKE_TUINT64_ZX(x)	((TUint64)((TUint32)(x)))




/**
@publishedAll
@released
*/
#define MAKE_TINT64(h,l)	( (_MAKE_TINT64_ZX(h)<<32) | _MAKE_TINT64_ZX(l) )




/**
@publishedAll
@released
*/
#define MAKE_TUINT64(h,l)	( (_MAKE_TUINT64_ZX(h)<<32) | _MAKE_TUINT64_ZX(l) )




/**
@publishedAll
@released

Generates code to access the high order 32 bits of a 64 bit number.
*/
#define	I64HIGH(x)			( (TUint32)((x)>>32) )




/**
@publishedAll
@released

Generates code to access the low order 32 bits of a 64 bit number.
*/
#define	I64LOW(x)			( (TUint32)(x) )




/**
@publishedAll
@released

Generates code to cast a 64 bit value as an signed integer.
*/
#define	I64INT(x)			( (TInt)(x) )




/**
@publishedAll
@released

Generates code to cast a 64 bit value as a TReal type.
*/
#define	I64REAL(x)			( (TReal)(x) )




/**
@publishedAll
@released

Generates code to logically shift a 64 bit integer right.
*/
#define	I64LSR(x, shift)	( *reinterpret_cast<TUint64*>(&(x)) >>= (shift) )



/**
@publishedAll
@released

Generates code to multiply a 64 bit integer by 10.
*/
#define	I64MUL10(x)			( (x) *= 10 )



/**
@publishedAll
@released

Generates code to divide a 64 bit integer by another and find the remainder.
*/
#define	I64DIVMOD(x, divisor, remainder)	( ((remainder) = (x) % (divisor), (x) /= (divisor)) )




/**
@publishedAll
@released

Generates code to cast a double to a 64 bit integer.
*/
#define	I64DOUBLECAST(x)	( static_cast<TInt64>(x) )




/**
@publishedAll
@deprecated Use _LIT8 instead.

8-bit literal.

The macro defines an explicit 8-bit constant literal which is suitable
for non-Unicode literal text, regardless of the build.

@see _L
@see _LIT8
@see _LIT
*/
#define _L8(a) (TPtrC8((const TText8 *)(a)))




/**
@publishedAll
@released

Defines an explicit 8-bit string which is suitable when non-Unicode text
is required, regardless of the build.

This is used by the deprecated literal descriptor _L8.
*/
#define _S8(a) ((const TText8 *)a)




/**
@publishedAll
@released

Constructs a constant literal descriptor of type TLitC8<TInt> with
the specified name and text.

The 8-bit build variant is generated for both non-Unicode and Unicode builds.

@param name The name of the C++ variable to be generated.
@param s    The literal text enclosed within a pair of double quotes. 

@see _LIT
*/
#define _LIT8(name,s) const static TLitC8<sizeof(s)> name={sizeof(s)-1,s}




/**
@publishedAll
@deprecated Use _LIT16 instead.

16-bit literal.

The macro defines an explicit 16-bit constant literal which is suitable
for Unicode literal text, regardless of the build.

@see _L
@see _LIT16
@see _LIT
*/
#define _L16(a) (TPtrC16((const TText16 *)L ## a))



/**
@publishedAll
@released

Defines an explicit 16-bit string which is suitable when Unicode text
is required, regardless of the build.

This is used by the deprecated literal descriptor _L16.
*/
#define _S16(a) ((const TText16 *)L ## a)




/**
@publishedAll
@released

Constructs a constant literal descriptor of type TLitC16<TInt> with
the specified name and text.

The 16-bit build variant is generated for both non-Unicode and Unicode builds.

@param name The name of the C++ variable to be generated.
@param s    The literal text enclosed within a pair of double quotes. 

@see _LIT
*/
#define _LIT16(name,s) const static TLitC16<sizeof(L##s)/2> name={sizeof(L##s)/2-1,L##s}




#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
/**
@publishedAll
@released

Build independent general text character.

In non-Unicode builds, this is mapped to TText8. In Unicode builds, this is 
mapped to TText16. Use the classes with explicit width only when you wish 
the width to be independent of the build variant.

Use this class rather than TChar for general use.
*/
typedef TText16 TText;



/**
@publishedAll
@deprecated Use _LIT instead.

Build independent literal. 

The macro defines either an 8-bit constant literal (for non-Unicode text),
or a 16-bit constant literal (for Unicode text) depending on the build.

@see _LIT
@see _L16
@see _L8
*/
#define _L(a) (TPtrC((const TText *)L ## a))




/**
@publishedAll
@released

Defines either an 8-bit string (for non-Unicode text),
or a 16-bit string (for Unicode text) depending on the build.

This is used by the deprecated build independent literal _L.
*/
#define _S(a) ((const TText *)L ## a)




/**
@publishedAll
@released

Constructs a build independent constant literal descriptor of type TLitC<TInt>
with the specified name and text.

An 8-bit build variant is generated for a non-Unicode build;
A 16-bit build variant is generated for a Unicode build.

@param name The name of the C++ variable to be generated.
@param s    The literal text enclosed within a pair of double quotes. 

@see _LIT16
@see _LIT8
*/
#define _LIT(name,s) const static TLitC<sizeof(L##s)/2> name={sizeof(L##s)/2-1,L##s}




#else
/**
@publishedAll
@released

Build independent general text character.

In non-Unicode builds, this is mapped to TText8. In Unicode builds, this is 
mapped to TText16. Use the classes with explicit width only when you wish 
the width to be independent of the build variant.

Use this class rather than TChar for general use.
*/
typedef TText8 TText;



/**
@publishedAll
@released

@deprecated Use _LIT instead.

Build independent literal. 

The macro defines either an 8-bit constant literal (for non-Unicode text),
or a 16-bit constant literal (for Unicode text) depending on the build.

@see _LIT
@see _L16
@see _L8
*/
#define _L(a) (TPtrC((const TText *)(a)))




/**
@publishedAll
@released

Defines either an 8-bit string (for non-Unicode text),
or a 16-bit string (for Unicode text) depending on the build.

This is used by the deprecated build independent literal _L.
*/
#define _S(a) ((const TText *)a)




/**
@publishedAll
@released

Constructs a build independent constant literal descriptor of type TLitC<TInt>
with the specified name and text.

An 8-bit build variant is generated for a non-Unicode build;
A 16-bit build variant is generated for a Unicode build.

@param name The name of the C++ variable to be generated.
@param s    The literal text enclosed within a pair of double quotes. 

@see _LIT16
@see _LIT8
*/
#define _LIT(name,s) const static TLitC<sizeof(s)> name={sizeof(s)-1,s}
#endif




#ifndef __VA_LIST_defined
/** 
@publishedAll
@released

Defines a 'C' style array of pointers to TInt8 types.

The type is most commonly used by code that needs to deal with a variable
number of arguments passed to a function.

@see TInt8
*/
typedef TInt8 *VA_LIST[1];
#endif

/** 
@publishedAll
@released

Asserts that a condition is true.

Code is generated for all builds.

This macro is used as a C++ statement to assert the truth of some condition,
and to take appropriate action if the condition is false. Unlike __ASSERT_DEBUG
it is defined in both release and debug builds.

The most common use for this macro is to check that the external environment of
a function or class is behaving as expected; for example, that parameters
passed to a function are credible, or that called functions are behaving as
expected; the macro is commonly placed at the beginning of a function.

The effect of the macro is to generate code which tests
the conditional expression c; if the expression is false, then
function p is called. In the majority of cases, the function p is one that
raises a panic.

Note that the macro definition is, in effect, equivalent to: 

@code
if !(c)p;
@endcode

@param c a conditional expression which results in true or false.
@param p a function which is called if the conditional expression c is false.

@see __ASSERT_DEBUG
*/
#define __ASSERT_ALWAYS(c,p) (void)((c)||(p,0))



#ifdef __WINS__
#ifdef __CW32__
/** 
@internalAll
@released
*/
#define __BREAKPOINT()			\
	{							\
	__asm { byte 0xcc };		\
	}
#else // !__CW32__
/** 
@internalAll
@released
*/
#define __BREAKPOINT()			\
	{							\
	__asm { int 3 };			\
	}
#endif //__CW32__
#else
/** 
@internalAll
@released
*/
#define __BREAKPOINT()
#endif

#if defined(_DEBUG)


/** 
@publishedAll
@released

Asserts that a condition is true.

Code is generated for debug builds only.

This macro is used as a C++ statement to assert the truth of some condition,
and to take appropriate action if the condition is false. It is used in
the same way as __ASSERT_ALWAYS, except that it is only defined for debug builds.

The macro may be used to insert extra checks at various points in source code
as desired; the code will only be generated in debug builds and not in release
builds.

@param c A conditional expression which results in true or false.
@param p A function which is called if the conditional expression c is false.

@see __ASSERT_ALWAYS
*/
#define __ASSERT_DEBUG(c,p) (void)((c)||(p,0))



/** 
@internalAll
@removed
*/
#define __DECLARE_NAME(t)




/** 
@publishedAll
@released

Calls the function for testing object invariance.

Classes can define a standard member function __DbgTestInvariant(),
which checks that the object is in a valid state, and panics if it is not.
In debug builds, this macro simply expands to call that function. For details on how
to define __DbgTestInvariant(), and an example of its use, see __DECLARE_TEST.

The macro is typically invoked at the beginning of all the member functions of
the class. For non-const functions (those which can change the object’s state),
you can ensure that the object has been left in a stable state by invoking
the macro at the end of the function.

In release builds, no code is generated for the macro. 
*/
#define __TEST_INVARIANT __DbgTestInvariant()




/**
@publishedAll
@released

Marks the start of checking the current thread's heap. 

This macro is defined only for debug builds.

This macro must be matched by a corresponding call to __UHEAP_MARKEND or __UHEAP_MARKENDC.

Calls to this macro can be nested but each call must be matched by corresponding 
call to __UHEAP_MARKEND or __UHEAP_MARKENDC.

@see User::__DbgMarkStart()
@see __UHEAP_MARKEND
@see __UHEAP_MARKENDC
*/
#define __UHEAP_MARK User::__DbgMarkStart(FALSE)




/**
@publishedAll
@released

Checks that the number of allocated cells at the current nested level on the 
current thread's heap is the same as the specified value.

This macro is defined only for debug builds.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

The macro assumes that:

1. the heap being checked is a user heap

2. checking is being done for the number of allocated cells at the current nested 
   level; i.e. that aCountAll is set to false

3. the line number is the line number of this source code statement.

4. the file name is the full path name of the file containing this source statement

@param aCount The number of heap cells expected to be allocated at
              the current nest level.

@see User::__DbgMarkCheck()
@see __KHEAP_CHECK
*/
#define __UHEAP_CHECK(aCount) User::__DbgMarkCheck(FALSE,FALSE,aCount,(TText8*)__FILE__,__LINE__)




/**
@publishedAll
@released

Checks that the total number of allocated cells on the current thread's heap 
is the same as the specified value.

This macro is defined only for debug builds.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

@param aCount The total number of heap cells expected to be allocated.

@see User::__DbgMarkCheck()
@see __KHEAP_CHECKALL
*/
#define __UHEAP_CHECKALL(aCount) User::__DbgMarkCheck(FALSE,TRUE,aCount,(TText8*)__FILE__,__LINE__)




/**
@publishedAll
@released

Marks the end of checking the current thread's heap. 

The macro expects zero heap cells to remain allocated at the current nest 
level. This macro is defined only for debug builds.

This macro must match an earlier call to __UHEAP_MARK.

@see User::__DbgMarkEnd()
@see __UHEAP_MARK
*/
#define __UHEAP_MARKEND User::__DbgMarkEnd(FALSE,0)




/**
@publishedAll
@released

Marks the end of checking the current thread's heap. 

The macro expects aCount heap cells to remain allocated at the current nest 
level.

This macro must match an earlier call to __UHEAP_MARK.

@param aCount The number of heap cells expected to remain allocated at
              the current nest level.

@see User::__DbgMarkEnd()
@see __UHEAP_MARK
*/
#define __UHEAP_MARKENDC(aCount) User::__DbgMarkEnd(FALSE,aCount)




/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap.

The failure occurs on the next call to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aCount Determines when the allocation will fail.
              Heap allocation fails on attempt number aCount - later
              allocations will succeed.
              For example, if aCount is 3, then heap allocation fails
              on the 3rd attempt, but all subsequent allocations succeed. 

@see User::__DbgSetAllocFail()
*/
#define __UHEAP_FAILNEXT(aCount) User::__DbgSetAllocFail(FALSE,RAllocator::EFailNext,aCount)

/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap.

The failures will occur for aBurst times from the next call to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aCount Determines when the allocation will fail.
              Heap allocation fails on attempt number aCount - later
              allocations will succeed.
              For example, if aCount is 3, then heap allocation fails
              on the 3rd attempt, but all subsequent allocations succeed.  
              Note when used with RHeap the maximum value aCount can be set 
              to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set to 
              is KMaxTUint16.

@see User::__DbgSetBurstAllocFail()
*/
#define __UHEAP_BURSTFAILNEXT(aCount,aBurst) User::__DbgSetBurstAllocFail(FALSE,RAllocator::EBurstFailNext,aCount,aBurst)



/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate The failure rate.

@see User::__DbgSetAllocFail()
@see RAllocator::TAllocFail
*/
#define __UHEAP_SETFAIL(aType,aRate) User::__DbgSetAllocFail(FALSE, aType, aRate)

/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate  The failure rate.  Note when used with RHeap the maximum value 
              aRate can be set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.    Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see User::__DbgSetBurstAllocFail()
@see RAllocator::TAllocFail
*/
#define __UHEAP_SETBURSTFAIL(aType,aRate,aBurst) User::__DbgSetBurstAllocFail(FALSE, aType, aRate, aBurst)



/**
@publishedAll
@released

Cancels simulated heap allocation failure for the current thread's heap. 

This macro is defined only for debug builds.

@see User::__DbgSetAllocFail()
*/
#define __UHEAP_RESET User::__DbgSetAllocFail(FALSE,RAllocator::ENone,1)


/**
@publishedAll
@released

Cancels simulated heap allocation failure for the current thread's heap. 
It walks the the heap and sets the nesting level for all allocated
cells to zero.

This macro is defined only for debug builds.
*/
#define __UHEAP_TOTAL_RESET User::__DbgSetAllocFail(FALSE,RAllocator::EReset,1)

/**
@publishedAll
@released

Returns the number of heap allocation failures the current debug allocator fail
function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
#define __UHEAP_CHECKFAILURE User::__DbgCheckFailure(FALSE)

/**
@publishedAll
@released

Returns the number of kernel heap allocation failures the current debug 
allocator fail function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
#define __KHEAP_CHECKFAILURE User::__DbgCheckFailure(TRUE)



/**
@publishedAll
@released

Marks the start of heap checking for the specific heap. 

This macro is defined only for debug builds.

This macro must be matched by a corresponding call to __RHEAP_MARKEND or __RHEAP_MARKENDC.

Calls to this macro can be nested but each call must be matched by corresponding 
call to __RHEAP_MARKEND or __RHEAP_MARKENDC.

@param aHeap A pointer to the specific RHeap

@see RHeap
@see RAllocator::__DbgMarkStart()
@see __RHEAP_MARKEND
@see __RHEAP_MARKENDC
*/
#define __RHEAP_MARK(aHeap) (aHeap)->__DbgMarkStart()




/**
@publishedAll
@released

Checks that the number of allocated cells at the current nested level on the 
specified heap is the same as the specified value. 

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail. 

This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The number of heap cells expected to be allocated at
              the current nest level.

@see RAllocator::__DbgMarkCheck()
*/
#define __RHEAP_CHECK(aHeap,aCount) (aHeap)->__DbgMarkCheck(FALSE,aCount,(TText8*)__FILE__,__LINE__)




/**
@publishedAll
@released

Checks that the total number of allocated cells on the specified heap is the 
same as the specified value.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The total number of heap cells expected to be allocated.

@see RAllocator::__DbgMarkCheck()
*/
#define __RHEAP_CHECKALL(aHeap,aCount) (aHeap)->__DbgMarkCheck(TRUE,aCount,(TText8*)__FILE__,__LINE__)




/**
@publishedAll
@released

Marks the end of heap checking for the specific heap.

The macro expects zero heap cells to remain allocated at the current nest 
level. This macro is defined only for debug builds.

This macro must match an earlier call to __RHEAP_MARK.

@param aHeap A pointer to the specific RHeap.

@see RAllocator::__DbgMarkEnd()
@see __RHEAP_MARK
*/
#define __RHEAP_MARKEND(aHeap) (aHeap)->__DbgMarkEnd(0)




/**
@publishedAll
@released

Marks the end of heap checking for the specific heap.

The macro expects aCount heap cells to remain allocated at the current nest 
level. This macro is defined only for debug builds.

This macro must match an earlier call to __RHEAP_MARK.

@param aHeap  A pointer to the specific RHeap.
@param aCount The number of heap cells expected to remain allocated at
              the current nest level

@see RAllocator::__DbgMarkEnd()
@see __RHEAP_MARK
*/
#define __RHEAP_MARKENDC(aHeap,aCount) (aHeap)->__DbgMarkEnd(aCount)




/**
@publishedAll
@released

Simulates an allocation failure for the specific heap.

The failure occurs on the next call to new or any of the functions which allocate 
memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The rate of failure - heap allocation fails every aCount attempt.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_FAILNEXT(aHeap,aCount) (aHeap)->__DbgSetAllocFail(RAllocator::EFailNext,aCount)

/**
@publishedAll
@released

Simulates aBurst allocation failures for the specific heap.

The failure occurs on the next call to new or any of the functions which allocate 
memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The heap allocation will fail after aCount-1 allocation attempts. 
              Note when used with RHeap the maximum value aCount can be set 
              to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see RAllocator::__DbgSetBurstAllocFail()
*/
#define __RHEAP_BURSTFAILNEXT(aHeap,aCount,aBurst) (aHeap)->__DbgSetBurstAllocFail(RAllocator::EBurstFailNext,aCount, aBurst)



/**
@publishedAll
@released

Simulates an allocation failure for the specific heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aType  The type of failure to be simulated. 
@param aRate The failure rate.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_SETFAIL(aHeap,aType,aRate) (aHeap)->__DbgSetAllocFail(aType,aRate)

/**
@publishedAll
@released

Simulates an allocation failure for the specific heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aType  The type of failure to be simulated. 
@param aRate  The failure rate.  Note when used with RHeap the maximum value 
              aRate can be set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see RAllocator::__DbgSetBurstAllocFail()
*/
#define __RHEAP_SETBURSTFAIL(aHeap,aType,aRate,aBurst) (aHeap)->__DbgSetBurstAllocFail(aType,aRate,aBurst)



/**
@publishedAll
@released

Cancels simulated allocation failure for the specific heap.

This macro is defined only for debug builds.

@param aHeap A pointer to the specific RHeap.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_RESET(aHeap) (aHeap)->__DbgSetAllocFail(RAllocator::ENone,1)



/**
@publishedAll
@released

Cancels simulated allocation failure for the specific heap.
It walks the the heap and sets the nesting level for all allocated
cells to zero.

This macro is defined only for debug builds.

@param aHeap A pointer to the specific RHeap.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_TOTAL_RESET(aHeap) (aHeap)->__DbgSetAllocFail(RAllocator::EReset,1)

/**
@publishedAll
@released

Returns the number of heap allocation failures the current debug allocator fail
function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
#define __RHEAP_CHECKFAILURE(aHeap) (aHeap)->__DbgCheckFailure()


#if defined (__WINS__) 

/**
@publishedAll
@released
*/
#define __DEBUGGER() {if (User::JustInTime()) __BREAKPOINT()}

#else
#define __DEBUGGER()
#endif


#if defined(__DLL__)
/**
@publishedAll
@released

Declares a function for testing object invariance.

For complex classes, it is often useful to provide a function that can
be called to check that the object is in a valid state.
The __DECLARE_TEST macro supplies a standard prototype for such a function
named __DbgTestInvariant(). A companion macro __TEST_INVARIANT is provided
to call the function.

For DLLs, as opposed to EXEs, __DbgTestInvariant() is exported,
i.e. the macro expands to:

@code
public: IMPORT_C void __DbgTestInvariant() const; void __DbgTest(TAny *aPtr) const
@endcode

This macro should placed as the last item in a class declaration (as it 
switches back to public access). Note that a terminating semi-colon must be used.

You should define the __DbgTestInvariant() function to check that the object
is in a healthy state. If it finds an error, it should call User::Invariant(),
which will cause a panic. 

If a class is derived from a base class, then the base class __DbgTestInvariant()
should be called first, and then any further checking done. 

The second function declared, __DbgTest(), is intended to allow test code a way
of directly accessing non-public members of a class. The function is
implemented by any test code that requires it, rather than in the class’s own
source code. The function is therefore not exported.

__DECLARE_TEST is defined for both debug and release builds. This point is
particularly important for DLLs, as otherwise the exported interfaces would
differ between the build versions, giving potential binary compatibility
problems. To avoid using memory unnecessarily in release builds, you can,
however, use preprocessor directives to define the code within
__DbgTestInvariant() only for debug builds. __DbgTestInvariant() is never
called in release builds.

@see __TEST_INVARIANT
*/
#define __DECLARE_TEST public: IMPORT_C void __DbgTestInvariant() const; void __DbgTest(TAny *aPtr) const
#else
#define __DECLARE_TEST public: void __DbgTestInvariant() const; void __DbgTest(TAny *aPtr) const
#endif

#else
#define __ASSERT_DEBUG(c,p)
#define __DECLARE_NAME(t)
#define __TEST_INVARIANT
#if defined(__DLL__)
#define __DECLARE_TEST public: IMPORT_C void __DbgTestInvariant() const; void __DbgTest(TAny *aPtr) const
#else
#define __DECLARE_TEST public: void __DbgTestInvariant() const; void __DbgTest(TAny *aPtr) const
#endif




/**
@publishedAll
@released

Marks the start of checking the current thread's heap. 

This macro is defined only for debug builds.

This macro must be matched by a corresponding call to __UHEAP_MARKEND or __UHEAP_MARKENDC.

Calls to this macro can be nested but each call must be matched by corresponding 
call to __UHEAP_MARKEND or __UHEAP_MARKENDC.

@see User::__DbgMarkStart()
@see __UHEAP_MARKEND
@see __UHEAP_MARKENDC
*/
#define __UHEAP_MARK




/**
@publishedAll
@released

Checks that the number of allocated cells at the current nested level on the 
current thread's heap is the same as the specified value.

This macro is defined only for debug builds.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

The macro assumes that:

1. the heap being checked is a user heap

2. checking is being done for the number of allocated cells at the current nested 
   level; i.e. that aCountAll is set to false

3. the line number is the line number of this source code statement.

4. the file name is the full path name of the file containing this source statement

@param aCount The number of heap cells expected to be allocated at
              the current nest level.

@see User::__DbgMarkCheck()
@see __KHEAP_CHECK
*/
#define __UHEAP_CHECK(aCount)




/**
@publishedAll
@released

Checks that the total number of allocated cells on the current thread's heap 
is the same as the specified value.

This macro is defined only for debug builds.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

@param aCount The total number of heap cells expected to be allocated.

@see User::__DbgMarkCheck()
@see __KHEAP_CHECKALL
*/
#define __UHEAP_CHECKALL(aCount)




/**
@publishedAll
@released

Marks the end of checking the current thread's heap. 

The macro expects zero heap cells to remain allocated at the current nest 
level. This macro is defined only for debug builds.

This macro must match an earlier call to __UHEAP_MARK.

@see User::__DbgMarkEnd()
@see __UHEAP_MARK
*/
#define __UHEAP_MARKEND




/**
@publishedAll
@released

Marks the end of checking the current thread's heap. 

The macro expects aCount heap cells to remain allocated at the current nest 
level.

This macro must match an earlier call to __UHEAP_MARK.

@param aCount The number of heap cells expected to remain allocated at
              the current nest level.

@see User::__DbgMarkEnd()
@see __UHEAP_MARK
*/
#define __UHEAP_MARKENDC(aCount)




/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap.

The failure occurs on the next call to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aCount Determines when the allocation will fail.
              Heap allocation fails on attempt number aCount - later
              allocations will succeed.
              For example, if aCount is 3, then heap allocation fails
              on the 3rd attempt, but all subsequent allocations succeed. 

@see User::__DbgSetAllocFail()
*/
#define __UHEAP_FAILNEXT(aCount)

/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap.

The failures will occur for aBurst times from the next call to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aCount Determines when the allocation will fail.
              Heap allocation fails on attempt number aCount - later
              allocations will succeed.
              For example, if aCount is 3, then heap allocation fails
              on the 3rd attempt, but all subsequent allocations succeed.   
              Note when used with RHeap the maximum value aBurst can be 
              set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see User::__DbgSetBurstAllocFail()
*/
#define __UHEAP_BURSTFAILNEXT(aCount,aBurst)



/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate The failure rate.

@see User::__DbgSetAllocFail()
*/
#define __UHEAP_SETFAIL(aType,aRate)

/**
@publishedAll
@released

Simulates heap allocation failure for the current thread's heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aType  The type of failure to be simulated.
@param aRate  The failure rate.  Note when used with RHeap the maximum value 
              aRate can be set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see User::__DbgSetBurstAllocFail()
@see RAllocator::TAllocFail
*/
#define __UHEAP_SETBURSTFAIL(aType,aRate,aBurst)



/**
@publishedAll
@released

Cancels simulated heap allocation failure for the current thread's heap. 

This macro is defined only for debug builds.

@see User::__DbgSetAllocFail()
*/
#define __UHEAP_RESET



/**
@publishedAll
@released

Cancels simulated heap allocation failure for the current thread's heap. 
It walks the the heap and sets the nesting level for all allocated
cells to zero.

This macro is defined only for debug builds.
*/
#define __UHEAP_TOTAL_RESET

/**
@publishedAll
@released

Returns the number of heap allocation failures the current debug allocator fail
function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
#define __UHEAP_CHECKFAILURE ((TUint)0)

/**
@publishedAll
@released

Returns the number of kernel heap allocation failures the current debug 
allocator fail function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
#define __KHEAP_CHECKFAILURE ((TUint)0)

/**
@publishedAll
@released

Marks the start of heap checking for the specific heap. 

This macro is defined only for debug builds.

This macro must be matched by a corresponding call to __RHEAP_MARKEND or __RHEAP_MARKENDC.

Calls to this macro can be nested but each call must be matched by corresponding 
call to __RHEAP_MARKEND or __RHEAP_MARKENDC.

@param aHeap A pointer to the specific RHeap

@see RHeap
@see RAllocator::__DbgMarkStart()
@see __RHEAP_MARKEND
@see __RHEAP_MARKENDC
*/
#define __RHEAP_MARK(aHeap)




/**
@publishedAll
@released

Checks that the number of allocated cells at the current nested level on the 
specified heap is the same as the specified value. 

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail. 

This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The number of heap cells expected to be allocated at
              the current nest level.

@see RAllocator::__DbgMarkCheck()
*/
#define __RHEAP_CHECK(aHeap,aCount)




/**
@publishedAll
@released

Checks that the total number of allocated cells on the specified heap is the 
same as the specified value.

The macro also takes the name of the file containing this source code statement 
and the line number of this source code statement; they are displayed as part 
of the panic category, if the checks fail.

This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The total number of heap cells expected to be allocated.

@see RAllocator::__DbgMarkCheck()
*/
#define __RHEAP_CHECKALL(aHeap,aCount)




/**
@publishedAll
@released

Marks the end of heap checking for the specific heap.

The macro expects zero heap cells to remain allocated at the current nest 
level. This macro is defined only for debug builds.

This macro must match an earlier call to __RHEAP_MARK.

@param aHeap A pointer to the specific RHeap.

@see RAllocator::__DbgMarkEnd()
@see __RHEAP_MARK
*/
#define __RHEAP_MARKEND(aHeap)




/**
@publishedAll
@released

Marks the end of heap checking for the specific heap.

The macro expects aCount heap cells to remain allocated at the current nest 
level. This macro is defined only for debug builds.

This macro must match an earlier call to __RHEAP_MARK.

@param aHeap  A pointer to the specific RHeap.
@param aCount The number of heap cells expected to remain allocated at
              the current nest level

@see RAllocator::__DbgMarkEnd()
@see __RHEAP_MARK
*/
#define __RHEAP_MARKENDC(aHeap,aCount)




/**
@publishedAll
@released

Simulates an allocation failure for the specific heap.

The failure occurs on the next call to new or any of the functions which allocate 
memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The rate of failure - heap allocation fails every aCount attempt.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_FAILNEXT(aHeap,aCount)

/**
@publishedAll
@released

Simulates aBurst allocation failures for the specific heap.

The failure occurs on the next call to new or any of the functions which allocate 
memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aCount The heap allocation will fail after aCount-1 allocation attempts. 
              Note when used with RHeap the maximum value aCount can be set 
              to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see RAllocator::__DbgSetBurstAllocFail()
*/
#define __RHEAP_BURSTFAILNEXT(aHeap,aCount,aBurst)



/**
@publishedAll
@released

Simulates an allocation failure for the specific heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aType  The type of failure to be simulated. 
@param aRate The failure rate.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_SETFAIL(aHeap,aType,aRate)

/**
@publishedAll
@released

Simulates an allocation failure for the specific heap. 

The failure occurs on subsequent calls to new or any of the functions which 
allocate memory from the heap. This macro is defined only for debug builds.

@param aHeap  A pointer to the specific RHeap.
@param aType  The type of failure to be simulated. 
@param aRate  The failure rate.  Note when used with RHeap the maximum value 
              aRate can be set to is KMaxTUint16.
@param aBurst The number of consecutive allocations that will fail.  Note 
              when used with RHeap the maximum value aBurst can be set 
              to is KMaxTUint16.

@see RAllocator::__DbgSetBurstAllocFail()
*/
#define __RHEAP_SETBURSTFAIL(aHeap,aType,aRate,aBurst)



/**
@publishedAll
@released

Cancels simulated allocation failure for the specific heap.

This macro is defined only for debug builds.

@param aHeap A pointer to the specific RHeap.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_RESET(aHeap)



/**
@publishedAll
@released

Cancels simulated allocation failure for the specific heap.
It walks the the heap and sets the nesting level for all allocated
cells to zero.

This macro is defined only for debug builds.

@param aHeap A pointer to the specific RHeap.

@see RAllocator::__DbgSetAllocFail()
*/
#define __RHEAP_TOTAL_RESET(aHeap)


/**
@publishedAll
@released

Returns the number of heap allocation failures the current debug allocator fail
function has caused so far.

This is intended to only be used with fail types RAllocator::EFailNext,
RAllocator::EBurstFailNext, RAllocator::EDeterministic and
RAllocator::EBurstDeterministic.  The return value is unreliable for 
all other fail types.

@return The number of heap allocation failures the current debug fail 
function has caused.

@see RAllocator::TAllocFail
*/
#define __RHEAP_CHECKFAILURE(aHeap) ((TUint)0)

#define __DEBUGGER()
#endif

#if defined (__WINS__)
/** @internalTechnology */
#define __EMULATOR_IMAGE_HEADER2(aUid0,aUid1,aUid2,aPriority,aCap0,aCap1,aSid,aVid,aVer,aFlags)	TEmulatorImageHeader uid={{aUid0,aUid1,aUid2},aPriority,{aSid,aVid,{aCap0,aCap1}},0,0,aVer,aFlags};
/** @internalTechnology */
#define __EMULATOR_IMAGE_HEADER(aUid0,aUid1,aUid2,aPriority,aCap,aFlags)					TEmulatorImageHeader uid={{aUid0,aUid1,aUid2},aPriority,{aUid2,0,{aCap,0}},0,0,0x00010000u,aFlags};
#else
#define __EMULATOR_IMAGE_HEADER2(aUid0,aUid1,aUid2,aPriority,aCap0,aCap1,aSid,aVer,aFlags)
#define __EMULATOR_IMAGE_HEADER(aUid0,aUid1,aUid2,aPriority,aCap,aFlags)
#endif

#if defined(_UNICODE)
#if !defined(UNICODE)
/**
@publishedAll
@deprecated
*/
#define UNICODE
#endif
#endif

#if !defined(ASSERT)
/**
@publishedAll
@released

Generates _ASSERT_DEBUG code that calls User::Invariant() if the specified
condition is not true.

@param x A conditional expression which results in true or false.
*/
#define ASSERT(x) __ASSERT_DEBUG(x,User::Invariant())
#endif




#if defined(_DEBUG)
/**
@publishedAll
@released
*/
#define __DEBUG_ONLY(x) x
#else
#define __DEBUG_ONLY(x)
#endif




#ifdef __KERNEL_MODE__

/** @internalComponent */
#define	KIMPORT_C	IMPORT_C

/** @internalComponent */
#define	KEXPORT_C	EXPORT_C

/** @internalComponent */
#define	UIMPORT_C

/** @internalComponent */
#define	UEXPORT_C
#else
#define	KIMPORT_C
#define	KEXPORT_C
#define	UIMPORT_C	IMPORT_C
#define	UEXPORT_C	EXPORT_C
#endif




/**
@publishedAll
@released

Asserts that a condition is true at compilation time.

@param x Condition to assert
*/
#define __ASSERT_COMPILE(x)		void __compile_time_assert(int __check[(x)?1:-1])

#ifdef __REMOVE_PLATSEC_DIAGNOSTICS__
/**
@publishedPartner
@released
*/
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#define __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#endif /*__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__*/
#endif /*__REMOVE_PLATSEC_DIAGNOSTICS__*/

/**
@internalComponent
*/
static const char* const KSuppressPlatSecDiagnosticMagicValue = (const char*)1;

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
/**
@internalComponent
*/
#define __PLATSEC_DIAGNOSTIC_FILE_AND_LINE_HELPER(l) #l
/**
@internalComponent
*/
#define __PLATSEC_DIAGNOSTIC_FILE_AND_LINE_HELPER2(f,l) f "(" __PLATSEC_DIAGNOSTIC_FILE_AND_LINE_HELPER(l) ")"
/**
@publishedPartner
@released
*/
#define __PLATSEC_DIAGNOSTIC_FILE_AND_LINE __PLATSEC_DIAGNOSTIC_FILE_AND_LINE_HELPER2(__FILE__,__LINE__)

/**
@publishedPartner
@released

A macro that should be used to enclose a platform security diagnostic
'C' style string that can be passed to a capability checking function such
as RThread::HasCapability() and Kern::CurrentThreadHasCapability().

The content of the string is emitted if the capability test finds that
the capability is not present.

The macro provides a convenient mechanism that allows the strings to
be removed from future versions of Symbian OS.

For example:

@code
if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetState")))
    {
    return KErrPermissionDenied;
    }			
@endcode

In this example, the string:

@code
Checked by Hal function EDisplayHalSetState
@endcode

is emitted if the calling process does not have the ECapabilityPowerMgmt capability.

@param s A C-style string.

@see RProcess::HasCapability()
@see RThread::HasCapability()
@see RMessagePtr2::HasCapability()
@see User::CreatorHasCapability()
*/
#define __PLATSEC_DIAGNOSTIC_STRING(s) s

/**
When this value is used in Platform Security APIs as the value for the aDiagnosticText
argument, these APIs will not emit any form of diagnostic message.
@publishedPartner
@released
*/
// Note this value is the same as KSuppressPlatSecDiagnosticMagicValue
// and used to be a set by it but that caused an error with GCCE compiler
static const char* const KSuppressPlatSecDiagnostic = (const char*)1;

#else /* __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__ */

#define __PLATSEC_DIAGNOSTIC_STRING(s) NULL

#ifndef __KERNEL_MODE__
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
/**
When this value is used in Platform Security APIs as the value for the aDiagnostic
argument, these APIs will not emit any form of diagnostic message.
@publishedPartner
@released
*/
#define KSuppressPlatSecDiagnostic		NULL, NULL

#else /* __REMOVE_PLATSEC_DIAGNOSTICS__ */

/**
When this value is used in Platform Security APIs as the value for the aDiagnostic
argument, these APIs will not emit any form of diagnostic message.
@publishedPartner
@released
*/
#define KSuppressPlatSecDiagnostic		NULL

#endif /* !__REMOVE_PLATSEC_DIAGNOSTICS__ */
#endif /* !__KERNEL_MODE__ */
#endif /* !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__ */

/*
 * MSVC operator new and operator new[] header guards
 */
#ifdef __PLACEMENT_NEW
#define __PLACEMENT_NEW_INLINE
#endif /* __PLACEMENT_NEW */

#if defined(__VC32__) && (_MSC_VER < 1300)
#define __PLACEMENT_VEC_NEW_INLINE
#define __OMIT_VEC_OPERATOR_NEW_DECL__
#endif /* version of MSVC that doesn't support overloaded operator new[] */

/**
Calling convention qualifier for functions involving floating point 
variables passed or returned by value.
@publishedAll
@released
*/
#ifndef __SOFTFP
#define __SOFTFP
#endif /* __SOFTFP */

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32def_private.h>
#endif

#endif /* __E32DEF_H__ */
