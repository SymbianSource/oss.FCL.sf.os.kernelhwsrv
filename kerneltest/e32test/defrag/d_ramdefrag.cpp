// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\defrag\d_testramdefrag.cpp
// 
//

//#define DEBUG_VER				// Uncomment for tracing

#include "platform.h"
#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "t_ramdefrag.h"

//
// Class definitions
//
const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;


const TInt KDefragCompleteThreadPriority = 27;
_LIT(KDefragCompleteThread,"DefragCompleteThread");

class DRamDefragFuncTestFactory : public DLogicalDevice
	{
public:

	DRamDefragFuncTestFactory();
	~DRamDefragFuncTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);

	TDynamicDfcQue* iDfcQ;
	};

class DRamDefragFuncTestChannel : public DLogicalChannelBase
	{
public:
	DRamDefragFuncTestChannel(TDfcQue* aDfcQ);

	DRamDefragFuncTestChannel();
	~DRamDefragFuncTestChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	TInt FreeAllFixedPages();
	TInt AllocFixedPages(TInt aNumPages);
	TInt AllocFixedArray(TInt aNumPages);
	TInt AllocateFixed2(TInt aNumPages);
	TInt GetAllocDiff(TUint aNumPages);
	TInt FreeAllFixedPagesRead();
	TInt AllocFixedPagesWrite(TInt aNumPages);
	TInt ZoneAllocContiguous(TUint aZoneID, TUint aNumBytes);
	TInt ZoneAllocContiguous(TUint* aZoneIdList, TUint aZoneIdCount, TUint aNumBytes);
	TInt ZoneAllocDiscontiguous(TUint aZoneID, TInt aNumPages);
	TInt ZoneAllocDiscontiguous(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages);
	TInt ZoneAllocToMany(TInt aZoneIndex, TInt aNumPages);
	TInt ZoneAllocToManyArray(TInt aZoneIndex, TInt aNumPages);
	TInt ZoneAllocToMany2(TInt aZoneIndex, TInt aNumPages);
	TInt AllocContiguous(TUint aNumBytes);
	TInt FreeZone(TInt aNumPages);
	TInt FreeZoneId(TUint aZoneId);
	TInt FreeFromAllZones();
	TInt FreeFromAddr(TInt aNumPages, TUint32 aAddr);
	TInt PageCount(TUint aId, STestUserSidePageCount* aPageData);
	TInt CancelDefrag();
	TInt CheckCancel(STestParameters* aParams);
	TInt CallDefrag(STestParameters* aParams);
	TInt CheckPriorities(STestParameters* aParams);
	TInt SetZoneFlag(STestFlagParams* aParams);
	TInt GetDefragOrder();
	TInt FreeRam();
	TInt DoSetDebugFlag(TInt aState);
	TInt ResetDriver();
	TInt ZoneAllocDiscontiguous2(TUint aZoneID, TInt aNumPages);
public:
	DRamDefragFuncTestFactory*	iFactory;

protected:
	static void DefragCompleteDfc(TAny* aSelf);
	void DefragComplete();
		static void Defrag2CompleteDfc(TAny* aSelf);
	void Defrag2Complete();
		static void Defrag3CompleteDfc(TAny* aSelf);
	void Defrag3Complete();
private:
	TPhysAddr				iContigAddr;		/**< The base address of fixed contiguous allocations*/
	TUint					iContigBytes;		/**< The no. of contiguous fixed bytes allocated*/
	TPhysAddr*				iAddrArray;	
	TUint					iAddrArrayPages;
	TUint					iAddrArraySize;
	TPhysAddr**				iAddrPtrArray;			
	TInt*					iNumPagesArray;
	TInt					iDebug;
	TInt					iThreadCounter;
	DChunk*					iChunk;
	TLinAddr				iKernAddrStart;
	TInt					iPageSize;
	TUint 					iPageShift;			/**< The system's page shift */
	TUint					iZoneCount;
	TRamDefragRequest		iDefragRequest;		//	Defrag request object
	TRamDefragRequest		iDefragRequest2;
	TRamDefragRequest		iDefragRequest3;
	TUint*					iZoneIdArray;		/**< Pointer to an kernel heap array of zone IDs*/


	DSemaphore*				iDefragSemaphore;	//	Semaphore enusre only one defrag operation is active per channel
	TRequestStatus*			iCompleteReq;		//	Pointer to a request status that will signal to the user side client once the defrag has completed
	TRequestStatus*			iCompleteReq2;
	TRequestStatus*			iCompleteReq3;		
	TRequestStatus			iTmpRequestStatus1;	
	TRequestStatus			iTmpRequestStatus2;
	DThread*				iRequestThread;		//	Pointer to the thread that made the defrag request
	DThread*				iRequestThread2;
	DThread*				iRequestThread3;		

	TDfcQue*				iDfcQ;				//	The DFC queue used for driver functions 
	TDfc					iDefragCompleteDfc;	//	DFC to be queued once a defrag operation has completed 
	TDfc					iDefragComplete2Dfc;
	TDfc					iDefragComplete3Dfc;
	TInt					iCounter;			//	Counts the number of defrags that have taken place
	TInt					iOrder;				//	Stores the order in which queued defrags took place
	};



//
// DRamDefragFuncTestFactory
//

DRamDefragFuncTestFactory::DRamDefragFuncTestFactory()
//
// Constructor
//
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }
    
TInt DRamDefragFuncTestFactory::Install()
	{
	return SetName(&KRamDefragFuncTestLddName);
	}

DRamDefragFuncTestFactory::~DRamDefragFuncTestFactory()
	{
	if (iDfcQ != NULL)
		{// Destroy the DFC queue created when this device drvier was loaded.
		iDfcQ->Destroy();
		}
	}

void DRamDefragFuncTestFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DRamDefragFuncTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	DRamDefragFuncTestChannel* channel=new DRamDefragFuncTestChannel(iDfcQ);
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	DRamDefragFuncTestFactory* factory = new DRamDefragFuncTestFactory;
	if (factory)
		{
		// Allocate a kernel thread to run the DFC 
		TInt r = Kern::DynamicDfcQCreate(factory->iDfcQ, KDefragCompleteThreadPriority, KDefragCompleteThread);

		if (r != KErrNone)
			{// Must close rather than delete factory as it is a DObject object.
			factory->AsyncClose();
			return NULL; 	
			} 	
		}
    return factory;
	}

//
// DRamDefragFuncTestChannel
//

TInt DRamDefragFuncTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{

	TInt ret = Kern::HalFunction(EHalGroupRam, ERamHalGetZoneCount, (TAny*)&iZoneCount, NULL);


	// Retrieve the page size and use it to detemine the page shift (assumes 32-bit system).
	TInt r = Kern::HalFunction(EHalGroupKernel, EKernelHalPageSizeInBytes, &iPageSize, 0);
	if (r != KErrNone)
		{
		TESTDEBUG(Kern::Printf("ERROR - Unable to determine page size"));
		return r;
		}
	TUint32 pageMask = iPageSize;
	TUint i = 0;
	for (; i < 32; i++)
		{
		if (pageMask & 1)
			{
			if (pageMask & ~1u)
				{
				TESTDEBUG(Kern::Printf("ERROR - page size not a power of 2"));
				return KErrNotSupported;
				}
			iPageShift = i;
			break;
			}
		pageMask >>= 1;
		}

	// Create a semaphore to protect defrag invocation.  OK to just use one name as
	// the semaphore is not global so it's name doesn't need to be unique.
	ret = Kern::SemaphoreCreate(iDefragSemaphore, _L("DefragRefSem"), 1);
	if (ret != KErrNone)
		{
		return ret;
		}
	iDefragCompleteDfc.SetDfcQ(iDfcQ);
	iDefragComplete2Dfc.SetDfcQ(iDfcQ);
	iDefragComplete3Dfc.SetDfcQ(iDfcQ);

	// Create an array to store some RAM zone IDs for use but the multi-zone 
	// specific allcoation methods.
	NKern::ThreadEnterCS();
	iZoneIdArray = new TUint[KMaxRamZones];
	if (iZoneIdArray == NULL)
		{
		ret = KErrNoMemory;
		}
	NKern::ThreadLeaveCS();

	return ret;
	}

DRamDefragFuncTestChannel::DRamDefragFuncTestChannel(TDfcQue* aDfcQ)
	: 
	iContigAddr(KPhysAddrInvalid),
	iContigBytes(0),
	iAddrArray(NULL), 
	iAddrArrayPages(0),
	iAddrArraySize(0),
	iAddrPtrArray(NULL),
	iNumPagesArray(NULL),
	iDebug(0), 
	iThreadCounter(1),
	iChunk(NULL),
	iPageSize(0), 
	iPageShift(0),
	iZoneCount(0),
	iZoneIdArray(NULL),
	iDefragSemaphore(NULL),
	iCompleteReq(NULL),
	iCompleteReq2(NULL),
	iCompleteReq3(NULL),
	iRequestThread(NULL),
	iRequestThread2(NULL),
	iRequestThread3(NULL),
	iDfcQ(aDfcQ),
	iDefragCompleteDfc(DefragCompleteDfc, (TAny*)this, 1),
	iDefragComplete2Dfc(Defrag2CompleteDfc, (TAny*)this, 1), 
	iDefragComplete3Dfc(Defrag3CompleteDfc, (TAny*)this, 1), 
	iCounter(0), 
	iOrder(0)
	{
	}

DRamDefragFuncTestChannel::~DRamDefragFuncTestChannel()
	{
	if (iDefragSemaphore != NULL)
		{
		iDefragSemaphore->Close(NULL);
		}
	if (iZoneIdArray != NULL)
		{
		NKern::ThreadEnterCS();
		delete[] iZoneIdArray;
		NKern::ThreadLeaveCS();
		}
	}

TInt DRamDefragFuncTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt threadCount = __e32_atomic_tas_ord32(&iThreadCounter, 1, 1, 0);
	if (threadCount >= 2)
		{
		Kern::Printf("DRamDefragFuncTestChannel::Request threadCount = %d\n", threadCount);
		}

	Kern::SemaphoreWait(*iDefragSemaphore);


	TInt retVal = KErrNotSupported;
	switch(aFunction)
		{
		case RRamDefragFuncTestLdd::EAllocateFixed:
			retVal = DRamDefragFuncTestChannel::AllocFixedPages((TInt)a1);
			break;
			
		case RRamDefragFuncTestLdd::EAllocFixedArray:
			retVal = DRamDefragFuncTestChannel::AllocFixedArray((TInt)a1);
			break;
						
		case RRamDefragFuncTestLdd::EAllocateFixed2:
			retVal = DRamDefragFuncTestChannel::AllocateFixed2((TInt)a1);
			break;
		
		case RRamDefragFuncTestLdd::EGetAllocDiff:
			retVal = DRamDefragFuncTestChannel::GetAllocDiff((TUint)a1);
			break;

		case RRamDefragFuncTestLdd::EFreeAllFixed:
			retVal = DRamDefragFuncTestChannel::FreeAllFixedPages();
			break;

		case RRamDefragFuncTestLdd::EAllocateFixedWrite:
			retVal = DRamDefragFuncTestChannel::AllocFixedPagesWrite((TInt)a1);
			break;
		
		case RRamDefragFuncTestLdd::EFreeAllFixedRead:
			retVal = DRamDefragFuncTestChannel::FreeAllFixedPagesRead();
			break;
		
		case RRamDefragFuncTestLdd::EZoneAllocContiguous:
			retVal = DRamDefragFuncTestChannel::ZoneAllocContiguous((TUint)a1, (TUint)a2);
			break;

		case RRamDefragFuncTestLdd::EMultiZoneAllocContiguous:
			{
			SMultiZoneAlloc multiZone;
			kumemget(&multiZone, a1, sizeof(SMultiZoneAlloc));
			retVal = DRamDefragFuncTestChannel::ZoneAllocContiguous(multiZone.iZoneId, multiZone.iZoneIdSize, (TUint)a2);
			}
			break;

		case RRamDefragFuncTestLdd::EZoneAllocDiscontiguous:
			retVal = DRamDefragFuncTestChannel::ZoneAllocDiscontiguous((TUint)a1, (TUint)a2);	
			break;

		case RRamDefragFuncTestLdd::EMultiZoneAllocDiscontiguous:
			{
			SMultiZoneAlloc multiZone;
			kumemget(&multiZone, a1, sizeof(SMultiZoneAlloc));
			retVal = DRamDefragFuncTestChannel::ZoneAllocDiscontiguous(multiZone.iZoneId, multiZone.iZoneIdSize, (TUint)a2);	
			}
			break;

		case RRamDefragFuncTestLdd::EZoneAllocDiscontiguous2:
			retVal = DRamDefragFuncTestChannel::ZoneAllocDiscontiguous2((TUint)a1, (TUint)a2);	
			break;

		case RRamDefragFuncTestLdd::EZoneAllocToMany:
			retVal = DRamDefragFuncTestChannel::ZoneAllocToMany((TUint)a1, (TInt)a2);	
			break;

		case RRamDefragFuncTestLdd::EZoneAllocToManyArray:
			retVal = DRamDefragFuncTestChannel::ZoneAllocToManyArray((TUint)a1, (TInt)a2);	
			break;

		case RRamDefragFuncTestLdd::EZoneAllocToMany2:
			retVal = DRamDefragFuncTestChannel::ZoneAllocToMany2((TUint)a1, (TInt)a2);	
			break;

		case RRamDefragFuncTestLdd::EAllocContiguous:
			retVal = DRamDefragFuncTestChannel::AllocContiguous((TUint)a1);	
			break;

		case RRamDefragFuncTestLdd::EFreeZone:
			retVal = DRamDefragFuncTestChannel::FreeZone((TInt)a1);
			break;

		case RRamDefragFuncTestLdd::EFreeZoneId:
			retVal = DRamDefragFuncTestChannel::FreeZoneId((TUint)a1);
			break;

		case RRamDefragFuncTestLdd::EFreeFromAllZones:
			retVal = DRamDefragFuncTestChannel::FreeFromAllZones();	
			break;
		
		case RRamDefragFuncTestLdd::EFreeFromAddr:
			retVal = DRamDefragFuncTestChannel::FreeFromAddr((TInt)a1, (TUint32)a2);	
			break;
			
		case RRamDefragFuncTestLdd::EPageCount:
			retVal = DRamDefragFuncTestChannel::PageCount((TUint)a1, (STestUserSidePageCount*)a2);	
			break;
		
		case RRamDefragFuncTestLdd::ECheckCancel:
			retVal = DRamDefragFuncTestChannel::CheckCancel((STestParameters*)a1);	
			break;

		case RRamDefragFuncTestLdd::ECallDefrag:
			retVal = DRamDefragFuncTestChannel::CallDefrag((STestParameters*)a1);	
			break;

		case RRamDefragFuncTestLdd::ESetZoneFlag:
			retVal = DRamDefragFuncTestChannel::SetZoneFlag((STestFlagParams*)a1);	
			break;

		case RRamDefragFuncTestLdd::ECheckPriorities:
			retVal = DRamDefragFuncTestChannel::CheckPriorities((STestParameters*)a1);	
			break;

		case RRamDefragFuncTestLdd::EGetDefragOrder:
			retVal = DRamDefragFuncTestChannel::GetDefragOrder();	
			break;

		case RRamDefragFuncTestLdd::EDoSetDebugFlag:
			retVal = DoSetDebugFlag((TInt) a1);
			break;
		
		case RRamDefragFuncTestLdd::EResetDriver:
			retVal = ResetDriver();
			break;

		default: 
			break;
		}

	Kern::SemaphoreSignal(*iDefragSemaphore);
	__e32_atomic_tas_ord32(&iThreadCounter, 1, -1, 0);
	return retVal;
	}


#define CHECK(c) { if(!(c)) { Kern::Printf("Fail  %d", __LINE__); ; retVal = __LINE__;} }


//
// FreeAllFixedPages
//
// Free ALL of the fixed pages that were allocated
//
TInt DRamDefragFuncTestChannel::FreeAllFixedPages()
	{
	NKern::ThreadEnterCS();

	TInt retVal = KErrNone;

	if (iAddrArray != NULL)
		{
		retVal = Epoc::FreePhysicalRam(iAddrArrayPages, iAddrArray);
		CHECK(retVal == KErrNone);

		delete[] iAddrArray;
		iAddrArray = NULL;
		iAddrArrayPages = 0;
		}
	
	if (iContigAddr != KPhysAddrInvalid)
		{
		retVal = Epoc::FreePhysicalRam(iContigAddr, iContigBytes);
		iContigAddr = KPhysAddrInvalid;
		iContigBytes = 0;
		CHECK(retVal == KErrNone);
		}
	NKern::ThreadLeaveCS();

	retVal = FreeFromAllZones();
	return retVal;
	}



//
// FreeAllFixedPagesRead()
//
// Read the fixed pages that were mapped to iChunk and verify that 
// the contents have not changed.  Then free the fixed pages 
// that were allocated for iChunk.
//
TInt DRamDefragFuncTestChannel::FreeAllFixedPagesRead()
	{

	TInt retVal = KErrNone;
	TUint index;
	
	if (iAddrArray == NULL || iChunk == NULL || !iAddrArrayPages)
		{
		return KErrCorrupt;
		}
	
	TInt r = Kern::ChunkAddress(iChunk, 0, iAddrArrayPages << iPageShift, iKernAddrStart);
	if (r != KErrNone)
		{
		Kern::Printf("ERROR ? FreeAllFixedPages : Couldn't get linear address of iChunk! %d", r);
		}
	else
		{
		for (index = 0; index < iAddrArrayPages; index ++)
			{
			if (iAddrArray[index] != NULL)
				{
				TUint* pInt = (TUint *)(iKernAddrStart + (index << iPageShift));
				TUint* pIntEnd = pInt + (iPageSize / sizeof(TInt));
				// Read each word in this the page and verify that 
				// they are still the index of the current page in the chunk.
				while (pInt < pIntEnd)
					{
					if (*pInt++ != index)
						{
						Kern::Printf("ERROR ? FreeAllFixedPages : page at index %d is corrupt! 0x%08x", index, *pInt);
						}
					}
				}
			}
		}
	NKern::ThreadEnterCS();

	// Must close chunk before we free memory otherwise it would still be 
	// possible to access memory that has been freed and potentially reused.
	Kern::ChunkClose(iChunk);
	iChunk = NULL;
	retVal = Epoc::FreePhysicalRam(iAddrArrayPages, iAddrArray);
	delete[] iAddrArray;

	NKern::ThreadLeaveCS();

	iAddrArray = NULL;
	iAddrArrayPages = 0;
	return retVal;
	}

//
// AllocFixedPagesWrite
//
// Allocate a number of fixed pages to memory then create a shared chunk and map these pages into the chunk
//
TInt DRamDefragFuncTestChannel::AllocFixedPagesWrite(TInt aNumPages)
	{

	TInt retVal = KErrNone;
	TUint index = 0;
	TChunkCreateInfo 	chunkInfo;
	TUint32 			mapAttr;

	if (iAddrArray != NULL || iChunk != NULL)
		{
		return KErrInUse;
		}

	if (aNumPages == FILL_ALL_FIXED)
		{// Fill memory with fixed pages, leaving room for the kernel to expand.
		TUint freePages = FreeRam() >> iPageShift;
		// Calculate how many page tables will be required:
		// 	1024 pages per page table 
		//	4 page table per page
		TUint pageTablePages = (freePages >> 10) >> 2;
		TUint physAddrPages = (sizeof(TPhysAddr) * freePages) >> iPageShift;
		TESTDEBUG(Kern::Printf("pageTablePages %d physAddrPages %d", pageTablePages, physAddrPages));
		// Determine how many heap pages will be required, with some extra space as well.
		TUint fixedOverhead = (pageTablePages + physAddrPages) << 4;
		TESTDEBUG(Kern::Printf("freePages %d fixedOverhead %d", freePages, fixedOverhead));
		aNumPages = freePages - fixedOverhead;
		TESTDEBUG(Kern::Printf("aNumPages = %d", aNumPages));
		}

	NKern::ThreadEnterCS();

	iAddrArray = new TPhysAddr[aNumPages];
	if(!iAddrArray)
		{
		retVal = KErrNoMemory;
		goto exit;
		}
	
	TESTDEBUG(Kern::Printf("amount of free pages = %d", FreeRam() >> iPageShift));	

	// create a shared chunk and map these pages into the chunk.
	
	chunkInfo.iType			= TChunkCreateInfo::ESharedKernelSingle;
	chunkInfo.iMaxSize		= aNumPages << iPageShift;
	chunkInfo.iMapAttr		= EMapAttrFullyBlocking;
	chunkInfo.iOwnsMemory	= EFalse;

	TESTDEBUG(Kern::Printf("Creating chunk - amount of free pages = %d\n", FreeRam() >> iPageShift));
	retVal = Kern::ChunkCreate(chunkInfo, iChunk, iKernAddrStart, mapAttr);
	if (retVal != KErrNone)
		{
		Kern::Printf("ChunkCreate failed retVal = %d", retVal);
		goto exit;
		}

	TESTDEBUG(Kern::Printf("Created chunk - amount of free pages = %d\n", FreeRam() >> iPageShift));

	retVal = Epoc::AllocPhysicalRam(aNumPages, iAddrArray);
	if (retVal != KErrNone)
		{
		TESTDEBUG(Kern::Printf("Alloc of %d pages was unsuccessful\n", aNumPages));
		goto exit;
		}
	iAddrArrayPages = aNumPages;
	TESTDEBUG(Kern::Printf("Committing chunk - amount of free pages = %d\n", FreeRam() >> iPageShift));
	retVal = Kern::ChunkCommitPhysical(iChunk, 0, iAddrArrayPages << iPageShift, iAddrArray);
	if (retVal != KErrNone)
		{
		Kern::Printf("Commit was bad retVal = %d", retVal);
		goto exit;
		}
	TESTDEBUG(Kern::Printf("Committed chunk - amount of free pages = %d\n", FreeRam() >> iPageShift));
	TESTDEBUG(Kern::Printf("Start - 0x%08x\n", iKernAddrStart));
	for (index = 0; index < iAddrArrayPages; index ++)
		{
		TInt* pInt = (TInt *)(iKernAddrStart + (index << iPageShift));
		TInt* pIntEnd = pInt + (iPageSize / sizeof(TInt));
		// write the index into all of the words of the page.
		while (pInt < pIntEnd)
			{
			*pInt++ = index;
			}
		}

	TESTDEBUG(Kern::Printf("Allocated %d pages\n", iAddrArrayPages));
exit:
	if (retVal != KErrNone)
		{// Cleanup as something went wrong
		if (iChunk)
			{
			Kern::ChunkClose(iChunk);
			iChunk = NULL;
			}
		if (iAddrArray != NULL)
			{
			Epoc::FreePhysicalRam(iAddrArrayPages, iAddrArray);
			delete[] iAddrArray;
			iAddrArray = NULL;
			}
		iAddrArrayPages = 0;
		}

	NKern::ThreadLeaveCS();
	return retVal;
	}

TInt DRamDefragFuncTestChannel::GetAllocDiff(TUint aNumPages)
	{
	TUint initialFreeRam = FreeRam();
	TInt ret = KErrNone;
	TInt ramDifference;

	NKern::ThreadEnterCS();

	if (iAddrArray != NULL)
		{
		ret = KErrInUse;
		goto exit;
		}
	iAddrArray = (TPhysAddr *)Kern::AllocZ(sizeof(TPhysAddr) * aNumPages);

	if(!iAddrArray)
		{
		ret = KErrNoMemory;
		goto exit;
		}
	
	ramDifference = initialFreeRam - FreeRam();
	
	Kern::Free(iAddrArray);
	iAddrArray = NULL;
	
	ret = ramDifference >> iPageShift;
exit:
	NKern::ThreadLeaveCS();
	return ret;
	}
//
// AllocFixedPages
//
// Allocate a number of fixed pages to memory
//
TInt DRamDefragFuncTestChannel::AllocFixedPages(TInt aNumPages)
	{
	TInt r = AllocFixedArray(aNumPages);
	if (r != KErrNone)
		{
		return r;
		}
	return AllocateFixed2(aNumPages);
	}

/**
Allocate the array required to store the physical addresses of 
number of fixed pages to be allocated.

@param aNumPages 	The number of fixed pages to be allocated.
@return KErrNone on success.
*/
TInt DRamDefragFuncTestChannel::AllocFixedArray(TInt aNumPages)
	{	
	if (iAddrArray != NULL)
		{
		return KErrInUse;
		}
	
	if (aNumPages == FILL_ALL_FIXED)
		{// Fill memory with fixed pages.
		aNumPages = FreeRam() >> iPageShift;
		TESTDEBUG(Kern::Printf("aNumPages %d FreeRam() %d", aNumPages, FreeRam()));
		}
	NKern::ThreadEnterCS();

	iAddrArray = new TPhysAddr[aNumPages];
	iAddrArraySize = aNumPages;	// Only required for AllocateFixed2() when aNumPages == FILL_ALL_FIXED.
	iAddrArrayPages = 0;	// No physical pages have been allocated yet.

	NKern::ThreadLeaveCS();

	if (!iAddrArray)
		{
		return KErrNoMemory;
		}
	return KErrNone;
	}


/**
Allocate the specified number of fixed pages.
This should only be invoked when iAddrArray has already been allocated

@param aNumPages 	The number of pages to allocate.
*/	
TInt DRamDefragFuncTestChannel::AllocateFixed2(TInt aNumPages)
	{
	if (iAddrArray == NULL)
		{
		return KErrGeneral;
		}
	TInt retVal = KErrNone;
	NKern::ThreadEnterCS();
	if (aNumPages == FILL_ALL_FIXED)
		{
		// Allocate a number of fixed pages to RAM a page at time so that the allocations
		// will always fill as much memory as possible.
		TPhysAddr* addrPtr = iAddrArray;
		TPhysAddr* addrPtrEnd = addrPtr + iAddrArraySize;
		while (addrPtr < addrPtrEnd)
			{
			retVal = Epoc::AllocPhysicalRam(1, addrPtr++);
			if (retVal != KErrNone)
				break;
			iAddrArrayPages++;
			}
		}
	else
		{
		retVal = Epoc::AllocPhysicalRam(aNumPages, iAddrArray);
		if (retVal != KErrNone)
			{
			TESTDEBUG(Kern::Printf("aNumPages %d FreeRam() %d", aNumPages, FreeRam()));
			delete[] iAddrArray;
			iAddrArray = NULL;
			TESTDEBUG(Kern::Printf("aNumPages %d FreeRam() %d", aNumPages, FreeRam()));
			TESTDEBUG(Kern::Printf("Fixed pages alloc was unsuccessful\n"));
			}
		else
			iAddrArrayPages = aNumPages;
		}

	NKern::ThreadLeaveCS();
	return retVal;
	}	
//
// CheckCancel
//
// Check that when a defrag is cancelled, the correct return value is reported
//
TInt DRamDefragFuncTestChannel::CheckCancel(STestParameters* aParams)
	{
	TInt returnValue = KErrNone;
	STestParameters params;
	kumemget(&params, aParams, sizeof(STestParameters));

	Kern::Printf(	"defragtype = %d, defragversion = %d, priority = %d, maxpages = %d, ID = %d", 
					params.iDefragType, params.iDefragVersion, params.iPriority, params.iMaxPages, params.iID);


	NFastSemaphore sem;
	NKern::FSSetOwner(&sem, 0);
	TPhysAddr zoneAddress;
	TInt maxPages = 0;
	TInt priority = (NKern::CurrentThread()->iPriority) - 2;

	if (params.iDefragType == DEFRAG_TYPE_GEN) // DefragRam
		{
		returnValue = iDefragRequest.DefragRam(&sem, priority, maxPages);
		}
	else if (params.iDefragType == DEFRAG_TYPE_EMPTY) // EmptyRamZone
		{
		returnValue = iDefragRequest.EmptyRamZone(params.iID, &sem, priority);
		}
	else if (params.iDefragType == DEFRAG_TYPE_CLAIM) // ClaimRamZone
		{
		returnValue = iDefragRequest.ClaimRamZone(params.iID, zoneAddress, &sem, priority);
		}
	else
		{
		Kern::Printf("A valid defrag type was not specified");
		return KErrGeneral;
		}

	iDefragRequest.Cancel();
	NKern::FSWait(&sem);
	returnValue = iDefragRequest.Result();
	return returnValue;
	}


//
// CheckPriorities
//
// Queue defrags with differing priorities and ensure they complete in the correct order 
//
TInt DRamDefragFuncTestChannel::CheckPriorities(STestParameters* aParams)
	{
	STestParameters params;
	kumemget(&params, aParams, sizeof(STestParameters));

	// Still have an outstanding defrag operation
	if (iCompleteReq != NULL | iCompleteReq2 != NULL | iCompleteReq3 != NULL)
		{
		return KErrInUse;
		}
	
	// Open a handle to the thread so that it isn't destroyed as defrag dfc may 
	// then try to complete the request on a destroyed thread.
	iRequestThread = &Kern::CurrentThread();
	iRequestThread->Open();
	iCompleteReq = params.iReqStat;

	// Open a reference on this channel to stop the destructor running before
	// this defrag request has completed.
	Open();
	TUint defragZone = params.iID - 1;	
	TInt returnValue = iDefragRequest.EmptyRamZone(defragZone, &iDefragCompleteDfc, 1);
	if (returnValue != KErrNone)
		{
		AsyncClose();
		iCompleteReq = NULL;
		iRequestThread->AsyncClose();
		iRequestThread = NULL;
		return returnValue;
		}

	// Open a handle to the thread so that it isn't destroyed as defrag dfc may 
	// then try to complete the request on a destroyed thread.
	iRequestThread2 = &Kern::CurrentThread();
	iRequestThread2->Open();
	iCompleteReq2 = params.iReqStat2;
	// Open a reference on this channel to stop the destructor running before
	// this defrag request has completed.
	Open();
	defragZone = params.iID;
	returnValue = iDefragRequest2.EmptyRamZone(defragZone, &iDefragComplete2Dfc, 30);
	if (returnValue != KErrNone)
		{
		// Cancel any successfully queued operations.
		// Set dfcs to signal dummy request statuses as user side
		// request status shouldn't be signalled.
		iCompleteReq = &iTmpRequestStatus1;
		iDefragRequest.Cancel();

		// Clean up this operation.
		AsyncClose();
		iCompleteReq2 = NULL;
		iRequestThread2->AsyncClose();
		iRequestThread2 = NULL;
		return returnValue;
		}

	// Open a handle to the thread so that it isn't destroyed as defrag dfc may 
	// then try to complete the request on a destroyed thread.
	iRequestThread3 = &Kern::CurrentThread();
	iRequestThread3->Open();
	iCompleteReq3 = params.iReqStat3;
	// Open a reference on this channel to stop the destructor running before
	// this defrag request has completed.
	Open();
	defragZone = params.iID + 2;
	returnValue = iDefragRequest3.EmptyRamZone(defragZone, &iDefragComplete3Dfc, 60);
	if (returnValue != KErrNone)
		{
		// Cancel any successfully queued operations.
		// Set dfcs to signal dummy request statuses as user side
		// request status shouldn't be signalled.
		iCompleteReq = &iTmpRequestStatus1;
		iCompleteReq2 = &iTmpRequestStatus2;
		iDefragRequest.Cancel();
		iDefragRequest2.Cancel();

		// clean up this defrag operation
		AsyncClose();
		iCompleteReq3 = NULL;
		iRequestThread3->AsyncClose();
		iRequestThread3 = NULL;
		return returnValue;
		}
	return returnValue;
	}

//
// GetDefragOrder
//
// Get the order in which the defrags were completed 
//
TInt DRamDefragFuncTestChannel::GetDefragOrder()
	{
	Kern::Printf("order = %d", iOrder);
	return iOrder;
	}


//
// CallDefrag
//
// Call a specific defrag depening on the parameters that it is called with
//
TInt DRamDefragFuncTestChannel::CallDefrag(STestParameters* aParams)
	{
	TInt returnValue = 0;
	STestParameters params;
	kumemget(&params, aParams, sizeof(STestParameters));
	
	TESTDEBUG(Kern::Printf("defragtype = %d, defragversion = %d, priority = %d, maxpages = %d, ID = %d", 
					params.iDefragType, params.iDefragVersion, params.iPriority, params.iMaxPages, params.iID));


	NFastSemaphore sem;
	NKern::FSSetOwner(&sem, 0);

	if (params.iDefragType == DEFRAG_TYPE_GEN) // DefragRam
		{
		switch(params.iDefragVersion) 
			{
			case DEFRAG_VER_SYNC: // Sync
				returnValue = iDefragRequest.DefragRam(params.iPriority, params.iMaxPages);
				break;
			
			case DEFRAG_VER_SEM: // Semaphore
				returnValue = iDefragRequest.DefragRam(&sem, params.iPriority, params.iMaxPages);
				NKern::FSWait(&sem);
				returnValue = iDefragRequest.Result();
				break;
		
			case DEFRAG_VER_DFC: // Dfc
				// Open a handle to the thread so that it isn't destroyed as defrag dfc may 
				// then try to complete the request on a destroyed thread.
				if (iCompleteReq == NULL)
					{
					iRequestThread = &Kern::CurrentThread();
					iRequestThread->Open();
					iCompleteReq = params.iReqStat;
					// Open a reference on this channel to stop the destructor running before
					// the defrag request has completed.
					Open();
					
					returnValue = iDefragRequest.DefragRam(&iDefragCompleteDfc, params.iPriority, params.iMaxPages);
					if (returnValue != KErrNone)
						{// defrag operation didn't start so close all openned handles
						AsyncClose();
						iRequestThread->AsyncClose();
						iRequestThread = NULL;
						iCompleteReq = NULL;
						}
					}
				else
					{// Still have a pending defrag request
					returnValue = KErrInUse;
					}
				break;
				
			default: 
			break;	
			}
		}

	else if (params.iDefragType == DEFRAG_TYPE_EMPTY) // EmptyRamZone
		{
		switch(params.iDefragVersion) 
			{
			case DEFRAG_VER_SYNC: // Sync
				
				returnValue = iDefragRequest.EmptyRamZone(params.iID, params.iPriority);
				break;
			
			case DEFRAG_VER_SEM: // Semaphore
				returnValue = iDefragRequest.EmptyRamZone(params.iID, &sem, params.iPriority);
				NKern::FSWait(&sem);
				returnValue = iDefragRequest.Result();
				break;
		
			case DEFRAG_VER_DFC: // Dfc
				if (iCompleteReq == NULL)
					{
					// Open a handle to the thread so that it isn't destroyed as defrag dfc may 
					// then try to complete the request on a destroyed thread.
					iRequestThread = &Kern::CurrentThread();
					iRequestThread->Open();
					iCompleteReq = params.iReqStat;
					// Open a reference on this channel to stop the destructor running before
					// the defrag request has completed.
					Open();
					
					returnValue = iDefragRequest.EmptyRamZone(params.iID, &iDefragCompleteDfc, params.iPriority);
					if (returnValue != KErrNone)
						{// defrag operation didn't start so close all openned handles
						AsyncClose();
						iRequestThread->AsyncClose();
						iRequestThread = NULL;
						iCompleteReq = NULL;
						}
					}
				else
					{// Still have a pending defrag request
					returnValue = KErrInUse;
					}
				break;
				
			default: 
				break;	
			}
		}

	else if (params.iDefragType == DEFRAG_TYPE_CLAIM) // ClaimRamZone
		{
		if (iContigAddr != KPhysAddrInvalid)
			{
			return KErrInUse;
			}
		switch(params.iDefragVersion) 
			{
			case DEFRAG_VER_SYNC: // Sync
				
				returnValue = iDefragRequest.ClaimRamZone(params.iID, iContigAddr, params.iPriority);
				break;
			
			case DEFRAG_VER_SEM: // Semaphore
				returnValue = iDefragRequest.ClaimRamZone(params.iID, iContigAddr, &sem, params.iPriority);
				NKern::FSWait(&sem);
				returnValue = iDefragRequest.Result();
				break;
		
			case DEFRAG_VER_DFC: // Dfc
				if (iCompleteReq == NULL)
					{
					// Open a handle to the thread so that it isn't destroyed as defrag dfc may 
					// then try to complete the request on a destroyed thread.
					iRequestThread = &Kern::CurrentThread();
					iRequestThread->Open();
					iCompleteReq = params.iReqStat;
					// Open a reference on this channel to stop the destructor running before
					// the defrag request has completed.
					Open();
					
					// If the claim is successful iContigAddr will be set just before the dfc 
					// callback function to the physical base address of the RAM zone claimed.  
					// Therefore, the check for iContigAddr is not necessarily safe so use 
					// this DFC version with care and don't use it combination with any 
					// contiguous allocation methods.
					returnValue = iDefragRequest.ClaimRamZone(params.iID, iContigAddr, &iDefragCompleteDfc, 
																params.iPriority);
					if (returnValue != KErrNone)
						{// defrag operation didn't start so close all openned handles
						AsyncClose();
						iRequestThread->AsyncClose();
						iRequestThread = NULL;
						iCompleteReq = NULL;
						}
					}
				else
					{// Still have a pending defrag request
					returnValue = KErrInUse;
					}
				break;
				
			default: 
				break;
			}
		if (returnValue == KErrNone && params.iDefragVersion != DEFRAG_VER_DFC)
			{
			// Get the size of the zone just claimed so that it can be freed.  Don't set
			// iContigBytes for DFC method as it will be cleared by address in t_ramdefrag

			NKern::ThreadEnterCS();

			SRamZonePageCount pageCount;
			returnValue = Epoc::GetRamZonePageCount(params.iID, pageCount);

			NKern::ThreadLeaveCS();

			__NK_ASSERT_ALWAYS(returnValue == KErrNone); // If this fails something is seriously wrong
			iContigBytes = pageCount.iFixedPages << iPageShift;
			}
		else
			{// The claim failed so allow other contiguous allocations.
			iContigAddr = KPhysAddrInvalid;
			}
		}

	return returnValue;
	} 



//
// SetZoneFlag
//
// Change the flag settings of a zone
//
TInt DRamDefragFuncTestChannel::SetZoneFlag(STestFlagParams* aParams)
	{

	TInt returnValue = 0;
	STestFlagParams flagParams;
	kumemget(&flagParams, aParams, sizeof(STestFlagParams));
	TUint setFlag = 0x0;
	switch(flagParams.iSetFlag)
		{ 
		case NO_FIXED_FLAG:
			setFlag = KRamZoneFlagNoFixed;
			break;

		case NO_MOVE_FLAG:
			setFlag = KRamZoneFlagNoMovable;
			break;

		case NO_DISCARD_FLAG:
			setFlag = KRamZoneFlagNoDiscard;
			break;

		case NO_ALLOC_FLAG:
			setFlag = KRamZoneFlagNoAlloc;
			break;

		case ONLY_DISCARD_FLAG:
			setFlag = KRamZoneFlagDiscardOnly;
			break;
		
		case RESET_FLAG:
			setFlag = 0x00;
			break;
		
		case ORIG_FLAG:
			setFlag = flagParams.iOptSetFlag;
			break;
		
			default: 
			break;	
		}

	NKern::ThreadEnterCS();

	returnValue = Epoc::ModifyRamZoneFlags(flagParams.iZoneID, flagParams.iZoneFlag, setFlag);

	NKern::ThreadLeaveCS();
	return returnValue;
	}
//
// PageCount
//
// Call the GetRamZonePageCount function
//
TInt DRamDefragFuncTestChannel::PageCount(TUint aId, STestUserSidePageCount* aPageData)
	{	
	TInt returnValue = 0;
	STestUserSidePageCount pageData;
	SRamZonePageCount pageCount; 

	NKern::ThreadEnterCS();

	returnValue = Epoc::GetRamZonePageCount(aId, pageCount);

	NKern::ThreadLeaveCS();

	pageData.iFreePages = pageCount.iFreePages;
	pageData.iFixedPages = pageCount.iFixedPages;
	pageData.iMovablePages = pageCount.iMovablePages;
	pageData.iDiscardablePages = pageCount.iDiscardablePages;

	kumemput(aPageData, &pageData, sizeof(STestUserSidePageCount));
	return returnValue;
	}

//
// ZoneAllocContiguous
//
// Call the contiguous overload of the Epoc::ZoneAllocPhysicalRam() function
//
TInt DRamDefragFuncTestChannel::ZoneAllocContiguous(TUint aZoneID, TUint aNumBytes)
	{
	TInt returnValue = KErrNone;
	
	if (iContigAddr != KPhysAddrInvalid)
		{
		return KErrInUse;
		}
	iContigBytes = aNumBytes;

	NKern::ThreadEnterCS();

	returnValue = Epoc::ZoneAllocPhysicalRam(aZoneID, iContigBytes, iContigAddr, 0);

	NKern::ThreadLeaveCS();
	
	if (returnValue != KErrNone)
		{
		iContigAddr = KPhysAddrInvalid;
		}
	return returnValue;
	}

//
// ZoneAllocContiguous
//
// Call the contiguous overload of the Epoc::ZoneAllocPhysicalRam() function
//
TInt DRamDefragFuncTestChannel::ZoneAllocContiguous(TUint* aZoneIdList, TUint aZoneIdCount, TUint aNumBytes)
	{
	TInt returnValue = KErrNone;
	
	if (iContigAddr != KPhysAddrInvalid)
		{
		return KErrInUse;
		}
	iContigBytes = aNumBytes;

	// Copy the RAM zone IDs from user side memory to kernel memory.
	if (aZoneIdCount > KMaxRamZones)
		{// Too many IDs.
		return KErrArgument;
		}
	kumemget32(iZoneIdArray, aZoneIdList, sizeof(TUint) * aZoneIdCount);

	NKern::ThreadEnterCS();

	returnValue = Epoc::ZoneAllocPhysicalRam(iZoneIdArray, aZoneIdCount, iContigBytes, iContigAddr, 0);

	NKern::ThreadLeaveCS();
	
	if (returnValue != KErrNone)
		{
		iContigAddr = KPhysAddrInvalid;
		}
	return returnValue;
	}

//
// AllocContiguous
//
// Call the contiguous overload of Epoc::AllocPhysicalRam()
//
TInt DRamDefragFuncTestChannel::AllocContiguous(TUint aNumBytes)
	{
	TInt returnValue = 0;

	if (iContigAddr != KPhysAddrInvalid)
		{
		return KErrInUse;
		}

	NKern::ThreadEnterCS();

	returnValue = Epoc::AllocPhysicalRam(aNumBytes, iContigAddr, 0);

	NKern::ThreadLeaveCS();

	if (returnValue != KErrNone)
		{
		iContigAddr = KPhysAddrInvalid;
		}
	iContigBytes = aNumBytes;
	return returnValue;
	}


//
// ZoneAllocDiscontiguous
//
// Call the discontiguous overload of Epoc::ZoneAllocPhysicalRam() function
//
TInt DRamDefragFuncTestChannel::ZoneAllocDiscontiguous(TUint aZoneId, TInt aNumPages)
	{
	TInt r = AllocFixedArray(aNumPages);
	if (r != KErrNone)
		{
		return r;
		}
	return ZoneAllocDiscontiguous2(aZoneId, aNumPages);
	}

/**
Allocate the specified number of fixed pages from the specified RAM zone.
This should only be invoked when iAddrArray has already been allocated

@param aZoneID		The ID of the RAM zone to allocate from
@param aNumPages 	The number of pages to allocate.
*/	
TInt DRamDefragFuncTestChannel::ZoneAllocDiscontiguous2(TUint aZoneID, TInt aNumPages)
	{
	if (iAddrArray == NULL)
		{
		return KErrGeneral;
		}

	NKern::ThreadEnterCS();

	TESTDEBUG(Kern::Printf("Allocating fixed pages"));
	TInt returnValue = Epoc::ZoneAllocPhysicalRam(aZoneID, aNumPages, iAddrArray);
	
	if (KErrNone != returnValue)
		{
		TESTDEBUG(Kern::Printf("Alloc was unsuccessful, r = %d\n", returnValue));
		TESTDEBUG(Kern::Printf("aNumPages = %d, aZoneID = %d", aNumPages, aZoneID));
		Kern::Free(iAddrArray);
		iAddrArray = NULL;
		goto exit;
		}
	iAddrArrayPages = aNumPages;
	TESTDEBUG(Kern::Printf("iAddrArrayPages = %d, aZoneID = %d", iAddrArrayPages, aZoneID));

exit:
	NKern::ThreadLeaveCS();
	return returnValue;
	}


//
// ZoneAllocDiscontiguous
//
// Call the discontiguous overload of Epoc::ZoneAllocPhysicalRam() function
//
TInt DRamDefragFuncTestChannel::ZoneAllocDiscontiguous(TUint* aZoneIdList, TUint aZoneIdCount, TInt aNumPages)
	{
	TInt returnValue = 0;
	
	if (iAddrArray != NULL)
		{
		return KErrInUse;
		}
	NKern::ThreadEnterCS();

	iAddrArray = new TPhysAddr[aNumPages];

	NKern::ThreadLeaveCS();

	if (iAddrArray == NULL)
		{
		return KErrNoMemory;
		}

	// copy user side data to kernel side buffer.
	if (aZoneIdCount > KMaxRamZones)
		{// Too many IDs.
		return KErrArgument;
		}
	kumemget(iZoneIdArray, aZoneIdList, sizeof(TUint) * aZoneIdCount);
	
	NKern::ThreadEnterCS();

	TESTDEBUG(Kern::Printf("Allocating fixed pages"));
	returnValue = Epoc::ZoneAllocPhysicalRam(iZoneIdArray, aZoneIdCount, aNumPages, iAddrArray);
	
	if (KErrNone != returnValue)
		{
		TESTDEBUG(Kern::Printf("Alloc was unsuccessful, r = %d\n", returnValue));
		TESTDEBUG(Kern::Printf("aNumPages = %d, aZoneID = %d", aNumPages, aZoneIdCount));
		delete[] iAddrArray;
		iAddrArray = NULL;
		goto exit;
		}
	iAddrArrayPages = aNumPages;
	TESTDEBUG(Kern::Printf("iAddrArrayPages = %d, zones = %d", iAddrArrayPages, aZoneIdCount));

exit:
	NKern::ThreadLeaveCS();
	return returnValue;
	}

//
// ZoneAllocToMany
//
// Call the overloaded Epoc::ZoneAllocPhysicalRam function on a number of zones
//
TInt DRamDefragFuncTestChannel::ZoneAllocToMany(TInt aZoneIndex, TInt aNumPages)
	{
	TInt r = ZoneAllocToManyArray(aZoneIndex, aNumPages);
	if (r != KErrNone)
		{
		return r;
		}
	return ZoneAllocToMany2(aZoneIndex, aNumPages);
	}

//
// ZoneAllocToManyArray
//
// Allocate the arrays required to store the physical addresses of the different zones 
// for the number of fixed pages to be allocated to that zone.
//
TInt DRamDefragFuncTestChannel::ZoneAllocToManyArray(TInt aZoneIndex, TInt aNumPages)
	{
	TInt returnValue = KErrNone;
	NKern::ThreadEnterCS();
	
	if (iAddrPtrArray == NULL)
		{
		iAddrPtrArray = (TPhysAddr**)Kern::AllocZ(sizeof(TPhysAddr*) * iZoneCount);
		}
	if (iNumPagesArray == NULL)
		{
		iNumPagesArray = (TInt *)Kern::AllocZ(sizeof(TInt) * iZoneCount);
		}
	
	if (iAddrPtrArray[aZoneIndex] != NULL)
		{
		returnValue = KErrInUse;
		goto exit;
		}
	
	iAddrPtrArray[aZoneIndex] = (TPhysAddr *)Kern::AllocZ(sizeof(TPhysAddr) * aNumPages);
	if (iAddrPtrArray[aZoneIndex] == NULL)
		{
		returnValue = KErrNoMemory;
		goto exit;
		}

exit:
	NKern::ThreadLeaveCS();
	return returnValue;
	}

//
// ZoneAllocToMany2
//
// Call the overloaded Epoc::ZoneAllocPhysicalRam function on a number of zones
// This should only be invoked when iAddrPtrArray, iNumPagesArray and iAddrPtrArray[aZoneIndex]
// have already been allocated
//
TInt DRamDefragFuncTestChannel::ZoneAllocToMany2(TInt aZoneIndex, TInt aNumPages)
	{
	TInt returnValue = KErrNone;
	struct SRamZoneConfig	zoneConfig;
	TUint zoneID = KRamZoneInvalidId;

	if (iAddrPtrArray == NULL ||
		iNumPagesArray == NULL ||
		iAddrPtrArray[aZoneIndex] == NULL)
		{
		return KErrGeneral;
		}


	NKern::ThreadEnterCS();
	
	// Get the zone ID
	Kern::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)aZoneIndex, (TAny*)&zoneConfig);
	zoneID = zoneConfig.iZoneId;
	returnValue = Epoc::ZoneAllocPhysicalRam(zoneID, aNumPages, iAddrPtrArray[aZoneIndex]);
	
	if (KErrNone != returnValue)
		{
		TESTDEBUG(Kern::Printf("Alloc was unsuccessful, r = %d\n", returnValue));
		Kern::Free(iAddrPtrArray[aZoneIndex]);
		iAddrPtrArray[aZoneIndex] = NULL;
		goto exit;
		}
	iNumPagesArray[aZoneIndex] = aNumPages;

exit:
	NKern::ThreadLeaveCS();
	return returnValue;
	}

//
// FreeZone
//
// Call the overloaded Epoc::FreePhysicalRam function
//
TInt DRamDefragFuncTestChannel::FreeZone(TInt aNumPages)
	{
	TInt returnValue = 0;

	if (iAddrArray == NULL)
		{
		return KErrCorrupt;
		}

	NKern::ThreadEnterCS();
	
	returnValue = Epoc::FreePhysicalRam(aNumPages, iAddrArray);
	
	Kern::Free(iAddrArray);
	iAddrArray = NULL;

	NKern::ThreadLeaveCS();
	return returnValue;
	}

//
// FreeZoneId
//
// Call Epoc::FreeRamZone()
//
TInt DRamDefragFuncTestChannel::FreeZoneId(TUint aZoneId)
	{
	NKern::ThreadEnterCS();
	
	TInt r = Epoc::FreeRamZone(aZoneId);
	if (r == KErrNone)
		{
		if (iContigAddr == KPhysAddrInvalid)
			{
			Kern::Printf("Error some how freed a RAM zone that wasn't previously claimed");
			NKern::ThreadLeaveCS();
			return KErrGeneral;
			}
		iContigAddr = KPhysAddrInvalid;
		}
	NKern::ThreadLeaveCS();
	return r;
	}

//
// FreeFromAllZones
//
// Call the overloaded Epoc::FreePhysicalRam function
//
TInt DRamDefragFuncTestChannel::FreeFromAllZones()
	{
	TInt returnValue = 0;

	if (iAddrPtrArray == NULL)
		{
		return KErrCorrupt;
		}

	NKern::ThreadEnterCS();

	for (TUint i=0; i<iZoneCount; i++)
		{
		if (iAddrPtrArray[i] != NULL)
			{
			returnValue = Epoc::FreePhysicalRam(iNumPagesArray[i], iAddrPtrArray[i]);
			iAddrPtrArray[i] = NULL;
			}
		}
	Kern::Free(iAddrPtrArray);
	iAddrPtrArray = NULL;

	Kern::Free(iNumPagesArray);
	iNumPagesArray = NULL;

	NKern::ThreadLeaveCS();	
	return returnValue;
	}
//
// FreeFromAddr
//
// Free a specific number of pages starting from a specific address
//
TInt DRamDefragFuncTestChannel::FreeFromAddr(TInt aNumPages, TUint32 aAddr)
	{
	TInt returnValue = 0;
	TPhysAddr address = aAddr;

	NKern::ThreadEnterCS();

	returnValue = Epoc::FreePhysicalRam(address, aNumPages << iPageShift);

	NKern::ThreadLeaveCS();

	return returnValue;
	}

//
// FreeRam
//
// Returns the current free RAM available in bytes
//
TInt DRamDefragFuncTestChannel::FreeRam()
	{
	return Kern::FreeRamInBytes();
	}

TInt DRamDefragFuncTestChannel::DoSetDebugFlag(TInt aState)
	{
	iDebug = aState;
	return KErrNone;
	}


//
//	DefragCompleteDfc
//
//	DFC callback called when a defrag operation has completed.
//
void DRamDefragFuncTestChannel::DefragCompleteDfc(TAny* aSelf)
	{
	// Just call non-static method
	TESTDEBUG(Kern::Printf("Calling DefragCompleteDfc"));
	((DRamDefragFuncTestChannel*)aSelf)->DefragComplete();
	}


//
//	DefragComplete
//
//	Invoked by the DFC callback which is called when a defrag 
//	operation has completed.
//
void DRamDefragFuncTestChannel::DefragComplete()
	{
	TESTDEBUG(Kern::Printf(">DDefragChannel::DefragComplete - First Defrag"));
	TInt result = iDefragRequest.Result();
	TESTDEBUG(Kern::Printf("complete code %d", result));

	// Complete the request and close the handle to the driver
	Kern::SemaphoreWait(*iDefragSemaphore);

	Kern::RequestComplete(iRequestThread, iCompleteReq, result);
	iCompleteReq = NULL;
	iRequestThread->Close(NULL);
	iRequestThread = NULL;

	Kern::SemaphoreSignal(*iDefragSemaphore);

	++iCounter;
	if (iCounter == 1)
		iOrder = 1;
	else if (iCounter == 2 && iOrder == 2)
		iOrder = 21;
	else if (iCounter == 2 && iOrder == 3)
		iOrder = 31;
	else if (iCounter == 3 && iOrder == 23)
		iOrder = 231;
	else if (iCounter == 3 && iOrder == 32)
		iOrder = 321;
	TESTDEBUG(Kern::Printf("order = %d", iOrder));
	TESTDEBUG(Kern::Printf("<DDefragChannel::DefragComplete"));

	// Close the handle on this channel - WARNING this channel may be 
	// deleted immmediately after this call so don't access any members
	AsyncClose();
	}


//
//	Defrag2CompleteDfc
//
//	DFC callback called when a defrag operation has completed.
//	This is used for a particular test case when 3 
//	defrags are queued at the same time. 
//
void DRamDefragFuncTestChannel::Defrag2CompleteDfc(TAny* aSelf)
	{
	// Just call non-static method
	TESTDEBUG(Kern::Printf("Calling DefragCompleteDfc"));
	((DRamDefragFuncTestChannel*)aSelf)->Defrag2Complete();
	}


//
//	Defrag2Complete
//
//	Invoked by the DFC callback which is called when a defrag 
//	operation has completed. This is used for a particular test case when 3 
//	defrags are queued at the same time. 
//
void DRamDefragFuncTestChannel::Defrag2Complete()
	{
	TESTDEBUG(Kern::Printf(">DDefragChannel::Defrag2Complete - Second Defrag"));
	TInt result = iDefragRequest2.Result();
	TESTDEBUG(Kern::Printf("complete code %d", result));
	// Complete the request and close the handle to the driver
	Kern::SemaphoreWait(*iDefragSemaphore);

	Kern::RequestComplete(iRequestThread2, iCompleteReq2, result);
	iCompleteReq2 = NULL;
	iRequestThread2->Close(NULL);
	iRequestThread2 = NULL;

	Kern::SemaphoreSignal(*iDefragSemaphore);

	++iCounter;
	if (iCounter == 1)
		iOrder = 2;
	else if (iCounter == 2 && iOrder == 1)
		iOrder = 12;
	else if (iCounter == 2 && iOrder == 3)
		iOrder = 32;
	else if (iCounter == 3 && iOrder == 13)
		iOrder = 132;
	else if (iCounter == 3 && iOrder == 31)
		iOrder = 312;
	TESTDEBUG(Kern::Printf("order = %d", iOrder));
	TESTDEBUG(Kern::Printf("<DDefragChannel::DefragComplete"));

	// Close the handle on this channel - WARNING this channel may be 
	// deleted immmediately after this call so don't access any members
	AsyncClose();
	}


//
//	Defrag3CompleteDfc
//
//	DFC callback called when a defrag operation has completed. 
//	This is used for a particular test case when 3 
//	defrags are queued at the same time. 
//
void DRamDefragFuncTestChannel::Defrag3CompleteDfc(TAny* aSelf)
	{
	// Just call non-static method
	TESTDEBUG(Kern::Printf("Calling DefragCompleteDfc"));
	((DRamDefragFuncTestChannel*)aSelf)->Defrag3Complete();
	}

//
//	Defrag3Complete
//
//	Invoked by the DFC callback which is called when a defrag 
//	operation has completed. This is used for a particular test case when 3 
//	defrags are queued at the same time. 
//
void DRamDefragFuncTestChannel::Defrag3Complete()
	{
	TESTDEBUG(Kern::Printf(">DDefragChannel::DefragComplete - Third Defrag"));
	TInt result = iDefragRequest3.Result();
	TESTDEBUG(Kern::Printf("complete code %d", result));

	Kern::SemaphoreWait(*iDefragSemaphore);

	Kern::RequestComplete(iRequestThread3, iCompleteReq3, result);
	iCompleteReq3 = NULL;
	iRequestThread3->Close(NULL);
	iRequestThread3 = NULL;

	Kern::SemaphoreSignal(*iDefragSemaphore);


	++iCounter;
	if (iCounter == 1)
		iOrder = 3;
	else if (iCounter == 2 && iOrder == 1)
		iOrder = 13;
	else if (iCounter == 2 && iOrder == 2)
		iOrder = 23;
	else if (iCounter == 3 && iOrder == 12)
		iOrder = 123;
	else if (iCounter == 3 && iOrder == 21)
		iOrder = 213;
	TESTDEBUG(Kern::Printf("order = %d", iOrder));
	TESTDEBUG(Kern::Printf("<DDefragChannel::DefragComplete"));

	// Close the handle on this channel - WARNING this channel may be 
	// deleted immmediately after this call so don't access any members
	AsyncClose();
	}

//
// ResetDriver
// 
// Reset all the member variables in the driver
//
TInt DRamDefragFuncTestChannel::ResetDriver()
	{
	iDebug = 0; 
	iThreadCounter = 1; 
	iCounter = 0;
	iOrder = 0;
	FreeAllFixedPages();

	return KErrNone;
	}
