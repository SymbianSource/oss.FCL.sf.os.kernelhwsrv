// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_demandpaging.h
// 
//

#ifndef __D_DEMANDPAGING_H__
#define __D_DEMANDPAGING_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KDemandPagingTestLddName,"D_DEMANDPAGING");

class RDemandPagingTestLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ELockTest,
		ESetRealtimeTrace,
		EDoConsumeContiguousRamTest,
		ELock,
		EUnlock,
		EReadHoldingMutexTest,
		ECreatePlatHwChunk,
		EDestroyPlatHwChunk
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KDemandPagingTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue); }
	inline TInt LockTest(const TAny* aBuffer, TInt aSize)
		{ return DoControl(ELockTest,(TAny*)aBuffer,(TAny*)aSize); }
	inline void SetRealtimeTrace(TBool aEnable)
		{ DoControl(ESetRealtimeTrace,(TAny*)aEnable); }
	inline TInt DoConsumeContiguousRamTest(TInt aAlign, TInt aPages)
		{ return DoControl(EDoConsumeContiguousRamTest,(TAny*)aAlign, (TAny*)aPages); }
	inline TInt Lock(const TAny* aBuffer, TInt aSize)
		{ return DoControl(ELock,(TAny*)aBuffer,(TAny*)aSize); }
	inline TInt Unlock()
		{ return DoControl(EUnlock); }
	inline TInt ReadHoldingMutexTest(TAny* aLocalBuf)
		{ return DoControl(EReadHoldingMutexTest, aLocalBuf); }
	inline TInt CreatePlatHwChunk(TInt aSize, TLinAddr& aAddr)
		{ return DoControl(ECreatePlatHwChunk, (TAny*)aSize, (TAny*)&aAddr); }
	inline TInt DestroyPlatHwChunk()
		{ return DoControl(EDestroyPlatHwChunk); }
#endif
	};


#endif
