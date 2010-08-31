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
// e32test\misc\d_gobble.cpp
// LDD for gobbling RAM
// 
//

#include "platform.h"
#include <kernel/kern_priv.h>
#include "d_gobble.h"

const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=1;
const TInt KBuildVersionNumber=1;

class DGobblerFactory : public DLogicalDevice
//
// Gobbler LDD factory
//
	{
public:
	DGobblerFactory();
	~DGobblerFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DGobbler : public DLogicalChannelBase
//
// RAM Gobbler LDD channel
//
	{
public:
	DGobbler();
	virtual ~DGobbler();
private:
	virtual TInt Request(TInt aFunc, TAny* a1, TAny* a2);
	virtual TInt DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer);
private:
	enum {ESmallBufferSize = 64};

	TUint32 Gobble(TUint32 aLeave);
#ifdef __EPOC32__
	TInt GobbleMultiPages(TUint32& aTake, TUint32* aMultiPageBuf, TInt aMaxMultiPages);
	void FreeMultiPage(TPhysAddr aMultiPage);
	TUint32 AllocMultiPage(TUint32 aSize);
	TUint32 Size(TUint32 aMultiPage);
#endif
private:
#ifdef __EPOC32__
	TPhysAddr iPhys[ESmallBufferSize];
#endif
	DChunk* iSharedChunk;
	TUint32 iPageShift;
	TUint32 iPageSize;
	};

DGobblerFactory::DGobblerFactory()
    {
    iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    //iParseMask=0;//No units, no info, no PDD
    //iUnitsMask=0;//Only one thing
    }

DGobblerFactory::~DGobblerFactory()
	{
	}

TInt DGobblerFactory::Create(DLogicalChannelBase*& aChannel)
    {
	aChannel = new DGobbler;
    return aChannel ? KErrNone : KErrNoMemory;
    }

TInt DGobblerFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
    return SetName(&KGobblerLddName);
    }

void DGobblerFactory::GetCaps(TDes8& aDes) const
//
// Get capabilities - overriding pure virtual
//
    {
    TCapsGobblerV01 b;
    b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
    }

DGobbler::DGobbler()
    {
	iPageSize = Kern::RoundToPageSize(1);
	iPageShift = __e32_find_ms1_32(iPageSize);
    }

DGobbler::~DGobbler()
	{
	// Free all the RAM we've gobbled

#ifdef __EPOC32__
	// Free the addresses held in the shared chunk
	if (iSharedChunk)
		{
		TLinAddr ka;
		TInt r = Kern::ChunkAddress(iSharedChunk, 0, 1, ka);
		if (r==KErrNone)
			{
			const TUint32* p = (const TUint32*)ka;
			const TUint32* pE = p + (iSharedChunk->Size() / sizeof(TUint32));
			while (p<pE)
				{
				TUint32 mp = *p++;
				if (mp)
					FreeMultiPage(mp);
				}
			}
		}
#endif
	if (iSharedChunk)
		Kern::ChunkClose(iSharedChunk);
#ifdef __EPOC32__
	TInt i;
	for (i=0; i<ESmallBufferSize; ++i)
		{
		TUint32 mp = iPhys[i];
		if (mp)
			FreeMultiPage(mp);
		}
#endif
	}

TInt DGobbler::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create channel
//
    {

    if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
    	return KErrNotSupported;
	return KErrNone;
	}


#ifdef __EPOC32__
void DGobbler::FreeMultiPage(TPhysAddr aMultiPage)
	{
	TPhysAddr base = (aMultiPage>>iPageShift) << iPageShift;
	TUint32 size = Size(aMultiPage);
	Epoc::FreePhysicalRam(base, size);
	}

TUint32 DGobbler::AllocMultiPage(TUint32 aSize)
	{
	TUint32 sz = 1u << __e32_find_ms1_32(aSize);	// round size down to power of 2
	while (sz >= iPageSize)
		{
		TPhysAddr pa;
		TInt r = Epoc::AllocPhysicalRam(sz, pa);
		if (r == KErrNone)
			return pa | __e32_find_ms1_32(sz);
		sz >>= 1;
		}
	return 0;
	}

TUint32 DGobbler::Size(TUint32 aMultiPage)
	{
	return 1u << (aMultiPage & 0x1f);
	}
#endif

#ifdef __EPOC32__
TInt DGobbler::GobbleMultiPages(TUint32& aTake, TUint32* aMultiPageBuf, TInt aMaxMultiPages)
	{
	TInt i = 0;
	TUint32 mp = 0;
	while (i<aMaxMultiPages && aTake)
		{
		mp = AllocMultiPage(aTake);
		if (mp==0)
			break;	// someone else gobbled all the RAM
		aTake -= Size(mp);
		aMultiPageBuf[i] = mp;
		++i;
		}
	if (mp == 0)
		return KErrNoMemory;	// someone else gobbled all the RAM
	if (aTake==0)
		return KErrNone;
	return KErrOverflow;		// buffer filled up
	}
#endif

TUint32 DGobbler::Gobble(TUint32 aLeave)
	{
	TUint32 free = Kern::FreeRamInBytes();
	if (free < aLeave)
		return 0;	// no need to gobble anything
	TUint32 take = free - aLeave;
	TUint32 take2 = take;
	TInt r = KErrNone;
#ifdef __EPOC32__
	r = GobbleMultiPages(take2, iPhys, ESmallBufferSize);
	if (r==KErrNoMemory)
		return take - take2;	// someone else gobbled all the RAM
	if (r==KErrNone)
		return take;			// small buffer did the job

	TUint32 chunkMax = (take >> iPageShift) * sizeof(TPhysAddr);
#else
	TUint32 chunkMax = take;
#endif
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelSingle;
	info.iMaxSize = chunkMax;
#ifdef __EPOC32__
	info.iMapAttr = EMapAttrCachedMax;
#else
	info.iMapAttr = 0;
#endif
	info.iOwnsMemory = 1;
	info.iDestroyedDfc = 0;
	TLinAddr ka = 0;
	TUint32 ma = 0;
	r = Kern::ChunkCreate(info, iSharedChunk, ka, ma);
	if (r!=KErrNone)
		return take - take2;	// someone else gobbled all the RAM
	TUint32 chunkSz = (chunkMax + iPageSize - 1) &~ (iPageSize - 1);
	r = Kern::ChunkCommit(iSharedChunk, 0, chunkSz);
	if (r!=KErrNone)
		return take - take2;	// someone else gobbled all the RAM
#ifndef __EPOC32__
	return take;				// on emulator we are finished here
#else
	TUint32* p = (TUint32*)ka;
	memclr(p, chunkSz);
	r = GobbleMultiPages(take2, p, chunkSz/sizeof(TUint32));
	if (r==KErrNoMemory)
		return take - take2;	// someone else gobbled all the RAM
	return take; // done
#endif
	}

TInt DGobbler::Request(TInt aFunc, TAny* a1, TAny*)
	{
	if (aFunc == RGobbler::EControlGobbleRAM)
		{
		NKern::ThreadEnterCS();
		TUint32 ret = Gobble(TUint32(a1));
		NKern::ThreadLeaveCS();
		return ret;
		}
	else
		return KErrNotSupported;
	}

DECLARE_STANDARD_LDD()
	{
    return new DGobblerFactory;
    }

