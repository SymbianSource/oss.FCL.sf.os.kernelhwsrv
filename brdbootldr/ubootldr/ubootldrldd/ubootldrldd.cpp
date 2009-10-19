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
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "ubootldrldd.h"

static TInt ChunkDestroyedCount=1;	// Test counter

//
// Class definitions
//

class DUBootldrFactory : public DLogicalDevice
	{
public:
	~DUBootldrFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	void LockWait();
	void LockSignal();
private:
	NFastMutex iLock;
	};

class DUBootldrChannel : public DLogicalChannelBase
	{
public:
	~DUBootldrChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	DChunk* OpenChunk(TLinAddr* aKernelAddr=0, TInt* aMaxSize=0);
	inline void LockWait()
		{ iFactory->LockWait(); }
	inline void LockSignal()
		{ iFactory->LockSignal(); }
public:
	DUBootldrFactory*	iFactory;
	DChunk*					iChunk;
	TLinAddr				iKernelAddress;
	TInt					iMaxSize;
	};

class TChunkCleanup : public TDfc
	{
public:
	TChunkCleanup(DUBootldrFactory* aFactory,TBool aReleasePhysicalMemory);
	~TChunkCleanup();
	static void ChunkDestroyed(TChunkCleanup* aSelf);
	void Cancel();
public:
	DUBootldrFactory* iFactory;
	};

//
// TChunkCleanup
//

TChunkCleanup::TChunkCleanup(DUBootldrFactory* aFactory,TBool aReleasePhysicalMemory)
	: TDfc((TDfcFn)TChunkCleanup::ChunkDestroyed,this,Kern::SvMsgQue(),0)
	, iFactory(0)
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
	DUBootldrFactory* factory = aSelf->iFactory;
	if(factory)
		{
		factory->LockWait();
		++ChunkDestroyedCount;
		factory->LockSignal();
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
// DUBootldrFactory
//

TInt DUBootldrFactory::Install()
	{
	return SetName(&KBootldrLddName);
	}

DUBootldrFactory::~DUBootldrFactory()
	{
	}

void DUBootldrFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DUBootldrFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DUBootldrChannel* channel=new DUBootldrChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

void DUBootldrFactory::LockWait()
	{
	NKern::FMWait(&iLock);
	}

void DUBootldrFactory::LockSignal()
	{
	NKern::FMSignal(&iLock);
	}

DECLARE_STANDARD_LDD()
	{
	return new DUBootldrFactory;
	}

//
// DUBootldrChannel
//

TInt DUBootldrChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DUBootldrChannel::~DUBootldrChannel()
	{
	if(iChunk)
		iChunk->Close(0);
	}

DChunk* DUBootldrChannel::OpenChunk(TLinAddr* aKernelAddr,TInt* aMaxSize)
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


TInt DUBootldrChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
//	Kern::Printf("Request a1 0x%x a2 0x%x", (TUint)a1, (TUint)a2);
	TInt r=KErrNotSupported;

	switch(aFunction)
		{

	case RUBootldrLdd::ECreateChunk:
		{
		if(ChunkDestroyedCount==0)
			NKern::Sleep(NKern::TimerTicks(100)); // Go idle for a while to let chunk cleanup DFCs to be called

		NKern::ThreadEnterCS();
		TInt chunksize = (TInt)a1;

		TChunkCleanup* cleanup = new TChunkCleanup(this->iFactory,ETrue);
		if(!cleanup)
			{
			NKern::ThreadLeaveCS();
			return KErrNoMemory;
			}

		// Try and create chunk...
		DChunk* chunk;
		TChunkCreateInfo info;

		info.iType=TChunkCreateInfo::ESharedKernelSingle;
		info.iMaxSize	 = chunksize;
		info.iMapAttr	 = EMapAttrFullyBlocking;
		info.iOwnsMemory = EFalse;
		info.iDestroyedDfc = cleanup;

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
			if(r==KErrNone)
				{
				iChunk = chunk;
				iKernelAddress = kernAddr;
				iMaxSize = info.iMaxSize;
				ChunkDestroyedCount = 0;
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


	case RUBootldrLdd::EGetChunkHandle:
		{
		NKern::ThreadEnterCS();
		DChunk* chunk=OpenChunk();
		if(chunk)
			{
			r = Kern::MakeHandleAndOpen(0,chunk);
			chunk->Close(0);
			}
		else
			r = KErrNotFound;
		NKern::ThreadLeaveCS();
		return r;
		}


	case RUBootldrLdd::ECloseChunkHandle:
		{
		NKern::ThreadEnterCS();
		r = Kern::CloseHandle(0,(TInt)a1);
		NKern::ThreadLeaveCS();
		return r;
		}


	case RUBootldrLdd::ECommitMemory:
		{
		NKern::ThreadEnterCS();
		TUint32 chunkKernelAddress;
		DChunk* chunk=OpenChunk(&chunkKernelAddress);
		if(chunk)
			{
			TUint sz=(TUint)a1;
			TUint physaddr=(TUint)a2;

			// Kern::Printf("Commit chunk = 0x%x sz = 0x%x, addr = 0x%x", chunk, sz, physaddr);	// XXX

			// LDD i/f guarantees page alignment of physaddr
			r = Kern::ChunkCommitPhysical(chunk,0,sz,physaddr);
			// chunk->Close(0);
			}
		else
			r = KErrNotFound;
		NKern::ThreadLeaveCS();
		return r;
		}


	case RUBootldrLdd::EIsDestroyed:
		{
		// First wait for short while to allow idle thread to run and do
		// cleanup of chunk
		NKern::Sleep(NKern::TimerTicks(100));
		return ChunkDestroyedCount;
		}


	case RUBootldrLdd::ECloseChunk:
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
	case RUBootldrLdd::ERestart:
		{
		TInt aReason = (TInt)a1;
		// Kern::Printf("Restart:: %d", aReason);
		Kern::Restart(aReason);
		return KErrNone; // Notreached
		}
	default:
		return KErrNotSupported;
		}
	}
