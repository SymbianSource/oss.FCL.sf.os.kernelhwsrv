// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\common\common.h
// 
//

#ifndef __E32_COMMON_H__
#define __E32_COMMON_H__

#ifdef __KERNEL_MODE__
#include <e32cmn.h>
#include <e32panic.h>
#include "u32std.h"
#else
#include <e32std.h>
#include <e32base.h>
#include <e32math.h>
#include <e32svr.h>
#include <e32ver.h>
#include <e32hal.h>
#include <e32panic.h>
#include <u32exec.h>
#endif

GLREF_C void Panic(TCdtPanic aPanic);
GLDEF_C void PanicBadArrayIndex();
GLREF_C TInt __DoConvertNum(TUint, TRadix, TUint, TUint8*&);
GLREF_C TInt __DoConvertNum(Uint64, TRadix, TUint, TUint8*&);

#ifdef __KERNEL_MODE__
GLREF_C void KernHeapFault(TCdtPanic aPanic);
GLREF_C void KHeapCheckThreadState();
TInt StringLength(const TUint16* aPtr);
TInt StringLength(const TUint8* aPtr);

#define	STD_CLASS					Kern
#define	STRING_LENGTH(s)			StringLength(s)
#define	STRING_LENGTH_16(s)			StringLength(s)
#define	PANIC_CURRENT_THREAD(c,r)	Kern::PanicCurrentThread(c, r)
#define __KERNEL_CHECK_RADIX(r)		__ASSERT_ALWAYS(((r)==EDecimal)||((r)==EHex),Panic(EInvalidRadix))
#define	APPEND_BUF_SIZE				10
#define	APPEND_BUF_SIZE_64			20
#define	HEAP_PANIC(r)				Kern::Printf("HEAP CORRUPTED %s %d", __FILE__, __LINE__), RHeapK::Fault(r)
#define	GET_PAGE_SIZE(x)			x = M::PageSizeInBytes()
#define	DIVISION_BY_ZERO()			FAULT()

#ifdef _DEBUG
#define	__CHECK_THREAD_STATE		RHeapK::CheckThreadState()
#else
#define	__CHECK_THREAD_STATE
#endif

#else

#define	STD_CLASS					User
#define	STRING_LENGTH(s)			User::StringLength(s)
#define	STRING_LENGTH_16(s)			User::StringLength(s)
#define	PANIC_CURRENT_THREAD(c,r)	User::Panic(c, r)
#define	MEM_COMPARE_16				Mem::Compare
#define __KERNEL_CHECK_RADIX(r)
#define	APPEND_BUF_SIZE				32
#define	APPEND_BUF_SIZE_64			64
#define	HEAP_PANIC(r)				RDebug::Printf("HEAP CORRUPTED %s %d", __FILE__, __LINE__), Panic(r)
#define	GET_PAGE_SIZE(x)			UserHal::PageSizeInBytes(x)
#define	DIVISION_BY_ZERO()			User::RaiseException(EExcIntegerDivideByZero)
#define	__CHECK_THREAD_STATE

#endif	// __KERNEL_MODE__

#endif
