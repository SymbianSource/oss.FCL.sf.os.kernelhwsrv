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
// e32\nkern\nk_old.cpp
// 
//

#include <e32atomics.h>

extern "C" {

EXPORT_C TInt __old_LockedInc(TInt& aCount)
	{ return __e32_atomic_add_ord32(&aCount,1); }

EXPORT_C TInt __old_LockedDec(TInt& aCount)
	{ return __e32_atomic_add_ord32(&aCount,0xffffffff); }

EXPORT_C TInt __old_LockedAdd(TInt& aDest, TInt aSrc)
	{ return __e32_atomic_add_ord32(&aDest,aSrc); }

EXPORT_C TInt64 __old_LockedInc64(TInt64& aCount)
	{ return __e32_atomic_add_ord64(&aCount,1); }

EXPORT_C TInt64 __old_LockedDec64(TInt64& aCount)
	{ return __e32_atomic_add_ord64(&aCount,TUint64(TInt64(-1))); }

EXPORT_C TInt64 __old_LockedAdd64(TInt64& aDest, TInt64 aSrc)
	{ return __e32_atomic_add_ord64(&aDest,aSrc); }

EXPORT_C TUint32 __old_LockedSetClear(TUint32& aDest, TUint32 aClearMask, TUint32 aSetMask)
	{ return __e32_atomic_axo_ord32(&aDest,~(aClearMask|aSetMask),aSetMask); }

EXPORT_C TUint16 __old_LockedSetClear16(TUint16& aDest, TUint16 aClearMask, TUint16 aSetMask)
	{ return __e32_atomic_axo_ord16(&aDest,TUint16(~(aClearMask|aSetMask)),aSetMask); }

EXPORT_C TUint8 __old_LockedSetClear8(TUint8& aDest, TUint8 aClearMask, TUint8 aSetMask)
	{ return __e32_atomic_axo_ord8(&aDest,TUint8(~(aClearMask|aSetMask)),aSetMask); }

EXPORT_C TInt __old_SafeInc(TInt& aCount)
	{ return __e32_atomic_tas_ord32(&aCount,1,1,0); }

EXPORT_C TInt __old_SafeDec(TInt& aCount)
	{ return __e32_atomic_tas_ord32(&aCount,1,-1,0); }

EXPORT_C TInt __old_AddIfGe(TInt& aCount, TInt aLimit, TInt aInc)
	{ return __e32_atomic_tas_ord32(&aCount,aLimit,aInc,0); }

EXPORT_C TInt __old_AddIfLt(TInt& aCount, TInt aLimit, TInt aInc)
	{ return __e32_atomic_tas_ord32(&aCount,aLimit,0,aInc); }

EXPORT_C TAny* __old_SafeSwap(TAny* aNewValue, TAny*& aPtr)
	{ return __e32_atomic_swp_ord_ptr(&aPtr, aNewValue); }

EXPORT_C TUint8 __old_SafeSwap8(TUint8 aNewValue, TUint8& aPtr)
	{ return __e32_atomic_swp_ord8(&aPtr, aNewValue); }

EXPORT_C TUint16 __old_SafeSwap16(TUint16 aNewValue, TUint16& aPtr)
	{ return __e32_atomic_swp_ord16(&aPtr, aNewValue); }

EXPORT_C TBool __old_CompareAndSwap(TAny*& aPtr, TAny* aExpected, TAny* aNew)
	{ return __e32_atomic_cas_ord_ptr(&aPtr, &aExpected, aNew); }

EXPORT_C TBool __old_CompareAndSwap8(TUint8& aPtr, TUint8 aExpected, TUint8 aNew)
	{ return __e32_atomic_cas_ord8(&aPtr, (TUint8*)&aExpected, (TUint8)aNew); }

EXPORT_C TBool __old_CompareAndSwap16(TUint16& aPtr, TUint16 aExpected, TUint16 aNew)
	{ return __e32_atomic_cas_ord16(&aPtr, (TUint16*)&aExpected, (TUint16)aNew); }

}
