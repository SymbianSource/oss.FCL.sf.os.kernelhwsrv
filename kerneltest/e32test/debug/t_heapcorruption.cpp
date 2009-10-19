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


#define __NEXT_CELL(p)				((SCell*)(((TUint8*)p)+p->len))

TBool gEnableMemoryMonitor = EFalse;


/**
Test heap that will corrupt some cells to generate BTrace events.
*/
class RMyDummyHeap : public RHeap
{
public:
	//EBadFreeCellAddress
	void CorruptFreeMemory1()
		{
		SCell* f = (SCell*)&iFree;
		f->next = (SCell*)iTop;
		f->next += sizeof(TUint8);
		}
	
	//EBadFreeCellSize
	void CorruptFreeMemory2()
		{
		SCell* p = (SCell*)&iFree;
		SCell* n = p->next; 
		n->len = iMinCell-1;
		}
	
	//EBadAllocatedCellAddress
	void CorruptAllocatedMemory1()
		{
		SCell* c = (SCell*)iBase;
		SCell* f = (SCell*)&iFree;
		
		f = f->next;
		f = f->next;
		c->len = (TInt)f->next - (TInt)c;
		}
	
	//additional utilities
	void CorruptAllocatedMemorySize(void* aAddress)
		{
		SCell* addres = GetAddress(aAddress);
		SCell* c = (SCell*)iBase;
		for(;;)
			{
			if(c == addres)
				{
				c->len = iMinCell-1;
				break;
				}
			c = __NEXT_CELL(c);
			}
		}
		
	void CorruptAllocatedMemoryAddress(void* aAddress)
		{
		SCell* pF = &iFree;				// free cells
		pF = pF->next;				// next free cell
		if (!pF)
			pF = (SCell*)iTop;	
		SCell* addres = GetAddress(aAddress);
		SCell* c = (SCell*)iBase;
		for(;;)
			{
			if(c == addres)
				{
				c->len = (TInt)pF->next - (TInt)c;
				break;
				}
			c = __NEXT_CELL(c);
			}
		}
	
	void EnableHeavyMemoryMonitoring()
		{
		iFlags |= EMonitorMemory;
		}
};


/**
Heap corruption 2:
- Overrunning an array using memset 
(EHeapCorruption - EBadAllocatedCellSize)
*/
void Memory_Corruption2()
	{
	if(gEnableMemoryMonitor)
		{
		RMyDummyHeap* h = (RMyDummyHeap*)&User::Heap();
		h->EnableHeavyMemoryMonitoring();	
		}
	
	char* buf = new char[10];  //will be aligned to 12
	char* buf2 = new char[10]; //will be aligned to 12
	TInt a = User::Heap().AllocLen(buf);
	memset(buf, 255, a+1); //memory corruption
	
	if(!gEnableMemoryMonitor)
			User::Heap().Check(); //force 'heap walker' to check the heap
	
	delete buf2;
	delete buf; //when heavy monitoring is ON should send trace
	}


//causes EBadFreeCellAddress corruption type
void Memory_Corruption3()
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
	
	RMyDummyHeap* h = (RMyDummyHeap*)&User::Heap();
	h->CorruptFreeMemory1();
	User::Heap().Check();
	
	delete p5;
	delete p3;
	delete p1;
	}


//causes EBadFreeCellSize RHeap corruption type
void Memory_Corruption4()
	{
	TInt* p1 = new TInt();
	TInt* p2 = new TInt();
	TInt* p3 = new TInt();
	delete p2;
	
	RMyDummyHeap* h = (RMyDummyHeap*)&User::Heap();
	h->CorruptFreeMemory2();
	User::Heap().Check();
	
	delete p3;
	
	delete p1;
	}


//causes EBadAllocatedCellAddress corruption type
void Memory_Corruption5()
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
	
	RMyDummyHeap* h = (RMyDummyHeap*)&User::Heap();
	//h->CorruptAllocatedMemory1();
	h->CorruptAllocatedMemoryAddress((void*)p7);
	User::Heap().Check();
	
	delete p7;
	delete p5;
	delete p3;
	delete p1;
	}


void Memory_Corruption_Special1()
	{
	char* buf = new char;
	RMyDummyHeap* h = (RMyDummyHeap*)&User::Heap();
	h->EnableHeavyMemoryMonitoring();
	h->CorruptAllocatedMemoryAddress((void*)buf);
	delete buf;// should output EHeapCorruption trace
	}



//  Local Functions
LOCAL_D TInt threadTraceHeapCorruptionTestThread(TAny* param)
	{
	TInt t = *((TInt*)param);
	switch(t)
		{
		case RHeap::EBadAllocatedCellSize:
			Memory_Corruption2();
			break;
		case RHeap::EBadFreeCellAddress:
			Memory_Corruption3();
			break;
		case RHeap::EBadFreeCellSize:
			Memory_Corruption4();
			break;
		case RHeap::EBadAllocatedCellAddress:
			Memory_Corruption5();
			break;
		case 1000:
			Memory_Corruption_Special1();
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
		case 0: ////RHeap::EBadAllocatedCellSize with heavy monitoring enabled
			type = RHeap::EBadAllocatedCellSize;
			gEnableMemoryMonitor = ETrue;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
					               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
			break;
			
		case 1: //RHeap::EBadFreeCellAddress:
			type = RHeap::EBadFreeCellAddress;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
					               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 2: //RHeap::EBadFreeCellSize:
			type = RHeap::EBadFreeCellSize;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
			                KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 3: //RHeap::EBadAllocatedCellSize:
			type = RHeap::EBadAllocatedCellSize;
			r = thread.Create(_L("t_tbrace_heapcorruption"), threadTraceHeapCorruptionTestThread, 
						               KDefaultStackSize, 0x2000, 0x2000, &type);
			thread.Logon(stat);
			thread.Resume();
			User::WaitForRequest(stat);
			thread.Close();
		break;
		
		case 4: //RHeap::EBadAllocatedCellAddress:
			type = RHeap::EBadAllocatedCellAddress;
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

