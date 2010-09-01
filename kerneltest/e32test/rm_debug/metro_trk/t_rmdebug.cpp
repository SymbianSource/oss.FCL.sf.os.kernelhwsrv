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
// Tests the functionality of the run mode debug device driver.
// 
//

#include <e32base.h>
#include <e32base_private.h>
#include <e32cons.h>
#include <e32test.h>
#include <e32ldr.h>
#include <f32dbg.h>
#include "d_rmdebugclient.h"
#include "d_rmdebugthread.h"
#include "t_rmdebug.h"

IMPORT_C TInt StartDebugThread(RThread& aServerThread);
IMPORT_D extern TInt TestData;
IMPORT_D extern TBuf8<SYMBIAN_RMDBG_MEMORYSIZE> gMemoryAccessBytes;

LOCAL_D RTest test(_L("T_RMDEBUG"));

CRunModeAgent::CRunModeAgent()
//
// CRunModeAgent constructor
//
	{
	}

CRunModeAgent* CRunModeAgent::NewL()
//
// CRunModeAgent::NewL
//
	{
	CRunModeAgent* self = new(ELeave) CRunModeAgent();

  	self->ConstructL();

   if (self->iState != ERunModeAgentRunning)
       {
       delete self;
       self = NULL;
       }       
	return self;
	}

CRunModeAgent::~CRunModeAgent()
//
// CRunModeAgent destructor
//
	{
	iServSession.Close();
	iDebugThread.Close();
   iState = ERunModeAgentUnInit;
	}

void CRunModeAgent::ConstructL()
//
// CRunModeAgent::ConstructL
//
	{
	TInt err;
	err = StartDebugThread(iDebugThread);

	if (err == KErrNone)
		{
		if (iServSession.Open() == KErrNone)
           {
           iState = ERunModeAgentRunning;
           }
       else
           {
           iState = ERunModeAgentUnInit;
           }
		}
	else
		{
		User::Panic(_L("Can't start debug thread"), err);
		}
}



CRunModeAgent *RunModeAgent;

// Test process names
_LIT(ProcessName1,"T_RMDEBUG");
_LIT(ProcessName1a,"t_rmdebug");
//_LIT(ProcessName2,"ekern");
//_LIT(ProcessName3,"efile");
_LIT(KWildCard,"*");


//---------------------------------------------
//! @SYMTestCaseID KBase-0185
//! @SYMTestType 
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test reading process list
//! @SYMTestActions Several calls to read the process list
//! @SYMTestExpectedResults KErrNone and the owning process ID set
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestProcessList()
	{
	TInt    err=KErrNone;
	TBool	found = FALSE;

	test.Next(_L("TestProcessList - Read Process List\n"));

	TFindProcess find(KWildCard);
	TFullName name;
	while(find.Next(name)==KErrNone)
		{
		RProcess process;
		err = process.Open(find);
		if (err == KErrNone)
			{
			if ((name.Find(ProcessName1) != KErrNotFound) ||
				(name.Find(ProcessName1a) != KErrNotFound))
				{				
					iProcessID = process.Id();
					found = TRUE;				
				}
			process.Close();				
			}
	   }   
	test(found== TRUE);   
	}

// Test thread name
_LIT(ThreadName1,"DebugThread");

//---------------------------------------------
//! @SYMTestCaseID KBase-0186
//! @SYMTestType 
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test reading thread list
//! @SYMTestActions Several calls to read the thread list
//! @SYMTestExpectedResults KErrNone and the debug thread ID set
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestThreadList()
	{
	TInt        err=KErrNone;
	TBool       found = FALSE;

	test.Next(_L("TestThreadList - Read Thread List\n"));

   	TFindThread find(KWildCard);
	TFullName name;
	while(find.Next(name)==KErrNone)
		{
		RThread thread;
		err = thread.Open(find);
       	if (err == KErrNone)
			{
			RProcess process;
			thread.Process(process);
			if (((TUint32)process.Id() == iProcessID) &&
				(name.Find(ThreadName1) != KErrNotFound))
				{
				found = TRUE;           
				iThreadID = thread.Id();
				}
			}
			thread.Close();
   		}   

	test(found==TRUE);   
	}

   
//---------------------------------------------
//! @SYMTestCaseID KBase-0187
//! @SYMTestType 
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test reading and writing thread memory
//! @SYMTestActions Several call to read and write blocks of thread memory
//! @SYMTestExpectedResults KErrNone
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestMemoryAccess()
{
	TInt err=KErrNone;
	TMemoryInfo MemoryInfo;
	TInt i;

	test.Next(_L("TestMemoryAccess - Read Memory\n"));     

	for (i = 0; i < SYMBIAN_RMDBG_MEMORYSIZE; i++)
		{
		gMemoryAccessBytes.Append(i);
		}

	MemoryInfo.iAddress = (TUint32)(&gMemoryAccessBytes[0]);
	MemoryInfo.iSize = SYMBIAN_RMDBG_MEMORYSIZE;

	HBufC8 *data = HBufC8::NewLC(SYMBIAN_RMDBG_MEMORYSIZE);
	TPtr8 ptr_memread(data->Des());   
	MemoryInfo.iDataPtr = &ptr_memread;

//	test.Printf(_L("Read address = 0x%x Read size = 0x%x\n"),MemoryInfo.iAddress,MemoryInfo.iSize);

	err = iServSession.ReadMemory(iThreadID, &MemoryInfo);

	for (i = 0; i < MemoryInfo.iSize; i++)
		{
		if (ptr_memread.Ptr()[i] != gMemoryAccessBytes[i])
			{
			err = KErrCorrupt;       		
			}
		}


	// Test out writing memory.   
	test.Next(_L("TestMemoryAccess - Write Memory\n"));
//	test.Printf(_L("Write address = 0x%x Write size = 0x%x\n"),MemoryInfo.iAddress,MemoryInfo.iSize);
	if (err== KErrNone)
		{
		// Now reset the buffer
		for (i = 0; i < SYMBIAN_RMDBG_MEMORYSIZE; i++)
			{
			gMemoryAccessBytes[i] = 0;
			}      

		// Write our data into the buffer
		err = iServSession.WriteMemory(iThreadID, &MemoryInfo);

		for (i = 0; i < MemoryInfo.iSize; i++)
			{
			if (ptr_memread.Ptr()[i] != gMemoryAccessBytes[i])
				{
				err = KErrCorrupt;       		
				}
			}

		}

	if (gMemoryAccessBytes[5] == 0)
		{
		err = KErrCorrupt;
		}

	CleanupStack::PopAndDestroy(data);       
	test(err==KErrNone);   
	}


//---------------------------------------------
//! @SYMTestCaseID KBase-0188
//! @SYMTestType 
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test suspending and resuming a task
//! @SYMTestActions Suspends a thread checks the contents of a variable then waits and tests it hasnt changed
//! @SYMTestExpectedResults KErrNone
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestSuspendResume()
	{
	TInt err;

	test.Next(_L("TestSuspendResume - Suspend\n"));
	// Suspend the thread
	err = iServSession.SuspendThread(iThreadID);
	test(err==KErrNone);
	TInt localtestdata;
	localtestdata = TestData;

	// Wait 3 seconds (suspends this thread) and hopefully resumes the
	// thread we are controlling via the iServSession.SuspendThread request
	User::After(3000000);

	// Now check data hasnt changed
	test(localtestdata==TestData);

	// Resume the thread
	test.Next(_L("TestSuspendResume - Resume\n"));
	err = iServSession.ResumeThread(iThreadID);
	test(err==KErrNone);

	// Wait 3 seconds (suspends this thread) and hopefully resumes the
	// thread we are controlling via the iServSession.SuspendThread request
	User::After(3000000);

	// Now check that the thread being controlled has resumed and is
	// updating the variable
	test(localtestdata!=TestData);
	}
   
void CRunModeAgent::ClientAppL()
//
// Performs each test in turn
//
	{
	test.Start(_L("ClientAppL"));

	TestProcessList();
	TestThreadList();
	TestMemoryAccess();
	TestSuspendResume();
	test.End();   
	}

GLDEF_C TInt E32Main()
//
// Entry point for run mode debug driver test
//
	{
   TInt ret = KErrNone;
   
	// client
	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;

   RunModeAgent = CRunModeAgent::NewL();
   if (RunModeAgent != NULL)
       {
   	test.Title();

        __UHEAP_MARK;
	    TRAPD(r,RunModeAgent->ClientAppL());
       ret = r;
	    __UHEAP_MARKEND;

	    delete RunModeAgent;
       }
       
	delete trap;

	return ret;
	}
