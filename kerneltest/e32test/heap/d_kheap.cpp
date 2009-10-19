// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\heap\d_kheap.cpp
// 
//

#include "d_kheap.h"
#include <kernel/kern_priv.h>

class  DKHeap : public DLogicalChannelBase
	{
public:
	DKHeap();
	~DKHeap();
	static TBool Handler (const TDesC8& aText, TTraceSource aTraceSource);
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	TInt TestBurstFailNext(TUint aCount, TUint aBurst);
	TInt TestBurstDeterministic(TUint aRate, TUint aBurst);
	};

DKHeap* KHeapDriver;

DKHeap::DKHeap() 
	{}

DKHeap::~DKHeap() 
	{KHeapDriver = NULL;}

TInt DKHeap::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
	{return KErrNone;}

/**User side request entry point.*/
TInt DKHeap::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt r = KErrNone;
	switch (aFunction)
	{
		case RKHeapDevice::ESetThreadPriorityHigh:
			{
			NKern::ThreadEnterCS();
			Kern::SetThreadPriority(47);
			NKern::ThreadLeaveCS();
			}
			break;

		case RKHeapDevice::ECreateSharedChunk:
			{
			TChunkCreateInfo info;
		    info.iType         = TChunkCreateInfo::ESharedKernelSingle;
			info.iMaxSize      = 0x40000;
		#ifdef __EPOC32__
			info.iMapAttr      = EMapAttrSupRw | EMapAttrCachedWBWA | EMapAttrL2CachedWBWA;
		#endif
			info.iOwnsMemory   = ETrue; // Use memory from system's free pool
			info.iDestroyedDfc = NULL;

		    TLinAddr chunkAddr;
		    TUint32 mapAttr;
		    DChunk* chunk;

			NKern::ThreadEnterCS();
		    if (KErrNone != (r = Kern::ChunkCreate(info, chunk, chunkAddr, mapAttr)))
				{
				NKern::ThreadLeaveCS();
				break;
				}
			Kern::ChunkClose(chunk);
			NKern::ThreadLeaveCS();
			}
			break;

		#ifdef __EPOC32__
		case RKHeapDevice::ECreatHwChunk:
			{
			const TInt KSize = 4*1024;
			TPhysAddr physbase;

			NKern::ThreadEnterCS();
			r = Epoc::AllocPhysicalRam(KSize, physbase);
			if (r)	{NKern::ThreadLeaveCS();break;}
			
			DPlatChunkHw* hwchunk;
			r = DPlatChunkHw::New(hwchunk, physbase, KSize, EMapAttrSupRw);
			if (r==KErrNone)	hwchunk->Close(NULL);
			
			Epoc::FreePhysicalRam(physbase, KSize);
			NKern::ThreadLeaveCS();
			}
			break;
		#endif//__EPOC32__

		case RKHeapDevice::ETestBurstFailNext:
			r = TestBurstFailNext((TUint)a1, (TUint)a2);
			break;

		case RKHeapDevice::ETestBurstDeterministic:
			r = TestBurstDeterministic((TUint)a1, (TUint)a2);
			break;

		default:
			r=KErrNotSupported;
		}
	return r;
	}



TInt DKHeap::TestBurstFailNext(TUint aCount, TUint aBurst)
	{
	NKern::ThreadEnterCS();

	TInt r = KErrNone;
	TUint i = 0;
	TInt* p = NULL;
	for (; i < aCount; i++)
		{
		if (i < aCount - 1)
			{
			p = new TInt;
			if (p == NULL)
				{// Shouldn't have failed
				r = KErrNoMemory;
				goto exit;
				}
			delete p;
			}
		else 
			{
			for (TUint j = 0; j < aBurst; j++)
				{
				p = new TInt;
				if (p != NULL)
					{// Should be failing
					delete p;
					r = KErrGeneral;
					goto exit;
					}
				}
			} 
		}
exit:
	NKern::ThreadLeaveCS();
	return r;
	}

TInt DKHeap::TestBurstDeterministic(TUint aRate, TUint aBurst)
	{
	NKern::ThreadEnterCS();

	TInt r = KErrNone;
	TInt* p = NULL;
	for (TUint i = 1; i <= aRate * KHeapFailCycles; i++)
		{
		if (i % aRate == 0)
			{
			for (TUint j = 0; j < aBurst; j++)
				{
				p = new TInt;
				if (p != NULL)
					{// Should have failed but didn't
					delete p;
					r = KErrGeneral;
					goto exit;
					}
				}
			}
		else
			{
			p = new TInt;
			if (p == NULL)
				{// Shouldn't have failed but did
				r = KErrNoMemory;
				goto exit;
				}
			delete p;
			}
		}
exit:
	NKern::ThreadLeaveCS();
	return r;
	}

//////////////////////////////////////////
class DTestFactory : public DLogicalDevice
	{
public:
	DTestFactory();
	// from DLogicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

DTestFactory::DTestFactory()
    {
    iParseMask = KDeviceAllowUnit;
    iUnitsMask = 0x3;
    }

TInt DTestFactory::Create(DLogicalChannelBase*& aChannel)
    {
	KHeapDriver = new DKHeap;
	aChannel = KHeapDriver;
	return (aChannel ? KErrNone : KErrNoMemory);
    }

TInt DTestFactory::Install()
    {return SetName(&KHeapTestDriverName);}

void DTestFactory::GetCaps(TDes8& /*aDes*/) const	
	{}

DECLARE_STANDARD_LDD()
	{return new DTestFactory;}
