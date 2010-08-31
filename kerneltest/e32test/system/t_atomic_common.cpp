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
// e32test\system\t_atomic_common.cpp
// 
//

#ifdef __KERNEL_MODE__
#include <kernel/kernel.h>
#else
#define	__E32TEST_EXTENSION__

#include <e32test.h>

extern RTest test;

#define __INCLUDE_FUNC_NAMES__
#endif

#define __INCLUDE_ATOMIC_FUNCTIONS__
#define __INCLUDE_CONTROL_FUNCTIONS__
#define __INCLUDE_FUNCTION_ATTRIBUTES__

#include "t_atomic.h"

#define DEBUGPRINTVAR(x)	\
	{	\
	const TUint8* p = (const TUint8*)&(x);	\
	DEBUGPRINT("Line %d: " #x "=%02x %02x %02x %02x  %02x %02x %02x %02x", __LINE__, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);	\
	}

#ifdef __WINS__
#pragma warning( disable : 4127 )   // disable warning warning C4127: conditional expression is constant
#endif
template<typename T> void DebugPrintVar(T x, char *name, TInt line)
	{
	const TUint8 *p = (const TUint8 *)&x;
	const TInt size = sizeof(T);
	if (size < 2)
		{
		DEBUGPRINT("Line %d: %s =%02x", line, name, p[0]);
		}
	else if (size < 4)
		{
		DEBUGPRINT("Line %d: %s =%02x %02x", line, name, p[0], p[1]);
		}
	else if (size < 8)
		{
		DEBUGPRINT("Line %d: %s =%02x %02x %02x %02x", line, name, p[0], p[1], p[2], p[3]);
		}
	else
		{
		DEBUGPRINT("Line %d: %s =%02x %02x %02x %02x  %02x %02x %02x %02x", line, name, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		}
	}
#ifdef __WINS__
#pragma warning( default : 4127 )   // disable warning warning C4127: conditional expression is constant
#endif


extern "C" {

// Simulated versions of atomic functions without the atomicity
#define __LOAD(T)	return *(T*)a
#define	__STORE(T)	*(T*)a=v; return v
#define	__SWP(T)	T oldv=*(T*)a; *(T*)a=v; return oldv
#define __CAS(T)	if (*(T*)a==*q) {*(T*)a=v; return 1;} *q=*(T*)a; return 0
#define __ADD(T)	T oldv=*(T*)a; *(T*)a=(T)(oldv+v); return oldv
#define __AND(T)	T oldv=*(T*)a; *(T*)a=(T)(oldv&v); return oldv
#define __IOR(T)	T oldv=*(T*)a; *(T*)a=(T)(oldv|v); return oldv
#define __XOR(T)	T oldv=*(T*)a; *(T*)a=(T)(oldv^v); return oldv
#define __AXO(T)	T oldv=*(T*)a; *(T*)a=(T)((oldv&u)^v); return oldv
#define __TA(T)		T oldv=*(T*)a; *(T*)a=(T)(oldv+((oldv>=t)?u:v)); return oldv

TUint8	__nonatomic_load8(const volatile TAny* a)
	{
	__LOAD(TUint8);
	}

TUint8	__nonatomic_store8(volatile TAny* a, TUint8 v)
	{
	__STORE(TUint8);
	}

TUint8	__nonatomic_swp8(volatile TAny* a, TUint8 v)
	{
	__SWP(TUint8);
	}

TBool	__nonatomic_cas8(volatile TAny* a, TUint8* q, TUint8 v)
	{
	__CAS(TUint8);
	}

TUint8	__nonatomic_add8(volatile TAny* a, TUint8 v)
	{
	__ADD(TUint8);
	}

TUint8	__nonatomic_and8(volatile TAny* a, TUint8 v)
	{
	__AND(TUint8);
	}

TUint8	__nonatomic_ior8(volatile TAny* a, TUint8 v)
	{
	__IOR(TUint8);
	}

TUint8	__nonatomic_xor8(volatile TAny* a, TUint8 v)
	{
	__XOR(TUint8);
	}

TUint8	__nonatomic_axo8(volatile TAny* a, TUint8 u, TUint8 v)
	{
	__AXO(TUint8);
	}

TUint8	__nonatomic_tau8(volatile TAny* a, TUint8 t, TUint8 u, TUint8 v)
	{
	__TA(TUint8);
	}

TInt8	__nonatomic_tas8(volatile TAny* a, TInt8 t, TInt8 u, TInt8 v)
	{
	__TA(TInt8);
	}


TUint16	__nonatomic_load16(const volatile TAny* a)
	{
	__LOAD(TUint16);
	}

TUint16	__nonatomic_store16(volatile TAny* a, TUint16 v)
	{
	__STORE(TUint16);
	}

TUint16	__nonatomic_swp16(volatile TAny* a, TUint16 v)
	{
	__SWP(TUint16);
	}

TBool	__nonatomic_cas16(volatile TAny* a, TUint16* q, TUint16 v)
	{
	__CAS(TUint16);
	}

TUint16	__nonatomic_add16(volatile TAny* a, TUint16 v)
	{
	__ADD(TUint16);
	}

TUint16	__nonatomic_and16(volatile TAny* a, TUint16 v)
	{
	__AND(TUint16);
	}

TUint16	__nonatomic_ior16(volatile TAny* a, TUint16 v)
	{
	__IOR(TUint16);
	}

TUint16	__nonatomic_xor16(volatile TAny* a, TUint16 v)
	{
	__XOR(TUint16);
	}

TUint16	__nonatomic_axo16(volatile TAny* a, TUint16 u, TUint16 v)
	{
	__AXO(TUint16);
	}

TUint16	__nonatomic_tau16(volatile TAny* a, TUint16 t, TUint16 u, TUint16 v)
	{
	__TA(TUint16);
	}

TInt16	__nonatomic_tas16(volatile TAny* a, TInt16 t, TInt16 u, TInt16 v)
	{
	__TA(TInt16);
	}


TUint32	__nonatomic_load32(const volatile TAny* a)
	{
	__LOAD(TUint32);
	}

TUint32	__nonatomic_store32(volatile TAny* a, TUint32 v)
	{
	__STORE(TUint32);
	}

TUint32	__nonatomic_swp32(volatile TAny* a, TUint32 v)
	{
	__SWP(TUint32);
	}

TBool	__nonatomic_cas32(volatile TAny* a, TUint32* q, TUint32 v)
	{
	__CAS(TUint32);
	}

TUint32	__nonatomic_add32(volatile TAny* a, TUint32 v)
	{
	__ADD(TUint32);
	}

TUint32	__nonatomic_and32(volatile TAny* a, TUint32 v)
	{
	__AND(TUint32);
	}

TUint32	__nonatomic_ior32(volatile TAny* a, TUint32 v)
	{
	__IOR(TUint32);
	}

TUint32	__nonatomic_xor32(volatile TAny* a, TUint32 v)
	{
	__XOR(TUint32);
	}

TUint32	__nonatomic_axo32(volatile TAny* a, TUint32 u, TUint32 v)
	{
	__AXO(TUint32);
	}

TUint32	__nonatomic_tau32(volatile TAny* a, TUint32 t, TUint32 u, TUint32 v)
	{
	__TA(TUint32);
	}

TInt32	__nonatomic_tas32(volatile TAny* a, TInt32 t, TInt32 u, TInt32 v)
	{
	__TA(TInt32);
	}


TUint64	__nonatomic_load64(const volatile TAny* a)
	{
	__LOAD(TUint64);
	}

TUint64	__nonatomic_store64(volatile TAny* a, TUint64 v)
	{
	__STORE(TUint64);
	}

TUint64	__nonatomic_swp64(volatile TAny* a, TUint64 v)
	{
	__SWP(TUint64);
	}

TBool	__nonatomic_cas64(volatile TAny* a, TUint64* q, TUint64 v)
	{
	__CAS(TUint64);
	}

TUint64	__nonatomic_add64(volatile TAny* a, TUint64 v)
	{
	__ADD(TUint64);
	}

TUint64	__nonatomic_and64(volatile TAny* a, TUint64 v)
	{
	__AND(TUint64);
	}

TUint64	__nonatomic_ior64(volatile TAny* a, TUint64 v)
	{
	__IOR(TUint64);
	}

TUint64	__nonatomic_xor64(volatile TAny* a, TUint64 v)
	{
	__XOR(TUint64);
	}

TUint64	__nonatomic_axo64(volatile TAny* a, TUint64 u, TUint64 v)
	{
	__AXO(TUint64);
	}

TUint64	__nonatomic_tau64(volatile TAny* a, TUint64 t, TUint64 u, TUint64 v)
	{
	__TA(TUint64);
	}

TInt64	__nonatomic_tas64(volatile TAny* a, TInt64 t, TInt64 u, TInt64 v)
	{
	__TA(TInt64);
	}

} // extern "C"


#define	DEBUGPRINTxyrc()	\
		DEBUGPRINTVAR(x);	\
		DebugPrintVar(y, "y", __LINE__);	\
		DebugPrintVar(r, "r", __LINE__);	\
		DebugPrintVar(c, "c", __LINE__)


template<class T> TInt DoLoadTest(TInt aIndex, TAny* aPtr, T aInitialValue)
	{
#ifdef __EXTRA_DEBUG__
	DEBUGPRINT("DoLoadTest %d %08x", aIndex, aPtr);
#endif
	typename TLoadFn<T>::F atomic = (typename TLoadFn<T>::F)AtomicFuncPtr[aIndex];
	typename TLoadFn<T>::F control = (typename TLoadFn<T>::F)ControlFuncPtr[aIndex];
	T& x = *(T*)aPtr;
	x = aInitialValue;
	T y = aInitialValue;
	T r = atomic(&x);
	T c = control(&y);
	if (r!=c || x!=y)
		{
		DEBUGPRINTxyrc();
		return __LINE__;
		}
	return 0;
	}

template<class T> TInt DoRmw1Test(TInt aIndex, TAny* aPtr, T aInitialValue, T a1)
	{
#ifdef __EXTRA_DEBUG__
	DEBUGPRINT("DoRmw1Test %d %08x", aIndex, aPtr);
#endif
	typename TRmw1Fn<T>::F atomic = (typename TRmw1Fn<T>::F)AtomicFuncPtr[aIndex];
	typename TRmw1Fn<T>::F control = (typename TRmw1Fn<T>::F)ControlFuncPtr[aIndex];
	T& x = *(T*)aPtr;
	x = aInitialValue;
	T y = aInitialValue;
	T r = atomic(&x,a1);
	T c = control(&y,a1);
	if (r!=c || x!=y)
		{
		DEBUGPRINTxyrc();
		return __LINE__;
		}
	return 0;
	}

template<class T> TInt DoRmw2Test(TInt aIndex, TAny* aPtr, T aInitialValue, T a1, T a2)
	{
#ifdef __EXTRA_DEBUG__
	DEBUGPRINT("DoRmw2Test %d %08x", aIndex, aPtr);
#endif
	typename TRmw2Fn<T>::F atomic = (typename TRmw2Fn<T>::F)AtomicFuncPtr[aIndex];
	typename TRmw2Fn<T>::F control = (typename TRmw2Fn<T>::F)ControlFuncPtr[aIndex];
	T& x = *(T*)aPtr;
	x = aInitialValue;
	T y = aInitialValue;
	T r = atomic(&x,a1,a2);
	T c = control(&y,a1,a2);
	if (r!=c || x!=y)
		{
		DEBUGPRINTxyrc();
		return __LINE__;
		}
	return 0;
	}

template<class T> TInt DoRmw3Test(TInt aIndex, TAny* aPtr, T aInitialValue, T a1, T a2, T a3)
	{
#ifdef __EXTRA_DEBUG__
	DEBUGPRINT("DoRmw3Test %d %08x", aIndex, aPtr);
#endif
	typename TRmw3Fn<T>::F atomic = (typename TRmw3Fn<T>::F)AtomicFuncPtr[aIndex];
	typename TRmw3Fn<T>::F control = (typename TRmw3Fn<T>::F)ControlFuncPtr[aIndex];
	T& x = *(T*)aPtr;
	x = aInitialValue;
	T y = aInitialValue;
	T r = atomic(&x,a1,a2,a3);
	T c = control(&y,a1,a2,a3);
	if (r!=c || x!=y)
		{
		DEBUGPRINTxyrc();
		return __LINE__;
		}
	return 0;
	}

template<class T> TInt DoCasTest(TInt aIndex, TAny* aPtr, T aInitialValue, T aExpectedValue, T aFinalValue)
	{
#ifdef __EXTRA_DEBUG__
	DEBUGPRINT("DoCasTest %d %08x", aIndex, aPtr);
#endif
	typename TCasFn<T>::F atomic = (typename TCasFn<T>::F)AtomicFuncPtr[aIndex];
	typename TCasFn<T>::F control = (typename TCasFn<T>::F)ControlFuncPtr[aIndex];
	T& x = *(T*)aPtr;
	x = aInitialValue;
	T ex = aExpectedValue;
	T y = aInitialValue;
	T ey = aExpectedValue;
	TBool r = atomic(&x,&ex,aFinalValue);
	TBool c = control(&y,&ey,aFinalValue);
	TInt line = 0;
	if (r && !c)
		line = __LINE__;
	else if (!r && c)
		line = __LINE__;
	else if (x!=y)
		line = __LINE__;
	else if (ex!=ey)
		line = __LINE__;
	else if (r && x!=aFinalValue)
		line = __LINE__;
	else if (!r && ex!=aInitialValue)
		line = __LINE__;
	if (line)
		{
		DEBUGPRINT("r=%d",r);
		DEBUGPRINTVAR(x);
		DebugPrintVar(ex, "ex", __LINE__);
		DEBUGPRINT("c=%d",c);
		DebugPrintVar(y, "y", __LINE__);
		DebugPrintVar(ey, "ey", __LINE__);
		}
	return line;
	}



TEnclosed::TEnclosed(TInt aSize)
	{
	iOffset = -1;
	iSize = aSize;
	iData = (TUint64*)((T_UintPtr(i_Data) + 7) &~ 7);	// align up to next 8 byte boundary
	iBackup = iData + 8;
	}

TAny* TEnclosed::Ptr()
	{
	return ((TUint8*)iData + iOffset);
	}

TInt TEnclosed::Next()
	{
	const TInt KLimit[8] = {8, 16, 0, 32, 0, 0, 0, 32};
	if (iOffset<0)
		iOffset = 0;
	else
		{
		TInt r = Verify();
		if (r!=0)
			return r;
		iOffset += iSize;
		}
	if (iOffset >= KLimit[iSize-1])
		return KErrEof;
	Init();
	return KErrNone;
	}

void TEnclosed::Init()
	{
	TUint32 x = iOffset+1;
	x |= (x<<8);
	x |= (x<<16);
	TUint32* d = (TUint32*)iData;
	TUint32* b = (TUint32*)iBackup;
	TInt i;
	for (i=0; i<16; ++i)
		{
		*d++ = x;
		*b++ = x;
		x = 69069*x + 41;
		}
	}

TInt TEnclosed::Verify()
	{
	TUint8* d = (TUint8*)iData;
	const TUint8* b = (const TUint8*)iBackup;
	TInt i;
	for (i=0; i<iSize; ++i)
		d[iOffset+i] = b[iOffset+i];
	if (memcompare(b,64,d,64))
		{
		DEBUGPRINT("FAIL! iOffset=%02x, sizeof(T)=%1d", iOffset, iSize);
		for (i=0; i<64; ++i)
			{
			if (d[i]!=b[i])
				{
				DEBUGPRINT("d[%02x]=%02x b[%02x]=%02x", i, d[i], i, b[i]);
				}
			}
		return __LINE__;
		}
	return 0;
	}


TInt TDGBase::Execute()
	{
	PFV af0 = AtomicFuncPtr[iIndex];
	PFV cf0 = ControlFuncPtr[iIndex];
	if (!af0 || !cf0)
		return __LINE__;
	TUint attr = FuncAttr[iIndex];
	TInt type = ATTR_TO_TYPE(attr);
	TInt size = ATTR_TO_SIZE(attr);
	TInt func = ATTR_TO_FUNC(attr);
	if (type==EFuncTypeInvalid)
		return __LINE__;
#ifdef __EXTRA_DEBUG__
	TInt ord = ATTR_TO_ORD(attr);
	DEBUGPRINT("A=%08x T=%d O=%d S=%d F=%d", attr, type, ord, size, func);
#endif
	TEnclosed enc(size);
	TInt res = 0;
	while ( (res = enc.Next()) == KErrNone )
		{
#ifdef __EXTRA_DEBUG__
		DEBUGPRINT("Offset %02x", enc.Offset());
#endif
		TAny* ptr = enc.Ptr();
		switch (type)
			{
			case EFuncTypeLoad:
				{
				switch (size)
					{
					case 1:	res = DoLoadTest<TUint8>(iIndex, ptr, (TUint8)i0); break;
					case 2:	res = DoLoadTest<TUint16>(iIndex, ptr, (TUint16)i0); break;
					case 4:	res = DoLoadTest<TUint32>(iIndex, ptr, (TUint32)i0); break;
					case 8:	res = DoLoadTest<TUint64>(iIndex, ptr, i0); break;
					default: res = __LINE__; break;
					}
				break;
				}
			case EFuncTypeRmw1:
				{
				switch (size)
					{
					case 1:	res = DoRmw1Test<TUint8>(iIndex, ptr, (TUint8)i0, (TUint8)i1); break;
					case 2:	res = DoRmw1Test<TUint16>(iIndex, ptr, (TUint16)i0, (TUint16)i1); break;
					case 4:	res = DoRmw1Test<TUint32>(iIndex, ptr, (TUint32)i0, (TUint32)i1); break;
					case 8:	res = DoRmw1Test<TUint64>(iIndex, ptr, i0, i1); break;
					default: res = __LINE__; break;
					}
				break;
				}
			case EFuncTypeRmw2:
				{
				switch (size)
					{
					case 1:	res = DoRmw2Test<TUint8>(iIndex, ptr, (TUint8)i0, (TUint8)i1, (TUint8)i2); break;
					case 2:	res = DoRmw2Test<TUint16>(iIndex, ptr, (TUint16)i0, (TUint16)i1, (TUint16)i2); break;
					case 4:	res = DoRmw2Test<TUint32>(iIndex, ptr, (TUint32)i0, (TUint32)i1, (TUint32)i2); break;
					case 8:	res = DoRmw2Test<TUint64>(iIndex, ptr, i0, i1, i2); break;
					default: res = __LINE__; break;
					}
				break;
				}
			case EFuncTypeRmw3:
				{
				if (func==EAtomicFuncTAU)
					{
					switch (size)
						{
						case 1:	res = DoRmw3Test<TUint8>(iIndex, ptr, (TUint8)i0, (TUint8)i1, (TUint8)i2, (TUint8)i3); break;
						case 2:	res = DoRmw3Test<TUint16>(iIndex, ptr, (TUint16)i0, (TUint16)i1, (TUint16)i2, (TUint16)i3); break;
						case 4:	res = DoRmw3Test<TUint32>(iIndex, ptr, (TUint32)i0, (TUint32)i1, (TUint32)i2, (TUint32)i3); break;
						case 8:	res = DoRmw3Test<TUint64>(iIndex, ptr, i0, i1, i2, i3); break;
						default: res = __LINE__; break;
						}
					}
				else if (func==EAtomicFuncTAS)
					{
					switch (size)
						{
						case 1:	res = DoRmw3Test<TInt8>(iIndex, ptr, (TInt8)i0, (TInt8)i1, (TInt8)i2, (TInt8)i3); break;
						case 2:	res = DoRmw3Test<TInt16>(iIndex, ptr, (TInt16)i0, (TInt16)i1, (TInt16)i2, (TInt16)i3); break;
						case 4:	res = DoRmw3Test<TInt32>(iIndex, ptr, (TInt32)i0, (TInt32)i1, (TInt32)i2, (TInt32)i3); break;
						case 8:	res = DoRmw3Test<TInt64>(iIndex, ptr, i0, i1, i2, i3); break;
						default: res = __LINE__; break;
						}
					}
				else
					res = __LINE__;
				break;
				}
			case EFuncTypeCas:
				{
				switch (size)
					{
					case 1:	res = DoCasTest<TUint8>(iIndex, ptr, (TUint8)i0, (TUint8)i1, (TUint8)i2); break;
					case 2:	res = DoCasTest<TUint16>(iIndex, ptr, (TUint16)i0, (TUint16)i1, (TUint16)i2); break;
					case 4:	res = DoCasTest<TUint32>(iIndex, ptr, (TUint32)i0, (TUint32)i1, (TUint32)i2); break;
					case 8:	res = DoCasTest<TUint64>(iIndex, ptr, i0, i1, i2); break;
					default: res = __LINE__; break;
					}
				break;
				}
			default:
				res = __LINE__;
				break;
			}
		if (res)
			return res;
		}
	if (res == KErrEof)
		res = 0;
	return res;
	}

#ifndef __KERNEL_MODE__
void TDGBase::Dump(const char* aTitle)
	{
	TPtrC8 fname8((const TText8*)FuncName[iIndex]);
	TBuf<64> fname;
	fname.Copy(fname8);
	DEBUGPRINT(aTitle);
	DEBUGPRINT("iIndex=%d (%S)", iIndex, &fname);
	DEBUGPRINT("i0 = %08x %08x", I64HIGH(i0), I64LOW(i0));
	DEBUGPRINT("i1 = %08x %08x", I64HIGH(i1), I64LOW(i1));
	DEBUGPRINT("i2 = %08x %08x", I64HIGH(i2), I64LOW(i2));
	DEBUGPRINT("i3 = %08x %08x", I64HIGH(i3), I64LOW(i3));
	}
#endif

template<class T> TInt DoSwap(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TRmw1Fn<T>::F atomic = (typename TRmw1Fn<T>::F)AtomicFuncPtr[aA.iIndex];
	T newv = (T)aA.i0;
	T orig = atomic(aPtr, newv);
	T xr = (T)(newv ^ orig);
	aT->iXor ^= xr;
	T diff = (T)(newv - orig);
	aT->iDiff += diff;
	return 0;
	}

template<class T> TInt DoAdd(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TRmw1Fn<T>::F atomic = (typename TRmw1Fn<T>::F)AtomicFuncPtr[aA.iIndex];
	T arg = (T)aA.i0;
	T orig = atomic(aPtr, arg);
	T xr = (T)((arg+orig) ^ orig);
	aT->iXor ^= xr;
	aT->iDiff += arg;
	return 0;
	}

template<class T> TInt DoXor(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TRmw1Fn<T>::F atomic = (typename TRmw1Fn<T>::F)AtomicFuncPtr[aA.iIndex];
	T arg = (T)aA.i0;
	T orig = atomic(aPtr, arg);
	T diff = (T)((arg^orig) - orig);
	aT->iDiff += diff;
	aT->iXor ^= arg;
	return 0;
	}

template<class T> TInt DoAndOr(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TRmw1Fn<T>::F atomic_and = (typename TRmw1Fn<T>::F)AtomicFuncPtr[aA.iIndex];
	typename TRmw1Fn<T>::F atomic_or = (typename TRmw1Fn<T>::F)AtomicFuncPtr[aA.iIndex+4];
	T aarg = (T)aA.i0;
	T oarg = (T)aA.i1;
	T aorig = atomic_and(aPtr, aarg);
	T oorig = atomic_or(aPtr, oarg);
	T adiff = (T)((aorig & aarg) - aorig);
	T odiff = (T)((oorig | oarg) - oorig);
	aT->iDiff += adiff + odiff;
	T axor = (T)((aorig & aarg) ^ aorig);
	T oxor = (T)((oorig | oarg) ^ oorig);
	aT->iXor ^= axor ^ oxor;
	return 0;
	}

template<class T> TInt DoAxo(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TRmw2Fn<T>::F atomic = (typename TRmw2Fn<T>::F)AtomicFuncPtr[aA.iIndex];
	T aarg = (T)aA.i0;
	T xarg = (T)aA.i1;
	T orig = atomic(aPtr, aarg, xarg);
	T newv = (T)((orig & aarg) ^ xarg);
	aT->iDiff += (newv - orig);
	aT->iXor ^= (newv ^ orig);
	return 0;
	}

template<class T> TInt DoThAdd(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TRmw3Fn<T>::F atomic = (typename TRmw3Fn<T>::F)AtomicFuncPtr[aA.iIndex];
	T thr = (T)aA.i0;
	T arg1 = (T)aA.i1;
	T arg2 = (T)aA.i2;
	T orig = atomic(aPtr, thr, arg1, arg2);
	T newv = (T)((orig >= thr) ? (orig + arg1) : (orig + arg2));
	aT->iDiff += (orig >= thr) ? arg1 : arg2;
	aT->iXor ^= (newv ^ orig);
	return 0;
	}

template<class T> TInt DoCas(TAny* aPtr, TPerThread* aT, TAtomicAction& aA, T*)
	{
	typename TCasFn<T>::F atomic = (typename TCasFn<T>::F)AtomicFuncPtr[aA.iIndex];
	T orig = *(const volatile T*)aPtr;
	T newv;
	TBool done = FALSE;
	TUint32 fails = 0xffffffffu;
	do	{
		++fails;
		newv = Transform<T>::F(orig);
		done = atomic(aPtr, &orig, newv);
		} while(!done);
	aT->iFailCount += fails;
	++aT->iDiff;
	aT->iXor ^= (newv ^ orig);
	return 0;
	}

volatile TUint Dummy;
extern "C" TInt DoAtomicAction(TAny* aPtr, TPerThread* aT, TAtomicAction& aA)
	{
	TUint x = TUint(aT)*0x9E3779B9u;
	x = (x>>8)&15;
	while(x--)
		++Dummy;
	TInt r = KErrNotSupported;
	TUint attr = FuncAttr[aA.iIndex];
	TUint func = ATTR_TO_FUNC(attr);
	TUint size = ATTR_TO_SIZE(attr);
	switch (size)
		{
		case 1:
			{
			TUint8 xx;
			TUint8* dummy = &xx;
			TInt8 yy;
			TInt8* sdummy = &yy;
			switch (func)
				{
				case EAtomicFuncSWP:	r=DoSwap<TUint8>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncADD:	r=DoAdd<TUint8>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAND:	r=DoAndOr<TUint8>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncXOR:	r=DoXor<TUint8>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAXO:	r=DoAxo<TUint8>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAU:	r=DoThAdd<TUint8>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAS:	r=DoThAdd<TInt8>(aPtr, aT, aA, sdummy); break;
				case EAtomicFuncCAS:	r=DoCas<TUint8>(aPtr, aT, aA, dummy); break;
				default: break;
				}
			break;
			}
		case 2:
			{
			TUint16 xx;
			TUint16* dummy = &xx;
			TInt16 yy;
			TInt16* sdummy = &yy;
			switch (func)
				{
				case EAtomicFuncSWP:	r=DoSwap<TUint16>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncADD:	r=DoAdd<TUint16>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAND:	r=DoAndOr<TUint16>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncXOR:	r=DoXor<TUint16>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAXO:	r=DoAxo<TUint16>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAU:	r=DoThAdd<TUint16>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAS:	r=DoThAdd<TInt16>(aPtr, aT, aA, sdummy); break;
				case EAtomicFuncCAS:	r=DoCas<TUint16>(aPtr, aT, aA, dummy); break;
				default: break;
				}
			break;
			}
		case 4:
			{
			TUint32 xx;
			TUint32* dummy = &xx;
			TInt32 yy;
			TInt32* sdummy = &yy;
			switch (func)
				{
				case EAtomicFuncSWP:	r=DoSwap<TUint32>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncADD:	r=DoAdd<TUint32>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAND:	r=DoAndOr<TUint32>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncXOR:	r=DoXor<TUint32>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAXO:	r=DoAxo<TUint32>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAU:	r=DoThAdd<TUint32>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAS:	r=DoThAdd<TInt32>(aPtr, aT, aA, sdummy); break;
				case EAtomicFuncCAS:	r=DoCas<TUint32>(aPtr, aT, aA, dummy); break;
				default: break;
				}
			break;
			}
		case 8:
			{
			TUint64A xx;
			TUint64* dummy = &xx;
			TInt64A yy;
			TInt64* sdummy = &yy;
			switch (func)
				{
				case EAtomicFuncSWP:	r=DoSwap<TUint64>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncADD:	r=DoAdd<TUint64>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAND:	r=DoAndOr<TUint64>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncXOR:	r=DoXor<TUint64>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncAXO:	r=DoAxo<TUint64>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAU:	r=DoThAdd<TUint64>(aPtr, aT, aA, dummy); break;
				case EAtomicFuncTAS:	r=DoThAdd<TInt64>(aPtr, aT, aA, sdummy); break;
				case EAtomicFuncCAS:	r=DoCas<TUint64>(aPtr, aT, aA, dummy); break;
				default: break;
				}
			break;
			}
		default:
			break;
		}
	++aT->iCount;
	return r;
	}



