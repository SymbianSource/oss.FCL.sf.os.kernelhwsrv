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
// e32test\demandpaging\d_pagestrss.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/cache.h>
#include "d_pagestress.h"

//
// Class definitions
//

class DPageStressTestFactory : public DLogicalDevice
	{
public:
	~DPageStressTestFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DPageStressTestChannel : public DLogicalChannelBase
	{
public:
	DPageStressTestChannel();
	~DPageStressTestChannel();
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);

	TInt DoConsumeRamSetup(TInt aNumPagesLeft, TInt aPagesInBlock);
	TInt DoConsumeRamFinish(void);
	TInt DoConsumeSomeRam(TInt aBlocks);
	TInt DoReleaseSomeRam(TInt aBlocks);
	TInt FreeRam();
	TInt DoSetDebugFlag(TInt aState);
public:
	DPageStressTestFactory*	iFactory;
	
private:
	TBool		 iRamAllocd;
	TInt		 iInitialFreeRam;
	TInt		 iTotalBlocks;
	TInt		 iBlockSize;
	TInt		 iLastBlockAllocd;
	TPhysAddr*	 iAddrArray;
	TInt		 iDebug;
	DMutex*      iMutex;
	TInt		 iThreadCounter;
	TPhysAddr	 iAddrMin;
	TPhysAddr	 iAddrMax;
	TBool		 iDemandPaging;
	};

//
// DPageStressTestFactory
//

TInt DPageStressTestFactory::Install()
	{
	return SetName(&KPageStressTestLddName);
	}

DPageStressTestFactory::~DPageStressTestFactory()
	{
	}

void DPageStressTestFactory::GetCaps(TDes8& /*aDes*/) const
	{
	// Not used but required as DLogicalDevice::GetCaps is pure virtual
	}

TInt DPageStressTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel = NULL;
	DPageStressTestChannel* channel=new DPageStressTestChannel;
	if(!channel)
		return KErrNoMemory;
	channel->iFactory = this;
	aChannel = channel;
	return KErrNone;
	}

DECLARE_STANDARD_LDD()
	{
	return new DPageStressTestFactory;
	}

//
// DPageStressTestChannel
//

TInt DPageStressTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
	return KErrNone;
	}

DPageStressTestChannel::DPageStressTestChannel()
	: iRamAllocd(EFalse), iInitialFreeRam(0), iTotalBlocks(0), iBlockSize(0), iLastBlockAllocd(0), 
	iAddrArray(NULL), iDebug(0), iThreadCounter(1), iAddrMin(0), iAddrMax(0), iDemandPaging(ETrue)
	{
	_LIT(KMutexName,"TPageStressMutex");
	NKern::ThreadEnterCS();
	Kern::MutexCreate(iMutex, KMutexName,KMutexOrdNone);
	NKern::ThreadLeaveCS();
	SVMCacheInfo tempPages;
	if (Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) != KErrNone)
		{
		iDemandPaging = EFalse;
		}
	}

DPageStressTestChannel::~DPageStressTestChannel()
	{
	if (iRamAllocd)
		{
		DoConsumeRamFinish();
		}
	if (iMutex)
        {
		iMutex->Close(NULL);
		iMutex = NULL;
		}
	}

TInt DPageStressTestChannel::Request(TInt aFunction, TAny* a1, TAny* a2)
	{
	TInt threadCount = __e32_atomic_tas_ord32(&iThreadCounter, 1, 1, 0);
	if (threadCount >= 2)
		{
		Kern::Printf("DPageStressTestChannel::Request threadCount = %d\n", threadCount);
		}
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iMutex);
	TInt retVal = KErrNotSupported;
	switch(aFunction)
		{
		case RPageStressTestLdd::EDoConsumeRamSetup:
			{
			retVal = DPageStressTestChannel::DoConsumeRamSetup((TInt)a1, (TInt)a2);
			}
		break;

		case RPageStressTestLdd::EDoConsumeRamFinish:
			{
			retVal = DPageStressTestChannel::DoConsumeRamFinish();
			}
		break;

		case RPageStressTestLdd::EDoConsumeSomeRam:
			{
			retVal = DPageStressTestChannel::DoConsumeSomeRam((TInt)a1);
			}
		break;

		case RPageStressTestLdd::EDoReleaseSomeRam:
			{
			retVal = DPageStressTestChannel::DoReleaseSomeRam((TInt)a1);
			}
		break;
		
		case RPageStressTestLdd::EDoSetDebugFlag:
			{
			retVal = DoSetDebugFlag((TInt) a1);
			}
		break;
		
		default: break;
		}
	Kern::MutexSignal(*iMutex);
	NKern::ThreadLeaveCS();
	__e32_atomic_tas_ord32(&iThreadCounter, 1, -1, 0);
	return retVal;
	}

// 
// DPageStressTestChannel::DoConsumeRamSetup
//
// This test attempts to consume most of the available Contiguous Ram until we need to ask the 
// demand paging code to release memory for it.
// 
// 
//
#define CHECK(c) { if(!(c)) { Kern::Printf("Fail  %d", __LINE__); ; retVal = __LINE__;} }

TInt DPageStressTestChannel::DoConsumeRamSetup(TInt aNumPagesLeft, TInt aPagesInBlock)
	{
	if (iRamAllocd)
		{
		Kern::Printf("DPageStressTestChannel trying to start again when RAM alloc'd\n");
		DoConsumeRamFinish();
		}

	TInt retVal = KErrNone;

	TInt pageSize = 0;
	CHECK(Kern::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&pageSize,0)==KErrNone);
	
	iBlockSize = aPagesInBlock * pageSize;
	iInitialFreeRam = FreeRam();
	iTotalBlocks = (iInitialFreeRam/iBlockSize) + 10;
	
	iAddrArray = (TPhysAddr *)Kern::AllocZ(sizeof(TPhysAddr) * iTotalBlocks);

	CHECK(iAddrArray);
	if(!iAddrArray)
		{
		return KErrNoMemory;
		}
	
	SVMCacheInfo tempPages;

	iRamAllocd = ETrue;

	// get the initial free ram again as the heap may have grabbed a page during the alloc
	iInitialFreeRam = FreeRam();


	if (iDemandPaging)
		{
		CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
		if (iDebug)
			{
			Kern::Printf("Start : min %d max %d current %d maxFree %d freeRam %d",
						 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize, FreeRam());
			}
		}
	// allocate blocks to use up RAM until we fail to allocate any further...
	TInt index;
	// only alloc upto the point where we have a number of pages left.
	iLastBlockAllocd = iTotalBlocks - (aNumPagesLeft / aPagesInBlock);
	for (index = 0; index < iLastBlockAllocd; index ++)
		{
		if (iDemandPaging)
			{
			CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
			}

		if (KErrNone != Epoc::AllocPhysicalRam(iBlockSize, iAddrArray[index], 0))
			{
			iAddrArray[index] = NULL;
			break;
			}

		if ((iAddrMin == 0) || (iAddrMin > iAddrArray[index]))
			{
			iAddrMin = iAddrArray[index];
			}
		
		if (iAddrMax < iAddrArray[index])
			{
			iAddrMax = iAddrArray[index];
			}

		if (iDemandPaging)
			{
			CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
			}
		}
	iLastBlockAllocd = index - 1;

	if (iDemandPaging)
		{
		CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
		if (iDebug)
			{
			Kern::Printf("Alloc'd : min %d max %d current %d maxFree %d freeRam %d lastIndex %d addrMin 0x%08x addrMax 0x%08x",
						 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize, FreeRam(), iLastBlockAllocd, iAddrMin, iAddrMax);
			}
		}
	return retVal;
	}

TInt DPageStressTestChannel::DoConsumeRamFinish(void)
	{
	TInt retVal = KErrNone;
	if (iRamAllocd)
		{
		SVMCacheInfo tempPages;

		if (iDemandPaging)
			{
			CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
			if (iDebug)
				{
				Kern::Printf("Cleanup cache info: iMinSize %d iMaxSize %d iCurrentSize %d iMaxFreeSize %d",
							 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize);
				}
			}
		TInt index = iTotalBlocks - 1;

		TBool firstTime = ETrue;

		// free the memory we allocated...
		while(index >= 0)
			{
			if (iAddrArray[index])
				{
				if ((iAddrArray[index] < iAddrMin) || (iAddrArray[index] > iAddrMax))
					{
					Kern::Printf("ERROR: DoConsumeRamFinish : index %d addr 0x%08x min 0x%08x max 0x%08x\n     : firstTime %d iLastBlockAllocd %d iTotalBlock %d", index, iAddrArray[index], iAddrMin, iAddrMax, firstTime, iLastBlockAllocd, iTotalBlocks);
					TInt tempIndex = index - 8;
					while (tempIndex < (index + 8))
						{
						Kern::Printf("      --> index %d addr 0x%08x", tempIndex, iAddrArray[tempIndex]);
						tempIndex ++;
						}
					
					iAddrArray[index] = NULL;
					}
				else
					{
					TInt r = Epoc::FreePhysicalRam(iAddrArray[index], iBlockSize);
					iAddrArray[index] = NULL;
					CHECK(r==KErrNone);
					}
				}

			firstTime = EFalse;
			--index;
			}

		if (iDebug)
			{
			Kern::Printf("DoConsumeRamFinish : FreeRam Initial 0x%x now 0x%x",iInitialFreeRam, FreeRam());
			}
		//CHECK(FreeRam() == iInitialFreeRam)
		if (FreeRam() != iInitialFreeRam)
			{
			Kern::Printf("DoConsumeRamFinish : FreeRam Initial 0x%x now 0x%x NOT EQUAL!",iInitialFreeRam, FreeRam());
			}

		Kern::Free(iAddrArray);
		iAddrArray = NULL;
		iAddrMin = 0;
		iAddrMax = 0;
		iLastBlockAllocd = -1;
		iTotalBlocks = 0;
		iRamAllocd = EFalse;

		if (iDemandPaging)
			{
			CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
			if (iDebug)
				{
				Kern::Printf("End cache info: iMinSize %d iMaxSize %d iCurrentSize %d iMaxFreeSize %d",
							 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize);
				}
			}
		}	
	return retVal;
	}

TInt DPageStressTestChannel::DoConsumeSomeRam(TInt aBlocks)
	{
	TInt retVal = KErrNone;

	return retVal;
	}

TInt DPageStressTestChannel::DoReleaseSomeRam(TInt aBlocks)
	{
	if(iLastBlockAllocd<0)
		return KErrUnderflow;

	TInt retVal = KErrNone;
	if (iRamAllocd)
		{
		SVMCacheInfo tempPages;

		if (iDemandPaging)
			{
			CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
			if (iDebug)
				{
				Kern::Printf("Release  : min %d max %d current %d maxFree %d freeRam %d lastIndex %d",
							 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize, FreeRam(), iLastBlockAllocd);
				}
			}
		
		TInt index = iLastBlockAllocd;
		iLastBlockAllocd -= aBlocks;
		
		TBool firstTime = ETrue;

		// free the memory we allocated...
		while((index >= 0) && (index > iLastBlockAllocd))
			{
			if (iAddrArray[index])
				{
				//Kern::Printf("DoReleaseSomeRam : index %d addr 0x%08x size %d", index, iAddrArray[index], iBlockSize);
				if ((iAddrArray[index] < iAddrMin) || (iAddrArray[index] > iAddrMax))
					{
					Kern::Printf("ERROR: DoReleaseSomeRam : index %d addr 0x%08x min 0x%08x max 0x%08x\n     : firstTime %d iLastBlockAllocd %d iTotalBlock %d", index, iAddrArray[index], iAddrMin, iAddrMax, firstTime, iLastBlockAllocd + aBlocks, iTotalBlocks);
					TInt tempIndex = index - 8;
					while (tempIndex < (index + 8))
						{
						Kern::Printf("      --> index %d addr 0x%08x", tempIndex, iAddrArray[tempIndex]);
						tempIndex ++;
						}

					iAddrArray[index] = NULL;
					}
				else
					{
					TInt r = Epoc::FreePhysicalRam(iAddrArray[index], iBlockSize);
					iAddrArray[index] = NULL;
					CHECK(r==KErrNone);
					}
				}
			else
				{
				Kern::Printf("ERROR: DoReleaseSomeRam : trying to free NULL index %d", index, iAddrArray[index], iAddrMin, iAddrMax);
				TInt tempIndex = index - 8;
				while (tempIndex < (index + 8))
					{
					Kern::Printf("      --> index %d addr 0x%08x", tempIndex, iAddrArray[tempIndex]);
					tempIndex ++;
					}

				}
			firstTime = EFalse;
			--index;
			}
		if (index <= 0)
			{
			Kern::Printf("WARNING : DoReleaseSomeRam : index %d !!!!!", index);
			}

		if (iDemandPaging)
			{
			CHECK(Kern::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0) == KErrNone);
			if (iDebug)
				{
				Kern::Printf("Released : min %d max %d current %d maxFree %d freeRam %d lastIndex %d",
							 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize, FreeRam(), iLastBlockAllocd);
				}	
			}
		}
	return retVal;
	}


TInt DPageStressTestChannel::FreeRam()
	{
	return Kern::FreeRamInBytes();
	}

TInt DPageStressTestChannel::DoSetDebugFlag(TInt aState)
	{
	iDebug = aState;
	return KErrNone;
	}
