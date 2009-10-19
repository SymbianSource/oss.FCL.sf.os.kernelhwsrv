// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\demandpaging\t_pagingexample.cpp
// Test device driver migration examples
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <dptest.h>
#include <e32hal.h>
#include <u32exec.h>
#include <e32svr.h>
#include <e32panic.h>
#include <e32rom.h>
#include <e32kpan.h>
#include "../mmu/t_codepaging_dll.h"
#include "d_pagingexample.h"

const TInt KBufferSize = KMaxTransferSize;
_LIT(KTCodePagingDll4, "t_codepaging_dll4.dll");

struct TTestData
	{
	TRequestStatus iStatus;
	RPagingExample::TConfigData iConfig;
	RPagingExample::TValueStruct iValue;
	TInt iIntValue1;
	TInt iIntValue2;
	TPtr8 iPtr;
	TUint8 iBuffer[KBufferSize];
public:
	TTestData() : iPtr(NULL, 0) { }
	};

RTest test(_L("T_PAGINGEXAMPLE"));
TInt PageSize = 0;
RLibrary PagedLibrary;
TTestData* UnpagedInputData = NULL;
TTestData* UnpagedOutputData = NULL;
TTestData* PagedInputData = NULL;
TTestData* PagedOutputData = NULL;

void InitInputData(TTestData* aData)
	{
	aData->iConfig.iParam1 = 2;
	aData->iConfig.iParam2 = 3;
	aData->iConfig.iParam3 = 5;
	aData->iConfig.iParam4 = 7;
	for (TInt i = 0 ; i < KBufferSize ; ++i)
		aData->iBuffer[i] = (TUint8)(i + 123);
	}

void DoTestDriver(const TDesC& aDriverName, const TTestData* aInputData, TTestData* aOutputData)
	{
	test.Start(_L("Load logical device"));
	TInt r = User::LoadLogicalDevice(aDriverName);
	test(r==KErrNone || r==KErrAlreadyExists);
	
	test.Next(_L("Open logical device"));
	RPagingExample ldd;
	test_KErrNone(ldd.Open(aDriverName));
	
	test.Next(_L("Set config"));
	DPTest::FlushCache();
	test_KErrNone(ldd.SetConfig(aInputData->iConfig));

	test.Next(_L("Get config"));
	Mem::FillZ(&aOutputData->iConfig, sizeof(RPagingExample::TConfigData));
	DPTest::FlushCache();
	test_KErrNone(ldd.GetConfig(aOutputData->iConfig));
	test_Equal(0, Mem::Compare((TUint8*)&aInputData->iConfig, sizeof(RPagingExample::TConfigData),
							   (TUint8*)&aOutputData->iConfig, sizeof(RPagingExample::TConfigData)));
	
	TRequestStatus& status = aOutputData->iStatus;

	test.Next(_L("Notify"));
	DPTest::FlushCache();
	ldd.Notify(status);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_Equal(KErrNone, status.Int());

	test.Next(_L("Async get value"));
	memclr(&aOutputData->iValue, sizeof(RPagingExample::TValueStruct));
	DPTest::FlushCache();
	ldd.AsyncGetValue(status, aOutputData->iValue);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_Equal(KErrNone, status.Int());
	test_Equal(1, aOutputData->iValue.iValue1);
	test(aOutputData->iValue.iValue2 == _L8("shrt"));

	test.Next(_L("Cancel async get value"));
	ldd.AsyncGetValue(status, aOutputData->iValue);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());

	test.Next(_L("Async get value 2"));
	aOutputData->iIntValue1 = 0;
	aOutputData->iIntValue2 = 0;
	DPTest::FlushCache();
	ldd.AsyncGetValue2(status, aOutputData->iIntValue1, aOutputData->iIntValue2);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_Equal(KErrNone, status.Int());
	test_Equal(1, aOutputData->iIntValue1);
	test_Equal(2, aOutputData->iIntValue2);

	test.Next(_L("Cancel async get value 2"));
	ldd.AsyncGetValue2(status, aOutputData->iIntValue1, aOutputData->iIntValue2);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());

	test.Next(_L("Write buffer too short"));
	ldd.Write(status, NULL, 0);
	User::WaitForRequest(status);
	test_Equal(KErrArgument, status.Int());

	test.Next(_L("Write buffer too long"));
	ldd.Write(status, NULL, KMaxTransferSize + 1);
	User::WaitForRequest(status);
	test_Equal(KErrArgument, status.Int());

	test.Next(_L("Write"));
	DPTest::FlushCache();
	ldd.Write(status, aInputData->iBuffer, KBufferSize);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	test.Next(_L("Cancel write"));
	ldd.Write(status, aInputData->iBuffer, KBufferSize);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());

	test.Next(_L("Read buffer too short"));
	ldd.Read(status, NULL, 0);
	User::WaitForRequest(status);
	test_Equal(KErrArgument, status.Int());
	
	test.Next(_L("Read buffer too long"));
	ldd.Read(status, NULL, KMaxTransferSize + 1);
	User::WaitForRequest(status);
	test_Equal(KErrArgument, status.Int());
	
	test.Next(_L("Read"));
	Mem::FillZ(aOutputData->iBuffer, KBufferSize);
	DPTest::FlushCache();
	ldd.Read(status, aOutputData->iBuffer, KBufferSize);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	test_Equal(0, Mem::Compare(aInputData->iBuffer, KBufferSize,
							   aOutputData->iBuffer, KBufferSize));

	test.Next(_L("Cancel read"));
	ldd.Read(status, aOutputData->iBuffer, KBufferSize);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());

	test.Next(_L("Cancel nothing"));
	ldd.Cancel();
	
	test.Next(_L("Write while write pending"));
	TRequestStatus status2;
	ldd.Write(status, aInputData->iBuffer, KBufferSize);
	ldd.Write(status2, aInputData->iBuffer, KBufferSize);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	User::WaitForRequest(status2);
	test_Equal(KErrInUse, status2.Int());

	test.Next(_L("Read while read pending"));
	ldd.Read(status, aOutputData->iBuffer, KBufferSize);
	ldd.Read(status2, aOutputData->iBuffer, KBufferSize);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	User::WaitForRequest(status2);
	test_Equal(KErrInUse, status2.Int());	

	test.Next(_L("Write des"));
	TPtrC8 writeDes(aInputData->iBuffer + 1, KBufferSize - 1);
	DPTest::FlushCache();
	ldd.WriteDes(status, writeDes);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	test.Next(_L("Cancel write des"));
	ldd.WriteDes(status, writeDes);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());
	
	test.Next(_L("Read des"));
	TPtr8 readDes(aOutputData->iBuffer, KBufferSize - 1);
	Mem::FillZ(aOutputData->iBuffer, KBufferSize);
	DPTest::FlushCache();
	ldd.ReadDes(status, readDes);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	test(readDes == writeDes);
		
	test.Next(_L("Read des 2"));  // has paged header but unpaged contnet, if output data is paged
	aOutputData->iPtr.Set(UnpagedOutputData->iBuffer, 0, KBufferSize - 1);
	Mem::FillZ(UnpagedOutputData->iBuffer, KBufferSize);
	DPTest::FlushCache();
	ldd.ReadDes(status, aOutputData->iPtr);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	test(aOutputData->iPtr == writeDes);
		
	test.Next(_L("Cancel read des"));
	ldd.ReadDes(status, readDes);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());
		
	test.Next(_L("Read and write at the same time"));
	ldd.Write(status, aInputData->iBuffer, KBufferSize);
	ldd.Read(status2, aOutputData->iBuffer, KBufferSize);
	DPTest::FlushCache();
	User::WaitForRequest(status);
	test_KErrNone(status.Int());
	User::WaitForRequest(status2);
	test_KErrNone(status2.Int());
		
	test.Next(_L("Cancel read and write"));
	ldd.Write(status, aInputData->iBuffer, KBufferSize);
	ldd.Read(status2, aOutputData->iBuffer, KBufferSize);
	ldd.Cancel();
	User::WaitForRequest(status);
	test_Equal(KErrCancel, status.Int());
	User::WaitForRequest(status2);
	test_Equal(KErrCancel, status2.Int());
	
	test.Next(_L("Close and free logical device"));
	ldd.Close();
	test_KErrNone(User::FreeLogicalDevice(aDriverName));
	
	test.End();
	}

void TestDriver(const TDesC& aDriverName, TBool aMigrated)
	{
	TBuf<64> string;
	string.Format(_L("Testing driver %S"), &aDriverName);
	test.Next(string);

	test.Start(_L("Test reading from unpaged memory and writing to unpaged memory"));
	DoTestDriver(aDriverName, UnpagedInputData, UnpagedOutputData);
	
	if (aMigrated && PagedInputData)
		{
		if (PagedOutputData)
			{
			test.Next(_L("Test reading from paged memory and writing to paged memory"));
			DoTestDriver(aDriverName, PagedInputData, PagedOutputData);
			}
		else
			{
			test.Next(_L("Test reading from paged memory and writing to unpaged memory"));
			DoTestDriver(aDriverName, PagedInputData, UnpagedOutputData);
			}
		}
	
	// todo: test pinning failures

	test.End();
	}

enum TTestAction
	{
	ETestRequestComplete,
	ETestRawRead,
	ETestRawWrite,
	ETestDesRead,
	ETestDesWrite
	};

enum TTestAccess
	{
	EAccessUnpaged,
	EAccessPaged
	};

enum TTestOutcome
	{
	EOutcomeSuccess,
	EOutcomeRealtimePanic
	};

TInt RealtimeTestFunc(TAny* aArg)
	{
	TTestAction action = (TTestAction)((TUint)aArg & 255);
	TTestAccess access = (TTestAccess)((TUint)aArg >> 8);

	TTestData* inputData = access == EAccessPaged ? PagedInputData : UnpagedInputData;
	TTestData* outputData = access == EAccessPaged ? PagedOutputData : UnpagedOutputData;
		
	RPagingExample ldd;
	TInt r = ldd.Open(KPagingExample1PreLdd);
	if (r != KErrNone)
		return r;
	ldd.SetDfcThreadRealtimeState(ETrue);
	
	TRequestStatus unpagedStatus;
	TRequestStatus* status = &unpagedStatus;
	
	switch(action)
		{
		case ETestRequestComplete:
			{
			RDebug::Printf("Test RequestComplete");
			status = &outputData->iStatus;
			RPagingExample::TValueStruct value;
			DPTest::FlushCache();
			ldd.AsyncGetValue(*status, value);
			DPTest::FlushCache();
			}
			break;
			
		case ETestRawRead:
			RDebug::Printf("Test ThreadRawRead");
			ldd.Write(*status, inputData->iBuffer, KBufferSize);
			DPTest::FlushCache();
			break;
			
		case ETestRawWrite:
			RDebug::Printf("Test ThreadRawWrite");
			ldd.Read(*status, outputData->iBuffer, KBufferSize);
			DPTest::FlushCache();
			break;
			
		case ETestDesRead:
			{
			RDebug::Printf("Test ThreadDesRead");
			TPtrC8 writeDes(inputData->iBuffer, KBufferSize);
			ldd.WriteDes(*status, writeDes);
			DPTest::FlushCache();
			}
			break;
			
		case ETestDesWrite:
			{
			RDebug::Printf("Test ThreadDesWrite");
			TPtr8 readDes(outputData->iBuffer, KBufferSize);
			ldd.ReadDes(*status, readDes);
			DPTest::FlushCache();
			}
			break;
		default:
			return KErrArgument;
		}
	User::WaitForAnyRequest();
	r = status->Int();
	ldd.Close();
	return r;
	}

void RunRealtimeTestThread(TTestAction aTestAction, TTestAccess aAccess, TTestOutcome aExpectedOutcome)
	{
	RThread thread;
	TUint arg = aTestAction | (aAccess << 8);
	test_KErrNone(thread.Create(KNullDesC, RealtimeTestFunc, 4096, NULL, (TAny*)arg));
	TRequestStatus status;
	thread.Logon(status);
	thread.Resume();
	User::WaitForRequest(status);
	switch (aExpectedOutcome)
		{
		case EOutcomeSuccess:
			test_Equal(EExitKill, thread.ExitType());
			test_Equal(KErrNone, thread.ExitReason());
			break;

		case EOutcomeRealtimePanic:
			test_Equal(EExitPanic, thread.ExitType());
			test_Equal(EIllegalFunctionForRealtimeThread, thread.ExitReason());
			break;

		default:
			test(EFalse);	
		}
	CLOSE_AND_WAIT(thread);
	}

void TestPagedAccessInRealtimeThread()
	{
	// Test driver access to paged memory from realtime DFC thread.  This can happen if a client
	// passes paged memory to a driver that doesn't expect it, and has set its realtime state to
	// enfore this.  The client should be panicked in this case.

	test.Start(_L("Test memory access from realtime threads"));
	test.Next(_L("Load logical device"));
	TInt r = User::LoadLogicalDevice(KPagingExample1PreLdd);
	test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Test access to unpaged memory from realtime thread"));
	RunRealtimeTestThread(ETestRequestComplete, EAccessUnpaged, EOutcomeSuccess);
	RunRealtimeTestThread(ETestRawRead, 		EAccessUnpaged, EOutcomeSuccess);
	RunRealtimeTestThread(ETestRawWrite, 		EAccessUnpaged, EOutcomeSuccess);
	RunRealtimeTestThread(ETestDesRead, 		EAccessUnpaged, EOutcomeSuccess);
	RunRealtimeTestThread(ETestDesWrite, 		EAccessUnpaged, EOutcomeSuccess);

	test.Next(_L("Test access to paged memory from realtime thread"));
	if (PagedInputData)
		{
		RunRealtimeTestThread(ETestRawRead, 		EAccessPaged, EOutcomeRealtimePanic);
		RunRealtimeTestThread(ETestDesRead, 		EAccessPaged, EOutcomeRealtimePanic);
		}
	if (PagedOutputData)
		{
		RunRealtimeTestThread(ETestRequestComplete, EAccessPaged, EOutcomeRealtimePanic);
		RunRealtimeTestThread(ETestRawWrite, 		EAccessPaged, EOutcomeRealtimePanic);
		RunRealtimeTestThread(ETestDesWrite, 		EAccessPaged, EOutcomeRealtimePanic);
		}
	
	test.Next(_L("Close and free logical device"));
	test_KErrNone(User::FreeLogicalDevice(KPagingExample1PreLdd));
	
	test.End();
	}

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Test device driver migration examples"));
	
	UnpagedInputData = (TTestData*)User::Alloc(sizeof(TTestData));
	test_NotNull(UnpagedInputData);
	UnpagedOutputData = (TTestData*)User::Alloc(sizeof(TTestData));
	test_NotNull(UnpagedOutputData);

	test_KErrNone(UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&PageSize,0));

	RChunk chunk;
	if (DPTest::Attributes() & DPTest::EDataPaging)
		{
		TChunkCreateInfo info;
		TInt size = (sizeof(TTestData) + PageSize - 1) & ~(PageSize - 1);
		info.SetNormal(size, size);
		info.SetPaging(TChunkCreateInfo::EPaged);
		test_KErrNone(chunk.Create(info));
		test(chunk.IsPaged());
		PagedOutputData = (TTestData*)chunk.Base();
		test.Printf(_L("Using data pagd output buffer at %08x\n"), PagedOutputData);
		}
	
	if (DPTest::Attributes() & DPTest::ERomPaging)
		{
		// use paged part of rom for read-only data
		TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
		test(romHeader->iPageableRomStart);
		// todo: for some reason the first part of page of paged rom doesn't seem to get paged out
		// when we flush the paging cache, hence PagedInputData starts some way into this
		PagedInputData = (TTestData*)((TUint8*)romHeader + romHeader->iPageableRomStart + 64 * PageSize); 
		TInt romDataSize = romHeader->iPageableRomSize - 64 * PageSize;
		test(romDataSize >= (TInt)sizeof(TTestData));		
		test.Printf(_L("Using rom paged input data at %08x\n"), PagedInputData);
		}
	else if (DPTest::Attributes() & DPTest::ECodePaging)
		{
		// use code paged DLL for read-only buffer
		test_KErrNone(PagedLibrary.Load(KTCodePagingDll4));		
		TGetAddressOfDataFunction func = (TGetAddressOfDataFunction)PagedLibrary.Lookup(KGetAddressOfDataFunctionOrdinal);
		TInt codeDataSize;
		PagedInputData = (TTestData*)func(codeDataSize);
		test_NotNull(PagedInputData);
		test(codeDataSize >= (TInt)sizeof(TTestData));		
		test.Printf(_L("Using code paged input data at %08x\n"), PagedInputData);
		}

	InitInputData(UnpagedInputData);
	
	TestDriver(KPagingExample1PreLdd, EFalse);
	TestDriver(KPagingExample1PostLdd, ETrue);
	TestDriver(KPagingExample2PreLdd, EFalse);
	TestDriver(KPagingExample2PostLdd, ETrue);
	TestPagedAccessInRealtimeThread();
	
	PagedLibrary.Close();
	User::Free(UnpagedInputData);
	User::Free(UnpagedOutputData);
	
	test.End();
	return 0;
	}
