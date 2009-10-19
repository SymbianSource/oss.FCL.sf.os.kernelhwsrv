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
// e32/drivers/paging/emulated/emulated_rom_paging.cpp
// 
//

#include <kernel/kern_priv.h>
#include <kernel/kernel.h>
#include <memmodel/epoc/platform.h>

class DEmulatedRomPagingDevice : public DPagingDevice
	{
public:
	static TInt Install();
private:
	TInt Construct();
	TInt Read(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt);
	static void ReadTimerCallback(TAny* aSem);
	static void ReadDfcCallback(TAny* aReq);
	void ReadDfc();
private:
	TLinAddr iRomStore;

	struct TReadRequest
		{
		TLinAddr iBuffer;
		TLinAddr iOffset;
		TUint	 iSize;
		NFastSemaphore* iSemaphore;
		TInt iResult;
		};

	TReadRequest iReadRequest;
	TDfcQue* iDfcQue;
	TInt iAccumulatedDelay;
	TInt iReadPageDelay;
	TInt iReadPageCPUDelay;
	DMutex* iMutex;
	};


TInt DEmulatedRomPagingDevice::Install()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DEmulatedRomPagingDevice::Install"));
	TInt r;
	DEmulatedRomPagingDevice* romDevice = new DEmulatedRomPagingDevice;
	if(!romDevice)
		r = KErrNoMemory;
	else
		{
		r = romDevice->Construct();
		if(r==KErrNone)
			Kern::InstallPagingDevice(romDevice);
		else
			delete romDevice;
		}
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<DEmulatedRomPagingDevice::Install returns %d",r));
	return r;
	}


TInt DEmulatedRomPagingDevice::Construct()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DEmulatedRomPagingDevice::Construct"));

	// Initialise DPagingDevice base class
	iType = ERom;
	iReadUnitShift = 9; // 512 byte units
	iName = "EmulatedRomPagingDevice";

	// Get info about ROM
	TPhysAddr* romPages;
	TInt numRomPages;
	TInt r=Kern::HalFunction(EHalGroupVM,EVMHalGetOriginalRomPages,&romPages,&numRomPages);
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("DEmulatedRomPagingDevice::Construct numRomPages=%08x",numRomPages));
	__NK_ASSERT_ALWAYS(r==KErrNone);

	r = Kern::MutexCreate(iMutex, _L("EmulRomPageDev"), KMutexOrdNone);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	// Create a shared chunk to map the ROM pages
	TInt pageSize = Kern::RoundToPageSize(1);
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelSingle;
	info.iMaxSize = numRomPages*pageSize;
	info.iMapAttr = EMapAttrFullyBlocking;
	info.iOwnsMemory = EFalse;
	DChunk* chunk;
	TUint32 mapAttr;
	r = Kern::ChunkCreate(info,chunk,iRomStore,mapAttr);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	r = Kern::ChunkCommitPhysical(chunk,0,numRomPages*pageSize,romPages);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// create DFC thread for NAND read simulation
	_LIT8(KDemandPagingDelay,"DemandPagingDelay");
	r = Kern::DfcQCreate(iDfcQue,24,&KDemandPagingDelay); // DFC thread of same priority as NAND driver
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// setup delays used for simulation
	SDemandPagingConfig config = Epoc::RomHeader().iDemandPagingConfig;
	iReadPageDelay = config.iSpare[0];
	iReadPageCPUDelay = config.iSpare[1];
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("DEmulatedRomPagingDevice::Construct emulated delays=%d,%d",iReadPageDelay,iReadPageCPUDelay));

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<DEmulatedRomPagingDevice::Construct"));
	return KErrNone;
	}


TInt DEmulatedRomPagingDevice::Read(TThreadMessage* /*aReq*/,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt)
	{
	aOffset <<= iReadUnitShift;
	aSize <<= iReadUnitShift;

	if(iReadPageCPUDelay==0 || KDebugNum(KTESTFAST))
		{
		// don't do emulated NAND delay, just copy it immediately...
		memcpy((TAny*)aBuffer,(TAny*)(iRomStore+aOffset),aSize);
		return KErrNone;
		}
	
	// make sure we are single threaded when we use the DFC.
	Kern::MutexWait(*iMutex);
	
	// get a DFC to do the simulated read
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,NULL);
	iReadRequest.iBuffer = aBuffer;
	iReadRequest.iOffset = aOffset;
	iReadRequest.iSize = aSize;
	iReadRequest.iSemaphore = &sem;
	TDfc dfc(ReadDfcCallback,this,iDfcQue,0);
	dfc.Enque();
	NKern::FSWait(&sem);
	TInt result = iReadRequest.iResult;
	
	// let any other threads have a go.
	Kern::MutexSignal(*iMutex);
	return result;
	}


void DEmulatedRomPagingDevice::ReadTimerCallback(TAny* aSem)
	{
	NKern::FSSignal((NFastSemaphore*)aSem);
	}


void DEmulatedRomPagingDevice::ReadDfcCallback(TAny* aSelf)
	{
	((DEmulatedRomPagingDevice*)aSelf)->ReadDfc();
	}


void DEmulatedRomPagingDevice::ReadDfc()
	{
	// calculate number of ticks to stall to emulate elapsed time for page read request
	iAccumulatedDelay += iReadPageDelay;
	TInt tick = NKern::TickPeriod();
	TInt delay = 0;
	if(iAccumulatedDelay>tick)
		{
		delay = iAccumulatedDelay/tick;
		iAccumulatedDelay -= delay*tick+(tick>>1);
		}
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,NULL);
	NTimer timer(ReadTimerCallback,&sem);
	if(delay)
		timer.OneShot(delay,ETrue);

	// emulate CPU load for processing read
	Kern::NanoWait(iReadPageCPUDelay*1000);

	// get data using memcpy
	memcpy((TAny*)iReadRequest.iBuffer,(TAny*)(iRomStore+iReadRequest.iOffset),iReadRequest.iSize);

	// wait for delay timer
	if(delay)
		NKern::FSWait(&sem);

	// signal done
	iReadRequest.iResult = KErrNone;
	NKern::FSSignal(iReadRequest.iSemaphore);
	}


DECLARE_STANDARD_EXTENSION()
	{
	return DEmulatedRomPagingDevice::Install();
	}
