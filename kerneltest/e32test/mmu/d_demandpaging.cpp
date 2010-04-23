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
// e32test\mmu\d_demandpaging.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "d_demandpaging.h"

/// Page attributes, cut-n-paste'd from mmubase.h
enum TType
	{
//	EInvalid=0,			// No physical RAM exists for this page
//	EFixed=1,			// RAM fixed at boot time
//	EUnused=2,			// Page is unused
//	EChunk=3,
//	ECodeSeg=4,
//	EHwChunk=5,
//	EPageTable=6,
//	EPageDir=7,
//	EPtInfo=8,
//	EShadow=9,

	EPagedROM=10,
	EPagedCode=11,
	EPagedData=12,
	EPagedCache=13,
	EPagedFree=14,
	};

enum TState
	{
	EStateNormal = 0,		// no special state
	EStatePagedYoung = 1,
	EStatePagedOld = 2,
	EStatePagedDead = 3,
	EStatePagedLocked = 4
	};

//
// Class definitions
//

class DDemandPagingTestFactory : public DLogicalDevice
	{
public:
	~DDemandPagingTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DDemandPagingTestChannel : public DLogicalChannelBase
	{
public:
	DDemandPagingTestChannel();
	~DDemandPagingTestChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	TInt LockTest(const TAny* aBuffer, TInt aSize);
	TInt LockTest2();
	TInt DoConsumeContiguousRamTest(TInt aAlign, TInt aPages);
	TInt DoCreatePlatHwChunk(TInt aSize, TAny* aLinAddr);
	TInt DoDestroyPlatHwChunk();
	TInt ReadHoldingMutexTest(TAny* aDest);

	TBool CheckPagedIn(TLinAddr aAddress);
	TBool CheckPagedOut(TLinAddr aAddress);
	TBool CheckLocked(TLinAddr aAddress);

	TInt FreeRam();
public:
	DDemandPagingTestFactory*	iFactory;
	DDemandPagingLock iLock;

	DPlatChunkHw* iHwChunk;
    TInt iChunkSize;
	TPhysAddr iPhysBase;		// This will be base physical address of the chunk
    TLinAddr iLinearBase;		// This will be base linear address of the chunk
	};

//
// DDemandPagingTestFactory
//

TInt DDemandPagingTestFactory::Install()
	{
	return SetName(&KDemandPagingTestLddName);
	}

DDemandPagingTestFactory::~DDemandPagingTestFactory()
	{
	}

void DDemandPagingTestFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DDemandPagingTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DDemandPagingTestChannel* channel=new DDemandPagingTestChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DDemandPagingTestFactory;
	}

//
// DDemandPagingTestChannel
//

TInt DDemandPagingTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DDemandPagingTestChannel::DDemandPagingTestChannel()
	{
	}

DDemandPagingTestChannel::~DDemandPagingTestChannel()
	{
	DoDestroyPlatHwChunk();
	}

TInt DDemandPagingTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	switch(aFunction)
		{
	case RDemandPagingTestLdd::ELockTest:
		{
		TInt r = LockTest(a1,(TInt)a2);
		if (r == KErrNone)
			r = LockTest2();
		return r;
		}

	case RDemandPagingTestLdd::ESetRealtimeTrace:
		{
#if defined(_DEBUG)
		TUint32 bit = TUint32(1U<<(KREALTIME&31));
		__e32_atomic_axo_ord32(&Kern::SuperPage().iDebugMask[KREALTIME>>5], ~bit, a1?bit:0);
#if 0 // can enable this to help debugging
		bit = (1<<(KPAGING&31));
		__e32_atomic_axo_ord32(&Kern::SuperPage().iDebugMask[KPAGING>>5], ~bit, a1?bit:0);
#endif
#endif //_DEBUG
		}
		return KErrNone;

	case RDemandPagingTestLdd::EDoConsumeContiguousRamTest:
		{
		return DDemandPagingTestChannel::DoConsumeContiguousRamTest((TInt)a1, (TInt)a2);
		}

	case RDemandPagingTestLdd::ECreatePlatHwChunk:
		{
		return DDemandPagingTestChannel::DoCreatePlatHwChunk((TInt)a1, a2);
		}

	case RDemandPagingTestLdd::EDestroyPlatHwChunk:
		{
		return DDemandPagingTestChannel::DoDestroyPlatHwChunk();
		}

	case RDemandPagingTestLdd::ELock:
		{
		TInt r=iLock.Alloc((TInt)a2);
		if(r!=KErrNone)
			return r;
		return iLock.Lock(&Kern::CurrentThread(),(TLinAddr)a1,(TInt)a2);
		}

	case RDemandPagingTestLdd::EUnlock:
		{
		iLock.Free();
		return KErrNone;
		}

	case RDemandPagingTestLdd::EReadHoldingMutexTest:
		return ReadHoldingMutexTest((TAny*)a1);

	default:
		return KErrNotSupported;
		}
	}

// 
// DDemandPagingTestChannel::DoCreatePlatHwChunk
//
// For some of the tests of IPC from demand-paged memory, we need a writable
// globally-mapped buffer; so this function creates a suitable chunk and
// returns its (global, virtual) address to the userland caller.  The caller
// should call DoDestroyPlatHwChunk() to release the memory when the tests
// are finished.
//
TInt DDemandPagingTestChannel::DoCreatePlatHwChunk(TInt aSize, TAny* aLinAddr)
	{
	TInt mapAttr = EMapAttrUserRw;		// Supervisor and user both have read/write permissions

	NKern::ThreadEnterCS();
	if (iHwChunk)						// Only one chunk at a atime
		{
		NKern::ThreadLeaveCS();
		return KErrAlreadyExists;
		}

	iChunkSize = Kern::RoundToPageSize(aSize);

	Kern::Printf("*** Attempting to allocate contiguous physical RAM ***");
	TInt free = Kern::FreeRamInBytes();
	Kern::Printf("      requested:  %08x", iChunkSize);
	Kern::Printf("      total free: %08x", free);
	
	TInt r = Epoc::AllocPhysicalRam(iChunkSize, iPhysBase, 0);	// Allocate RAM; result in iPhysBase
	if (r)
		{
		NKern::ThreadLeaveCS();
		Kern::Printf("      failed with error %d", r);
		return r;
		}
	else
		Kern::Printf("      success");

	r = DPlatChunkHw::New(iHwChunk, iPhysBase, iChunkSize, mapAttr);	// Create chunk
	if (r)
		{
		Epoc::FreePhysicalRam(iPhysBase, iChunkSize);
		iHwChunk = 0;
		NKern::ThreadLeaveCS();
		return r;
		}
	NKern::ThreadLeaveCS();

	// Return the virtual address to userland
	iLinearBase = iHwChunk->LinearAddress();
	kumemput(aLinAddr, &iLinearBase, sizeof(iLinearBase));

	Kern::Printf("CreatePlatHwChunk@%08x: iLinearBase %08x, iPhysBase %08x, size %d",
		iHwChunk, iLinearBase, iPhysBase, iChunkSize);

	return KErrNone;
	}

TInt DDemandPagingTestChannel::DoDestroyPlatHwChunk()
	{
	Kern::Printf("DestroyPlatHwChunk@%08x: iLinearBase %08x, iPhysBase %08x, size %d",
		iHwChunk, iLinearBase, iPhysBase, iChunkSize);
	NKern::ThreadEnterCS();
	if (iHwChunk)
		{
		iHwChunk->Close(NULL);
		Epoc::FreePhysicalRam(iPhysBase, iChunkSize);
		iPhysBase = 0;
		iChunkSize = 0;
		iHwChunk = 0;
		}
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

// 
// DDemandPagingTestChannel::DoConsumeContiguousRamTest
//
// This test attempts to consume all available Contiguous Ram until we need to ask the 
// demand paging code to release memory for it.
// 
// On completion free all the memory allocated.
//
#define CHECK(c) { if(!(c)) { Kern::Printf("Fail  %d", __LINE__); ; retVal = __LINE__;} }

TInt DDemandPagingTestChannel::DoConsumeContiguousRamTest(TInt aAlign, TInt aSize)
	{
	TInt retVal = KErrNone;
	TInt initialFreeRam = FreeRam();
	TInt totalBlocks = initialFreeRam/aSize;

	NKern::ThreadEnterCS();
	TPhysAddr*	 pAddrArray = (TPhysAddr *)Kern::Alloc(sizeof(TPhysAddr) * totalBlocks);
	NKern::ThreadLeaveCS();
	CHECK(pAddrArray);
	if(!pAddrArray)
		return retVal;
	
	SVMCacheInfo tempPages;

	// get the initial free ram again as the heap may have grabbed a page during the alloc
	initialFreeRam = FreeRam();
	Kern::Printf("ConsumeContiguousRamTest: align %d size %d initialFreeRam %d", aAlign, aSize, initialFreeRam);

	CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
	Kern::Printf("Start cache info: iMinSize %d iMaxSize %d iCurrentSize %d iMaxFreeSize %d",
				 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize);

	TInt initialFreePages = tempPages.iMaxFreeSize;
	CHECK(initialFreePages != 0);
	
	// allocate blocks to use up RAM until we fail to allocate any further...
	TBool freedPagesToAlloc = EFalse;
	TInt index;
	TUint32 alignMask = (1 << aAlign) - 1;
	for (index = 0; index < totalBlocks; )
		{
		CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
		TInt beforePages = tempPages.iMaxFreeSize;

		NKern::ThreadEnterCS();
		TInt r = Epoc::AllocPhysicalRam(aSize, pAddrArray[index], aAlign);
		if(r==KErrNone)
			{
			// check the alignment of the returned pages
			CHECK((pAddrArray[index] & alignMask) == 0);
			++index;
			}
		NKern::ThreadLeaveCS();
		if(r!=KErrNone)
			{
			break;
			}
		CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
		TInt afterPages = tempPages.iMaxFreeSize;

		if (afterPages != beforePages)
			freedPagesToAlloc = ETrue; // the alloc reclaimed memory from the paging cache
		}

	if (!index)
		Kern::Printf("WARNING : DoConsumeContiguousRamTest no allocations were successful");
	// free the memory we allocated...
	while(--index>=0)
		{
		NKern::ThreadEnterCS();
		TInt r = Epoc::FreePhysicalRam(pAddrArray[index], aSize);
		NKern::ThreadLeaveCS();
		CHECK(r==KErrNone);
		}

	CHECK(FreeRam() == initialFreeRam);

	NKern::ThreadEnterCS();
	Kern::Free(pAddrArray);
	NKern::ThreadLeaveCS();

	CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
	Kern::Printf("End cache info: iMinSize %d iMaxSize %d iCurrentSize %d iMaxFreeSize %d",
				 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize);

	if (!freedPagesToAlloc)
		Kern::Printf("WARNING : DoConsumeContiguousRamTest freedPagesToAlloc was eFalse");
	//CHECK(freedPagesToAlloc);		

	return retVal;
	}
#undef CHECK


TUint8 ReadByte(volatile TUint8* aPtr)
	{
	return *aPtr;
	}

#define CHECK(c) { if(!(c)) return __LINE__; }

#define READ(a) ReadByte((volatile TUint8*)(a))

TInt DDemandPagingTestChannel::LockTest(const TAny* aBuffer, TInt aSize)
	{
	// Get page size info
	TInt pageSize = 0;
	CHECK(Kern::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0)==KErrNone);
	TInt pageMask = pageSize-1;

	// See if were running of the Flexible Memory Model
  	TUint32 memModelAttrib = (TUint32)Kern::HalFunction(EHalGroupKernel,EKernelHalMemModelInfo,0,0);	
	TBool fmm = (memModelAttrib&EMemModelTypeMask)==EMemModelTypeFlexible;

	// Round buffer to page boundaries
	TLinAddr start = ((TLinAddr)aBuffer+pageMask)&~pageMask;
	TLinAddr end = ((TLinAddr)aBuffer+aSize)&~pageMask;
	aSize = end-start;
	Kern::Printf("Test buffer is %08x, %x\n",start,aSize);
	CHECK(aSize>pageSize*2);

	// Flush all paged memory
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);

	TInt initialFreeRam;
	TInt freeRam1;
	TInt freeRam2;
	TLinAddr addr;
	TUint lockBytesUsed = fmm ? 0 : 0; // free ram change on locking (zero or aSize depending on implementation)

	{ // this brace is essential for correctness
	DDemandPagingLock lock2; // construct a lock;

	Kern::Printf("Check reading from buffer pages it in\n");
	for(addr=start; addr<end; addr+=pageSize) READ(addr);
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckPagedIn(addr));
	initialFreeRam = FreeRam();

	Kern::Printf("Check Alloc reserves pages\n");
	CHECK(iLock.Alloc(aSize)==KErrNone);
	freeRam1 = FreeRam();

	Kern::Printf("Check flushing pages out the buffer\n");
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckPagedOut(addr));

	Kern::Printf("Check Lock\n");
	CHECK(iLock.Lock(&Kern::CurrentThread(),start,aSize));
	CHECK((TUint)FreeRam()==TUint(freeRam1-lockBytesUsed));
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckLocked(addr));

	Kern::Printf("Check flushing doesn't page out the buffer\n");
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckLocked(addr));
	CHECK((TUint)FreeRam()==TUint(freeRam1-lockBytesUsed));

	Kern::Printf("Check second Alloc\n");
	CHECK(lock2.Alloc(aSize)==KErrNone);
	freeRam2 = FreeRam();

	Kern::Printf("Check second Lock\n");
	CHECK(lock2.Lock(&Kern::CurrentThread(),start,aSize));
	CHECK(FreeRam()==freeRam2);
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckLocked(addr));

	Kern::Printf("Check deleting second lock\n");
	// lock2 is deleted here because it goes out of scope...
	} // this brace is essential for correctness
	CHECK((TUint)FreeRam()==TUint(freeRam1-lockBytesUsed));
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckLocked(addr));

	Kern::Printf("Check Unlock\n");
	iLock.Unlock();
	CHECK(FreeRam()==freeRam1);
	for(addr=start; addr<end; addr+=pageSize) CHECK(CheckPagedIn(addr));
	iLock.Unlock();
	CHECK(FreeRam()==initialFreeRam);

	Kern::Printf("Check Free\n");
	iLock.Free();
	CHECK(FreeRam()==initialFreeRam);
	iLock.Free();
	CHECK(FreeRam()==initialFreeRam);

	return KErrNone;
	}

#undef CHECK
#define CHECK(c) { if(!(c)) { r = __LINE__; goto cleanup; } }

TInt DDemandPagingTestChannel::LockTest2()
	{
	Kern::Printf("Check allocating locks eventually increases size of live list\n");
	TInt r = KErrNone;

	DDemandPagingLock* lock = NULL;
	RPointerArray<DDemandPagingLock> lockArray;
	
	const TInt KLockMax = 1000; // make this a bit bigger than current min page count?
	TInt i;
	
	NKern::ThreadEnterCS();
	for (i = 0 ; i < KLockMax ; ++i)
		{
		lock = new DDemandPagingLock;
		CHECK(lock);
		CHECK(lockArray.Append(lock) == KErrNone);
		lock = NULL;

		TInt initialFreeRam = FreeRam();
		CHECK(lockArray[i]->Alloc(1) == KErrNone);
		if (FreeRam() < initialFreeRam)
			{
			Kern::Printf("Live list size increased after %d locks allocated", i + 1);
			break;
			}
		}

	CHECK(i < KLockMax);
	
cleanup:

	delete lock;
	lock = NULL;
	for (i = 0 ; i < lockArray.Count() ; ++i)
		{
		delete lockArray[i];
		lockArray[i] = NULL;
		}
	lockArray.Reset();

	NKern::ThreadLeaveCS();

	return r;
	}

TInt DDemandPagingTestChannel::FreeRam()
	{
	Kern::HalFunction(EHalGroupKernel, EKernelHalSupervisorBarrier, 0, 0);
	TInt freeRam = Kern::FreeRamInBytes();
	Kern::Printf("...free RAM: %x\n",freeRam);
	return freeRam;
	}


TUint32 PageState(TLinAddr aAddress)
	{
	TUint32 state = Kern::HalFunction(EHalGroupVM, EVMPageState, (TAny*)aAddress, 0);
	Kern::Printf("PageState: %08x=%08x",aAddress,state);
	return state;
	}


TBool DDemandPagingTestChannel::CheckPagedIn(TLinAddr aAddress)
	{
	TUint32 state = PageState(aAddress);
	return (state&0xff00) == (EStatePagedYoung<<8);
	}


TBool DDemandPagingTestChannel::CheckPagedOut(TLinAddr aAddress)
	{
	TUint32 state = PageState(aAddress);
	return (state&0xffff) == 0;
	}


TInt DDemandPagingTestChannel::CheckLocked(TLinAddr aAddress)
	{
	TUint32 state = PageState(aAddress);
	return (state&0xff00) == (EStatePagedLocked<<8);
	}


TInt DDemandPagingTestChannel::ReadHoldingMutexTest(TAny* aDest)
	{
	_LIT(KMutexName, "DPTestMutex");

	NKern::ThreadEnterCS();
	
	DMutex* mutex;
	TInt r = Kern::MutexCreate(mutex, KMutexName, KMutexOrdDebug);  // Mutex order < demand paging
	if (r != KErrNone)
		{
		NKern::ThreadLeaveCS();
		return r;
		}
	Kern::MutexWait(*mutex);
	
	const TRomHeader& romHeader = Epoc::RomHeader();
	TLinAddr unpagedRomStart = (TLinAddr)&romHeader;
	TLinAddr unpagedRomEnd;
	if (romHeader.iPageableRomStart)
		unpagedRomEnd = unpagedRomStart + romHeader.iPageableRomStart;
	else
		unpagedRomEnd = unpagedRomStart + romHeader.iUncompressedSize;
	
	const TInt length = 16;
	TUint8 localBuf[length];
	if(!aDest)
		aDest = localBuf;
	Kern::Printf("Local buffer at %08x", aDest);

	TAny* src1 = (TAny*)unpagedRomStart;
	TAny* src2 = (TAny*)(unpagedRomEnd - length);
	
	DThread* thread = &Kern::CurrentThread();

	Kern::Printf("Attempting to access %08x", src1);
	Kern::ThreadRawWrite(thread, aDest, src1, length);
	Kern::Printf("Attempting to access %08x", src2);
	Kern::ThreadRawWrite(thread, aDest, src2, length);

	TUint8 stackData[length];
	Kern::Printf("Attempting to access %08x", stackData);
	Kern::ThreadRawWrite(thread, aDest, stackData, length);
	
	TAny* heapData = Kern::Alloc(length);
	if (heapData)
		{
		Kern::Printf("Attempting to access %08x", heapData);
		Kern::ThreadRawWrite(thread, aDest, heapData, length);
		Kern::Free(heapData);
		}
	else
		r = KErrNoMemory;
	
	Kern::MutexSignal(*mutex);
	mutex->Close(NULL);
	
	NKern::ThreadLeaveCS();
	
	return r;  // a kernel fault indicates that the test failed
	}

