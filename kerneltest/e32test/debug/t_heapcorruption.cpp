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
// e32test\debug\t_heapcorruption.cpp
// This is a test application that will cause heap corruption 
// to generate BTrace events (EHeapCorruption).
// 
//

//  Include Files  
#include "t_heapcorruption.h"
#include <e32base.h>
#include <e32base_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"

TBool gEnableMemoryMonitor = EFalse;

#ifdef _DEBUG
const TInt KDbgHeaderSize = (TInt)RHeap::EDebugHdrSize;
#else
const TInt KDbgHeaderSize = 0;
#endif

/**
Friend class of RHeapHybrid to access to hybrid heap metadata
*/
class TestHybridHeap
{
	public:
		TBool Init();
		TBool Check();
		TInt  AllocLen(TAny* aBfr);
		void  EnableHeavyMemoryMonitoring();
		void  CorruptFreeDLBfr(TAny* aBfr);
		void  CorruptFreeDLBfrLth(TAny* aBfr);
		void  CorruptAllocatedDLBfrSize(TAny* aBfr);
		TAny* CorruptAllocatedDLMemoryAddress(TAny* aBfr);		

	private:
		RHybridHeap* iHybridHeap;
};



TBool TestHybridHeap::Init()
{
	RHybridHeap::STestCommand cmd;
	cmd.iCommand = RHybridHeap::EHeapMetaData;
	RAllocator& heap = User::Allocator();
	TInt ret = heap.DebugFunction(RHeap::EHybridHeap, &cmd, 0);
	if (ret != KErrNone)
		return EFalse;
	iHybridHeap = (RHybridHeap*) cmd.iData;

	return ETrue;
}

TBool TestHybridHeap::Check()
{
	if ( iHybridHeap )
		{
		iHybridHeap->Check();  
		}

	return EFalse;
}

TInt TestHybridHeap::AllocLen(TAny* aBfr)
{
	if ( iHybridHeap )
		{
		return iHybridHeap->AllocLen(aBfr);  
		}
	return 0;
}

void TestHybridHeap::EnableHeavyMemoryMonitoring()
{
	if ( iHybridHeap )
		{
		iHybridHeap->iFlags |= RAllocator::EMonitorMemory;
		}

}


void TestHybridHeap::CorruptFreeDLBfr(TAny* aBfr)
{

	if ( aBfr )
		{
		mchunkptr p	= MEM2CHUNK((TUint8*)aBfr-KDbgHeaderSize);
		p->iHead |= CINUSE_BIT;
		}
}

void TestHybridHeap::CorruptFreeDLBfrLth(TAny* aBfr)
{

	if ( aBfr )
		{
		mchunkptr p	= MEM2CHUNK((TUint8*)aBfr-KDbgHeaderSize);
		p->iHead &= INUSE_BITS; // Set zero length
		}
}

void TestHybridHeap::CorruptAllocatedDLBfrSize(TAny* aBfr)
{

	if ( aBfr )
		{
		mchunkptr p	= MEM2CHUNK((TUint8*)aBfr-KDbgHeaderSize);
		TInt size = CHUNKSIZE(p);
		size  >>= 1;  // Set double length
		p->iHead = size | INUSE_BITS; 
		}
}

TAny* TestHybridHeap::CorruptAllocatedDLMemoryAddress(TAny* aBfr)
{

	if ( aBfr )
		{
		TUint8* p = (TUint8*)aBfr;
		p += 3;
		aBfr = (TAny*)p;
		}
	return aBfr;
}


/**
Heap corruption 0:
- Allocate (DL) buffer, corrupt it and free
*/
void Memory_Corruption0(TestHybridHeap& aHeap)
	{
	if(gEnableMemoryMonitor)
		aHeap.EnableHeavyMemoryMonitoring();	
	
	char* buf = new char[10];  //will be aligned to 12
	char* buf2 = new char[10]; //will be aligned to 12
	TInt a = aHeap.AllocLen(buf);
	memset(buf, 0xfc, a+a); //memory corruption
	
	if(!gEnableMemoryMonitor)
		aHeap.Check(); //force 'heap walker' to check the heap
	
	delete buf2;
	delete buf; //when heavy monitoring is ON should send trace and panic
	}

//Corrupt free DL memory and Check()
void Memory_Corruption1(TestHybridHeap& aHeap)
	{
	TInt* p1 = new TInt();
	TInt* p2 = new TInt();
	TInt* p3 = new TInt();
	TInt* p4 = new TInt();
	TInt* p5 = new TInt();
	TInt* p6 = new TInt();
	delete p2;
	delete p4;
	delete p6;
	
	aHeap.CorruptFreeDLBfr(p4);
	aHeap.Check();  // Should panic here
	
	delete p5;
	delete p3;
	delete p1;
	}


//corrupt free DL buffer length 
void Memory_Corruption2(TestHybridHeap& aHeap)
	{
	TInt* p1 = new TInt();
	TInt* p2 = new TInt();
	TInt* p3 = new TInt();
	delete p2;
	
	aHeap.CorruptFreeDLBfrLth(p2);
	aHeap.Check(); // Should panic here
	
	delete p3;
	
	delete p1;
	}


//Corrupt allocated DL buffer size
void Memory_Corruption3(TestHybridHeap& aHeap)
	{
	TInt* p1 = new TInt;
	TInt* p2 = new TInt;
	TInt* p3 = new TInt;
	TInt* p4 = new TInt;
	TInt* p5 = new TInt;
	TInt* p6 = new TInt;
	TInt* p7 = new TInt;
	delete p2;
	delete p4;
	delete p6;
	
	aHeap.CorruptAllocatedDLBfrSize(p7);
	aHeap.Check();
	
	delete p7;
	delete p5;
	delete p3;
	delete p1;
	}


void Memory_Corruption4(TestHybridHeap& aHeap)
	{
	char* buf = new char;
	aHeap.EnableHeavyMemoryMonitoring();
	buf = (char*)aHeap.CorruptAllocatedDLMemoryAddress((TAny*)buf);
	delete buf;// should output EHeapCorruption trace
	}



//  Local Functions
LOCAL_D TInt threadTraceHeapCorruptionTestThread(TAny* param)
	{
	TestHybridHeap heap;
	heap.Init();
	
	TInt t = *((TInt*)param);
	switch(t)
		{
		case 0:  // Corrupt allocated buffer and free it
			Memory_Corruption0(heap);
			break;
		case 1:
			Memory_Corruption1(heap);
			break;
		case 2:
			Memory_Corruption2(heap);
			break;
		case 3:
			Memory_Corruption3(heap);
			break;
		case 1000:
			Memory_Corruption4(heap);
			break;
		default:
			User::Invariant();
			break;
		}
	return 0;
	}


//Function to execute corruption cases.
TInt ExecuteTest(TInt aTestType)
	{
	RThread thread;
	TInt type;
	TRequestStatus stat;
	TInt r = KErrNone;
	gEnableMemoryMonitor = EFalse;
	
	switch(aTestType)
		{
		case 0: ////Corrupt allocated DL buffer and free it with heavy monitoring enabled
			type = 0;
			gEnableMemoryMonitor = ETrue;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
					               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
			break;
			
		case 1: //RHeap::EBadFreeCellAddress:
			type = 1;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
					               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 2: //RHeap::EBadFreeCellSize:
			type = 2;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
			                KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 3: //RHeap::EBadAllocatedCellSize:
			type = 0;    // Without memory monitorin this time
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
						               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 4: //RHeap::EBadAllocatedCellAddress:
			type = 3;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
						               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 1000:
			type = 1000;
			gEnableMemoryMonitor = ETrue;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
			                 KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		default:
			User::Invariant();
			break;
		}
	
	return r;
	}


LOCAL_C void MainL ()
	{
	//reading command line
	TInt testType = 0; //unknown test
	TInt cmdLength = User::CommandLineLength();
	HBufC* cmdLine = HBufC::NewLC(cmdLength);
	TPtr clp(cmdLine->Des());
	User::CommandLine(clp);
	TLex argv(clp);
	for(TInt i=0; !argv.Eos(); i++)
		{
		TPtrC token(argv.NextToken());

		if(token.Compare(_L("0")) == 0)
			testType = 0;
		if(token.Compare(_L("1")) == 0)
			testType = 1;
		else if(token.Compare(_L("2")) == 0)
			testType = 2;
		else if(token.Compare(_L("3")) == 0)
			testType = 3;
		else if(token.Compare(_L("4")) == 0)
			testType = 4;
		else if(token.Compare(_L("1000")) == 0)
			testType = 1000;
		}
	CleanupStack::PopAndDestroy(); //cmdLine
	
	ExecuteTest(testType);
	}

LOCAL_C void DoStartL ()
	{
	// Create active scheduler (to run active objects)
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
	CleanupStack::PushL (scheduler);
	CActiveScheduler::Install (scheduler);

	MainL ();

	// Delete active scheduler
	CleanupStack::PopAndDestroy (scheduler);
	}

//  Global Functions

GLDEF_C TInt E32Main()
	{
	// Create cleanup stack
	CTrapCleanup* cleanup = CTrapCleanup::New();

	// Run application code inside TRAP harness, wait keypress when terminated
	TRAPD(mainError, DoStartL());
	if (mainError)
		return mainError;

	delete cleanup;
	return KErrNone;
	}

