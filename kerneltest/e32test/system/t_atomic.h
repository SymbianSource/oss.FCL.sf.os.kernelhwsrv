// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_atomic.h
// 
//

#include <e32atomics.h>

#ifdef __VC32__
#pragma warning( disable : 4244 )	/* conversion to shorter type - possible loss of data */
#endif

const TInt KMaxThreads = 8;

#ifdef __KERNEL_MODE__
#include <kernel/kernel.h>
#undef	DEBUGPRINT
#define	DEBUGPRINT	Kern::Printf
#else
extern void UPrintf(const char*, ...);
#undef	DEBUGPRINT
#define DEBUGPRINT	UPrintf
#endif

#undef	__INT64_ALIGNED__
#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__EABI__)
#define	__INT64_ALIGNED__
#endif

#ifdef __INT64_ALIGNED__
typedef	TUint64	TUint64A;
typedef	TInt64	TInt64A;
#else

class TUint64A
	{
public:
	inline const TUint64* operator&() const
		{ return ((const TUint64*)((T_UintPtr(this)+7)&~7)); }
	inline TUint64* operator&()
		{ return ((TUint64*)((T_UintPtr(this)+7)&~7)); }
private:
	TUint64 i_Data[2];
	};

class TInt64A
	{
public:
	inline const TInt64* operator&() const
		{ return ((const TInt64*)((T_UintPtr(this)+7)&~7)); }
	inline TInt64* operator&()
		{ return ((TInt64*)((T_UintPtr(this)+7)&~7)); }
private:
	TUint64 i_Data[2];
	};

#endif

struct TDGBase
	{
	TInt Execute();
	void Dump(const char*);

	TUint64	i0;
	TUint64	i1;
	TUint64	i2;
	TUint64	i3;
	TInt	iIndex;
	};

struct TAtomicAction
	{
	TUint64	i0;			// first parameter to operation
	TUint64	i1;			// second parameter to operation
	TUint64	i2;			// third parameter to operation
	TInt	iIndex;		// index of atomic function
	TInt	iThread;	// thread identifier
	};

struct TPerThread
	{
	TUint64	iDiff;		// accumulated difference
	TUint64	iXor;		// accumulated XOR
	TUint64 iFailCount;	// failure count for CAS operations
	TUint64 iCount;		// iteration count
	};

extern "C" TInt DoAtomicAction(TAny* aPtr, TPerThread* aT, TAtomicAction& aA);

enum TMemoryOrder
	{
	EOrderRelaxed=0,
	EOrderAcquire=1,
	EOrderRelease=2,
	EOrderOrdered=3,
	};

enum TAtomicFunc
	{
	EAtomicFuncLOAD=0,
	EAtomicFuncSTORE=1,
	EAtomicFuncSWP=2,
	EAtomicFuncADD=3,
	EAtomicFuncAND=4,
	EAtomicFuncIOR=5,
	EAtomicFuncXOR=6,
	EAtomicFuncAXO=7,
	EAtomicFuncTAU=8,
	EAtomicFuncTAS=9,
	EAtomicFuncCAS=10,
	EAtomicFuncN
	};

enum TFuncType
	{
	EFuncTypeInvalid=0,
	EFuncTypeLoad=1,
	EFuncTypeRmw1=2,
	EFuncTypeRmw2=3,
	EFuncTypeRmw3=4,
	EFuncTypeCas=5,
	};

#define	FUNCS_PER_SIZE			(TUint(EAtomicFuncN)*4)
#define TOTAL_FUNCS				(FUNCS_PER_SIZE*4)
#define	INDEXES_PER_SIZE		(16*4)
#define	TOTAL_INDEXES			(INDEXES_PER_SIZE*4)

#define FUNCATTR(func,size,ord,type)	((TUint(func)<<24)|(TUint(size)<<16)|(TUint(ord)<<8)|(TUint(type)))
#define	ATTR_TO_TYPE(attr)				((attr)&0xff)
#define	ATTR_TO_ORD(attr)				(((attr)>>8)&0xff)
#define	ATTR_TO_SIZE(attr)				(((attr)>>16)&0xff)
#define	ATTR_TO_FUNC(attr)				(((attr)>>24)&0xff)
#define	FUNCATTR2(func,size,type)	\
			FUNCATTR(func,size,EOrderRelaxed,type),	FUNCATTR(func,size,EOrderAcquire,type), FUNCATTR(func,size,EOrderRelease,type), FUNCATTR(func,size,EOrderOrdered,type)
#define	FUNCATTR2A(func,size,type)	\
												0,	FUNCATTR(func,size,EOrderAcquire,type),										0,										0
#define	FUNCATTR2B(func,size,type)	\
												0,										0,	FUNCATTR(func,size,EOrderRelease,type),	FUNCATTR(func,size,EOrderOrdered,type)
#define	FUNCATTR3(size)				\
			FUNCATTR2A(EAtomicFuncLOAD,size,EFuncTypeLoad),		\
			FUNCATTR2B(EAtomicFuncSTORE,size,EFuncTypeRmw1),	\
			FUNCATTR2(EAtomicFuncSWP,size,EFuncTypeRmw1),		\
			FUNCATTR2(EAtomicFuncADD,size,EFuncTypeRmw1),		\
			FUNCATTR2(EAtomicFuncAND,size,EFuncTypeRmw1),		\
			FUNCATTR2(EAtomicFuncIOR,size,EFuncTypeRmw1),		\
			FUNCATTR2(EAtomicFuncXOR,size,EFuncTypeRmw1),		\
			FUNCATTR2(EAtomicFuncAXO,size,EFuncTypeRmw2),		\
			FUNCATTR2(EAtomicFuncTAU,size,EFuncTypeRmw3),		\
			FUNCATTR2(EAtomicFuncTAS,size,EFuncTypeRmw3),		\
			FUNCATTR2(EAtomicFuncCAS,size,EFuncTypeCas),		\
			0,	0,	0,	0,										\
			0,	0,	0,	0,										\
			0,	0,	0,	0,										\
			0,	0,	0,	0,										\
			0,	0,	0,	0


#define	__DO_STRINGIFY__(x)			#x
#define	__STRINGIFY__(x)			__DO_STRINGIFY__(x)
#define __concat3__(a,b,c)			a##b##c
#define __concat5__(a,b,c,d,e)		a##b##c##d##e
#define FUNCNAME(func,size,ord)		__STRINGIFY__(__concat3__(func,size,ord))
#define ATOMICFUNC(func,size,ord)	__concat5__(__e32_atomic_,func,_,ord,size)
#define CONTROLFUNC(func,size,ord)	__concat3__(__nonatomic_,func,size)
#define	FUNCNAME2(func,size)		FUNCNAME(func,size,rlx), FUNCNAME(func,size,acq), FUNCNAME(func,size,rel), FUNCNAME(func,size,ord)
#define	FUNCNAME3(size)	\
			FUNCNAME2(load,size),	\
			FUNCNAME2(store,size),	\
			FUNCNAME2(swp,size),	\
			FUNCNAME2(add,size),	\
			FUNCNAME2(and,size),	\
			FUNCNAME2(ior,size),	\
			FUNCNAME2(xor,size),	\
			FUNCNAME2(axo,size),	\
			FUNCNAME2(tau,size),	\
			FUNCNAME2(tas,size),	\
			FUNCNAME2(cas,size),	\
			"", "", "", "",			\
			"", "", "", "",			\
			"", "", "", "",			\
			"", "", "", "",			\
			"", "", "", ""


#define	ATOMICFUNC2(func,size)		(PFV)&ATOMICFUNC(func,size,rlx), (PFV)&ATOMICFUNC(func,size,acq), (PFV)&ATOMICFUNC(func,size,rel), (PFV)&ATOMICFUNC(func,size,ord)
#define	ATOMICFUNC2A(func,size)									0,	(PFV)&ATOMICFUNC(func,size,acq),								0,							0
#define	ATOMICFUNC2B(func,size)									0,								0,	(PFV)&ATOMICFUNC(func,size,rel), (PFV)&ATOMICFUNC(func,size,ord)
#define	ATOMICFUNC3(size)				\
			ATOMICFUNC2A(load,size),	\
			ATOMICFUNC2B(store,size),	\
			ATOMICFUNC2(swp,size),		\
			ATOMICFUNC2(add,size),		\
			ATOMICFUNC2(and,size),		\
			ATOMICFUNC2(ior,size),		\
			ATOMICFUNC2(xor,size),		\
			ATOMICFUNC2(axo,size),		\
			ATOMICFUNC2(tau,size),		\
			ATOMICFUNC2(tas,size),		\
			ATOMICFUNC2(cas,size),		\
			0, 0, 0, 0,					\
			0, 0, 0, 0,					\
			0, 0, 0, 0,					\
			0, 0, 0, 0,					\
			0, 0, 0, 0


#define	CONTROLFUNC2(func,size)		(PFV)&CONTROLFUNC(func,size,rlx), (PFV)&CONTROLFUNC(func,size,acq), (PFV)&CONTROLFUNC(func,size,rel), (PFV)&CONTROLFUNC(func,size,ord)
#define	CONTROLFUNC2A(func,size)								0,	(PFV)&CONTROLFUNC(func,size,acq),								0,							0
#define	CONTROLFUNC2B(func,size)								0,								0,	(PFV)&CONTROLFUNC(func,size,rel), (PFV)&CONTROLFUNC(func,size,ord)
#define	CONTROLFUNC3(size)				\
			CONTROLFUNC2A(load,size),	\
			CONTROLFUNC2B(store,size),	\
			CONTROLFUNC2(swp,size),		\
			CONTROLFUNC2(add,size),		\
			CONTROLFUNC2(and,size),		\
			CONTROLFUNC2(ior,size),		\
			CONTROLFUNC2(xor,size),		\
			CONTROLFUNC2(axo,size),		\
			CONTROLFUNC2(tau,size),		\
			CONTROLFUNC2(tas,size),		\
			CONTROLFUNC2(cas,size),		\
			0, 0, 0, 0,					\
			0, 0, 0, 0,					\
			0, 0, 0, 0,					\
			0, 0, 0, 0,					\
			0, 0, 0, 0


#ifdef __INCLUDE_FUNC_NAMES__
extern "C" const char* FuncName[] =
	{
	FUNCNAME3(8),
	FUNCNAME3(16),
	FUNCNAME3(32),
	FUNCNAME3(64)
	};
#endif

typedef void (*PFV)();

#ifdef __INCLUDE_ATOMIC_FUNCTIONS__
extern "C" const PFV AtomicFuncPtr[] =
	{
	ATOMICFUNC3(8),
	ATOMICFUNC3(16),
	ATOMICFUNC3(32),
	ATOMICFUNC3(64)
	};
#endif

#ifdef __INCLUDE_CONTROL_FUNCTIONS__
extern "C" {

// Simulated versions of atomic functions without the atomicity
extern TUint8	__nonatomic_load8(const volatile TAny* a);
extern TUint8	__nonatomic_store8(volatile TAny* a, TUint8 v);
extern TUint8	__nonatomic_swp8(volatile TAny* a, TUint8 v);
extern TBool	__nonatomic_cas8(volatile TAny* a, TUint8* q, TUint8 v);
extern TUint8	__nonatomic_add8(volatile TAny* a, TUint8 v);
extern TUint8	__nonatomic_and8(volatile TAny* a, TUint8 v);
extern TUint8	__nonatomic_ior8(volatile TAny* a, TUint8 v);
extern TUint8	__nonatomic_xor8(volatile TAny* a, TUint8 v);
extern TUint8	__nonatomic_axo8(volatile TAny* a, TUint8 u, TUint8 v);
extern TUint8	__nonatomic_tau8(volatile TAny* a, TUint8 t, TUint8 u, TUint8 v);
extern TInt8	__nonatomic_tas8(volatile TAny* a, TInt8 t, TInt8 u, TInt8 v);

extern TUint16	__nonatomic_load16(const volatile TAny* a);
extern TUint16	__nonatomic_store16(volatile TAny* a, TUint16 v);
extern TUint16	__nonatomic_swp16(volatile TAny* a, TUint16 v);
extern TBool	__nonatomic_cas16(volatile TAny* a, TUint16* q, TUint16 v);
extern TUint16	__nonatomic_add16(volatile TAny* a, TUint16 v);
extern TUint16	__nonatomic_and16(volatile TAny* a, TUint16 v);
extern TUint16	__nonatomic_ior16(volatile TAny* a, TUint16 v);
extern TUint16	__nonatomic_xor16(volatile TAny* a, TUint16 v);
extern TUint16	__nonatomic_axo16(volatile TAny* a, TUint16 u, TUint16 v);
extern TUint16	__nonatomic_tau16(volatile TAny* a, TUint16 t, TUint16 u, TUint16 v);
extern TInt16	__nonatomic_tas16(volatile TAny* a, TInt16 t, TInt16 u, TInt16 v);

extern TUint32	__nonatomic_load32(const volatile TAny* a);
extern TUint32	__nonatomic_store32(volatile TAny* a, TUint32 v);
extern TUint32	__nonatomic_swp32(volatile TAny* a, TUint32 v);
extern TBool	__nonatomic_cas32(volatile TAny* a, TUint32* q, TUint32 v);
extern TUint32	__nonatomic_add32(volatile TAny* a, TUint32 v);
extern TUint32	__nonatomic_and32(volatile TAny* a, TUint32 v);
extern TUint32	__nonatomic_ior32(volatile TAny* a, TUint32 v);
extern TUint32	__nonatomic_xor32(volatile TAny* a, TUint32 v);
extern TUint32	__nonatomic_axo32(volatile TAny* a, TUint32 u, TUint32 v);
extern TUint32	__nonatomic_tau32(volatile TAny* a, TUint32 t, TUint32 u, TUint32 v);
extern TInt32	__nonatomic_tas32(volatile TAny* a, TInt32 t, TInt32 u, TInt32 v);

extern TUint64	__nonatomic_load64(const volatile TAny* a);
extern TUint64	__nonatomic_store64(volatile TAny* a, TUint64 v);
extern TUint64	__nonatomic_swp64(volatile TAny* a, TUint64 v);
extern TBool	__nonatomic_cas64(volatile TAny* a, TUint64* q, TUint64 v);
extern TUint64	__nonatomic_add64(volatile TAny* a, TUint64 v);
extern TUint64	__nonatomic_and64(volatile TAny* a, TUint64 v);
extern TUint64	__nonatomic_ior64(volatile TAny* a, TUint64 v);
extern TUint64	__nonatomic_xor64(volatile TAny* a, TUint64 v);
extern TUint64	__nonatomic_axo64(volatile TAny* a, TUint64 u, TUint64 v);
extern TUint64	__nonatomic_tau64(volatile TAny* a, TUint64 t, TUint64 u, TUint64 v);
extern TInt64	__nonatomic_tas64(volatile TAny* a, TInt64 t, TInt64 u, TInt64 v);

} // extern "C"


extern "C" const PFV ControlFuncPtr[] =
	{
	CONTROLFUNC3(8),
	CONTROLFUNC3(16),
	CONTROLFUNC3(32),
	CONTROLFUNC3(64)
	};
#endif

#ifdef __INCLUDE_FUNCTION_ATTRIBUTES__
extern "C" const TUint FuncAttr[] =
	{
	FUNCATTR3(1),
	FUNCATTR3(2),
	FUNCATTR3(4),
	FUNCATTR3(8)
	};
#endif

template<class T>
struct TLoadFn	//	load
	{
	typedef T (*F)(const volatile TAny*);
	};

template<class T>
struct TRmw1Fn	// store, swp, add, and, ior, xor
	{
	typedef T (*F)(volatile TAny*, T);
	};

template<class T>
struct TRmw2Fn	// axo
	{
	typedef T (*F)(volatile TAny*, T, T);
	};

template<class T>
struct TRmw3Fn	// tau, tas
	{
	typedef T (*F)(volatile TAny*, T, T, T);
	};

template<class T>
struct TCasFn	// cas
	{
	typedef TBool (*F)(volatile TAny*, T*, T);
	};

class TEnclosed
	{
public:
	TEnclosed(TInt aSize);
	TAny* Ptr();
	TInt Next();
	void Init();
	TInt Verify();
	TInt Offset() const {return iOffset;}
private:
	TUint64* iData;
	TUint64* iBackup;
	TUint64	i_Data[17];
	TInt iOffset;
	TInt iSize;
	};

template<class T>
class Transform
	{
public:
	inline static T A();
	inline static T B();
	static T F(T aOrig);						// return Ax+B mod M (M=2^n, n=number of bits in T)
	static T Pow(T aBase, TUint64 aExp);		// return aBase^aExp mod M
	static T PowerSum(T aBase, TUint64 aExp);	// return 1 + T + T^2 + ... + T^(aExp-1) mod M
	static T F_iter(T aOrig, TUint64 aCount);	// return result of applying F iterated aCount times to aOrig
	};

TEMPLATE_SPECIALIZATION inline TUint8 Transform<TUint8>::A()
	{ return 19; }
TEMPLATE_SPECIALIZATION inline TUint8 Transform<TUint8>::B()
	{ return 29; }

TEMPLATE_SPECIALIZATION inline TUint16 Transform<TUint16>::A()
	{ return 487; }
TEMPLATE_SPECIALIZATION inline TUint16 Transform<TUint16>::B()
	{ return 12983; }

TEMPLATE_SPECIALIZATION inline TUint32 Transform<TUint32>::A()
	{ return 29943829; }
TEMPLATE_SPECIALIZATION inline TUint32 Transform<TUint32>::B()
	{ return 104729; }

TEMPLATE_SPECIALIZATION inline TUint64 Transform<TUint64>::A()
	{ return UI64LIT(2862933555777941757); }
TEMPLATE_SPECIALIZATION inline TUint64 Transform<TUint64>::B()
	{ return UI64LIT(104917093); }

template<class T>
T Transform<T>::F(T aOrig)
	{
	return (T)(aOrig * Transform<T>::A() + Transform<T>::B());
	}

template<class T>
T Transform<T>::Pow(T aBase, TUint64 aExp)
	{
	T result(1);
	T multiplier(aBase);
	while (aExp)
		{
		if (aExp&1)
			result *= multiplier;
		aExp >>= 1;
		if (aExp)
			multiplier *= multiplier;
		}
	return (T)result;
	}

template<class T>
T Transform<T>::PowerSum(T aBase, TUint64 aExp)
	{
	T result(0);
	T multiplier(aBase);
	T inter(1);
	while (aExp)
		{
		if (aExp&1)
			{
			result *= multiplier;
			result += inter;
			}
		aExp >>= 1;
		if (aExp)
			{
			inter *= (multiplier + 1);
			multiplier *= multiplier;
			}
		}
	return (T)result;
	}

template<class T>
T Transform<T>::F_iter(T aOrig, TUint64 aCount)
	{
	return (T)(Pow(A(),aCount)*aOrig + PowerSum(A(),aCount)*B());
	}



#ifdef __EPOC32__
_LIT(KAtomicTestLddName,"D_ATOMIC");

class RTestAtomic : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ETDGExecuteK=0,
		EInitialise=1,
		ERetrieve=2,
		ESetCurrentThreadTimeslice=3,
		ESwitchExecTables=4,
		EGetKernelMemoryAddress=5,
		EMaxControl
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KAtomicTestLddName,TVersion(),KNullUnit,NULL,NULL); }
public:
	inline TInt TDGExecuteK(TDGBase& a)
		{ return DoControl(ETDGExecuteK, &a); }
	inline TInt Initialise(TUint64 aValue)
		{ return DoControl(EInitialise, &aValue); }
	inline TUint64 Retrieve()
		{ TUint64 x; DoControl(ERetrieve, &x); return x; }
	inline TInt SetCurrentThreadTimeslice(TInt aTimeslice)
		{ return DoControl(ESetCurrentThreadTimeslice, (TAny*)aTimeslice); }
	inline TInt SwitchExecTables(TInt aThread)
		{ return DoControl(ESwitchExecTables, (TAny*)aThread); }
	inline TAny* KernelMemoryAddress()
		{ return (TAny*)DoControl(EGetKernelMemoryAddress); }

	static TInt GetThreadInfo(TPerThread& aInfo);
	static TInt SetThreadInfo(const TPerThread& aInfo);
	static TInt AtomicAction(TAtomicAction& aAction);
	static TInt RestoreExecTable();
#endif
	};
#endif


