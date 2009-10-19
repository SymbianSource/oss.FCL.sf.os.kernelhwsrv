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

class DEmulatedDataPagingDevice : public DPagingDevice
	{
public:
	static TInt Install();
private:
	TInt Construct();
	TInt Read(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt);
	TInt Write(TThreadMessage* aReq,TLinAddr aBuffer,TUint aOffset,TUint aSize,TBool aBackground);

	static void ReadTimerCallback(TAny* aSem);
	static void ReadDfcCallback(TAny* aReq);
	void ReadDfc();

	static void WriteTimerCallback(TAny* aSem);
	static void WriteDfcCallback(TAny* aReq);
	void WriteDfc();
private:
	TLinAddr iDataStore;
	TDfcQue* iDfcQue;
	DMutex* iMutex;

	struct TDataRequest
		{
		TLinAddr iBuffer;
		TLinAddr iOffset;
		TUint	 iSize;
		NFastSemaphore* iSemaphore;
		TInt iResult;
		};

	TDataRequest iReadRequest;
	TInt iAccumulatedReadDelay;
	TInt iReadPageDelay;
	TInt iReadPageCPUDelay;

	TDataRequest iWriteRequest;
	TInt iAccumulatedWriteDelay;
	TInt iWritePageDelay;
	TInt iWritePageCPUDelay;
	};


TInt DEmulatedDataPagingDevice::Install()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DEmulatedDataPagingDevice::Install"));
	TInt r;
	DEmulatedDataPagingDevice* dataDevice = new DEmulatedDataPagingDevice;
	if(!dataDevice)
		r = KErrNoMemory;
	else
		{
		r = dataDevice->Construct();
		if(r==KErrNone)
			Kern::InstallPagingDevice(dataDevice);
		else
			delete dataDevice;
		}
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<DEmulatedDataPagingDevice::Install returns %d",r));
	return r;
	}


TInt DEmulatedDataPagingDevice::Construct()
	{
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf(">DEmulatedDataPagingDevice::Construct"));

	// Initialise DPagingDevice base class
	iType = EData;
	iReadUnitShift = 9; // 512 byte units
	iName = "EmulatedDataPagingDevice";

	// Calculate swap size
	TInt free = Kern::FreeRamInBytes()>>20; // megabytes free
	TInt swapSize = free/2;
	if(swapSize>256)
		swapSize = 256;
	swapSize <<= 20;
	iSwapSize = swapSize>>iReadUnitShift;
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("DEmulatedDataPagingDevice::Construct swap size 0x%x",swapSize));

	TInt r = Kern::MutexCreate(iMutex, _L("EmulDataPageDev"), KMutexOrdNone);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	// Create a shared chunk to map the ROM pages
	TChunkCreateInfo info;
	info.iType = TChunkCreateInfo::ESharedKernelSingle;
	info.iMaxSize = swapSize;
	info.iMapAttr = EMapAttrFullyBlocking;
	info.iOwnsMemory = ETrue;
	DChunk* chunk;
	TUint32 mapAttr;
	r = Kern::ChunkCreate(info,chunk,iDataStore,mapAttr);
	__NK_ASSERT_ALWAYS(r==KErrNone);
	r = Kern::ChunkCommit(chunk,0,swapSize);
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// create DFC thread for NAND read simulation
	_LIT8(KDemandPagingDelay,"DataPagingDelay");
	r = Kern::DfcQCreate(iDfcQue,24,&KDemandPagingDelay); // DFC thread of same priority as NAND driver
	__NK_ASSERT_ALWAYS(r==KErrNone);

	// setup delays used for simulation
	SDemandPagingConfig config = Epoc::RomHeader().iDemandPagingConfig;
	iReadPageDelay = config.iSpare[0];
	iReadPageCPUDelay = config.iSpare[1];
	iWritePageDelay = config.iSpare[0]*2;
	iWritePageCPUDelay = config.iSpare[1];
	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("DEmulatedDataPagingDevice::Construct emulated delays=%d,%d",iReadPageDelay,iReadPageCPUDelay));

	__KTRACE_OPT2(KPAGING,KBOOT,Kern::Printf("<DEmulatedDataPagingDevice::Construct"));
	return KErrNone;
	}


TInt DEmulatedDataPagingDevice::Read(TThreadMessage* /*aReq*/,TLinAddr aBuffer,TUint aOffset,TUint aSize,TInt)
	{
	aOffset <<= iReadUnitShift;
	aSize <<= iReadUnitShift;

	if(iReadPageCPUDelay==0 || KDebugNum(KTESTFAST))
		{
		// don't do emulated NAND delay, just copy it immediately...
		memcpy((TAny*)aBuffer,(TAny*)(iDataStore+aOffset),aSize);
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


void DEmulatedDataPagingDevice::ReadTimerCallback(TAny* aSem)
	{
	NKern::FSSignal((NFastSemaphore*)aSem);
	}


void DEmulatedDataPagingDevice::ReadDfcCallback(TAny* aSelf)
	{
	((DEmulatedDataPagingDevice*)aSelf)->ReadDfc();
	}


void DEmulatedDataPagingDevice::ReadDfc()
	{
	// calculate number of ticks to stall to emulate elapsed time for page read request
	iAccumulatedReadDelay += iReadPageDelay;
	TInt tick = NKern::TickPeriod();
	TInt delay = 0;
	if(iAccumulatedReadDelay>tick)
		{
		delay = iAccumulatedReadDelay/tick;
		iAccumulatedReadDelay -= delay*tick+(tick>>1);
		}
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,NULL);
	NTimer timer(ReadTimerCallback,&sem);
	if(delay)
		timer.OneShot(delay,ETrue);

	// emulate CPU load for processing read
	Kern::NanoWait(iReadPageCPUDelay*1000);

	// get data using memcpy
	memcpy((TAny*)iReadRequest.iBuffer,(TAny*)(iDataStore+iReadRequest.iOffset),iReadRequest.iSize);

	// wait for delay timer
	if(delay)
		NKern::FSWait(&sem);

	// signal done
	iReadRequest.iResult = KErrNone;
	NKern::FSSignal(iReadRequest.iSemaphore);
	}


TInt DEmulatedDataPagingDevice::Write(TThreadMessage* /*aReq*/,TLinAddr aBuffer,TUint aOffset,TUint aSize,TBool aBackground)
	{
	aOffset <<= iReadUnitShift;
	aSize <<= iReadUnitShift;

	if(iWritePageCPUDelay==0 || KDebugNum(KTESTFAST))
		{
		// don't do emulated NAND delay, just copy it immediately...
		memcpy((TAny*)(iDataStore+aOffset),(TAny*)aBuffer,aSize);
		return KErrNone;
		}
	
	// make sure we are single threaded when we use the DFC.
	Kern::MutexWait(*iMutex);
	
	// get a DFC to do the simulated write
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,NULL);
	iWriteRequest.iBuffer = aBuffer;
	iWriteRequest.iOffset = aOffset;
	iWriteRequest.iSize = aSize;
	iWriteRequest.iSemaphore = &sem;
	TDfc dfc(WriteDfcCallback,this,iDfcQue,0);
	dfc.Enque();
	NKern::FSWait(&sem);
	TInt result = iWriteRequest.iResult;
	
	// let any other threads have a go.
	Kern::MutexSignal(*iMutex);
	return result;
	}


void DEmulatedDataPagingDevice::WriteTimerCallback(TAny* aSem)
	{
	NKern::FSSignal((NFastSemaphore*)aSem);
	}


void DEmulatedDataPagingDevice::WriteDfcCallback(TAny* aSelf)
	{
	((DEmulatedDataPagingDevice*)aSelf)->WriteDfc();
	}


void DEmulatedDataPagingDevice::WriteDfc()
	{
	// calculate number of ticks to stall to emulate elapsed time for page read request
	iAccumulatedWriteDelay += iWritePageDelay;
	TInt tick = NKern::TickPeriod();
	TInt delay = 0;
	if(iAccumulatedWriteDelay>tick)
		{
		delay = iAccumulatedWriteDelay/tick;
		iAccumulatedWriteDelay -= delay*tick+(tick>>1);
		}
	NFastSemaphore sem;
	NKern::FSSetOwner(&sem,NULL);
	NTimer timer(WriteTimerCallback,&sem);
	if(delay)
		timer.OneShot(delay,ETrue);

	// emulate CPU load for processing read
	Kern::NanoWait(iWritePageCPUDelay*1000);

	// write data using memcpy
	memcpy((TAny*)(iDataStore+iWriteRequest.iOffset),(TAny*)iWriteRequest.iBuffer,iWriteRequest.iSize);

	// wait for delay timer
	if(delay)
		NKern::FSWait(&sem);

	// signal done
	iWriteRequest.iResult = KErrNone;
	NKern::FSSignal(iWriteRequest.iSemaphore);
	}


DECLARE_STANDARD_EXTENSION()
	{
	return DEmulatedDataPagingDevice::Install();
	}
