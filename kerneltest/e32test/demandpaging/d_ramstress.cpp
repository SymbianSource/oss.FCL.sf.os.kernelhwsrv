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
// e32test\demandpaging\d_ramsterss.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>

#include "t_ramstress.h"

//
// Class definitions
//

class DRamStressTestFactory : public DLogicalDevice
	{
public:
	~DRamStressTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DRamStressTestChannel : public DLogicalChannelBase
	{
public:
	DRamStressTestChannel();
	~DRamStressTestChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	TInt FreeRam();
	TInt DoSetDebugFlag(TInt aState);
	TInt DoSetEndFlag(TInt aState);
public:
	DRamStressTestFactory*	iFactory;
	
private:
	TInt DoMovePagesInZone(TUint aZoneIndex);
	TInt DoPageMove(TLinAddr aAddr);
	TInt DoAllocPagesInZone(TUint aZoneIndex);

private:
	TInt		 iDebug;
	TInt		 iFinish;
	TInt		 iThreadCounter;
	TInt		 iPageSize;
	};

//
// DRamStressTestFactory
//

TInt DRamStressTestFactory::Install()
	{
	return SetName(&KRamStressTestLddName);
	}

DRamStressTestFactory::~DRamStressTestFactory()
	{
	}

void DRamStressTestFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DRamStressTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DRamStressTestChannel* channel=new DRamStressTestChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DRamStressTestFactory;
	}

//
// DRamStressTestChannel
//

TInt DRamStressTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

//
// DRamStressTestChannel::DRamStressTestChannel()
//

DRamStressTestChannel::DRamStressTestChannel()
	: iDebug(0), iFinish(0), iThreadCounter(1), iPageSize(0)
	{
	TInt pageSize = 0;
	if (Kern::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0) == KErrNone)
		{
		iPageSize = pageSize;
		}
	}

//
// DRamStressTestChannel::~DRamStressTestChannel()
//

DRamStressTestChannel::~DRamStressTestChannel()
	{
	}

//
// DRamStressTestChannel::Request
//
TInt DRamStressTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt threadCount = __e32_atomic_tas_ord32(&iThreadCounter, 1, 1, 0);
	if (threadCount >= 2)
		if (iDebug)
			Kern::Printf("DRamStressTestChannel::Request threadCount = %d\n", threadCount);
	NKern::ThreadEnterCS();
	TInt retVal = KErrNotSupported;
	switch(aFunction)
		{
		case RRamStressTestLdd::EDoMovePagesInZone:
			{
			retVal = DoMovePagesInZone((TUint)a1);
			}
		break;
	
		case RRamStressTestLdd::EDoSetDebugFlag:
			{
			retVal = DoSetDebugFlag((TInt) a1);
			}
		break;

		case RRamStressTestLdd::EDoSetEndFlag:
			{
			retVal = DoSetEndFlag((TInt) a1);
			}
		break;
		
		default: break;
		}
	NKern::ThreadLeaveCS();
	__e32_atomic_tas_ord32(&iThreadCounter, 1, -1, 0);
	return retVal;
	}

TInt DRamStressTestChannel::DoAllocPagesInZone(TUint aZoneIndex)
	{
	TInt retVal = KErrNone;



	return retVal;
	}

//
// DRamStressTestChannel::DoMovePagesInZone
//

TInt DRamStressTestChannel::DoMovePagesInZone(TUint aZoneIndex)
	{
	if (iDebug)
		{
		Kern::Printf("DRamStressTestChannel::DoMovePagesInZone(%d)", aZoneIndex);
		}

	// first get info on the 2 zones....
	struct SRamZoneConfig	zoneConfig;
	TInt retVal;
	if (KErrNone != Kern::HalFunction(EHalGroupRam,ERamHalGetZoneConfig,(TAny*)aZoneIndex, (TAny*)&zoneConfig))
		{
		Kern::Printf("Error ::: DoMovePagesInZone : bad zone %d", aZoneIndex); 
		retVal = KErrArgument;
		}
	else
		{
		//Kern::Printf("DoMovePagesInZone : zone %d", aZoneIndex); 
		TPhysAddr addr = zoneConfig.iPhysBase;
		TUint moved = 0;
		TUint failed = 0;
		TUint unused = 0;
		TUint bad = 0;
		TUint nosupport = 0;
		retVal = KErrNone;
		while (addr < zoneConfig.iPhysEnd)
			{
			//Kern::Printf("DoMovePagesInZone(%d) moving 0x%08x", aZoneIndex, addr);
			retVal = DoPageMove(addr);
			if (retVal == KErrNone)
				{
				moved ++;
				}
			else
				{
				switch (retVal)
					{
					case KErrArgument:
						bad ++;		
					break;

					case KErrNotFound:
						unused ++;	
					break;

					case KErrNotSupported:
						nosupport ++;
					break;

					case KErrGeneral:
					default:
						failed ++;
						if (iDebug)
							{
							Kern::Printf("DRamStressTestChannel::DoMovePagesInZone(%d) failed 0x%08x err=%d", aZoneIndex, addr, retVal);
							}
					break;
					}
				}
			addr += iPageSize;
			NKern::Sleep(1);
			if (iFinish)
				{
				Kern::Printf("DRamStressTestChannel::DoMovePagesInZone(%d) Finishing", aZoneIndex);
				break;
				}
			}
		if (iDebug)
			{
			Kern::Printf("DRamStressTestChannel::DoMovePagesInZone(%d) moved %u failed %u bad %u unused %u nosupport %u", aZoneIndex, moved, failed, bad, unused, nosupport);
			}
		retVal = (failed==0) ? KErrNone : KErrGeneral;
		}
	return retVal;
	}

//
// DRamStressTestChannel::DoPageMove
//

TInt DRamStressTestChannel::DoPageMove(TPhysAddr aAddr)
	{
	aAddr = _ALIGN_DOWN(aAddr, iPageSize);

	TInt r;
	TPhysAddr aNew;
	r=Epoc::MovePhysicalPage(aAddr, aNew);
	if ((r==KErrNone) && (aNew == aAddr))
		{
		Kern::Printf("DRamStressTestChannel::DoPageMove moved bad 0x%08x now 0x%08x ", aAddr, aNew);
		r=KErrGeneral;
		}
	return r;
	}


TInt DRamStressTestChannel::FreeRam()
	{
	return Kern::FreeRamInBytes();
	}

TInt DRamStressTestChannel::DoSetDebugFlag(TInt aState)
	{
	iDebug = aState;
	return KErrNone;
	}

TInt DRamStressTestChannel::DoSetEndFlag(TInt aState)
	{
	iFinish = aState;
	return KErrNone;
	}
