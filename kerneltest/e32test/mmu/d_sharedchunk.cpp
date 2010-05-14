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
// e32test\mmu\d_sharedchunk.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "d_sharedchunk.h"

TBool PhysicalCommitSupported = ETrue;

#ifdef __EPOC32__
#define TEST_PHYSICAL_COMMIT
#endif

static volatile TInt ChunkDestroyedCount=1;	// Test counter

//
// Class definitions
//

class DSharedChunkFactory : public DLogicalDevice
	{
public:
	~DSharedChunkFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	TInt ClaimMemory();
	void ReleaseMemory();
	TInt AllocMemory(TInt aSize, TUint32& aPhysAddr);
	void FreeMemory(TInt aSize,TUint32 aPhysAddr);
	void LockWait();
	void LockSignal();
private:
	NFastMutex iLock;
public:
	TBool iMemoryInUse;
	TUint32 iPhysBase;
	TUint32 iPhysEnd;
	TUint32 iPhysNext;
	TInt* iDummyCell;
	};

class DSharedChunkChannel : public DLogicalChannelBase
	{
public:
	DSharedChunkChannel();
	~DSharedChunkChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	DChunk* OpenChunk(TLinAddr* aKernelAddr=0, TInt* aMaxSize=0);
	inline void LockWait()
		{ iFactory->LockWait(); }
	inline void LockSignal()
		{ iFactory->LockSignal(); }
	TUint32 DfcReadWrite(TUint32* aPtr, TUint32 aValue);
	TUint32 IsrReadWrite(TUint32* aPtr, TUint32 aValue);
public:
	DSharedChunkFactory*	iFactory;
	DChunk*					iChunk;
	TLinAddr				iKernelAddress;
	TInt					iMaxSize;
	};

class TChunkCleanup : public TDfc
	{
public:
	TChunkCleanup(DSharedChunkFactory* aFactory,TBool aReleasePhysicalMemory);
	~TChunkCleanup();
	static void ChunkDestroyed(TChunkCleanup* aSelf);
	void Cancel();
public:
	DSharedChunkFactory* iFactory;
	TBool iReleasePhysicalMemory;
	};

//
// TChunkCleanup
//

TChunkCleanup::TChunkCleanup(DSharedChunkFactory* aFactory,TBool aReleasePhysicalMemory)
	: TDfc((TDfcFn)TChunkCleanup::ChunkDestroyed,this,Kern::SvMsgQue(),0)
	, iFactory(0), iReleasePhysicalMemory(aReleasePhysicalMemory)
	{
	aFactory->Open();
	iFactory = aFactory;
	}

TChunkCleanup::~TChunkCleanup()
	{
	if(iFactory)
		iFactory->Close(0);
	}

void TChunkCleanup::ChunkDestroyed(TChunkCleanup* aSelf)
	{
	__KTRACE_OPT(KMMU,Kern::Printf("D_SHAREDCHUNK ChunkDestroyed DFC\n"));
	DSharedChunkFactory* factory = aSelf->iFactory;
	if(factory)
		{
		factory->LockWait();
		if(aSelf->iReleasePhysicalMemory)
			factory->ReleaseMemory();
		factory->LockSignal();
		__e32_atomic_add_ord32(&ChunkDestroyedCount, 1);
		__KTRACE_OPT(KMMU,Kern::Printf("D_SHAREDCHUNK ChunkDestroyedCount=%d\n",ChunkDestroyedCount));
		}
	delete aSelf;
	}

void TChunkCleanup::Cancel()
	{
	if(iFactory)
		{
		iFactory->Close(0);
		iFactory = 0;
		}
	};

//
// DSharedChunkFactory
//

TInt DSharedChunkFactory::Install()
	{
	TUint mm=Kern::HalFunction(EHalGroupKernel,EKernelHalMemModelInfo,0,0)&EMemModelTypeMask;
	PhysicalCommitSupported = mm!=EMemModelTypeDirect && mm!=EMemModelTypeEmul;
#ifdef __EPOC32__
	if(PhysicalCommitSupported)
		{
		TInt physSize = 4096*1024;
		TInt r=Epoc::AllocPhysicalRam(physSize, iPhysBase);
		if(r!=KErrNone)
			return r;
		iPhysNext = iPhysBase;
		iPhysEnd = iPhysBase+physSize;
		iMemoryInUse = EFalse;
		}
#endif
	// Make sure there is enough space on kernel heap to that heap doesn't need
	// to expand when allocating objects. (Required for OOM and memory leak testing.)
	TAny* expandHeap = Kern::Alloc(16*1024);
	iDummyCell = new TInt;
	Kern::Free(expandHeap);

	return SetName(&KSharedChunkLddName);
	}

DSharedChunkFactory::~DSharedChunkFactory()
	{
#ifdef __EPOC32__
	if(PhysicalCommitSupported)
		Epoc::FreePhysicalRam(iPhysBase, iPhysEnd-iPhysBase);
#endif
	delete iDummyCell;
	}

void DSharedChunkFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DSharedChunkFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DSharedChunkChannel* channel=new DSharedChunkChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

void DSharedChunkFactory::LockWait()
	{
	NKern::FMWait(&iLock);
	}

void DSharedChunkFactory::LockSignal()
	{
	NKern::FMSignal(&iLock);
	}

TInt DSharedChunkFactory::AllocMemory(TInt aSize, TUint32& aPhysAddr)
	{
	if(!PhysicalCommitSupported)
		aSize = 0;
	TInt r=KErrNone;
	Kern::RoundToPageSize(aSize);
	LockWait();
	if(iPhysNext+aSize>iPhysEnd)
		r = KErrNoMemory;
	else
		{
		aPhysAddr = iPhysNext;
		iPhysNext += aSize;
		}
	LockSignal();
	return r;
	}

TInt DSharedChunkFactory::ClaimMemory()
	{
	if (__e32_atomic_swp_ord32(&iMemoryInUse, 1))
		return KErrInUse;
	iPhysNext = iPhysBase;	// reset allocation pointer
	return KErrNone;
	}

void DSharedChunkFactory::ReleaseMemory()
	{
	iMemoryInUse=EFalse;
	}

void DSharedChunkFactory::FreeMemory(TInt aSize,TUint32 aPhysAddr)
	{
	if(!PhysicalCommitSupported)
		aSize = 0;
	if(iPhysNext!=aPhysAddr+aSize)
		{ FAULT(); }	// Only support freeing from the end
	Kern::RoundToPageSize(aSize);
	LockWait();
	iPhysNext -= aSize;
	LockSignal();
	}

DECLARE_STANDARD_LDD()
	{
	return new DSharedChunkFactory;
	}

//
// DSharedChunkChannel
//

TInt DSharedChunkChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DSharedChunkChannel::DSharedChunkChannel()
	{
	}

DSharedChunkChannel::~DSharedChunkChannel()
	{
	if(iChunk)
		iChunk->Close(0);
	}


void DoDfcReadWrite(TUint32* aArgs)
	{
	TUint32* ptr = (TUint32*)aArgs[0];
	TUint32 value = aArgs[1];
	aArgs[1] = *ptr;
	*ptr = value;
	NKern::FSSignal((NFastSemaphore*)aArgs[2]);
	}

TUint32 DSharedChunkChannel::DfcReadWrite(TUint32* aPtr, TUint32 aValue)
	{
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,0);

	TUint32 args[3];
	args[0] = (TUint32)aPtr;
	args[1] = aValue;
	args[2] = (TUint32)&sem;

	TDfc dfc((TDfcFn)DoDfcReadWrite,&args,Kern::SvMsgQue(),0);
	dfc.Enque();
	NKern::FSWait(&sem);

	return args[1];
	}


void DoIsrReadWrite(TUint32* aArgs)
	{
	TUint32* ptr = (TUint32*)aArgs[0];
	TUint32 value = aArgs[1];
	aArgs[1] = *ptr;
	*ptr = value;
	((TDfc*)aArgs[2])->Add();
	}

void DoIsrReadWriteDfcCallback(TUint32* aArgs)
	{
	NKern::FSSignal((NFastSemaphore*)aArgs);
	}

TUint32 DSharedChunkChannel::IsrReadWrite(TUint32* aPtr, TUint32 aValue)
	{
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,0);

	TDfc dfc((TDfcFn)DoIsrReadWriteDfcCallback,&sem,Kern::SvMsgQue(),0);

	TUint32 args[3];
	args[0] = (TUint32)aPtr;
	args[1] = aValue;
	args[2] = (TUint32)&dfc;

	NTimer timer((NTimerFn)DoIsrReadWrite,&args);
	timer.OneShot(1);

	NKern::FSWait(&sem);
	return args[1];
	}


DChunk* DSharedChunkChannel::OpenChunk(TLinAddr* aKernelAddr,TInt* aMaxSize)
	{
	__ASSERT_CRITICAL	// Thread must be in critical section (to avoid leaking access count on chunk)
	LockWait();
	DChunk* chunk=iChunk;
	if(chunk)
		if(chunk->Open()!=KErrNone)
			chunk = NULL;
	if(aKernelAddr)
		*aKernelAddr = chunk ? iKernelAddress : NULL;
	if(aMaxSize)
		*aMaxSize = chunk ? iMaxSize : 0;
	LockSignal();
	return chunk;
	}


TUint8 ReadByte(volatile TUint8* aPtr)
	{
	return *aPtr;
	}

void signal_sem(TAny* aPtr)
	{
	NKern::FSSignal((NFastSemaphore*)aPtr);
	}

TInt WaitForIdle()
	{
	NFastSemaphore s(0);
	TDfc idler(&signal_sem, &s, Kern::SvMsgQue(), 0);	// supervisor thread, priority 0, so will run after destroyed DFC
	NTimer timer(&signal_sem, &s);
	idler.QueueOnIdle();
	timer.OneShot(NKern::TimerTicks(5000), ETrue);	// runs in DFCThread1
	NKern::FSWait(&s);	// wait for either idle DFC or timer
	TBool timeout = idler.Cancel();	// cancel idler, return TRUE if it hadn't run
	TBool tmc = timer.Cancel();	// cancel timer, return TRUE if it hadn't expired
	if (!timeout && !tmc)
		NKern::FSWait(&s);	// both the DFC and the timer went off - wait for the second one
	if (timeout)
		return KErrTimedOut;
	return KErrNone;
	}


TInt WaitForIdle2()
	{
	TInt r = WaitForIdle(); // wait for chunk async delete
	if(r==KErrNone)
		r = WaitForIdle();	// wait for chunk destroyed notification DFC
	return r;
	}


TInt DSharedChunkChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt i1 = (TInt)a1;
	TInt i2 = (TInt)a2;

	TInt r=KErrNotSupported;

	switch(aFunction)
		{

	case RSharedChunkLdd::ECreateChunk:
		{
		NKern::ThreadEnterCS();
		if (__e32_atomic_load_acq32(&ChunkDestroyedCount)==0)
			{
			WaitForIdle2(); // Go idle for a while to let chunk cleanup DFCs to be called
			}

		// Create cleanup item
		TBool chunkUsesPhysicalMemory = (i1&EOwnsMemory)==0;

		TChunkCleanup* cleanup = new TChunkCleanup(this->iFactory,chunkUsesPhysicalMemory);
		if(!cleanup)
			{
			NKern::ThreadLeaveCS();
			return KErrNoMemory;
			}

		// Try and create chunk...
		DChunk* chunk;
		TChunkCreateInfo info;

		info.iType		 = (i1&EMultiple)
							? TChunkCreateInfo::ESharedKernelMultiple
							: TChunkCreateInfo::ESharedKernelSingle;

		info.iMaxSize	 = i1&~ECreateFlagsMask;
#ifdef __EPOC32__
		info.iMapAttr	 = (i1&ECached) ? EMapAttrCachedMax
						 : (i1&EBuffered) ? EMapAttrBufferedC
						 : EMapAttrFullyBlocking;
#endif
		info.iOwnsMemory = (i1&EOwnsMemory)!=0;

		info.iDestroyedDfc = cleanup;

		if(i1&EBadType) *(TUint8*)&info.iType = 0xff;

		TUint32 mapAttr;
		TUint32 kernAddr;
		r = Kern::ChunkCreate(info, chunk, kernAddr, mapAttr);
		if(r!=KErrNone)
			{
			delete cleanup;
			NKern::ThreadLeaveCS();
			return r;
			}

		// Setup data members
		LockWait();
		if(iChunk)
			r = KErrAlreadyExists;
		else
			{
			if(chunkUsesPhysicalMemory)
				r = iFactory->ClaimMemory();
			if(r==KErrNone)
				{
				iChunk = chunk;
				iKernelAddress = kernAddr;
				iMaxSize = info.iMaxSize;
				__e32_atomic_store_ord32(&ChunkDestroyedCount,0);
				}
			}
		LockSignal();

		if(r!=KErrNone)
			{
			// There was an error, so discard created chunk
			cleanup->Cancel();
			Kern::ChunkClose(chunk);
			NKern::ThreadLeaveCS();
			return r;
			}

		NKern::ThreadLeaveCS();

		// Write back kernel address of chunk
		if(a2)
			kumemput32(a2,(TAny*)&kernAddr,4);

		return KErrNone;
		}


	case RSharedChunkLdd::EGetChunkHandle:
		{
		TInt isThreadLocal = (TInt)a1;
		TOwnerType ownertype;
		if (isThreadLocal)
			ownertype = EOwnerThread;
		else
			ownertype = EOwnerProcess;

		NKern::ThreadEnterCS();
		DChunk* chunk=OpenChunk();
		if(chunk)
			{
			r = Kern::MakeHandleAndOpen(0,chunk,ownertype);
			chunk->Close(0);
			}
		else
			r = KErrNotFound;
		NKern::ThreadLeaveCS();
		return r;
		}


	case RSharedChunkLdd::ECloseChunkHandle:
		{
		NKern::ThreadEnterCS();
		r = Kern::CloseHandle(0,i1);
		NKern::ThreadLeaveCS();
		return r;
		}


	case RSharedChunkLdd::ECommitMemory:
		{
		NKern::ThreadEnterCS();
		TUint32 chunkKernelAddress;
		DChunk* chunk=OpenChunk(&chunkKernelAddress);
		if(chunk)
			{
			TInt type = i1&ECommitTypeMask;
			i1 &= ~ECommitTypeMask;
			switch(type)
				{
			case EDiscontiguous:
				r = Kern::ChunkCommit(chunk,i1,i2);
				break;

			case EContiguous:
				{
				TUint32 physAddr=~0u;
				r = Kern::ChunkCommitContiguous(chunk,i1,i2,physAddr);
				if(r!=KErrNone || i2==0)
					break;
				if(physAddr==~0u)
					{ r=KErrGeneral; break; }

				// Check that ChunkPhysicalAddress returns addresses consistant with the commit
				TUint32 kernAddr;
				TUint32 mapAttr;
				TUint32 physAddr2;
				r = Kern::ChunkPhysicalAddress(chunk, i1, i2, kernAddr, mapAttr, physAddr2);
				if(r==KErrNone)
					if(kernAddr!=chunkKernelAddress+i1 || physAddr2!=physAddr)
						r=KErrGeneral;

				if(r==KErrNone)
					{
					// Exercise memory sync functions
					Cache::SyncMemoryBeforeDmaRead(kernAddr, i2, mapAttr);
					Cache::SyncMemoryBeforeDmaWrite(kernAddr, i2, mapAttr);
					}
				}
				break;

			case EDiscontiguousPhysical|EBadPhysicalAddress:
			case EDiscontiguousPhysical:
				{
				TUint32 physAddr;
				r = iFactory->AllocMemory(i2,physAddr);
				if(r!=KErrNone)
					break;

				TInt pageSize =	Kern::RoundToPageSize(1);
				TInt numPages = Kern::RoundToPageSize(i2)/pageSize;
				TUint32* physAddrList = new TUint32[numPages];
				TInt i;
				for(i=0; i<numPages; i++)
					physAddrList[i] = physAddr+i*pageSize;
				if(type&EBadPhysicalAddress)
					physAddrList[i-1] |= 1;
				r = Kern::ChunkCommitPhysical(chunk,i1,i2,physAddrList);
				delete[] physAddrList;
				if(r!=KErrNone || i2==0)
					{
					iFactory->FreeMemory(i2,physAddr);
					break;
					}

				// Check that ChunkPhysicalAddress returns the same addresses we used in the commit
				TUint32 kernAddr;
				TUint32 mapAttr;
				TUint32 physAddr2;
				TUint32* physAddrList2 = new TUint32[numPages];
				r = Kern::ChunkPhysicalAddress(chunk, i1, i2, kernAddr, mapAttr, physAddr2, physAddrList2);
				if(r==KErrNone)
					{
					if(kernAddr!=chunkKernelAddress+i1 || physAddr2!=physAddr)
						r=KErrGeneral;
					else
						for(i=0; i<numPages; i++)
							if(physAddrList2[i] != physAddr+i*pageSize)
								r = KErrGeneral;
					}
				delete[] physAddrList2;

				if(r==KErrNone)
					{
					// Exercise memory sync functions
					Cache::SyncMemoryBeforeDmaRead(kernAddr, i2, mapAttr);
					Cache::SyncMemoryBeforeDmaWrite(kernAddr, i2, mapAttr);
					}
				}
				break;

			case EContiguousPhysical|EBadPhysicalAddress:
			case EContiguousPhysical:
				{
				TUint32 physAddr;
				r = iFactory->AllocMemory(i2,physAddr);
				if(r==KErrNone)
					{
					if(type&EBadPhysicalAddress)
						r = Kern::ChunkCommitPhysical(chunk,i1,i2,physAddr|1);
					else
						r = Kern::ChunkCommitPhysical(chunk,i1,i2,physAddr);
					}
				if(r!=KErrNone || i2==0)
					{
					iFactory->FreeMemory(i2,physAddr);
					break;
					}

				// Check that ChunkPhysicalAddress returns the same addresses we used in the commit
				TUint32 kernAddr;
				TUint32 mapAttr;
				TUint32 physAddr2;
				r = Kern::ChunkPhysicalAddress(chunk, i1, i2, kernAddr, mapAttr, physAddr2);
				if(r==KErrNone)
					if(kernAddr!=chunkKernelAddress+i1 || physAddr2!=physAddr)
						r=KErrGeneral;

				if(r==KErrNone)
					{
					// Exercise memory sync functions
					Cache::SyncMemoryBeforeDmaRead(kernAddr, i2, mapAttr);
					Cache::SyncMemoryBeforeDmaWrite(kernAddr, i2, mapAttr);
					}
				}
				break;

			default:
				r = KErrNotSupported;
				break;

				}
			chunk->Close(0);
			}
		else
			r = KErrNotFound;
		NKern::ThreadLeaveCS();
		return r;
		}


	case RSharedChunkLdd::EIsDestroyed:
		{
		NKern::ThreadEnterCS();
		TInt r = WaitForIdle2();
		NKern::ThreadLeaveCS();
		if (r==KErrNone)
			return __e32_atomic_load_acq32(&ChunkDestroyedCount);
		return 0;		// never went idle so can't have been destroyed
		}


	case RSharedChunkLdd::ECloseChunk:
		{
		NKern::ThreadEnterCS();

		// Claim ownership of the chunk
		LockWait();
		DChunk* chunk=iChunk;
		iChunk = 0;
		LockSignal();

		// Close the chunk
		if(chunk)
			r = Kern::ChunkClose(chunk);
		else
			r = KErrNotFound;

		NKern::ThreadLeaveCS();
		return r;
		}


	case RSharedChunkLdd::ECheckMemory:
	case RSharedChunkLdd::EReadMemory:
	case RSharedChunkLdd::EWriteMemory:
		{
		TUint32 value=0;

		NKern::ThreadEnterCS();
		TLinAddr kernAddr;
		TInt maxSize;
		DChunk* chunk=OpenChunk(&kernAddr,&maxSize);
		if(chunk)
			{
			if((TUint)i1>=(TUint)maxSize)
				r = KErrArgument;
			else
				{
				TInt addr = kernAddr+i1;
#ifdef _DEBUG
				TInt debugMask = Kern::CurrentThread().iDebugMask;
				Kern::CurrentThread().iDebugMask = debugMask&~(1U<<KPANIC);
#endif
				XTRAP(r, XT_DEFAULT, 
					if(aFunction==RSharedChunkLdd::ECheckMemory)
						ReadByte((volatile TUint8*)addr);
					else if(aFunction==RSharedChunkLdd::EReadMemory)
						value = *(volatile TUint32*)addr;
					else if(aFunction==RSharedChunkLdd::EWriteMemory)
						*(volatile TUint32*)addr = i2;
					);
#ifdef _DEBUG
				Kern::CurrentThread().iDebugMask = debugMask;
#endif
				if(aFunction==RSharedChunkLdd::ECheckMemory)
					r = r==KErrNone;
				}
			chunk->Close(0);
			}
		else
			r = KErrNotFound;

		NKern::ThreadLeaveCS();

		if(aFunction==RSharedChunkLdd::EReadMemory)
			kumemput32(a2,&value,sizeof(value));

		return r;
		}


	case RSharedChunkLdd::EDfcReadWrite:
	case RSharedChunkLdd::EIsrReadWrite:
		{
		TUint32 value=0;
		kumemget32(&value,a2,sizeof(value));

		NKern::ThreadEnterCS();
		TLinAddr kernAddr;
		TInt maxSize;
		DChunk* chunk=OpenChunk(&kernAddr,&maxSize);
		if(chunk)
			{
			if((TUint)i1>=(TUint)maxSize)
				r = KErrArgument;
			else
				{
				TInt addr = kernAddr+i1;
				if(aFunction==RSharedChunkLdd::EDfcReadWrite)
					value = DfcReadWrite((TUint32*)addr,value);
				else if(aFunction==RSharedChunkLdd::EIsrReadWrite)
					value = IsrReadWrite((TUint32*)addr,value);
				r = KErrNone;
				}
			chunk->Close(0);
			}
		else
			r = KErrNotFound;
		NKern::ThreadLeaveCS();

		kumemput32(a2,&value,sizeof(value));
		return r;
		}


	case RSharedChunkLdd::ETestOpenAddress:
		{
		NKern::ThreadEnterCS();

		TLinAddr kernAddr;
		DChunk* chunk=OpenChunk(&kernAddr);
		if(!chunk)
			{
			NKern::ThreadLeaveCS();
			return KErrNotReady;
			}

		TInt offset;
		DChunk* chunk2 = Kern::OpenSharedChunk(0,a1,EFalse,offset);
		if(chunk2)
			{
			if(chunk2!=chunk)
				r = KErrGeneral;
			else
				r = KErrNone;
			chunk2->Close(0);
			}
		else
			r = KErrNotFound;

		chunk->Close(0);

		NKern::ThreadLeaveCS();
		return r;
		}

	case RSharedChunkLdd::ETestOpenHandle:
		{
		NKern::ThreadEnterCS();

		TLinAddr kernAddr;
		DChunk* chunk=OpenChunk(&kernAddr);
		if(!chunk)
			{
			NKern::ThreadLeaveCS();
			return KErrNotReady;
			}

		DChunk* chunk2 = Kern::OpenSharedChunk(0,i1,EFalse);
		if(chunk2)
			{
			if(chunk2==chunk)
				r = KErrNone;
			else
				r = KErrGeneral;
			chunk2->Close(0);
			}
		else
			r = KErrNotFound;

		chunk->Close(0);

		NKern::ThreadLeaveCS();
		return r;
		}

	case RSharedChunkLdd::ETestAddress:
		{
		NKern::ThreadEnterCS();

		TLinAddr kernAddr;
		DChunk* chunk=OpenChunk(&kernAddr);
		if(!chunk)
			{
			NKern::ThreadLeaveCS();
			return KErrNotReady;
			}

		TLinAddr kernAddr2;
		r = Kern::ChunkAddress(chunk,i1,i2,kernAddr2);
		if(r==KErrNone)
			if(kernAddr2!=kernAddr+i1)
				r = KErrGeneral;

		chunk->Close(0);

		NKern::ThreadLeaveCS();
		return r;
		}
		
	case RSharedChunkLdd::EChunkUserBase:
		{
		NKern::ThreadEnterCS();

		DChunk* chunk=OpenChunk();
		if(!chunk)
			{
			NKern::ThreadLeaveCS();
			return KErrNotReady;
			}

		TUint8* baseAddress = Kern::ChunkUserBase(chunk, &Kern::CurrentThread());

		chunk->Close(0);
		if(a1)
			kumemput32(a1,(TAny*)&baseAddress,4);

		NKern::ThreadLeaveCS();
		return KErrNone;
		}		

	case RSharedChunkLdd::EChunkCloseAndFree:
		{
#ifdef __EPOC32__
		// Allocate and then commit some physical ram to a chunk
		NKern::ThreadEnterCS();
		const TUint KPhysPages = 5;
		TUint pageSize =	Kern::RoundToPageSize(1);
		TUint physBytes = KPhysPages * pageSize;
		TPhysAddr addrArray[KPhysPages];
		TLinAddr linAddr;
		TUint32 mapAttr;
		DChunk* chunk;

		TChunkCreateInfo chunkInfo;
		chunkInfo.iType			= TChunkCreateInfo::ESharedKernelSingle;
		chunkInfo.iMaxSize		= physBytes;
		chunkInfo.iMapAttr		= EMapAttrFullyBlocking;
		chunkInfo.iOwnsMemory	= EFalse;

		r = Kern::ChunkCreate(chunkInfo, chunk, linAddr, mapAttr);
		if (r != KErrNone)
			{
			NKern::ThreadLeaveCS();
			return r;
			}
		r = Epoc::AllocPhysicalRam(KPhysPages, addrArray);
		if (r != KErrNone)
			{
			Kern::ChunkClose(chunk);
			NKern::ThreadLeaveCS();
			return r;
			}
		r = Kern::ChunkCommitPhysical(chunk, 0, physBytes, addrArray);
		if (r != KErrNone)
			{
			Kern::ChunkClose(chunk);
			r = Epoc::FreePhysicalRam(KPhysPages, addrArray);
			NKern::ThreadLeaveCS();
			return r;
			}
		// Now attempt to free the physical ram immediately after the chunk 
		// has been closed.
		Kern::ChunkClose(chunk);
		r = Epoc::FreePhysicalRam(KPhysPages, addrArray);
		NKern::ThreadLeaveCS();
		return r;
#endif
		}

	default:
		return KErrNotSupported;
		}
	}

