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
// e32test\mmu\d_memorytest.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "d_memorytest.h"

//
// Class definitions
//

class DMemoryTestFactory : public DLogicalDevice
	{
public:
	~DMemoryTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DMemoryTestChannel : public DLogicalChannelBase
	{
public:
	DMemoryTestChannel();
	~DMemoryTestChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TInt TestAllocZerosMemory();
	TInt TestReAllocZerosMemory();
	TInt AllocTest1(TInt aSize);
	TInt ReAllocTest1();
	TInt ReAllocTest2(TUint8*& mem1, TUint8*& mem2, TUint8*& mem3);
	TInt AllocPhysTest(TUint32 aIters, TUint32 aSize); 
	TInt AllocPhysTest1(TUint32 aIters, TUint32 aSize); 
public:
	DMemoryTestFactory*	iFactory;
	TVirtualPinObject* iVirtualPinObject;
	
	struct{
		TPhysicalPinObject* iObject;
		TPhysAddr iPhysAddr;
		TPhysAddr iPhysPageList[KUCPageCount];
		TUint 	iColour;
		TUint32 iActualMapAttr;
		}iPhysicalPinning;

	struct{
		TKernelMapObject* iObject;
		TPhysAddr iPhysPageList[KUCPageCount];
		TLinAddr iLinAddr;
		}iKernelMapping;

	TUint32 iPageSize;
	};

//
// DMemoryTestFactory
//

TInt DMemoryTestFactory::Install()
	{
	return SetName(&KMemoryTestLddName);
	}

DMemoryTestFactory::~DMemoryTestFactory()
	{
	}

void DMemoryTestFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DMemoryTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DMemoryTestChannel* channel=new DMemoryTestChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DMemoryTestFactory;
	}

//
// DMemoryTestChannel
//

TInt DMemoryTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DMemoryTestChannel::DMemoryTestChannel()
	{
	iPageSize = Kern::RoundToPageSize(1);
	}

DMemoryTestChannel::~DMemoryTestChannel()
	{
	Kern::DestroyVirtualPinObject(iVirtualPinObject);
	}


TInt DMemoryTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r=KErrNotSupported;

	switch(aFunction)
		{
	case RMemoryTestLdd::EReadWriteMemory:
	case RMemoryTestLdd::EReadMemory:
	case RMemoryTestLdd::EWriteMemory:
		{
		TUint32 value=(TUint32)a2;
#ifdef _DEBUG
		TInt debugMask = Kern::CurrentThread().iDebugMask;
		Kern::CurrentThread().iDebugMask = debugMask&~(1U<<KPANIC);
#endif
		XTRAP(r, XT_DEFAULT,
			if(aFunction==RMemoryTestLdd::EReadWriteMemory)
				{
				kumemget32(&value,a1,4);
				kumemput32(a1,&value,4);
				}
			else if(aFunction==RMemoryTestLdd::EReadMemory)
				kumemget32(&value,a1,4);
			else if(aFunction==RMemoryTestLdd::EWriteMemory)
				kumemput32(a1,&value,4);
			);
#ifdef _DEBUG
		Kern::CurrentThread().iDebugMask = debugMask;
#endif
		if(aFunction==RMemoryTestLdd::EReadMemory)
			kumemput32(a2,&value,sizeof(value));

		return r;
		}

	case RMemoryTestLdd::ETestAllocZerosMemory:
	case RMemoryTestLdd::ETestReAllocZerosMemory:
		{
		NKern::ThreadEnterCS();		
		TInt r;
		if (aFunction==RMemoryTestLdd::ETestAllocZerosMemory)
			r=TestAllocZerosMemory();
		else
			r=TestReAllocZerosMemory();
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::ETestAllocPhysTest:
		{
		NKern::ThreadEnterCS();
		r=AllocPhysTest((TUint32)a1,(TUint32)a2);
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::ETestAllocPhysTest1:
		{
		NKern::ThreadEnterCS();
		r=AllocPhysTest1((TUint32)a1,(TUint32)a2);
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::ECreateVirtualPinObject:
		{
		NKern::ThreadEnterCS();
		r=Kern::CreateVirtualPinObject(iVirtualPinObject);
		NKern::ThreadLeaveCS();
		return r;
		}
		
	case RMemoryTestLdd::EPinVirtualMemory:
		return Kern::PinVirtualMemory(iVirtualPinObject, (TLinAddr)a1, (TUint)a2);

	case RMemoryTestLdd::EUnpinVirtualMemory:
		Kern::UnpinVirtualMemory(iVirtualPinObject);
		return KErrNone;

	case RMemoryTestLdd::EDestroyVirtualPinObject:
		{
		NKern::ThreadEnterCS();
		Kern::DestroyVirtualPinObject(iVirtualPinObject);
		NKern::ThreadLeaveCS();
		return KErrNone;
		}

	case RMemoryTestLdd::ESetPanicTrace:
		{
		TBool old = false;
#ifdef _DEBUG
		DThread& thread = Kern::CurrentThread();
		TInt debugMask = thread.iDebugMask;
		if(debugMask&(1U<<KPANIC))
			old = true;
		if(a1)
			debugMask |= (1U<<KPANIC);
		else
			debugMask &= ~(1U<<KPANIC);
		thread.iDebugMask = debugMask;
#endif
		return old;
		}

	case RMemoryTestLdd::EIsMemoryPresent:
#ifndef __WINS__
		return Epoc::LinearToPhysical((TLinAddr)a1) != KPhysAddrInvalid;
#else
		Kern::PanicCurrentThread(_L("IsMemoryPresent should not be used on the emulator"), KErrNotSupported);
		return KErrNotSupported;
#endif

	case RMemoryTestLdd::ECreatePhysicalPinObject:
		{
		NKern::ThreadEnterCS();
		r=Kern::CreatePhysicalPinObject(iPhysicalPinning.iObject);
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::EPinPhysicalMemory:
		return Kern::PinPhysicalMemory(iPhysicalPinning.iObject, (TLinAddr)a1, (TUint)a2, EFalse, iPhysicalPinning.iPhysAddr,
							iPhysicalPinning.iPhysPageList, iPhysicalPinning.iActualMapAttr, iPhysicalPinning.iColour, NULL);

	case RMemoryTestLdd::EPinPhysicalMemoryRO:
		return Kern::PinPhysicalMemory(iPhysicalPinning.iObject, (TLinAddr)a1, (TUint)a2, ETrue, iPhysicalPinning.iPhysAddr,
							iPhysicalPinning.iPhysPageList, iPhysicalPinning.iActualMapAttr, iPhysicalPinning.iColour, NULL);

	case RMemoryTestLdd::ECheckPageList:
		{
#ifdef __WINS__
		return KErrNotSupported;
#else
		TInt i;
		for (i=0;i<KUCPageCount; i++)
			{
			TPhysAddr addr = Epoc::LinearToPhysical((TLinAddr)a1 + i*iPageSize);
			if (addr==KPhysAddrInvalid) 				 return KErrGeneral;
			if (addr!=iPhysicalPinning.iPhysPageList[i]) return KErrNotFound;
			}
		return KErrNone;
#endif		
		}

	case RMemoryTestLdd::ESyncPinnedPhysicalMemory:
		return Cache::SyncPhysicalMemoryBeforeDmaWrite(iPhysicalPinning.iPhysPageList,
									iPhysicalPinning.iColour, (TUint)a1, (TUint)a2, iPhysicalPinning.iActualMapAttr);

	case RMemoryTestLdd::EMovePinnedPhysicalMemory:
		{
#ifdef __WINS__
		return KErrNotSupported;
#else
		TPhysAddr newPage;
		NKern::ThreadEnterCS();
		r = Epoc::MovePhysicalPage(iPhysicalPinning.iPhysPageList[(TUint)a1], newPage);		
		NKern::ThreadLeaveCS();
		return r;
#endif
		}

	case RMemoryTestLdd::EInvalidatePinnedPhysicalMemory:
		{
		r = Cache::SyncPhysicalMemoryBeforeDmaRead(iPhysicalPinning.iPhysPageList,
									iPhysicalPinning.iColour, (TUint)a1, (TUint)a2, iPhysicalPinning.iActualMapAttr);
		if (r==KErrNone)
			r = Cache::SyncPhysicalMemoryAfterDmaRead(iPhysicalPinning.iPhysPageList,
										iPhysicalPinning.iColour, (TUint)a1, (TUint)a2, iPhysicalPinning.iActualMapAttr);
		return r;	
		}
		
	case RMemoryTestLdd::EUnpinPhysicalMemory:
		return Kern::UnpinPhysicalMemory(iPhysicalPinning.iObject);

	case RMemoryTestLdd::EDestroyPhysicalPinObject:
		{
		NKern::ThreadEnterCS();
		r=Kern::DestroyPhysicalPinObject(iPhysicalPinning.iObject);
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::EPinKernelPhysicalMemory:
		{
		TPhysicalPinObject* pinObject;
		TPhysAddr aAddress;
		TPhysAddr aPages[2];
		TUint aColour=0;
		TUint32 actualMemAttr;
		NKern::ThreadEnterCS();
		Kern::CreatePhysicalPinObject(pinObject);
		r = Kern::PinPhysicalMemory(pinObject, (TLinAddr)&aAddress, 4, EFalse, aAddress, aPages, actualMemAttr, aColour, NULL);
		Cache::SyncPhysicalMemoryBeforeDmaWrite(aPages, aColour, 10, 30, actualMemAttr);
		Kern::UnpinPhysicalMemory(pinObject);
		Kern::DestroyPhysicalPinObject(pinObject);
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::ECreateKernelMapObject:
		{
		NKern::ThreadEnterCS();
		r=Kern::CreateKernelMapObject(iKernelMapping.iObject, (TUint)a1);
		NKern::ThreadLeaveCS();
		return r;
		}

	case RMemoryTestLdd::EKernelMapMemory:
		return Kern::MapAndPinMemory(	iKernelMapping.iObject, NULL, (TLinAddr)a1, (TUint)a2, 0,
										iKernelMapping.iLinAddr, iKernelMapping.iPhysPageList);

	case RMemoryTestLdd::EKernelMapMemoryRO:
		return Kern::MapAndPinMemory(	iKernelMapping.iObject, NULL, (TLinAddr)a1, (TUint)a2, Kern::EKernelMap_ReadOnly,
										iKernelMapping.iLinAddr, iKernelMapping.iPhysPageList);

	case RMemoryTestLdd::EKernelMapMemoryInvalid:
		return Kern::MapAndPinMemory(	iKernelMapping.iObject, NULL, (TLinAddr)a1, (TUint)a2, (TUint)~Kern::EKernelMap_ReadOnly,
										iKernelMapping.iLinAddr, iKernelMapping.iPhysPageList);

	case RMemoryTestLdd::EKernelMapCheckPageList:
		{
#ifdef __WINS__
		return KErrNotSupported;
#else
		TUint i = 0;
		for (; i < (TUint)KUCPageCount; i++)
			{
			// Compare the user side address to physical addresses
			TPhysAddr addr = Epoc::LinearToPhysical((TLinAddr)a1 + i*iPageSize);
			if (addr == KPhysAddrInvalid) 				 
				return KErrGeneral;
			if (addr != iKernelMapping.iPhysPageList[i]) 
				return KErrNotFound;
			// Compare the kernel side address to physical addresses
			addr = Epoc::LinearToPhysical(iKernelMapping.iLinAddr + i*iPageSize);
			if (addr == KPhysAddrInvalid) 				 
				return KErrGeneral;
			if (addr != iKernelMapping.iPhysPageList[i])
				return KErrNotFound;
			}
		return KErrNone;
#endif		
		}

	case RMemoryTestLdd::EKernelMapSyncMemory:
		Cache::SyncMemoryBeforeDmaWrite(iKernelMapping.iLinAddr, KUCPageCount*iPageSize);
		return KErrNone;

	case RMemoryTestLdd::EKernelMapInvalidateMemory:
		{
		Cache::SyncMemoryBeforeDmaRead(iKernelMapping.iLinAddr, KUCPageCount*iPageSize);
		Cache::SyncMemoryAfterDmaRead(iKernelMapping.iLinAddr, KUCPageCount*iPageSize);
		return KErrNone;
		}

	case RMemoryTestLdd::EKernelMapMoveMemory:
		{
#ifdef __WINS__
		return KErrNotSupported;
#else
		TPhysAddr newPage;
		NKern::ThreadEnterCS();
		r = Epoc::MovePhysicalPage(iKernelMapping.iPhysPageList[(TUint)a1], newPage);		
		NKern::ThreadLeaveCS();
		return r;
#endif
		}

	case RMemoryTestLdd::EKernelMapReadModifyMemory:
		{
		TUint8* p = (TUint8*)iKernelMapping.iLinAddr;
		// Verify the contents of the data when accessed via the kernel mapping.
		TUint i = 0;
		for (i = 0; i < KUCPageCount*iPageSize; i++)
			{
			if (*p++ != (TUint8)i)
				return KErrCorrupt;
			}
		// Modify the data via the kernel mapping.
		p = (TUint8*)iKernelMapping.iLinAddr;
		for (i = 0; i < KUCPageCount*iPageSize; i++)
			{
			*p++ = (TUint8)(i + 1);
			}
		return KErrNone;
		}
		
	case RMemoryTestLdd::EKernelUnmapMemory:
		Kern::UnmapAndUnpinMemory(iKernelMapping.iObject);
		return KErrNone;

	case RMemoryTestLdd::EDestroyKernelMapObject:
		{
		NKern::ThreadEnterCS();
		Kern::DestroyKernelMapObject(iKernelMapping.iObject);
		NKern::ThreadLeaveCS();
		return KErrNone;
		}
		
	default:
		return KErrNotSupported;
		}
	}

// Fail a test by returning an error code indicating the problem
#define FAIL_ALLOC_TEST(testIndex, byteOffset, unexepectedValue) \
	err = ((testIndex) << 16) | ((byteOffset) << 8) | (unexepectedValue)

TInt DMemoryTestChannel::TestAllocZerosMemory()
	{
	TInt count = 100;
	TInt r = KErrNotSupported;
	TInt size = 256;

	do	{	//re-try up to 100 times if memory conditions are not correct
		r=AllocTest1(size);
		size -= 2;
		} while(((r == KErrNoMemory)||(r == KErrUnknown)) && --count );

	return r;
	}

TInt DMemoryTestChannel::AllocTest1(TInt aSize)
	{
	TInt err = KErrNone;
	TUint8* mem1 = (TUint8*)Kern::Alloc(aSize);
	if (!mem1)
		return KErrNoMemory;
  	memset(mem1, 0xff, aSize);	
	Kern::Free(mem1);
	TUint8* mem2 = (TUint8*)Kern::Alloc(aSize);
	if (!mem2)
		return KErrNoMemory;
	
	if (mem1 != mem2)
		err = KErrUnknown;	// Test inconclusive, can retry

	for (TInt i = 0 ; i<aSize && err==KErrNone; ++i)
		{
		if (mem2[i] != 0)
			FAIL_ALLOC_TEST(1, i, mem2[i]);
		}
	Kern::Free(mem2);

	return err;
	}

TInt DMemoryTestChannel::TestReAllocZerosMemory()
	{
	TInt count = 100;
	TInt r = KErrNotSupported;

	do	{	//re-try up to 100 times if memory conditions are not correct
		r=ReAllocTest1();
		} while(((r == KErrNoMemory)||(r == KErrUnknown)) && --count);

	if (r!=KErrNone)	
		return r;

	count = 100;
	do	{	//	re-try up to 100 times if memory conditions are not correct
		TUint8* mem1 = NULL;
		TUint8* mem2 = NULL;
		TUint8* mem3 = NULL;
		r=ReAllocTest2(mem1, mem2, mem3);
		if (mem1)
			Kern::Free(mem1);
		if (mem2)
			Kern::Free(mem2);
		if (mem3)
			Kern::Free(mem3);
		} while(((r == KErrNoMemory)||(r == KErrUnknown)) && --count);
	
	return r;
	}

// The actual size of the block allocated given the size requested.
#define ALIGNED_SIZE(aReqSize) (_ALIGN_UP(aReqSize + RHeap::EAllocCellSize, RHeap::ECellAlignment) - RHeap::EAllocCellSize)

// We only allocate blocks where the size we get is the size we ask for - this
// just makes testing easier.
const TInt KSize = ALIGNED_SIZE(200), KHalfSize = ALIGNED_SIZE(100), KSmallSize = ALIGNED_SIZE(50);

TInt DMemoryTestChannel::ReAllocTest1()
	{
	// Test case where cell grows
	// 
	// Expected heap layout:
	//   1: [-mem1-------]
	//   2: [-mem1-]
	//   3: [-mem1-------]

	TInt err = KErrNone;
	TUint8* mem1 = (TUint8*)Kern::Alloc(KSize); // 1
	if (!mem1)
		return KErrNoMemory;
	memset(mem1, 0xff, KSize);
	TUint8* mem2 = (TUint8*)Kern::ReAlloc(mem1, KHalfSize); // 2
	if (mem1 != mem2)
		{
		mem1 = 0;
		Kern::Free(mem2);
		return KErrUnknown; // Don't expect move on shrink
		}
	//
	// With DL allocator growth into original area cannot be expected !
	//
//	mem2 = (TUint8*)Kern::ReAlloc(mem1, KSize); // 3
/*	
	if (mem1 != mem2)
		{
		mem1 = 0;
		Kern::Free(mem2);
		return KErrUnknown; // Expect growth into original area
		}
*/
  	mem1 = (TUint8*)Kern::ReAlloc(mem1, KSize); // 3	
	TInt i;
	for (i = 0 ; i<KHalfSize && err==KErrNone; ++i)
		{
		if (mem1[i] != 0xff)
			FAIL_ALLOC_TEST(2, i, mem1[i]);
		}
	for (i = KHalfSize ; i<KSize && err==KErrNone; ++i)
		{
		if (mem1[i] != 0)
			FAIL_ALLOC_TEST(3, i, mem1[i]);
		}

	Kern::Free(mem1);
	return err;
	}

TInt DMemoryTestChannel::ReAllocTest2(TUint8*& mem1, TUint8*& mem2, TUint8*& mem3)
	{	
	// Test case where cell is moved
	// 
	// Expected heap layout:
	//   1: [ mem1 ]
	//   2: [ mem1 ] [ mem2 ]
	//   3: [ mem1 ] [ mem2 ] [ mem3       ]
	//   4:          [ mem2 ] [ mem1       ] 

	mem1 = (TUint8*)Kern::Alloc(KSmallSize); // 1
	if (!mem1)
		return KErrNoMemory;
	memset(mem1, 0xff, KSmallSize);	
	mem2 = (TUint8*)Kern::Alloc(KSmallSize); // 2
	if (!mem2)
		return KErrNoMemory;
	//
	// The following exception is not possible with DL allocator
	//
/*	
	if (mem2 <= (mem1 + KSmallSize))
		return KErrUnknown;	// Expect mem2 higher than mem1
*/		
	memset(mem2, 0xee, KSmallSize);		
	mem3 = (TUint8*)Kern::Alloc(KSize); // 3
	if (!mem3)
		return KErrNoMemory;
	//
	// The following exception is not possible with DL allocator
	//
/*	
	if (mem3 <= (mem2 + KSmallSize))
		return KErrUnknown+2;	// Expect mem3 higher than mem2
*/		
	memset(mem3, 0xdd, KSize);
	Kern::Free(mem3);

/*	
	TUint8* m3 = mem3;
*/	
	mem3 = NULL;

	TUint8* mem4 = (TUint8*)Kern::ReAlloc(mem1, KSize); // 4
	if (!mem4)
		return KErrNoMemory;
	//
	// The following exceptions are not possible with DL allocator
	//
/*	
	if (mem3 <= (mem2 + KSmallSize))
		return KErrUnknown+2;	// Expect mem3 higher than mem2
	if (mem4 == mem1)
		return KErrUnknown; // Expect move on grow
	mem1=mem4;
	if (mem4 != m3)
		return KErrUnknown; // Expect to realloc to use old mem3 space
*/
	mem1=mem4;	
	TInt i;
	TInt err = KErrNone;
	for (i = 0 ; i<KSmallSize && err==KErrNone; ++i)
		{
		if (mem1[i] != 0xff)
			FAIL_ALLOC_TEST(4, i, mem1[i]);
		}
	for (i = KSmallSize; i<KSize && err==KErrNone; ++i)
		{
		if (mem1[i] != 0)
			FAIL_ALLOC_TEST(5, i, mem1[i]);
		}

	return err;
	}

#ifdef __EPOC32__
#define CHECK(c) { if(!(c)) { Kern::Printf("Fail  %d", __LINE__); ; ret = __LINE__;} }

TInt DMemoryTestChannel::AllocPhysTest(TUint32 aIters, TUint32 aSize)
	{
	TInt	ret = KErrNone;
	TUint32	index;

	TUint32  pageSize = 0;
	CHECK(Kern::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0)==KErrNone);
	TUint32  numPages = aSize / pageSize;
	TUint32  pageIndex;
	TPhysAddr*	 addrArray = (TPhysAddr *)Kern::AllocZ(sizeof(TPhysAddr) * numPages);
	CHECK(addrArray);
	if(!addrArray)
		{
		return KErrNoMemory;
		}

	for (index = 0; index < aIters; index ++)
		{
		for (pageIndex = 0; pageIndex < numPages; pageIndex ++)
			{
			ret = Epoc::AllocPhysicalRam(pageSize, addrArray[pageIndex], 0);
			if (ret != KErrNone)
				{
				break;
				}
			}
		for (pageIndex = 0; pageIndex < numPages; pageIndex ++)
			{
			if (addrArray[pageIndex])
				{
				Epoc::FreePhysicalRam(addrArray[pageIndex], pageSize);
				addrArray[pageIndex] = NULL;
				}
			}
		if (ret != KErrNone)
			{
			break;
			}
		}

	Kern::Free(addrArray);
	return ret;
	}

#else

TInt DMemoryTestChannel::AllocPhysTest(TUint32 , TUint32 )
	{
	return KErrNone;
	}

#endif

#ifdef __EPOC32__

TInt DMemoryTestChannel::AllocPhysTest1(TUint32 aIters, TUint32 aSize)
	{
	TInt	ret = KErrNone;
	TUint32	index;

	TUint32  pageSize = 0;
	CHECK(Kern::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0)==KErrNone);
	TUint32 numPages = aSize / pageSize;
	TPhysAddr*	 addrArray = (TPhysAddr *)Kern::AllocZ(sizeof(TPhysAddr) * numPages);
	for (index = 0; index < aIters; index ++)
		{
		ret = Epoc::AllocPhysicalRam(numPages, addrArray);
		if (ret != KErrNone)
			{
			break;
			}
		Epoc::FreePhysicalRam(numPages, addrArray);
		}
	Kern::Free(addrArray);
	return ret;
	}
#else

TInt DMemoryTestChannel::AllocPhysTest1(TUint32 , TUint32 )
	{
	return KErrNone;
	}

#endif
