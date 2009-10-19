// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_memorytest.h
// 
//

#ifndef __D_MEMORYTEST_H__
#define __D_MEMORYTEST_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KMemoryTestLddName,"d_memorytest");

const TInt UCPageCount = 2;//Page count of user chunk used in physical pinning tests.

class RMemoryTestLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EReadWriteMemory,
		EReadMemory,
		EWriteMemory,
		ETestAllocZerosMemory,
		ETestReAllocZerosMemory,
		ETestAllocPhysTest,
		ETestAllocPhysTest1,
		ECreateVirtualPinObject,
		EPinVirtualMemory,
		EUnpinVirtualMemory,
		EDestroyVirtualPinObject,
		ECreatePhysicalPinObject,
		EPinPhysicalMemory,
		EPinPhysicalMemoryRO,
		ECheckPageList,
		ESyncPinnedPhysicalMemory,
		EMovePinnedPhysicalMemory,
		EInvalidatePinnedPhysicalMemory,
		EUnpinPhysicalMemory,
		EDestroyPhysicalPinObject,
		EPinKernelPhysicalMemory,
		ESetPanicTrace,
		EIsMemoryPresent
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{
		TInt r=User::LoadLogicalDevice(KMemoryTestLddName);
		if(r==KErrNone || r==KErrAlreadyExists)
			r=DoCreate(KMemoryTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue);
		return r;
		};
	inline TInt ReadWriteMemory(TAny* aPtr)
		{ return DoControl(EReadWriteMemory,aPtr); }
	inline TInt ReadMemory(TAny* aPtr,TUint32& aValue)
		{ return DoControl(EReadMemory,aPtr,&aValue); }
	inline TInt WriteMemory(TAny* aPtr,TUint32 aValue)
		{ return DoControl(EWriteMemory,aPtr,(TAny*)aValue); }
	inline TInt TestAllocZerosMemory()
		{ return DoControl(ETestAllocZerosMemory,NULL,NULL); }
	inline TInt TestReAllocZerosMemory()
		{ return DoControl(ETestReAllocZerosMemory,NULL,NULL); }
	inline TInt AllocPhysTest(TUint32 aIters, TUint32 aSize)
		{ return DoControl(ETestAllocPhysTest,(TAny*)aIters, (TAny*)aSize); } 
	inline TInt AllocPhysTest1(TUint32 aIters, TUint32 aSize)
		{ return DoControl(ETestAllocPhysTest1,(TAny*)aIters, (TAny*)aSize); } 
	inline TInt CreateVirtualPinObject()
		{ return DoControl(ECreateVirtualPinObject); }
	inline TInt PinVirtualMemory(TLinAddr aStart,TUint aSize)
		{ return DoControl(EPinVirtualMemory,(TAny*)aStart,(TAny*)aSize); }
	inline TInt UnpinVirtualMemory()
		{ return DoControl(EUnpinVirtualMemory); }
	inline TInt DestroyVirtualPinObject()
		{ return DoControl(EDestroyVirtualPinObject); }
	inline TInt CreatePhysicalPinObject()
		{ return DoControl(ECreatePhysicalPinObject); }
	inline TInt PinPhysicalMemory(TLinAddr aStart,TUint aSize)
		{ return DoControl(EPinPhysicalMemory,(TAny*)aStart,(TAny*)aSize); }
	inline TInt PinPhysicalMemoryRO(TLinAddr aStart,TUint aSize)
		{ return DoControl(EPinPhysicalMemoryRO,(TAny*)aStart,(TAny*)aSize); }
	inline TInt CheckPageList(TUint8* aStart)
		{ return DoControl(ECheckPageList,(TAny*)aStart); }
	inline TInt SyncPinnedPhysicalMemory(TUint aOffset,TUint aSize)
		{ return DoControl(ESyncPinnedPhysicalMemory,(TAny*)aOffset,(TAny*)aSize); }
	inline TInt MovePinnedPhysicalMemory(TInt aPageNumber)
		{ return DoControl(EMovePinnedPhysicalMemory,(TAny*)aPageNumber); }
	inline TInt InvalidatePinnedPhysicalMemory(TUint aOffset,TUint aSize)
		{ return DoControl(EInvalidatePinnedPhysicalMemory,(TAny*)aOffset,(TAny*)aSize); }
	inline TInt UnpinPhysicalMemory()
		{ return DoControl(EUnpinPhysicalMemory); }
	inline TInt DestroyPhysicalPinObject()
		{ return DoControl(EDestroyPhysicalPinObject); }
	inline TInt PinKernelPhysicalMemory()
		{ return DoControl(EPinKernelPhysicalMemory); }
	inline TBool SetPanicTrace(TBool aEnable)
		{ return DoControl(ESetPanicTrace,(TAny*)aEnable); }
	inline TInt IsMemoryPresent(const TAny* aPtr)
		{ return DoControl(EIsMemoryPresent,(TAny*)aPtr); }
#endif
	};


#endif
