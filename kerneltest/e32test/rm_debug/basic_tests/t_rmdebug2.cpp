// Copyright (c) 2006-2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <f32dbg.h>
#include <f32file.h>
#include <hal.h>
#include <u32hal.h>
#include <e32property.h>

#include "t_rmdebug_dll.h"

#include <rm_debug_api.h>
#include "d_rmdebugthread2.h"
#include "t_rmdebug2.h"
#include "t_rmdebug_app.h"

#ifdef __MARM_ARMV4__
#include "d_rmdebug_step_test_armv4.h"
#endif

#ifdef __MARM_ARMV5__
#include "d_rmdebug_step_test.h"
#include "d_rmdebug_bkpt_test.h"
#endif

#include "d_demand_paging.h"

#ifdef KERNEL_OOM_TESTING
	#ifdef USER_OOM_TESTING
		#error "Cannot define both KERNEL_OOM_TESTING and USER_OOM_TESTING"
	#endif
#endif

_LIT8(KCrashDummyData, "This is a sample write");

using namespace Debug;

const TVersion securityServerVersion(0,1,1);

const TVersion testVersion(2,1,0);

IMPORT_C TInt StartDebugThread(RThread& aServerThread, const TDesC& aDebugThreadName);
IMPORT_D extern TInt TestData;
IMPORT_D extern TTestFunction FunctionChooser;
IMPORT_D extern TBuf8<SYMBIAN_RMDBG_MEMORYSIZE> gMemoryAccessBytes;
IMPORT_C TInt TestFunction();
IMPORT_C void TestPagedCode();
IMPORT_C extern TInt RMDebugDemandPagingTest();

// Device driver name
_LIT(KDebugDriverFileName,"rm_debug.ldd");

#ifdef SYMBIAN_STANDARDDEBUG
LOCAL_D RTest test(_L("T_RMDEBUG2"));
#endif

#ifdef SYMBIAN_OEMDEBUG
LOCAL_D RTest test(_L("T_RMDEBUG2_OEM"));
#endif

#ifdef SYMBIAN_OEM2DEBUG
LOCAL_D RTest test(_L("T_RMDEBUG2_OEM2"));
#endif

TBool gUseDelay;

CRunModeAgent::CRunModeAgent()
//
// CRunModeAgent constructor
//
	{
	FillArray();
	RProcess thisProcess;
	iFileName = thisProcess.FileName();
	thisProcess.Close();
	}

CRunModeAgent* CRunModeAgent::NewL()
//
// CRunModeAgent::NewL
//
	{
	CRunModeAgent* self = new(ELeave) CRunModeAgent();

  	self->ConstructL();

	return self;
	}

CRunModeAgent::~CRunModeAgent()
//
// CRunModeAgent destructor
//
	{
	User::FreeLogicalDevice(KDebugDriverFileName);
	iServSession.Close();
	iDebugThread.Close();
	}

void CRunModeAgent::ConstructL()
//
// CRunModeAgent::ConstructL
//
	{
	// nothing to do here
	}

void CRunModeAgent::SetupAndAttachToDSS()
//
// CRunModeAgent::SetupAndAttachToDSS
//
	{
	TInt err = StartDebugThread(iDebugThread, KDebugThreadName);

	// get the thread id for use in the tests
	iThreadID = iDebugThread.Id();

	if (err != KErrNone)
		{
		User::Panic(_L("Can't start debug thread"), err);
		}

	err = iServSession.Connect(securityServerVersion);
	if (err != KErrNone)
		{
		User::Panic(_L("Can't open server session"), err);
		}
	}

CRunModeAgent *RunModeAgent;

// helper function to check whether the listing of type aListId is supported for a scope of aListScope
TBool CRunModeAgent::ListingSupported(const TListId aListId, const TListScope aListScope)
	{
	TTag tag = GetTag(ETagHeaderList, aListId);

	return (tag.iValue) & aListScope;
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0426
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test getting the list of XIP libraries
//! @SYMTestActions The XIP library list should be successfully obtained
//! @SYMTestExpectedResults The specified ldd file should be present in the obtained listing
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestGetXipLibrariesList()
	{
	test.Next(_L("TestGetXipLibrariesList\n"));

	test(ListingSupported(EXipLibraries, EScopeGlobal));
	test(!ListingSupported(EXipLibraries, EScopeProcessSpecific));
	test(!ListingSupported(EXipLibraries, EScopeThreadSpecific));

	//allocate a very small buffer so the GetList call initially fails
	RBuf8 buffer;
	test(KErrNone == buffer.Create(1));
	TUint32 size = 0;

	//get the list data
	DoGetList(EXipLibraries, EScopeGlobal, buffer, size);

	//search the buffer for entry corresponding to the debug kernel driver
	//which should be in the rom
	_LIT(KRmDebugLddName, "z:\\sys\\bin\\rm_debug.ldd");

	//iterate through the buffer and set found to ETrue if we find the driver
	TBool found = EFalse;
	TUint8* ptr = (TUint8*)buffer.Ptr();
	const TUint8* ptrEnd = ptr + size;
	while(ptr < ptrEnd)
		{
		TXipLibraryListEntry& xipLibrary = *(TXipLibraryListEntry*)ptr;

		//get the name of the library
		TPtr name(&xipLibrary.iName[0], xipLibrary.iNameLength, xipLibrary.iNameLength);
		if(name.CompareF(KRmDebugLddName()) == 0)
			{
			//found the library but continue reading the rest of the buffer to
			//check nothing bad happens towards the end
			found = ETrue;
			}
		//move pointer on to next library
		ptr += Align4(xipLibrary.GetSize());
		}
	test(found);

	//do cleanup
	buffer.Close();
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0427
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test getting the list of executables
//! @SYMTestActions The list of debuggable executable files should be obtained
//! @SYMTestExpectedResults The client exe should appear in the list
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestGetExecutablesList()
	{
	test.Next(_L("TestGetExecutablesList\n"));

	test(ListingSupported(EExecutables, EScopeGlobal));
	test(!ListingSupported(EExecutables, EScopeProcessSpecific));
	test(!ListingSupported(EExecutables, EScopeThreadSpecific));

	//allocate a very small buffer so the GetList call initially fails
	RBuf8 buffer;
	test(KErrNone == buffer.Create(1));
	TUint32 size = 0;

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//get the list data
	DoGetList(EExecutables, EScopeGlobal, buffer, size);

	//get this process' name
	RProcess thisProcess;
	TFileName thisProcessName = thisProcess.FileName();

	//look through the buffer and check if the target debug thread is there
	TBool found = EFalse;
	TUint8* ptr = (TUint8*)buffer.Ptr();
	const TUint8* ptrEnd = ptr + size;
	while(ptr < ptrEnd)
		{
		TExecutablesListEntry& entry = *(TExecutablesListEntry*)ptr;
		//get name
		TPtr name(&entry.iName[0], entry.iNameLength, entry.iNameLength);
		if( (entry.iIsActivelyDebugged != 0) && (0 == thisProcessName.CompareF(name)) )
			{
			//found this process and asserted it is being actively debugged
			found = ETrue;
			}
		//move pointer on to next entry
		ptr += Align4(entry.GetSize());
		}
	test(found);

	//clean up
	buffer.Close();

	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0428
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test error conditions for the GetList calls
//! @SYMTestActions Multiple calls to test calling GetList with bad arguments
//! @SYMTestExpectedResults All tests should fail with the appropriate error codes
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestGetListInvalidData()
	{
	test.Next(_L("TestGetListInvalidData\n"));

	//allocate a buffer, the size should not matter as expecting all calls to fail
	RBuf8 buffer;
	test(KErrNone == buffer.Create(1));
	TUint32 size = 0;

	//test what happens if we ask for an unsupported list type globally
	test(KErrNotSupported == iServSession.GetList((TListId)1234, buffer, size));

	//test what happens if we ask for an unsupported list type
	test(KErrNotSupported == iServSession.GetList(RThread().Id(), (TListId)1234, buffer, size));

	//test what happens if we try to get a non-global libraries list
	test(KErrArgument == iServSession.GetList(RThread().Id(), EXipLibraries, buffer, size));

	//test what happens if we try to get a non-global executables list
	test(KErrArgument == iServSession.GetList(RThread().Id(), EExecutables, buffer, size));

	//test what happens if we try to get a non-global process list
	test(KErrArgument == iServSession.GetList(RThread().Id(), EProcesses, buffer, size));

	//check that using a process id fails
	test(KErrArgument == iServSession.GetList(RProcess().Id(), EProcesses, buffer, size));

	//check that specifying a non-existant thread id fails
	test(KErrArgument == iServSession.GetList((TThreadId)0x12345678, EThreads, buffer, size));

	//check that specifying a non-existant process id fails
	test(KErrArgument == iServSession.GetList((TProcessId)0x12345678, EThreads, buffer, size));

	//check that specifying a non-existant thread id fails
	test(KErrArgument == iServSession.GetList((TThreadId)0x12345678, ECodeSegs, buffer, size));

	//check that specifying a non-existant process id fails
	test(KErrArgument == iServSession.GetList((TProcessId)0x12345678, ECodeSegs, buffer, size));

	//cleanup
	buffer.Close();
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0429
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test getting the process list
//! @SYMTestActions Get the process listing
//! @SYMTestExpectedResults The process listing should be successfully obtained and the current process should be present in the list
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestGetProcessList()
	{
	test.Next(_L("TestGetProcessList\n"));

	test(ListingSupported(EProcesses, EScopeGlobal));
	test(!ListingSupported(EProcesses, EScopeProcessSpecific));
	test(!ListingSupported(EProcesses, EScopeThreadSpecific));

	//allocate a very small buffer so the GetList call fails
	RBuf8 buffer;
	test(KErrNone == buffer.Create(1));
	TUint32 size = 0;

	//get the list data
	DoGetList(EProcesses, EScopeGlobal, buffer, size);

	//initialise data about the target debug thread to compare the kernel's data against
	RProcess thisProcess;
	TFileName thisProcessName = thisProcess.FileName();
	TUint32 processId = thisProcess.Id().Id();

	//look through the buffer and check if the target debug thread is there
	TBool found = EFalse;
	TUint8* ptr = (TUint8*)buffer.Ptr();
	const TUint8* ptrEnd = ptr + size;
	while(ptr < ptrEnd)
		{
		TProcessListEntry& entry = *(TProcessListEntry*)ptr;
		if( (RProcess().Id().Id() == entry.iProcessId) &&
			(0 == thisProcessName.CompareF(TPtr(&(entry.iNames[0]), entry.iFileNameLength, entry.iFileNameLength))) &&
		 	(0 == thisProcess.FullName().CompareF(TPtr(&(entry.iNames[0]) + entry.iFileNameLength, entry.iDynamicNameLength, entry.iDynamicNameLength))) &&
			0x4321bbbb /* Magic */ == entry.iUid3)
			{
			//if all match then we've found it
			found = ETrue;
			}
		ptr += Align4(entry.GetSize());
		}

	//check whether the expected result happened
	test(found);

	//clean up
	buffer.Close();
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0430
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test getting the thread list
//! @SYMTestActions Get the thread listing globally and for a specified thread or process
//! @SYMTestExpectedResults The thread listings should all be successfully obtained and the current thread should be present in all listings
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestGetThreadList()
	{
	test.Next(_L("TestGetThreadList\n"));

	test(ListingSupported(EThreads, EScopeGlobal));
	test(ListingSupported(EThreads, EScopeProcessSpecific));
	test(ListingSupported(EThreads, EScopeThreadSpecific));

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	TBool found = EFalse;
	
	/* We need these loops because on some system the kernel run mode debugger does not 
	 immediately present the thread in the thread list. 
	 */
	
	for(TInt retryCount = 0; retryCount < 10 && !found; retryCount++ )
		{
		//test getting this process's thread list, ETrue as should find the target debug thread
		User::After(50000);
		found = DoTestGetThreadList(ETrue, EScopeProcessSpecific, RProcess().Id().Id());
		}
	test( found );
	found = EFalse;

	for(TInt retryCount = 0; retryCount < 10 && !found; retryCount++ )
		{
		//test getting the global list, ETrue as should find the target debug thread
		User::After(50000);
		found = DoTestGetThreadList(ETrue, EScopeGlobal);
		}
	test( found );

	found = EFalse;
	for(TInt retryCount = 0; retryCount < 10 && !found; retryCount++ )
		{
		//test getting this thread's thread list, ETrue as should find the target debug thread
		User::After(50000);
		found = DoTestGetThreadList(ETrue, EScopeThreadSpecific, RThread().Id().Id());
		}
	test( found );

	test(KErrNone == iServSession.ResumeThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}
			
TBool CRunModeAgent::DoTestGetThreadList(const TBool aShouldPass, const TListScope aListScope, const TUint64 aTargetId)
	{
	//create data to pass
	RBuf8 buffer;
	TUint32 size = 0;

	//perform the call to get the thread list
	DoGetList(EThreads, aListScope, buffer, size, aTargetId);

	//initialise data about the target debug thread to compare the kernel's data against
	TFileName name = iDebugThread.FullName();
	RProcess thisProcess;
	TUint64 processId = thisProcess.Id();
	TUint64 threadId = iDebugThread.Id();

	//look through the buffer and check if the target debug thread is there
	TBool found = EFalse;
	TUint8* ptr = (TUint8*)buffer.Ptr();
	const TUint8* ptrEnd = ptr + size;
	while(ptr < ptrEnd)
		{
		TThreadListEntry* entry = (TThreadListEntry*)ptr;
		TPtr entryName(&(entry->iName[0]), entry->iNameLength, entry->iNameLength);

		if( (threadId == entry->iThreadId) && (processId == entry->iProcessId) && (0 == name.CompareF(entryName)) )
			{
			test(entry->iSupervisorStackBaseValid);
			test(entry->iSupervisorStackSizeValid);
			//if all match then we've found it
			found = ETrue;
			break;
			}

		ptr += Align4(entry->GetSize());
		}

	//clean up
	buffer.Close();
	return found;

	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0431
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test getting the code segment list
//! @SYMTestActions Get the code segment list global and for a specified thread
//! @SYMTestExpectedResults The listings should be returned successfully
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestGetCodeSegsList()
	{
	test.Next(_L("TestGetCodeSegsList\n"));

	test(ListingSupported(ECodeSegs, EScopeGlobal));
	test(ListingSupported(ECodeSegs, EScopeProcessSpecific));
	test(ListingSupported(ECodeSegs, EScopeThreadSpecific));

	// Cannot perform this test with OEM2 debug token, as the t_rmdebug2 app
	// needs AllFiles, and the OEM2 debug token does not authorise this.
	// It seems reasonable to suppose that it would work anyway

#ifndef SYMBIAN_OEM2DEBUG
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

 	//test getting the global list, ETrue as should find this process' main codeSeg
	DoTestGetCodeSegsList(ETrue, EScopeGlobal);

	//test getting this process' codeSegs, ETrue as should find this process' main codeSeg
	DoTestGetCodeSegsList(ETrue, EScopeProcessSpecific, RProcess().Id().Id());

	//test getting this thread's codeSegs, ETrue as should find this process' main codeSeg
	DoTestGetCodeSegsList(ETrue, EScopeThreadSpecific, RThread().Id().Id());

	test(KErrNone == iServSession.DetachExecutable(iFileName));
#endif // SYMBIAN_OEM2DEBUG

	}

void CRunModeAgent::DoTestGetCodeSegsList(const TBool aShouldPass, const TListScope aListScope, const TUint64 aTargetId)
	{
	//create data to pass
	RBuf8 buffer;
	TUint32 size = 0;

	//perform the call to get the Code segs
	DoGetList(ECodeSegs, aListScope, buffer, size, aTargetId);

	//create memoryInfo to contain info about this process
	RProcess thisProcess;
	TModuleMemoryInfo memoryInfo;
	test(KErrNone == thisProcess.GetMemoryInfo(memoryInfo));

	// check whether this process came from a file in ROM so we know whether to
	// expect the code seg to be XIP or not.
	RFs fs;
	test(KErrNone == fs.Connect());
	TBool thisFileIsInRom = EFalse;
	if(fs.IsFileInRom(iFileName))
		{
		thisFileIsInRom = ETrue;
		}

	//look through the buffer to find this process' main code seg
	TBool found = EFalse;
	TUint8* ptr = (TUint8*)buffer.Ptr();
	const TUint8* ptrEnd = ptr + size;
	while(ptr < ptrEnd)
		{
		TCodeSegListEntry* codeSeg = (TCodeSegListEntry*)ptr;

		if( (codeSeg->iIsXip == thisFileIsInRom) && (0 == iFileName.CompareF(TPtr(&(codeSeg->iName[0]), codeSeg->iNameLength, codeSeg->iNameLength))) )
			{
			if( (memoryInfo.iCodeBase == codeSeg->iCodeBase) &&
					(memoryInfo.iCodeSize == codeSeg->iCodeSize) &&
					(memoryInfo.iConstDataSize == codeSeg->iConstDataSize) &&
					(memoryInfo.iInitialisedDataBase == codeSeg->iInitialisedDataBase) &&
					(memoryInfo.iInitialisedDataSize == codeSeg->iInitialisedDataSize) &&
					(memoryInfo.iUninitialisedDataSize == codeSeg->iUninitialisedDataSize))
				{
				//all matched so means we've found the codeSeg we're looking for
				found = ETrue;
				}
			}
		ptr += Align4(codeSeg->GetSize());
		}

	//check whether the result was as expected
	test(found == aShouldPass);

	// only care about rm_debug.ldd if we have global scope (belongs to the system not this process)
	if (aListScope == EScopeGlobal)
	{
		// Search for rm_debug.ldd library and check its UID3 is correct
		found = EFalse;

_LIT(KRMDebugDriverFileName,"Z:\\sys\bin\\rm_debug.ldd");

		TFileName rmdebugFilename(KRMDebugDriverFileName);

		// reset the Ptr
		ptr = (TUint8*)buffer.Ptr();
		ptrEnd = ptr+size;
		while(ptr < ptrEnd)
		{
			TCodeSegListEntry* codeSeg = (TCodeSegListEntry*)ptr;

			if( rmdebugFilename.CompareF(TPtr(&(codeSeg->iName[0]), codeSeg->iNameLength, codeSeg->iNameLength)))
				{
				if(codeSeg->iUid3 == 0x101f7157 /* Magic */)
					{
					//all matched so means we've found the codeSeg we're looking for
					found = ETrue;
					}
				}
			ptr += Align4(codeSeg->GetSize());
		}
		test((TUint32)found == (TUint32)ETrue);
	}

	//clean up
	buffer.Close();

	}


/**
 * Get a list from the run mode debug system. Most list calls will initially return KErrTooBig, 
 * since the initial size of the buffer is 0. However it is sometimes valid for a list to be empty
 * given its filtering and scope. These calls should return KErrNone.
 */
void CRunModeAgent::DoGetList(const TListId aListId, const TListScope aListScope, RBuf8& aBuffer, TUint32& aSize, const TUint64 aTargetId)
	{
	//close the buffer in case there's stuff allocated in it
	aBuffer.Close();
	//initialise it to be one byte big, which will guarantee data won't fit in it
	test(KErrNone == aBuffer.Create(1));
	aSize = 0;
	
	TInt ret = KErrNone;
	//should pass this test (assuming we've passed in sensible arguments above...)
	if(EScopeGlobal == aListScope)
		{
		ret = iServSession.GetList(aListId, aBuffer, aSize);
		}
	else if(EScopeThreadSpecific == aListScope)
		{
		ret = iServSession.GetList((TThreadId)aTargetId, aListId, aBuffer, aSize);
		}
	else if(EScopeProcessSpecific == aListScope)
		{
		ret = iServSession.GetList((TProcessId)aTargetId, aListId, aBuffer, aSize);
		}
	else
		{
		// unknown list scope
		test(0);
		}

	if( KErrNone == ret )
		{
		/* In the case that there is no data, just return and let the caller check
		the buffer. It is valid for a caller to not expect any data to be returned.
		*/
		return;
		}
	
	// The only other allowed return is KErrTooBig
	test( ret == KErrTooBig );

	//keep allocating larger buffers, beginning with the aSize returned by the above call,
	//and hopefully we'll eventually make a large enough one
	test(KErrNone == aBuffer.ReAlloc(aSize));

	for(;;)
		{
		TInt err = KErrNone;
		if(EScopeGlobal == aListScope)
			{
			err = iServSession.GetList(aListId, aBuffer, aSize);
			}
		else if(EScopeThreadSpecific == aListScope)
			{
			err = iServSession.GetList((TThreadId)aTargetId, aListId, aBuffer, aSize);
			}
		else if(EScopeProcessSpecific == aListScope)
			{
			err = iServSession.GetList((TProcessId)aTargetId, aListId, aBuffer, aSize);
			}
		else
			{
			// unknown list scope
			test(0);
			}
		if(err == KErrTooBig)
			{
			//wasn't big enough so double it
			aSize = aSize << 1;
			err = aBuffer.ReAlloc(aSize);
			if(err != KErrNone)
				{
				//print out a message if couldn't allocate memory and quit
				test.Printf(_L("Out ot memory when attempting to allocate %d bytes."), aSize);
				test(KErrNone == err);
				}

			//fairly arbitrary test, we don't have a max size for these calls.
			//In reality a list would have to have many thousands of elements
			//to break this test which shouldn't really happen
			test(aSize <= 0x4000);
			}
		else
			{
			test(KErrNone == err);
			test(aBuffer.Length() == aSize);
			//break out of the loop if the list has been successfully read in
			break;
			}
		}
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0432
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test reading and writing memory
//! @SYMTestActions Multiple calls to read and write memory, with various sizes and at various locations.
//!	Also test that bad input values cause appropriate errors to be returned.
//! @SYMTestExpectedResults All tests should pass and the target process should be left unaffected
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestMemoryAccess()
{
	TInt err;

	test.Next(_L("TestMemoryAccess - Read Memory\n"));

	//initialise buffer
	gMemoryAccessBytes.SetLength(0);
	for (TInt i=0; i<SYMBIAN_RMDBG_MEMORYSIZE; i++)
		{
		gMemoryAccessBytes.Append(i);
		}

	TUint32 address = (TUint32)(&gMemoryAccessBytes[0]);
	TUint32 dataSize = SYMBIAN_RMDBG_MEMORYSIZE;

	//create size for buffer that is rounded up to nearest 4 bytes if not
	//already 4 byte aligned
	TUint32 size = dataSize;
	if(size % 4 != 0)
		{
		size += (4 - (size % 4));
		}

	RBuf8 dataBlock;
	err = dataBlock.Create(size);
	test(err==KErrNone);
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//suspend the thread prior to memory operations
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	err = iServSession.ReadMemory(iThreadID, address, size, dataBlock, EAccess32, EEndLE8);
	test(err==KErrNone);

	for (TInt i=0; i<dataSize; i++)
		{
		test(dataBlock.Ptr()[i] == gMemoryAccessBytes[i]);
		}

	test.Next(_L("TestMemoryAccess - Write Memory\n"));

	// Now reset the buffer
	for (TInt i=0; i<dataSize; i++)
		{
		gMemoryAccessBytes[i] = 0;
		}

	// Write our data into the buffer
	err = iServSession.WriteMemory(iThreadID, address, size, dataBlock, EAccess32, EEndLE8);
	test(err==KErrNone);

	for (TInt i=0; i<dataSize; i++)
		{
		test(dataBlock.Ptr()[i] == gMemoryAccessBytes[i]);
		}

	//final test that everything's not been going wrong
	test(gMemoryAccessBytes[5] != 0);

	test.Next(_L("TestMemoryAccess - Invalid arguments\n"));
	test.Printf(_L("This test may emit crash-like information. This is intended.\n"));

	//test address that is not 32 bit aligned
	err = iServSession.ReadMemory(iThreadID, address + 1, size, dataBlock, EAccess32, EEndLE8);
	test(err == KErrArgument);

	//test size that is not multiple of 4 bytes
	err = iServSession.WriteMemory(iThreadID, address, size + 2, dataBlock, EAccess32, EEndLE8);
	test(err == KErrArgument);

	//test size > max block size
	err = iServSession.ReadMemory(iThreadID, address, (1<<15), dataBlock, EAccess32, EEndLE8);
	test(err == KErrArgument);

	//test access size == 2 bytes
	err = iServSession.ReadMemory(iThreadID, address, size, dataBlock, EAccess16, EEndLE8);
	test(err == KErrNotSupported);

	//test access size == 1 byte
	err = iServSession.WriteMemory(iThreadID, address, size, dataBlock, EAccess8, EEndLE8);
	test(err == KErrNotSupported);

	//test endianess == EEndBE8
	err = iServSession.ReadMemory(iThreadID, address, size, dataBlock, EAccess32, EEndBE8);
	test(err == KErrNotSupported);

	//test endianess == EEndBE32
	err = iServSession.WriteMemory(iThreadID, address, size, dataBlock, EAccess32, EEndBE32);
	test(err == KErrNotSupported);

	//test reading off end of memory
	err = iServSession.ReadMemory(iThreadID, 0xffffff00, 0x00000101, dataBlock, EAccess32, EEndLE8);
	test(err == KErrArgument);

	//The following three tests check that edge conditions in the range check are handled correctly.
	err = iServSession.ReadMemory(iThreadID, 0xffffff00, 0x000000FF, dataBlock, EAccess32, EEndLE8);
	test(err == KErrArgument);

	err = iServSession.ReadMemory(iThreadID, 0xffffff00, 0x000000F0, dataBlock, EAccess32, EEndLE8);
	test(err == KErrBadDescriptor);

	//Third range check test. Check that range check is handled correctly even when base + size wraps to 0.
	err = iServSession.ReadMemory(iThreadID, 0xffffff00, 0x00000100, dataBlock, EAccess32, EEndLE8);
	test(err == KErrBadDescriptor);
	//end of range check tests

	//test size == 0
	err = iServSession.WriteMemory(iThreadID, address, 0, dataBlock, EAccess32, EEndLE8);
	test(err == KErrArgument);

	//attempt to write to address outside of process data segments,
	//this address corresponds to the vectors so shouldn't be able to write
	err = iServSession.WriteMemory(iThreadID, 0xffff0000, size, dataBlock, EAccess32, EEndLE8);
	test(err == KErrBadDescriptor);

	//attempt to read and write to address in process code segment

	//open a handle to the thread
	RThread debugThread;
	test(debugThread.Open(iThreadID) == KErrNone);

	//get a reference to the debug process
	RProcess debugProcess;
	test(debugThread.Process(debugProcess) == KErrNone);

	//get the memory info for the process
	TProcessMemoryInfo info;
	test(debugProcess.GetMemoryInfo(info) == KErrNone);

	address = info.iCodeBase;
	if(size <= info.iCodeSize)
		{
		test(KErrNone == iServSession.ReadMemory(iThreadID, address, size, dataBlock, EAccess32, EEndLE8));
		test(KErrBadDescriptor == iServSession.WriteMemory(iThreadID, address, size, dataBlock, EAccess32, EEndLE8));
		}

	// Some performance tests now
	TUint32 bytesRead = 0;

	// Allocate a data buffer
	TUint32* p = (TUint32*)User::Alloc(size);
	test(p != 0);

	TInt nanokernel_tick_period;
	HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);
	test (nanokernel_tick_period != 0);

	static const TInt KOneMillion = 1000000;

	TInt nkTicksPerSecond = KOneMillion/nanokernel_tick_period;

	TUint32 stopTickCount = User::NTickCount() + nkTicksPerSecond;

	while (User::NTickCount() < stopTickCount)
		{
		err = iServSession.ReadMemory(iThreadID, (TUint32)p, size, dataBlock, EAccess32, EEndLE8);
		test(err==KErrNone);

		// Increase the count of bytes read
		bytesRead += size;
		}

	test(bytesRead != 0);
	iMemoryReadKbytesPerSecond = bytesRead/1024;

	// write memory test
	TUint32 bytesWritten = 0;

	stopTickCount = User::NTickCount() + nkTicksPerSecond;

	while (User::NTickCount() < stopTickCount)
		{
		err = iServSession.WriteMemory(iThreadID, (TUint32)p, size, dataBlock, EAccess32, EEndLE8);
		test(err==KErrNone);

		// Increase the count of bytes read
		bytesWritten += size;
		}

	test (bytesWritten != 0);
	iMemoryWriteKbytesPerSecond = bytesWritten/1024;

	User::Free(p);

	//resume the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));

	debugThread.Close();
	dataBlock.Close();

	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0433
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test suspending and resuming threads
//! @SYMTestActions Multiple calls to suspend and resume threads with and without attaching to the thread
//! @SYMTestExpectedResults All tests should pass and the target process should be left unaffected
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestSuspendResume()
	{
	TInt err;

	test.Next(_L("TestSuspendResume - Suspend\n"));

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
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

	test(KErrNone == iServSession.DetachExecutable(iFileName));

	// Wait 3 seconds (suspends this thread) and hopefully resumes the
	// thread we are controlling via the iServSession.SuspendThread request
	User::After(3000000);

	// Now check that the thread being controlled has resumed and is
	// updating the variable
	test(localtestdata!=TestData);

	// check that agent can resume thread which it previously detached from
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrNone == iServSession.SuspendThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrNone == iServSession.ResumeThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));

	// check that agent cannot suspend thread which it previously suspended and then detached from
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrNone == iServSession.SuspendThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrAlreadyExists == iServSession.SuspendThread(iThreadID));
	test(KErrNone == iServSession.ResumeThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0434
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test getting the debug functionality from the driver
//! @SYMTestActions Get the size and contents of the debug functionality block
//! @SYMTestExpectedResults All tests should pass and the expected data should appear in the functionality block
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestDebugFunctionality()
	{

	TInt err;

	test.Next(_L("TestDebugFunctionality - GetDebugFunctionalityBufSize\n"));

	TUint32 bufsize = 0;	// Safe default size

	// Get functionality block size
	err = iServSession.GetDebugFunctionalityBufSize(&bufsize);
	test(err==KErrNone);
	test.Next(_L("TestDebugFunctionality - GetDebugFunctionality\n"));

	// Ensure we have a finite buffer size
	test(bufsize!=0);

	// Allocate space for the functionality data
	HBufC8* dftext = HBufC8::NewLC(bufsize);

	// create an empty TPtr8 refering to dftext
	TPtr8 dftextPtr(dftext->Des());

	// Get the functionality block
	err = iServSession.GetDebugFunctionality(dftextPtr);
	test(err==KErrNone);

	// Check that the first entry is correct
	TTagHeader RefHdr =
	{
		ETagHeaderIdCore,ECoreLast,
	};

	// First header passed from rm_debug.ldd
	TTagHeader* TestHdr = (TTagHeader*)dftextPtr.Ptr();

	// Check
	test(RefHdr.iTagHdrId==TestHdr->iTagHdrId);
	// this test might fail if the agent is used with a Debug Security Server different from
	// the one it was compiled against. So removing it for now.
	//test(RefHdr.iNumTags==TestHdr->iNumTags);

	// read a value from the data to check it has come through as expected
	TTagHeader* header = GetTagHdr(dftext->Des(), ETagHeaderIdApiConstants);
	test(header != NULL);
	TTag* tag = GetTag(header, EApiConstantsTEventInfoSize);
	test(tag != NULL);
	// this test might fail if the agent is used with a Debug Security Server different from
	// the one it was compiled against. So removing it for now.
	//test(sizeof(TEventInfo) == tag->iValue);

	// Remove our temporary buffer
	CleanupStack::PopAndDestroy(dftext);
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0435
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test setting and clearing consecutive breakpoints
//! @SYMTestActions Set and clear consecutive breakpoints of all combinations of breakpoint types
//! @SYMTestExpectedResults All breakpoints should be set and cleared without error
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestConsecutiveBreakPoints()
	{
	test.Next(_L("TestConsecutiveBreakPoints\n"));

	test(KErrNone == iServSession.SuspendThread(iThreadID));

	// just a temporary structure for storing info about a breakpoint
	struct TBreakPoint
		{
	public:
		TBreakPoint()
			:iId(0),
			iMode((TArchitectureMode)0),
			iAddress(0)
			{}
		TBreakId iId;
		TArchitectureMode iMode;
		TUint32 iAddress;
		inline TInt Size() { return (EArmMode == iMode) ? 4 : 2; }
		};

	//an address in the target debug thread
	TUint32 address = (TUint32)(&TestFunction);

	// there are six orders in which three breakpoints can be set, these are looped
	// through below to check setting and clearing consecutive breakpoints works
	TUint8 order[6][3] =
		{
			{0,1,2},
			{0,2,1},
			{1,0,2},
			{1,2,0},
			{2,0,1},
			{2,1,0}
		};

	// The following code checks that setting and clearing consecutive breakpoints works correctly:
	// It checks that setting all combinations of three arm and thumb breakpoints succeeds, and check that the
	// breakpoints can be set in any order, and then cleared in any order

	// the 3 least significant bits of i control whether each of the three breakpoints should be arm or thumb
	for(TInt i=0; i<8; i++)
		{
		// controls the order in which the breakpoints should be set
		for(TInt j=0; j<6; j++)
			{
			// create the three breakpoints and set their modes
			TBreakPoint bp[3];
			bp[0].iMode = (i&1) ? EArmMode : EThumbMode;
			bp[1].iMode = (i&2) ? EArmMode : EThumbMode;
			bp[2].iMode = (i&4) ? EArmMode : EThumbMode;

			// set the address of each of the breakpoints
			bp[0].iAddress = address;
			if(EArmMode == bp[0].iMode)
				{ // if an arm breakpoint then must be on a four byte boundary
				bp[0].iAddress = Align4(bp[0].iAddress);
				}
			bp[1].iAddress = bp[0].iAddress + bp[0].Size();
			if(EArmMode == bp[1].iMode)
				{ // if an arm breakpoint then must be on a four byte boundary
				bp[1].iAddress = Align4(bp[1].iAddress);
				}
			bp[2].iAddress = bp[1].iAddress + bp[1].Size();
			if(EArmMode == bp[2].iMode)
				{ // if an arm breakpoint then must be on a four byte boundary
				bp[2].iAddress = Align4(bp[2].iAddress);
				}
			for(TInt k=0; k<6; k++)
				{
				// set the three breakpoints in the order defined by j and then clear them in the order defined by k
				test(KErrNone==iServSession.SetBreak(bp[order[j][0]].iId, iThreadID, bp[order[j][0]].iAddress, bp[order[j][0]].iMode));
				test(KErrNone==iServSession.SetBreak(bp[order[j][1]].iId, iThreadID, bp[order[j][1]].iAddress, bp[order[j][1]].iMode));
				test(KErrNone==iServSession.SetBreak(bp[order[j][2]].iId, iThreadID, bp[order[j][2]].iAddress, bp[order[j][2]].iMode));
				test(KErrNone==iServSession.ClearBreak(bp[order[k][0]].iId));
				test(KErrNone==iServSession.ClearBreak(bp[order[k][1]].iId));
				test(KErrNone==iServSession.ClearBreak(bp[order[k][2]].iId));
				}
			}
		}

	// resume the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0436
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test breakpoint functionality
//! @SYMTestActions Multiple calls to set and clear breakpoints. Checking bad input produces appropriate errors.
//! @SYMTestExpectedResults All tests should pass and the target debug thread should be left unaffected
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestBreakPoints()
	{
	TInt err;

	test.Next(_L("TestBreakPoints - Set\n"));

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	TestConsecutiveBreakPoints();

	//an address in the target debug thread
	TUint32 address = (TUint32)(&TestFunction);

	/*
	 * Ensure that breakpoint operations don't
	 * affect memory read/write by checking that reads/writes
	 * in locations containing breakpoints don't change behaviour
	 * because of the breakpoints.
	 */

	TUint32 size = SYMBIAN_RMDBG_MEMORYSIZE;

	RBuf8 originalDataBlock;
	err = originalDataBlock.Create(size);
	test(err==KErrNone);

	//suspend the thread
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	err = iServSession.ReadMemory(iThreadID, address, size, originalDataBlock, EAccess32, EEndLE8);
	test(err==KErrNone);

	// Test data block for comparison
	RBuf8 testDataBlock;
	err = testDataBlock.Create(size);
	test(err==KErrNone);

	/*
	 * set an arm breakpoint
	 */
	TBreakId armBreakId = 0;
	err = iServSession.SetBreak(armBreakId, iThreadID, address, EArmMode);
	test(err == KErrNone);

	// Ensure that memory read is not corrupted
	err = iServSession.ReadMemory(iThreadID, address, size, testDataBlock, EAccess32, EEndLE8);
	test(err==KErrNone);

	test (testDataBlock == originalDataBlock);

	/*
	 * set a thumb breakpoint
	 */
	TBreakId thumbBreakId = 0;
	err = iServSession.SetBreak(thumbBreakId, iThreadID, address+4, EThumbMode);
	test(err == KErrNone);

	/*
	 * set a thumb2EE breakpoint
	 */
	TBreakId thumb2EEBreakId = 0;
	err = iServSession.SetBreak(thumb2EEBreakId, iThreadID, address+8, EThumb2EEMode);
	test(err == KErrNotSupported);

	/*
	 * overlapping breakpoint (same address/threadId/mode)
	 */
	TBreakId overlapBreakId = 0;
	err = iServSession.SetBreak(overlapBreakId, iThreadID, address, EArmMode);
	test(err == KErrAlreadyExists);

	/*
	 * overlapping breakpoint (different address/same threadId/different mode)
	 *
	 * address - EArmBreakpoint
	 * address+2 - EThumbBreakpoint
	 */
	TBreakId overlap2BreakId = 0;
	err = iServSession.SetBreak(overlap2BreakId, iThreadID, address+2, EThumbMode);
	test(err == KErrAlreadyExists);

	/*
	 * Un-aligned address (arm)
	 */
	TBreakId armUnalignedBreakId = 0;
	err = iServSession.SetBreak(armUnalignedBreakId, iThreadID, address+6, EArmMode);
	test(err == KErrArgument);

	/*
	 * Un-aligned address (thumb)
	 */
	TBreakId thumbUnalignedBreakId = 0;
	err = iServSession.SetBreak(thumbUnalignedBreakId, iThreadID, address+7, EThumbMode);
	test(err == KErrArgument);

	/*
	 * Invalid address (arm)
	 */
	TBreakId armBadAddressBreakId = 0;
	err = iServSession.SetBreak(armBadAddressBreakId, iThreadID, 0 /* address */, EThumbMode);
	test(err == KErrBadDescriptor);

	/*
	 * Different thread, same address. Should fail for the same process, but succeed
	 * for a different process.
	 */

	/*
	 * Invalid thread
	 */
	TBreakId invalidThreadBreakId = 0;
	err = iServSession.SetBreak(invalidThreadBreakId, 0xbabababa, address, EThumbMode);
	test(err == KErrPermissionDenied);

	// Clear the ARM breakpoint
	err = iServSession.ClearBreak(armBreakId);
	test(err == KErrNone);

	// Clear the Thumb breakpoint
	err = iServSession.ClearBreak(thumbBreakId);
	test(err == KErrNone);

	// to do : two threads at the same address
	// to do : two processes at the same address

	// Ensure that memory read is not corrupted after clearing the breakpoints
	err = iServSession.ReadMemory(iThreadID, address, size, testDataBlock, EAccess32, EEndLE8);
	test(err==KErrNone);

	test (testDataBlock == originalDataBlock);

	/*
	 * How fast can we set breakpoints?
	 *
	 * Measure the time by setting/clearing breakpoints for 1 second.
     */
	TInt nanokernel_tick_period;
	HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);
	test (nanokernel_tick_period != 0);

	TInt nkTicksPerSecond = HelpTicksPerSecond();

	TInt breaksPerSecond = 0;

	TUint32 stopTickCount = User::NTickCount() + nkTicksPerSecond;

	while (User::NTickCount() < stopTickCount)
		{
		// set the breakpoint
		TBreakId armBreakId = 0;
		err = iServSession.SetBreak(armBreakId, iThreadID, address, EArmMode);
		test(err == KErrNone);

		// Clear the breakpoint
		err = iServSession.ClearBreak(armBreakId);
		test(err == KErrNone);

		// Update the count of breakpoints
		breaksPerSecond++;

		// Gone wrong if we wrap to negative breakpoints (cannot set 2billion/second!)
		test(breaksPerSecond >0);
		}

	// Store the results for later
	iBreakpointsPerSecond = breaksPerSecond;

	/*
	 * How many breakpoints can we set?
	 */

	TBool done = EFalse;

	// We assume all the breakpoints id's are issued in ascending order
	TInt maxBreakPoints = 0;

	// Temporary buffer
	RArray<TBreakId> breakIdList;

	TUint32 testAddress = address;

	while(!done)
		{
		TBreakId breakId = 0;

		// set the breakpoint
		testAddress += 4;	// ensure the addresses don't overlap

		err = iServSession.SetBreak(breakId, iThreadID, testAddress, EArmMode);
		test (err == KErrNone || err == KErrOverflow);
		if (err != KErrNone)
			{
			// we've reached the limit of the number of breaks we can set
			done = ETrue;
			break;
			}

		// store the id of this breakpoint
		breakIdList.Append(breakId);

		// Increase the count of breakpoints
		maxBreakPoints++;
		test(maxBreakPoints > 0);
		}

	// How many breakpoints can we set?
	iMaxBreakpoints = maxBreakPoints;

	// now clear all those breakpoints again
	while(breakIdList.Count() != 0)
		{
		// Place it into a TBreakId
		TBreakId id = breakIdList[0];

		err = iServSession.ClearBreak(id);
		test(err == KErrNone);

		// next id
		breakIdList.Remove(0);
		}

	breakIdList.Close();

	// close our temporary buffers
	originalDataBlock.Close();
	testDataBlock.Close();

	err = iServSession.ResumeThread(iThreadID);
	test (err == KErrNone);

	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0437
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test modifying breakpoints
//! @SYMTestActions Several calls to modify breakpoints
//! @SYMTestExpectedResults Valid requests should result in the breakpoints being changed, invalid requests should return errors
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestModifyBreak()
	{
	test.Next(_L("TestModifyBreak\n"));

	DoTestModifyBreak(ETrue);
	DoTestModifyBreak(EFalse);
	}

void CRunModeAgent::DoTestModifyBreak(TBool aThreadSpecific)
	{
	test.Printf(_L("DoTestModifyBreak: aThreadSpecific: %d\n"), aThreadSpecific?1:0);

	TInt err;

	RProcess process;
	TProcessId processId = process.Id();
	process.Close();

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//suspend the thread
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	//an address in the target debug thread
	TUint32 address = (TUint32)(&TestFunction);

	//set an arm mode break point
	TBreakId armBreakId = 0;
	err = aThreadSpecific
		? iServSession.SetBreak(armBreakId, iThreadID, address, EArmMode)
		: iServSession.SetProcessBreak(armBreakId, processId, address, EArmMode);
	test(err == KErrNone);

	/*
	 * Invalid thread
	 */
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, 0xbabababa, address, EArmMode)
		: iServSession.ModifyProcessBreak(armBreakId, 0xbabababa, address, EArmMode);
	test(err == KErrPermissionDenied);

	/*
	 * Valid address
	 */
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, address+4, EArmMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, address+4, EArmMode);
	test(err == KErrNone);

	/*
	 * Invalid address
	 */
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, 0, EArmMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, 0, EArmMode);
	test(err == KErrBadDescriptor);

	/*
	 * Thumb mode
	 */
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, address, EThumbMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, address, EThumbMode);
	test(err == KErrNone);

	/*
	 * Thumb2EE mode
	 */
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, address, EThumb2EEMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, address, EThumb2EEMode);
	test(err == KErrNotSupported);

	/*
	 * Arm mode
	 */
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, address, EArmMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, address, EArmMode);
	test(err == KErrNone);

	// Finally, clear the breakpoint
	err = iServSession.ClearBreak(armBreakId);
	test(err == KErrNone);

	//resume the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0438
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test extracting information about breakpoints
//! @SYMTestActions Several calls to get information about breakpoints
//! @SYMTestExpectedResults All tests should pass and the target process should be left unaffected
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestBreakInfo()
	{
	test.Next(_L("TestBreakInfo\n"));

	DoTestBreakInfo(ETrue);
	DoTestBreakInfo(EFalse);
	}

void CRunModeAgent::DoTestBreakInfo(TBool aThreadSpecific)
	{
	test.Printf(_L("DoTestModifyBreak: aThreadSpecific: %d\n"), aThreadSpecific?1:0);

	TInt err;

	RProcess process;
	TProcessId processId = process.Id();
	process.Close();

	//an address in the target debug thread
	TUint32 address = (TUint32)(&TestFunction);

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//suspend thread
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	//set an arm mode break point
	TBreakId armBreakId = 0;
	err = aThreadSpecific
		? iServSession.SetBreak(armBreakId, iThreadID, address, EArmMode)
		: iServSession.SetProcessBreak(armBreakId, processId, address, EArmMode);
	test(err == KErrNone);

	// Read back the information and check it is correct
	TThreadId testThreadId = TThreadId(0);
	TProcessId testProcessId = TProcessId(0);
	TUint32 testAddress = 0;
	TArchitectureMode testMode = EArmMode;

	err = aThreadSpecific
		? iServSession.BreakInfo(armBreakId,testThreadId,testAddress, testMode)
		: iServSession.ProcessBreakInfo(armBreakId, testProcessId, testAddress, testMode);
	test (err == KErrNone);
	test (aThreadSpecific ? (testThreadId == iThreadID) : (testProcessId == processId));
	test (testAddress == address);
	test (testMode == EArmMode);

	//change the address
	TUint32 changeAddress = address + 64;
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, changeAddress,EArmMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, changeAddress, EArmMode);
	test(err == KErrNone);

	// Check the address has changed
	err = aThreadSpecific
		? iServSession.BreakInfo(armBreakId,testThreadId,testAddress, testMode)
		: iServSession.ProcessBreakInfo(armBreakId, testProcessId, testAddress, testMode);
	test (err == KErrNone);
	test (testAddress == changeAddress);

	// change the architecture type
	TArchitectureMode checkMode = EThumbMode;
	err = aThreadSpecific
		? iServSession.ModifyBreak(armBreakId, iThreadID, address,checkMode)
		: iServSession.ModifyProcessBreak(armBreakId, processId, address, checkMode);
	test (err == KErrNone);

	// Check the mode has changed
	err = aThreadSpecific
		? iServSession.BreakInfo(armBreakId,testThreadId,testAddress,testMode)
		: iServSession.ProcessBreakInfo(armBreakId, testProcessId, testAddress, testMode);
	test (err == KErrNone);
	test (testMode == checkMode);

	// clear the breakpoint again
	err = iServSession.ClearBreak(armBreakId);
	test (err == KErrNone);

	//resume thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

// Needed for the RunToBreak test
IMPORT_C extern void RMDebug_BranchTst1();
IMPORT_C extern void RMDebug_BranchTst2();

//---------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-0439
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test hitting various types of breakpoints
//! @SYMTestActions Several calls to register to observe breakpoints and to hit breakpoints of different types
//! @SYMTestExpectedResults All tests should pass and the target process should be left unaffected
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestRunToBreak()
	{
	test.Next(_L("TestRunToBreak\n"));

	DoTestRunToBreak(ETrue);
	DoTestRunToBreak(EFalse);
	}

void CRunModeAgent::DoTestRunToBreak(TBool aThreadSpecific)
	{
	test.Printf(_L("DoTestRunToBreak: aThreadSpecific: %d\n"), aThreadSpecific?1:0);

	TInt err = KErrNone;

	RProcess process;
	TProcessId processId = process.Id();
	process.Close();

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	// we should suspend the thread first, then set the breakpoint
	err = iServSession.SuspendThread(iThreadID);
	test (err == KErrNone);

	// Try to set the breakpoint
	TBreakId armBreakId;
	TUint32 address = (TUint32)(&RMDebug_BranchTst1);

	err = aThreadSpecific
		? iServSession.SetBreak(armBreakId,iThreadID,address,EArmMode)
		: iServSession.SetProcessBreak(armBreakId, processId, address, EArmMode);
	test(err == KErrNone);

	err = aThreadSpecific
		? iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionContinue)
		: iServSession.SetEventAction(iFileName,EEventsProcessBreakPoint, EActionContinue);
	test (err == KErrNone);

	// Continue the thread
	err = iServSession.ResumeThread(iThreadID);
	test (err == KErrNone);

	// wait for the breakpoint to be hit
	TEventInfo info;
	static TRequestStatus status;

	TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));

	iServSession.GetEvent(iFileName,status,infoPtr);

	// Wait for notification of the breakpoint hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// info should now be filled with the details
	test(info.iEventType == (aThreadSpecific ? EEventsBreakPoint : EEventsProcessBreakPoint));
	test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address);
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);

	// Not interested in breakpoint events any more
	err = aThreadSpecific
		? iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionIgnore)
		: iServSession.SetEventAction(iFileName, EEventsProcessBreakPoint, EActionIgnore);
	test (err == KErrNone);

	// Clear the breakpoint again
	err = iServSession.ClearBreak(armBreakId);
	test(err == KErrNone);

	// continue the thread again
	err = iServSession.ResumeThread(iThreadID);
	test (err == KErrNone);
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-rmdebug2-2704
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test breakpoints in a loop
//! @SYMTestActions Several calls to register to verify breakpoints are stopping at correct address
//! @SYMTestExpectedResults All tests should pass and the target thread should be left unaffected
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CRunModeAgent::TestBreakPointsInLoop()
	{
	test.Next(_L("TestBreakPointsInLoop\n"));

	DoTestBreakPointsInLoop(ETrue);
	DoTestBreakPointsInLoop(EFalse);
	}

void CRunModeAgent::DoTestBreakPointsInLoop(TBool aThreadSpecific)
	{
	test.Printf(_L("DoTestBreakPointsInLoop: aThreadSpecific: %d\n"), aThreadSpecific?1:0);

	TInt err = KErrNone;
	TProcessId processId = RProcess().Id(); 

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	
	// We should suspend the thread first, then set the breakpoint
	err = iServSession.SuspendThread(iThreadID);
	test (err == KErrNone);

	// 2 breakpoints are sufficient to find issues with hitting breakpoints in a loop
	const TInt numOfBreakPointsInLoop = 2;

	TBreakId armBreakId[numOfBreakPointsInLoop];
	TUint32 address[numOfBreakPointsInLoop];
   	
	TUint32 entryAddress = (TUint32)(&RMDebug_Bkpt_Test_Entry);
	TBreakId entryArmBreakId;

	// Copy breakpoint address's in array
	address[0] = (TUint32)(&RMDebug_Bkpt_Test_Loop_Break_1);
	address[1] = (TUint32)(&RMDebug_Bkpt_Test_Loop_Break_2);

	err = aThreadSpecific
		? iServSession.SetBreak(entryArmBreakId,iThreadID,entryAddress,EArmMode)
		: iServSession.SetProcessBreak(entryArmBreakId, processId, entryAddress, EArmMode);
	test(err == KErrNone);

	// Try to set the breakpoints inside loop
	for (TInt i = 0; i < numOfBreakPointsInLoop; i++)
		{
		err = aThreadSpecific
			? iServSession.SetBreak(armBreakId[i],iThreadID,address[i],EArmMode)
			: iServSession.SetProcessBreak(armBreakId[i], processId, address[i], EArmMode);
		test(err == KErrNone);
		}

	err = aThreadSpecific
		? iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionContinue)
		: iServSession.SetEventAction(iFileName,EEventsProcessBreakPoint, EActionContinue);
	test (err == KErrNone);

	// Continue the thread
	err = iServSession.ResumeThread(iThreadID);
	test (err == KErrNone);

	// Wait for the breakpoint to be hit
	TEventInfo info;
	TRequestStatus status;

	TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));
	iServSession.GetEvent(iFileName,status,infoPtr);

	// Wait for notification of breakpoint event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// Info should now be filled with the details
	test(info.iEventType == (aThreadSpecific ? EEventsBreakPoint : EEventsProcessBreakPoint));

	// Have we stopped at the correct breakpoint?
	test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == entryAddress);
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);

	// Don't require the entry breakpoint anymore
	err = iServSession.ClearBreak(entryArmBreakId);
	test(err == KErrNone);
	
	// Stress the system by setting loop count to 100
	const TUint32 loopCount = 100;

	for (TInt i = 0; i < loopCount; i++)
		{
		// Continue the thread
		err = iServSession.ResumeThread(iThreadID);
		test (err == KErrNone);

		// Wait for the breakpoint to be hit
		iServSession.GetEvent(iFileName,status,infoPtr);
		
		// Wait for notification of the breakpoint hit event
		User::WaitForRequest(status);
		test(status==KErrNone);
		
		// Info should now be filled with the details
		test(info.iEventType == (aThreadSpecific ? EEventsBreakPoint : EEventsProcessBreakPoint));
		
		// Have we stopped at the correct breakpoint?
		test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address[i%numOfBreakPointsInLoop]);
		
		// Check process and thread id too
		test(info.iProcessIdValid);
		test(info.iThreadIdValid);
		}

	// Not interested in breakpoint events any more
	err = aThreadSpecific
		? iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionIgnore)
		: iServSession.SetEventAction(iFileName, EEventsProcessBreakPoint, EActionIgnore);
	test (err == KErrNone);

	// Clear breakpoints
	for (TInt i = 0; i < numOfBreakPointsInLoop; i++)
		{
		err = iServSession.ClearBreak(armBreakId[i]);
		test(err == KErrNone);
		}
	
	// Continue the thread again
	err = iServSession.ResumeThread(iThreadID);
	test (err == KErrNone);
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0440
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Test access to target user-side registers.
//! @SYMTestActions     Suspends a target thread, and reads/writes target thread register contents
//!
//! @SYMTestExpectedResults KErrNone. Should access target registers without problems.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestRegisterAccess()
	{
	TInt err;

	test.Next(_L("TestRegisterAccess - Read\n"));

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//suspend the thread to read registers
	err = iServSession.SuspendThread(iThreadID);
	test(err==KErrNone);

	//we'll try to read/write registers ERegisterR0 - ERegisterCPSR and ERegisterR13_IRQ
	//this way should get valid register values back, invalid ones and not supported ones, and it
	//means that the register IDs are not completely contiguous

	TInt firstRegister = 0;
	TInt lastRegister = 17;
	TInt numberOfRegisters = (lastRegister - firstRegister) + 1;

	RBuf8 ids;
	err = ids.Create(numberOfRegisters * sizeof(TRegisterInfo));
	test(err == KErrNone);

	for(TInt i=0; i<numberOfRegisters - 1; i++)
		{
		TRegisterInfo reg = (TRegisterInfo)((i + firstRegister)<<8);
		ids.Append(reinterpret_cast<const TUint8*>(&reg), sizeof(TRegisterInfo));
		}

	TRegisterInfo reg = ERegisterR13Irq;
	ids.Append(reinterpret_cast<const TUint8*>(&reg), sizeof(TRegisterInfo));

	//create a buffer to store the register values in
	RBuf8 originalValues;
	err = originalValues.Create(numberOfRegisters*sizeof(TUint32));
	test(err == KErrNone);

	//create a buffer to store the register flags in
	RBuf8 originalFlags;
	err = originalFlags.Create(numberOfRegisters*sizeof(TUint8));
	test(err == KErrNone);

	//read register values
	err = iServSession.ReadRegisters(iThreadID, ids, originalValues, originalFlags);
	test(err == KErrNone);

	//create a buffer containing data to write into the registers
	RBuf8 tempValues;
	err = tempValues.Create(numberOfRegisters*sizeof(TUint32));
	test(err == KErrNone);

	TUint cpsrId = 16;
	for(TUint8 i=0; i<numberOfRegisters*sizeof(TUint32); i++)
		{
		if(i/sizeof(TUint32) == cpsrId)
			{
			//For the CPSR we wish to write data that makes sense - for USR mode we are
			//allowed change all except the mode, ie. we must stay in usr mode. We try that here
			//(allowedCPSRValue[4:0] = 10000) thus not changing the mode.
			TUint32 allowedCPSRValue = 0x50000010;
			tempValues.Append((TUint8*)&allowedCPSRValue, 4);
			i += 3;
			}
		else
			{
			tempValues.Append(&i, 1);
			}
		}

	test.Next(_L("TestRegisterAccess - Write\n"));

	//create a buffer to store the register flags in
	RBuf8 tempWriteFlags;
	err = tempWriteFlags.Create(numberOfRegisters*sizeof(TUint8));
	test(err == KErrNone);

	//write the temp data into the registers
	err = iServSession.WriteRegisters(iThreadID, ids, tempValues, tempWriteFlags);
	test(err == KErrNone);

	//create another buffer to store the register flags in
	RBuf8 tempReadFlags;
	err = tempReadFlags.Create(numberOfRegisters*sizeof(TUint8));
	test(err == KErrNone);

	RBuf8 tempReadValues;
	err = tempReadValues.Create(numberOfRegisters*sizeof(TUint32));
	test(err == KErrNone);

	//read the temp data out again
	err = iServSession.ReadRegisters(iThreadID, ids, tempReadValues, tempReadFlags);
	test(err == KErrNone);

	//check values are correct
	for(TInt i=0; i<numberOfRegisters; i++)
		{
		TRegisterFlag writeFlag;
		err = GetFlag(tempWriteFlags, i, writeFlag);
		test(err == KErrNone);

		TRegisterFlag readFlag;
		err = GetFlag(tempReadFlags, i, readFlag);
		test(err == KErrNone);

		if((writeFlag == EValid) && (readFlag == EValid))
			{
			TUint8 offset = i * sizeof(TUint32);
			for(TUint j = offset; j< offset + sizeof(TUint32); j++)
				{
				test(tempValues.Ptr()[j] == tempReadValues.Ptr()[j]);
				}
			}
		}

	//write the original data into the registers
	err = iServSession.WriteRegisters(iThreadID, ids, originalValues, originalFlags);
	test(err == KErrNone);

	//read the data out again
	err = iServSession.ReadRegisters(iThreadID, ids, tempValues, tempReadFlags);
	test(err == KErrNone);

	//check values are correct
	for(TInt i=0; i<numberOfRegisters; i++)
		{
		TRegisterFlag writeFlag;
		err = GetFlag(originalFlags, i, writeFlag);
		test(err == KErrNone);

		TRegisterFlag readFlag;
		err = GetFlag(tempReadFlags, i, readFlag);
		test(err == KErrNone);

		if((writeFlag == EValid) && (readFlag == EValid))
			{
			TUint8 offset = i * sizeof(TUint32);
			for(TUint j = offset; j< offset + sizeof(TUint32); j++)
				{
				test(tempValues.Ptr()[j] == originalValues.Ptr()[j]);
				}
			}
		}

	test.Next(_L("TestRegisterAccess - Invalid data\n"));

	//create a buffer of max size 1
	RBuf8 emptyBuffer;
	emptyBuffer.Create(1);

	//test register IDs buffer not being a multiple of sizeof(TRegisterInfo)
	err = iServSession.ReadRegisters(iThreadID, emptyBuffer, tempValues, tempReadFlags);
	test(err == KErrArgument);

	//test register values buffer not being a multiple of sizeof(TUint32)
	err = iServSession.ReadRegisters(iThreadID, ids, emptyBuffer, tempReadFlags);
	test(err == KErrArgument);

	//test flags buffer being representing different number of registers from other two
	err = iServSession.ReadRegisters(iThreadID, ids, tempValues, emptyBuffer);
	test(err == KErrArgument);

	//set max length to 0
	emptyBuffer.ReAlloc(0);

	//test ids buffer being of 0 max length
	err = iServSession.ReadRegisters(iThreadID, emptyBuffer, tempValues, tempReadFlags);
	test(err == KErrArgument);

	//do cleanup
	emptyBuffer.Close();
	tempValues.Close();
	tempWriteFlags.Close();
	tempReadFlags.Close();
	tempReadValues.Close();

	test.Next(_L("TestRegisterAccess - Setting PC value\n"));

	//create buffer containing PC register ID
	RBuf8 pcId;
	err = pcId.Create(sizeof(TRegisterInfo));
	test(err == KErrNone);
	TRegisterInfo reg1 = (TRegisterInfo)0x00000f00;
	pcId.Append(reinterpret_cast<const TUint8*>(&reg1), sizeof(TRegisterInfo));

	//create buffer containing desired PC value
	RBuf8 pcValue;
	err = pcValue.Create(sizeof(TUint32));
	test(err == KErrNone);
	TUint32 address = (TUint32)(&TestFunction);
	pcValue.Append(reinterpret_cast<const TUint8*>(&address), sizeof(TUint32));

	//craete buffer for PC flag value
	RBuf8 pcFlag;
	err = pcFlag.Create(sizeof(TUint8));

	//write the new PC value
	err = iServSession.WriteRegisters(iThreadID, pcId, pcValue, pcFlag);
	test(err==KErrNone);

	//get the flag and check the PC value was written ok
	TRegisterFlag flag = ENotSupported;
	err = GetFlag(pcFlag, 0, flag);
	test(err==KErrNone);

	if(flag == EValid)
		{
		/* The PC value was changed to execute the function TestFunction.
		* TestFunction changes the value of TestData to a given value and 
		* then calls RMDebug_BranchTst1.
		* We place a breakpoint on RMDebug_BranchTst1 so that to we are able 
		* to test the value of TestData.
		*/

		test(KErrNone == iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionSuspend));
		TBreakId armBreakId;
		TUint32 address = (TUint32)(&RMDebug_BranchTst1);
		test(KErrNone == iServSession.SetBreak(armBreakId,iThreadID,address,EArmMode));

		// Continue the thread
		test(KErrNone == iServSession.ResumeThread(iThreadID));

		// wait for the breakpoint to be hit
		TEventInfo info;
		static TRequestStatus status;

		TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));
		iServSession.GetEvent(iFileName,status,infoPtr);

		// Wait for notification of the breakpoint hit event
		User::WaitForRequest(status);
		test(status==KErrNone);

		// info should now be filled with the details
		test(info.iEventType == EEventsBreakPoint);
		test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address);
		test(info.iProcessIdValid);
		test(info.iThreadIdValid);

		test(KErrNone == iServSession.ClearBreak(armBreakId));

		// Finally test the value
		test(TestData == 0xffeeddcc);
		}

	//Make sure we cannot change the CPSR
	test.Next(_L("Verifying we cannot change the CPSR mode from USR Mode"));

	TUint32 disallowedCpsr = 0x50000013;

	RBuf8 cpsrRegId;
	err = cpsrRegId.Create(sizeof(TUint32));
	test(err == KErrNone);

	TRegisterInfo cpsr = (TRegisterInfo)((cpsrId + firstRegister)<<8);
	cpsrRegId.Append(reinterpret_cast<const TUint8*>(&cpsr), sizeof(TRegisterInfo));

	RBuf8 cpsrRegFlags;
	err = cpsrRegFlags.Create(sizeof(TUint8));
	test(err == KErrNone);

	RBuf8 cpsrVal;
	err = cpsrVal.Create(sizeof(TUint32));
	test(err == KErrNone);

	cpsrVal.Append((TUint8*)&disallowedCpsr, 4);

	//attempt to write disallowed CPSR in
	err = iServSession.WriteRegisters(iThreadID, cpsrRegId, cpsrVal, cpsrRegFlags);
	test(err == KErrNone);

	RBuf8 cpsrReadVal;
	err = cpsrReadVal.Create(sizeof(TUint32));
	test(err == KErrNone);

	//Read back the CPSR
	err = iServSession.ReadRegisters(iThreadID, cpsrRegId, cpsrReadVal, cpsrRegFlags);
	test(err == KErrNone);

	//Make sure we havent switched modes ie. its not what we wrote
	TUint32* readVal = (TUint32*)cpsrReadVal.Ptr();
	test(*readVal != disallowedCpsr);

	cpsrRegId.Close();
	cpsrRegFlags.Close();
	cpsrVal.Close();
	cpsrReadVal.Close();

	//write the original values back into here
	err = iServSession.WriteRegisters(iThreadID, ids, originalValues, originalFlags);
	test(err == KErrNone);

	// Resume the thread
	err = iServSession.ResumeThread(iThreadID);
	test(err==KErrNone);

	test(KErrNone == iServSession.DetachExecutable(iFileName));

	//do cleanup
	pcId.Close();
	pcValue.Close();
	pcFlag.Close();
	ids.Close();
	originalValues.Close();
	originalFlags.Close();
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0441
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Test registration/de-registration of debug interest in target exe with the Debug Security Server
//! @SYMTestActions     As per description
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestAttachExecutable()
	{

	test.Next(_L("TestAttachExecutable - Attach\n"));

	//attach to process passively
	test(KErrNone == iServSession.AttachExecutable(iFileName, ETrue));

	//make a thread id for a non-existent thread
	TThreadId threadId(0x12345678);

	//get a handle to the target thread
	RThread targetThread;
	TInt err = targetThread.Open(threadId);
	test(err != KErrNone);

	//not registered for this thread's process (as it doesn't exist)
	//so should fail security check
	err = iServSession.ResumeThread(threadId);
	test(err==KErrPermissionDenied);

	//try to attach to the same process (and fail)
	test(KErrAlreadyExists == iServSession.AttachExecutable(iFileName, EFalse));

	test.Next(_L("TestAttachExecutable - Detach\n"));

	//detach from process
	test(KErrNone == iServSession.DetachExecutable(iFileName));

	//attach non-passively
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//not registered for this thread's process (as it doesn't exist)
	//so should fail security check
	err = iServSession.ResumeThread(0x12345678);
	test(err==KErrPermissionDenied);
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0442
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests single-stepping target threads.
//! @SYMTestActions     Steps target thread assembly level instructions, mainly branch/change PC
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestStep()
	{
	test.Next(_L("TestStep\n"));

	DoTestStep(EFalse);
	DoTestStep(ETrue);
	}

void CRunModeAgent::DoTestStep(TBool aThreadSpecific)
	{
	test.Printf(_L("DoTestStep: aThreadSpecific: %d\n"), aThreadSpecific?1:0);

	TInt err = KErrNone;

	RProcess process;
	TProcessId processId = process.Id();
	process.Close();

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	//set the target thread to execute the stepping functions
	test(KErrNone == SwitchTestFunction(EStepFunction));

	err = iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionContinue);
	test (err == KErrNone);

	if(!aThreadSpecific)
		{
		err = iServSession.SetEventAction(iFileName, EEventsProcessBreakPoint, EActionContinue);
		test (err == KErrNone);
		}

	TUint32	startAddress;
	TUint32	endAddress;

	/*
	 * RMDebug_StepTest_Non_PC_Modifying
	 */
	test.Next(_L("TestStep - Non-PC modifying\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Non_PC_Modifying);

	endAddress = (TUint32)(&RMDebug_StepTest_Non_PC_Modifying_OK);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Branch
	 */
	test.Next(_L("TestStep - Branch\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Branch);

	endAddress = (TUint32)(&RMDebug_StepTest_Branch_1);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Branch_And_Link
	 */
	test.Next(_L("TestStep - Branch_And_Link\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Branch_And_Link_1);

	endAddress = (TUint32)(&RMDebug_StepTest_Branch_And_Link_2);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_MOV_PC
	 */
	test.Next(_L("TestStep - MOV PC,X\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_MOV_PC_1);

	endAddress = (TUint32)(&RMDebug_StepTest_MOV_PC_2);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_LDR_PC
	 */
	test.Next(_L("TestStep - LDR PC\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_LDR_PC);

	endAddress = (TUint32)(&RMDebug_StepTest_LDR_PC_1);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1, EFalse, processId);
	test(err==KErrNone);

// thumb and interworking tests are not supported on armv4
#ifdef __MARM_ARMV5__

	/*
	 * RMDebug_StepTest_Thumb_Non_PC_Modifying
	 */
	test.Next(_L("TestStep - Thumb Non PC-Modifying\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Thumb_Non_PC_Modifying_1);

	endAddress = (TUint32)(&RMDebug_StepTest_Thumb_Non_PC_Modifying_2);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Thumb_Branch
	 */
	test.Next(_L("TestStep - Thumb Branch\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Thumb_Branch_1);

	endAddress = (TUint32)(&RMDebug_StepTest_Thumb_Branch_2);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Thumb_Branch_And_Link
	 */
	test.Next(_L("TestStep - Thumb Branch_And_Link\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Thumb_Branch_And_Link_2);

	endAddress = (TUint32)(&RMDebug_StepTest_Thumb_Branch_And_Link_3);

	TInt muid=0;
    test(HAL::Get(HAL::EMachineUid, muid)==KErrNone);

	// check if running on ARMv7 core
	if(muid==HAL::EMachineUid_OmapH6 || muid==HAL::EMachineUid_OmapZoom || muid==HAL::EMachineUid_EmuBoard)
        {
        // Note: ARMv7 treats BL instructions as single 32-bit instructions
        err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1, EFalse, processId);
        }
    else
	    {
        // Note: Due to the fact that the stepper treats BL instructions
		// as two instructions (as the hardware does), then we must step
		// the first half instruction first)

		err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,startAddress+2,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,startAddress+2,EThumbMode,1, EFalse, processId);
		test(err==KErrNone);

	// Now we actually do the BL
	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress+2,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress+2,endAddress,EThumbMode,1, EFalse, processId);
        }
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Thumb_Back_Branch_And_Link
	 */
	test.Next(_L("TestStep - Thumb Back_Branch_And_Link\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Thumb_Back_Branch_And_Link_2);

	endAddress = (TUint32)(&RMDebug_StepTest_Thumb_Back_Branch_And_Link_3);

	// check if running on ARMv7 core
	if(muid==HAL::EMachineUid_OmapH6 || muid==HAL::EMachineUid_OmapZoom || muid==HAL::EMachineUid_EmuBoard)
		{
		// Note: ARMv7 treats BL instructions as single 32-bit instructions
		err = aThreadSpecific
			? HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1)
			: HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1, EFalse, processId);
		}
	else
		{
		// Note: Due to the fact that the stepper treats BL instructions
		// as two instructions (as the hardware does), then we must step
		// the first half instruction first)

		err = aThreadSpecific
	   		? HelpTestStep(iThreadID,startAddress,startAddress+2,EThumbMode,1)
			: HelpTestStep(iThreadID,startAddress,startAddress+2,EThumbMode,1, EFalse, processId);
		test(err==KErrNone);

	   	// Now we actually do the BL
		err = aThreadSpecific
   			? HelpTestStep(iThreadID,startAddress+2,endAddress,EThumbMode,1)
			: HelpTestStep(iThreadID,startAddress+2,endAddress,EThumbMode,1, EFalse, processId);
		}
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Thumb_AddPC
	 */
	test.Next(_L("TestStep - Thumb ADD PC, PC, R0\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Thumb_AddPC_2);

	endAddress = (TUint32)(&RMDebug_StepTest_Thumb_AddPC_3);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1, EFalse, processId);
	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Interwork ARM to Thumb
	 */
	test.Next(_L("TestStep - Interworking ARM to Thumb - BLX \n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Interwork_1);

	endAddress = (TUint32)(&RMDebug_StepTest_Interwork_2);

	err = aThreadSpecific // nb initial breakpoint in ARM code
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,1, EFalse, processId);

	test(err==KErrNone);

	/*
	 * RMDebug_StepTest_Interwork Thumb to ARM
	 */
	test.Next(_L("TestStep - Interworking Thumb to ARM - BLX\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_Interwork_2);

	endAddress = (TUint32)(&RMDebug_StepTest_Interwork_3);

	// check if running on ARMv7 core
	if(muid==HAL::EMachineUid_OmapH6 || muid==HAL::EMachineUid_OmapZoom || muid==HAL::EMachineUid_EmuBoard)
        {
        // ARMv7 treats BLX instructions as single 32-bit instructions
        err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,endAddress,EThumbMode,1, EFalse, processId);
        }
    else
        {
    	// Stepper treats this as a two-stage instruction (just like the hardware)
	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,startAddress+2,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress,startAddress+2,EThumbMode,1, EFalse, processId);
	test(err == KErrNone);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress+2,endAddress,EThumbMode,1)
		: HelpTestStep(iThreadID,startAddress+2,endAddress,EThumbMode,1, EFalse, processId);
        }
	test(err == KErrNone);

#endif // __MARM_ARMV5__

	/*
	 * Test multiple-step of ARM code
	 */
	test.Next(_L("TestStep - ARM Multiple instruction step\n"));

	startAddress = (TUint32)(&RMDebug_StepTest_ARM_Step_Multiple);

	endAddress = (TUint32)(&RMDebug_StepTest_ARM_Step_Multiple_1);

	err = aThreadSpecific
		? HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,5)
		: HelpTestStep(iThreadID,startAddress,endAddress,EArmMode,5, EFalse, processId);
	test(err == KErrNone);
	// stepping performance
	test.Next(_L("TestStep - Steps per second\n"));

	// run until we reach RMDebug_StepTest_Count_1
	TBreakId stepBreakId;
	startAddress = (TUint32)(&RMDebug_StepTest_Count_1);
	endAddress = (TUint32)(&RMDebug_StepTest_Count_2);

	err = aThreadSpecific
		? HelpTestStepSetBreak(stepBreakId,iThreadID,startAddress,EArmMode)
		: HelpTestStepSetBreak(stepBreakId,iThreadID,startAddress,EArmMode,EFalse,processId);
	test (err == KErrNone);

	// wait until we hit the breakpoint
	TEventInfo info;
	err = HelpTestStepWaitForBreak(iFileName,info);
	test (err == KErrNone);

	// Now clear the breakpoint
	err = iServSession.ClearBreak(stepBreakId);
	test(err == KErrNone);

	if(aThreadSpecific)
		{
		// now step the code
		TInt stepsPerSecond = 0;

		TUint32 stopTickCount = User::NTickCount() + HelpTicksPerSecond();

		while (User::NTickCount() < stopTickCount)
			{
			err = iServSession.Step(iThreadID,1);
			test (err == KErrNone);

			// we need to wait now until the step completes before asking for the next step
				{
				TEventInfo info;
				static TRequestStatus status;

				TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));

				iServSession.GetEvent(iFileName,status,infoPtr);

				// Wait for notification of the breakpoint hit event
				User::WaitForRequest(status);
				test(status==KErrNone);
				}

			// Update the count of steps
			stepsPerSecond += 1;

			// Gone wrong if we do too many
			test(stepsPerSecond < 10000);
			}

		iStepsPerSecond = stepsPerSecond;
		test(iStepsPerSecond != 0);
		}

	// finally resume the thread
	err = iServSession.ResumeThread(iThreadID);
	test (err == KErrNone);

	err = iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionIgnore);
	test (err == KErrNone);

	if(!aThreadSpecific)
		{
		err = iServSession.SetEventAction(iFileName, EEventsProcessBreakPoint, EActionIgnore);
		test (err == KErrNone);
		}

	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0443
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests registration and occurrence of target thread event (in this case panic)
//! @SYMTestActions     Registers for a panic in the target thread, causes it, and catches the panic notification.
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestEvents()
	{
	TInt err = KErrNone;

	test.Next(_L("TestEvents\n"));

	TInt panicReason = 12345;

	test.Printf(_L("Thread t_rmdebug.exe::DebugThread should panic with reason %d.\n"), panicReason);

	//attach non-passively
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	RThread threadToPanic;
	test(KErrNone == StartDebugThread(threadToPanic, _L("EventsThread")));
	TThreadId threadToPanicId = threadToPanic.Id();
	TEventInfo info;

	// Set things up to wait for a thread kill event
	err = iServSession.SetEventAction(iFileName, EEventsKillThread, EActionContinue);
	test(err==KErrNone);

	// Wait for an event to occur in this process - nothing should have happened yet.
	static TRequestStatus status;

	TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));

	iServSession.GetEvent(iFileName,status,infoPtr);

	// Test Request cancellation
	err = iServSession.CancelGetEvent(iFileName);
	test (err==KErrNone);

	// Again wait for an event to occur in our process - we will provoke the
	// thread kill event by panic'ing the test thread.
	iServSession.GetEvent(iFileName,status,infoPtr);

	// Panic the debug thread to cause a thread kill event
	threadToPanic.Panic(_L("t_rmdebug panic thread test"), panicReason);

	// Wait for notification of the Thread Kill event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// Check we are really recieving information about the panic
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);
	test(info.iProcessId==RProcess().Id());
	test(info.iThreadId==threadToPanicId);
	test(info.iEventType==EEventsKillThread);
	test(info.iThreadKillInfo.iExitType==EExitPanic);

	// Ignore other panic events
	err = iServSession.SetEventAction(iFileName, EEventsKillThread, EActionIgnore);
	test(err==KErrNone);

	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0444
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests registration and occurence of target thread events in separate process.
//! @SYMTestActions     Registers for a hardware exception and kill thread events, and receives them.
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void CRunModeAgent::TestEventsForExternalProcess()
	{
	test.Next(_L("TestEventsForExternalProcess\n"));

	for(TInt main=0; main<3; main++)
		{
		for(TInt extra=0; extra<3; extra++)
			{
			TestEventsWithExtraThreads((TKernelEventAction)main, (TKernelEventAction)extra, 0);
			TestEventsWithExtraThreads((TKernelEventAction)main, (TKernelEventAction)extra, 2);
			}
		}
	}

void CRunModeAgent::TestEventsWithExtraThreads(TKernelEventAction aActionMain, TKernelEventAction aActionExtra, TUint32 aExtraThreads)
	{
	const TInt KNumberOfTypes = 8;
	struct TEventStruct
		{
		public:
		TDebugFunctionType iDebugFunctionType;
		TEventType iEventType;
		};

	TEventStruct type[KNumberOfTypes] =
		{
			{EStackOverflowFunction, EEventsHwExc},
			{EUserPanicFunction, EEventsKillThread},
			{EPrefetchAbortFunction, EEventsHwExc},
			{EDataAbortFunction, EEventsHwExc},
			{EUndefInstructionFunction, EEventsHwExc},
			{EDataReadErrorFunction, EEventsHwExc},
			{EDataWriteErrorFunction, EEventsHwExc},
			{EUserExceptionFunction, EEventsSwExc},
		};

	for(TInt j=0; j<KNumberOfTypes; j++)
		{
		if( gUseDelay ) User::After(500000);

		RDebug::Printf("CRunModeAgent::TestEventsWithExtraThreads type: %d, main action: %d, extra action: %d, extraThreads: %d", 
			j, (TUint32)aActionMain, (TUint32)aActionExtra, aExtraThreads);

		// do this check as it seems to hard to do these cases with the current set up
		if(EEventsKillThread == type[j].iEventType)
			{
			if(EActionSuspend != aActionMain)
				{
				if(aActionMain != aActionExtra)
					{
					return;
					}
				}
			}
		// attach to KRMDebugTestApplication
		test(KErrNone == iServSession.AttachExecutable(KRMDebugTestApplication, EFalse));

		// Set things up to wait for the expected exception in KRMDebugTestApplication
		test(KErrNone == iServSession.SetEventAction(KRMDebugTestApplication, type[j].iEventType, aActionMain));

		if(EActionSuspend != aActionMain)
			{
			test(KErrNone == iServSession.SetEventAction(KRMDebugTestApplication, EEventsKillThread, aActionExtra));
			}

		// declare a TRequestStatus object for asynchronous calls
		TRequestStatus status;

		TEventInfo info;
		TPtr8 infoBuffer = TPtr8((TUint8*)&info,0,sizeof(TEventInfo));
		if(EActionIgnore != aActionMain)
			{
			iServSession.GetEvent(KRMDebugTestApplication(), status, infoBuffer);
			}

		// launch the target process to trigger the expected exception
		RProcess targetProcess;
		test(KErrNone == LaunchProcess(targetProcess, KRMDebugTestApplication(), type[j].iDebugFunctionType, 0, aExtraThreads));
		TProcessId processId(targetProcess.Id());
		targetProcess.Close();

		if(EActionIgnore != aActionMain)
			{
			// wait for notification of the exception
			User::WaitForRequest(status);
			test(KErrNone == status.Int());

			// check that this is the event we were expecting
			test(info.iProcessIdValid);
			test(info.iThreadIdValid);
			test(info.iProcessId==processId);
			test(info.iEventType==type[j].iEventType);
			}

		if(EActionSuspend == aActionMain)
			{
			//RDebug::Printf("CRunModeAgent::TestEventsWithExtraThreads EActionSuspend == aActionMain, j=%d", j);
			// read the thread list, partly to check the call works, and partly to check the thread still exists
			test(ThreadExistsForProcess(info.iThreadId, info.iProcessId));

			// register to catch all the thread kills which will occur
			test(KErrNone == iServSession.SetEventAction(KRMDebugTestApplication, EEventsKillThread, aActionExtra));
			// we specified EActionSuspend earlier so need to call resume on this thread
			test(KErrNone == iServSession.ResumeThread(info.iThreadId));
			}

		// find out how many threads there are in the process and catch all the thread kill events,
		// the number of kill thread events should correspond to the number of extra threads launched,
		// plus one if the main thread panicked with a Sw/Hw exception
		if(EActionIgnore != aActionExtra)
			{
			TInt dyingThreads = aExtraThreads + ( (type[j].iEventType != EEventsKillThread) ? 1 : 0);
			for(TInt k=0; k<dyingThreads; k++)
				{
				//RDebug::Printf("CRunModeAgent::TestEventsWithExtraThreads dyingThreads, k=%d, j=%d", k,j);
				iServSession.GetEvent(KRMDebugTestApplication(), status, infoBuffer);

				// wait for notification of the kill thread
				User::WaitForRequest(status);
				test(KErrNone == status.Int());

				// check that this is the event we were expecting
				test(info.iProcessIdValid);
				test(info.iThreadIdValid);
				test(info.iProcessId==processId);
				test(info.iEventType==EEventsKillThread);
				if(EActionSuspend == aActionExtra)
					{
					// do some calls to check listings work ok at this stage
					test(ProcessExists(info.iProcessId));
					test(ThreadExistsForProcess(info.iThreadId, info.iProcessId));
					// we specified EActionSuspend earlier so need to call resume on this thread
					test(KErrNone == iServSession.ResumeThread(info.iThreadId));
					}
				}
			}

		if( gUseDelay ) User::After(500000);

		// reset the thread kill event
		test(KErrNone == iServSession.SetEventAction(KRMDebugTestApplication(), EEventsKillThread, EActionIgnore));

		// reset events for KRMDebugTestApplication
		test(KErrNone == iServSession.SetEventAction(KRMDebugTestApplication(), type[j].iEventType, EActionIgnore));

		// finished debugging KRMDebugTestApplication so detach
		test(KErrNone == iServSession.DetachExecutable(KRMDebugTestApplication()));
	
		// want to validate that the process has really exited, i.e. we're not accidentally keeping a handle to it...
		TInt waitCount = 10;
		while((waitCount-- > 0) && ProcessExists(processId))
			{
			/* Wait a little while and try again, just in case the process is still being removed.
			This can happen on a very busy system or when a popup for the events is still active
			*/
			RDebug::Printf("CRunModeAgent::TestEventsWithExtraThreads. ProcessExists(id=%d), waiting count exit=%d", 
				I64LOW(processId), waitCount);
			User::After(50000);
			}
		test(!ProcessExists(processId));
		}
	}

// helper function to check whether a thread with id aThreadId exists in the process with id aProcessId
TBool CRunModeAgent::ThreadExistsForProcess(const TThreadId aThreadId, const TProcessId aProcessId)
	{
	RThread lThread;
	TInt ret = lThread.Open( aThreadId.Id() );

	if( ret != KErrNone )
		{
		RDebug::Printf("ThreadExistsForProcess: thread id=%d opening returned %d",
			I64LOW( aThreadId.Id() ), ret );
		lThread.Close();
		return EFalse;
		}

	RProcess lProcess;
	ret = lThread.Process( lProcess );

	lThread.Close();

	if( ret != KErrNone )
		{
		RDebug::Printf("ThreadExistsForProcess: proc opening returned %d", ret );
		ret = KErrNotFound;
		}
	else if( lProcess.Id() != aProcessId )
		{
		RDebug::Printf("ThreadExistsForProcess: lProcess.Id()(%d)!= aProcessId(%d)",
				I64LOW(lProcess.Id().Id()), I64LOW(aProcessId.Id()));
		ret = KErrNotFound;
		}

	lProcess.Close();
	
	return ( ret == KErrNone );
	}

// helper function to check whether a process with id aProcessId exists
TBool CRunModeAgent::ProcessExists(const TProcessId aProcessId)
	{
	TUint32 size;
	RBuf8 buffer;
	test(KErrNone == buffer.Create(1024));
	TInt err = iServSession.GetList(EProcesses, buffer, size);
	while(KErrTooBig == err)
		{
		size*=2;
		test(size<=16*1024);
		test(KErrNone == buffer.ReAlloc(size));
		err = iServSession.GetList(EProcesses, buffer, size);
		}
	test(KErrNone == err);

	//look through the buffer and check if the target debug thread is there
	TUint8* ptr = (TUint8*)buffer.Ptr();
	const TUint8* ptrEnd = ptr + size;
	while(ptr < ptrEnd)
		{
		TProcessListEntry& entry = *(TProcessListEntry*)ptr;
		if(aProcessId.Id() == entry.iProcessId)
			{
			buffer.Close();
			return ETrue;
			}
		ptr += Align4(entry.GetSize());
		}
	buffer.Close();
	return EFalse;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0445
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests basic debug functions work on demand-paged target threads
//! @SYMTestActions     Checks it can r/w memory, set breakpoints etc in a demand paged target.
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestDemandPaging(void)
	{
	test.Next(_L("TestDemandPaging\n"));

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	// get the address of a function in code that will be paged in
	TUint32 address = (TUint32)(&RMDebugDemandPagingTest);
	const TUint32 armInstSize = 4;

	// read the memory at &RMDebugDemandPagingTest to check that reading memory in demand paged code works
	TUint32 demandPagedInst = 0;
	TPtr8 demandPagedInstBuf((TUint8*)&demandPagedInst, armInstSize);
	test(KErrNone == iServSession.ReadMemory(iThreadID, address, armInstSize, demandPagedInstBuf, EAccess32, EEndLE8));

	// this is the MOVS instruction that we expect to find in RMDebugDemandPagingTest
	TUint32 expectedDemandPagedInst = 0xe1b02000;

	// check that the instruction we read is as expected
	test(demandPagedInst == expectedDemandPagedInst);

	// set event action for break points
	test(KErrNone == iServSession.SetEventAction(RProcess().FileName(), EEventsBreakPoint, EActionContinue));

	// set an arm breakpoint on RMDebugDemandPagingTest
	TBreakId armBreakId = 0;
	test(KErrNone == iServSession.SetBreak(armBreakId, iThreadID, address, EArmMode));

	// Ensure that after setting the breakpoint the memory read returns the correct value
	TUint32 demandPagedInstWithBreakPoint = 0;
	TPtr8 spinForeverInstWithBreakPointBuf((TUint8*)&demandPagedInstWithBreakPoint, armInstSize);
	test(KErrNone == iServSession.ReadMemory(iThreadID, address, armInstSize, spinForeverInstWithBreakPointBuf, EAccess32, EEndLE8));
	test(demandPagedInst == demandPagedInstWithBreakPoint);

	// switch the target thread to run the demand paging function
	test(KErrNone == SwitchTestFunction(EDemandPagingFunction));

	// set up event watcher to catch breakpoint being hit in demand paged code
	TEventInfo info;
	static TRequestStatus status;
	TPtr8 infoPtr((TUint8*)&info,sizeof(TEventInfo));
	iServSession.GetEvent(RProcess().FileName(), status, infoPtr);

	// resume the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));
	// wait for notification of the breakpoint hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// info should now be filled with the details
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);
	test(info.iEventType == EEventsBreakPoint);
	test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address);

	// remove the break point and resume the thread
	test(KErrNone == iServSession.ClearBreak(armBreakId));

	// switch the target thread to run the default function
	test(KErrNone == SwitchTestFunction(EDefaultFunction));

	test(KErrNone == iServSession.ResumeThread(iThreadID));
	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

// Names of some test programs used for testing security
_LIT(KRMDebugSecurity0FileName,"z:\\sys\\bin\\t_rmdebug_security0.exe"); // Debuggable
_LIT(KRMDebugSecurity1FileName,"z:\\sys\\bin\\t_rmdebug_security1.exe"); // Not debuggable
_LIT(KRMDebugSecurity2FileName,"z:\\sys\\bin\\t_rmdebug_security2.exe"); // AllFiles
_LIT(KRMDebugSecurity3FileName,"z:\\sys\\bin\\t_rmdebug_security3.exe"); // TCB AllFiles

// include the test header file here
#include "rm_debug_kerneldriver.h"

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0446
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests Debug Device Driver is locked to the SID of the Debug Security Svr.
//! @SYMTestActions     Loads rm-debug.ldd and tries to open a handle to it. This should fail.
//!
//! @SYMTestExpectedResults KErrPermissionDenied.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestDriverSecurity(void)
	{
	test.Next(_L("TestDriverSecurity\n"));

	RRM_DebugDriver kernelDriver;

	// Load the debug device driver
	TInt err = User::LoadLogicalDevice( KDebugDriverFileName );
	test((KErrNone == err) || (KErrAlreadyExists == err));

	// we were allowed to load the driver, or its already loaded.

	// Try to open a handle to the driver - this should return KErrPermissionDenied as we don't have the DSS SID
	TRM_DebugDriverInfo driverInfo;
	driverInfo.iUserLibraryEnd = 0;
	err = kernelDriver.Open(driverInfo);
	test((err == KErrInUse) || (err == KErrPermissionDenied));

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0447
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests Debug driver can only be access via the DSS. Also tests DSS cannot
//!						be subverted. Tests functionality of two representative OEM Debug Tokens.
//! @SYMTestActions     Tries to open rm_debug.ldd (should fail). Tries to debug various processes
//!						(only debuggable one should succeed). Checks that DSS behaves correctly
//!						when different versions are passed in to Connect().
//!
//! @SYMTestExpectedResults KErrPermissionDenied.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestSecurity(void)
	{
	// Things to test
	//
	// try to use debug driver directly ( should have the wrong UID/SID value!)
	test.Next(_L("TestSecurity - Bypass Debug Security Server to Debug Device Driver - DSS running\n"));

	// Things to test
	//
	// Load the debug device driver
	RRM_DebugDriver kernelDriver;
	TInt err = User::LoadLogicalDevice( KDebugDriverFileName );
	test((KErrNone == err) || (KErrAlreadyExists == err));

	// we were allowed to load the driver, or its already loaded.

	// Try to open handle a to the driver - this should return KErrPermission/KErrInUse as we don't have the DSS SID
	// and we expect the DSS to already be using it.
	TRM_DebugDriverInfo driverInfo;
	driverInfo.iUserLibraryEnd = 0;
	err = kernelDriver.Open(driverInfo);
	test(err == KErrInUse);

	// Try requesting an unsupported version of DSS
	test.Next(_L("TestSecurity - requesting unsupported versions of DSS\n"));
	RSecuritySvrSession dss;
	err = dss.Connect(TVersion(999999, 0, 0));
	test(err == KErrNotSupported); // Prior to DEF142018 this would crash, causing a KErrServerTerminated
	err = dss.Connect(TVersion(KDebugServMajorVersionNumber, 999999, 0));
	test(err == KErrNotSupported); // Explicitly asking for a minor version should give KErrNotSupported too if it's newer than what's running.
	err = dss.Connect(TVersion(KDebugServMajorVersionNumber, 0, 0));
	test(err == KErrNone); // But the correct major version and no explicit minor version should always succeed
	dss.Close();
	
	//
	// Attach to the Debug Security Server (passive)
	//
	test.Next(_L("TestSecurity - Attach to the Debug Security Server (passive)\n"));

	_LIT(KSecurityServerProcessName, "z:\\sys\\bin\\rm_debug_svr.exe");

	test(KErrPermissionDenied == iServSession.AttachExecutable(KSecurityServerProcessName, ETrue));

	//
	// Attach to the Debug Security Server (active)
	//
	test.Next(_L("TestSecurity - Attach to the Debug Security Server (active)\n"));

	test(KErrPermissionDenied == iServSession.AttachExecutable(KSecurityServerProcessName, EFalse));

	//
	// Attach to Process 0
	//
	// Target: Debuggable
	//
	test.Next(_L("TestSecurity - Attach to test process 0\n"));

	// Agent can debug the target app as it is marked debuggable - ie capabilities are ignored)
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity0FileName,ETrue);

	//
	// Attach to Process - 1
	//
	// Target: Non-debuggable for ordinary debug agent, debuggable for OEM/OEM2 token authorised agent
	//
	// Note: This target app has no PlatSec capabilities
	//
	// Agent cannot debug the app unless it has an OEM/OEM2 Debug Token
	test.Next(_L("TestSecurity - Attach to test process 1\n"));

#ifdef SYMBIAN_STANDARDDEBUG
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity1FileName,EFalse);
#endif

#ifdef SYMBIAN_OEMDEBUG
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity1FileName,ETrue);
#endif

#ifdef SYMBIAN_OEM2DEBUG
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity1FileName,ETrue);
#endif

	//
	// Attach to Process - 2
	//
	// Target: Non-debuggable for ordinary debug agent, non-debuggable for OEM2 authorised agent (insufficient caps)
	//
	// Note: This target app has AllFiles capability
	//
	// Agent cannot debug the app unless it has an OEM Debug Token
	test.Next(_L("TestSecurity - Attach to test process 2\n"));

#ifdef SYMBIAN_STANDARDDEBUG
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity2FileName,EFalse);
#endif

#ifdef SYMBIAN_OEMDEBUG
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity2FileName,ETrue);
#endif

#ifdef SYMBIAN_OEM2DEBUG
	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity2FileName,EFalse);
#endif

	//
	// Attach to Process - 3
	//
	// Target: Non-debuggable for ordinary debug agent, non-debuggable for OEM authorised agent (insufficient caps)
	//
	// Note: This target app has AllFiles and TCB and NetworkControl capabilities
	//
	test.Next(_L("TestSecurity - Attach to test process 3\n"));

	HelpTestSecurityAttachDetachExecutable(KRMDebugSecurity3FileName,EFalse);

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0543
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Validates that a dll can be built which #include's the rm_debug_api.h header, i.e. rm_debug_api.h contains no static data.
//! @SYMTestActions     Calls a dummy function in t_rmdebug_dll.dll which implies the dll has been built correctly.
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void CRunModeAgent::TestDllUsage(void)
	{
	test.Next(_L("TestDllUsage\n"));
	test(KUidDebugSecurityServer == GetDSSUid());
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0812
//! @SYMTestType
//! @SYMPREQ            PREQ1700
//! @SYMTestCaseDesc    Writes a known data to the crash flash and validates the data written
//!						using the read operation and finally erase the data. In the absence
//!						of an OEM debug token, access to the crash partition should not be allowed
//! @SYMTestActions     Invoke the flash write method in DSS and call the read method in DSS
//!						to validate the data is written correctly and then erase the written area
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void CRunModeAgent::TestCrashFlash(void)
	{
#if  defined (SYMBIAN_STANDARDDEBUG)  || defined (SYMBIAN_OEM2DEBUG)

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-006 Testing We cannot Erase the Crash Flash with insufficient privileges"));

	TUint32 size = 0;
	TInt err = iServSession.EraseCrashLog(0, 1);
	test(KErrPermissionDenied == err);

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-005 Testing We can't Write to the Crash Flash with insufficient privileges"));

	err = iServSession.WriteCrashConfig(0, KCrashDummyData, size);
	test(KErrPermissionDenied == err);
	test(size == 0);

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-008 Testing We can't Read from the Crash Flash with insufficient privileges"));

	TUint32 readSize = 0x10;
	RBuf8 buf;
	buf.CleanupClosePushL();
	err = buf.Create(readSize);

	test(err == KErrNone);

	err = iServSession.ReadCrashLog(0, buf, readSize);
	test(KErrPermissionDenied == err);

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-004 Testing Writing To an invalid location"));

	TUint32 writeSize = 0;
	err = iServSession.WriteCrashConfig(0xFFFFFFFF, KCrashDummyData, writeSize);

	test(err == KErrPermissionDenied);

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-003 Testing Reading from an invalid location"));

	buf.FillZ();
	err = iServSession.ReadCrashLog(0, buf, writeSize);

	test(err == KErrPermissionDenied);

	CleanupStack::PopAndDestroy(&buf);

#endif

#ifdef SYMBIAN_OEMDEBUG

	TInt err = KErrNone;

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-007 Testing We can Erase the Crash Flash with sufficient privileges"));

	err = iServSession.EraseCrashLog(0, 1);

	// For platforms where NAND flash is not currently supported we get a KErrNotSupported - this is still a pass
	if (KErrNotSupported == err)
		{
		test.Printf(_L("Nand flash not supported - continue"));
		return;
		}

 	//For platforms without a flash partition we get KErrNotFound - this is still a pass
 	if(KErrNotFound == err)
 		{
 		test.Printf(_L("Platform has no flash partition - continue"));
 		return;
 		}

	test(KErrNone == err);

	//Read back the start of the block to make sure its 0xFFFFFFFF
	const TUint numBytesToCheck = 0x80;  //We dont know the block size
	TBuf8<numBytesToCheck> eraseCheck;
	eraseCheck.SetLength(numBytesToCheck);

	err = iServSession.ReadCrashLog(0, eraseCheck, numBytesToCheck);
	test(err == KErrNone);

	TBool dataIsOk = ETrue;
	for(TUint cnt = 0; cnt < numBytesToCheck; cnt++)
		{
		if(eraseCheck[cnt] != 0xFF)
			{
			dataIsOk = EFalse;
			}
		}

	test(dataIsOk);

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-002 Testing We can Write to the Crash Flash with sufficient privileges"));

	TUint32 writeSize = 0;
	err = iServSession.WriteCrashConfig(0, KCrashDummyData, writeSize);

	test(writeSize == KCrashDummyData().Length());

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-001 Testing We can Read from the Crash Flash with sufficient privileges"));

	RBuf8 buf;
	buf.CleanupClosePushL();
	err = buf.Create(writeSize);

	test(err == KErrNone);

	buf.FillZ();

	err = iServSession.ReadCrashLog(0, buf, writeSize);

	test(0 == buf.Compare(KCrashDummyData));

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-004 Testing Writing To an invalid location"));

	writeSize = 0;
	err = iServSession.WriteCrashConfig(0xFFFFFFFF, KCrashDummyData, writeSize);

	test(err == KErrArgument);

	test.Next(_L("@SYMTestCaseID:DT-debug-securityserver-003 Testing Reading from an invalid location"));

	buf.FillZ();
	err = iServSession.ReadCrashLog(0xFFFFFFFF, buf, writeSize);

	test(err == KErrArgument);

	CleanupStack::PopAndDestroy(&buf);

#endif
	}
//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0735
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests the Kill Process functionality. Only can kill a debuggable process.
//! @SYMTestActions     Launches a debuggable and non-debuggable process and tries to kill both.
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void CRunModeAgent::TestKillProcess(void)
	{
	test.Next(_L("TestKillProcess\n"));

	// Kill a debuggable process

	// check that killing a process is supported
	TTag tag = GetTag(ETagHeaderIdKillObjects, EFunctionalityKillProcess);
	test(tag.iValue);
	// check that killing a thread is not supported
	tag = GetTag(ETagHeaderIdKillObjects, EFunctionalityKillThread);
	test(!tag.iValue);

	// attach first!
	TInt err = iServSession.AttachExecutable(KRMDebugTestApplication, EFalse /* Active */);
	test(err == KErrNone);

	// first launch a debuggable process
	RProcess process;
	err = LaunchProcess(process, KRMDebugTestApplication(),ESpinForever, 0, 0);
	test (err == KErrNone);

	// try to find the process in the list
_LIT(KRMDebugAppName, "t_rmdebug_app");

	TBool found = ProcessExists(KRMDebugAppName);
	test (found);

	TInt processId = process.Id();
	process.Close();

	// program now running, so try to kill it
	err = iServSession.KillProcess(processId, 0 /* kill reason */);
	test(err == KErrNone);

	User::After(2000000);	// should die within two seconds.

	// can we still find it? Should be gone
	found = ProcessExists(KRMDebugAppName);
	test (!found);

	// release the program again.
	err = iServSession.DetachExecutable(KRMDebugTestApplication);
	test(err == KErrNone);

	// Try to kill a non-debuggable process and fail.

	// first launch a non-debuggable process
	RProcess process2;
	err = LaunchProcess(process2, KRMDebugSecurity1FileName(),ESpinForever, 0, 0);
	test (err == KErrNone);

	// try to find the process in the list
_LIT(KRMDebugAppName2, "t_rmdebug_security1");

	TBool found2 = ProcessExists(KRMDebugAppName2);
	test (found2);

	TInt process2Id = process2.Id();
	process2.Close();

	// program now running, so try to kill it
	err = iServSession.KillProcess(process2Id, 0 /* kill reason */);
	test(err == KErrPermissionDenied);

	User::After(2000000);	// should die within two seconds if it is going to die.

	// can we still find it? Should be still around!
	found2 = ProcessExists(KRMDebugAppName2);
	test (found2);

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-1388
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Tests the correct operation of the AddProcess and Remove Process
//! @SYMTestActions     1. Registers for AddProcess and Remove Process events
//!                     2. Starts a test process z:\sys\bin\t_rmdebug_security0.exe
//!                     3. Wait for the AddProcess event to be reported
//!                     4. Kill the newly started test process
//!                     5. Wait for the RemoveProcess event to be reported
//!                     6. Tell the DSS it is no longer interested in AddProcess and RemoveProcess events
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestAddRemoveProcessEvents()
	{
	test.Next(_L("TestAddRemoveProcessEvents\n"));

	// attach to a process (e.g. one of the simple security test programs)
	// launch the security program
	// wait for the add event
	// continue the program.
	// wait for the remove event
	// detach process

	test(KErrNone == iServSession.AttachExecutable(KRMDebugSecurity0FileName, EFalse));

	test(KErrNone == iServSession.SetEventAction(KRMDebugSecurity0FileName,EEventsAddProcess, EActionContinue));

	test(KErrNone == iServSession.SetEventAction(KRMDebugSecurity0FileName,EEventsRemoveProcess, EActionContinue));

	// Creator thread ID of the current thread (to be creator of test application)
	TInt creatorThreadId = RThread().Id();

	RProcess process;
	TInt err = process.Create(KRMDebugSecurity0FileName, KNullDesC, EOwnerProcess);
	test (err == KErrNone);

	// Rendezvous with process
	TRequestStatus status;
	process.Rendezvous(status);

	// Start the test program
	process.Resume();
	User::WaitForRequest(status);
	test(status==KErrNone);

	// Wait for the addprocess event
	TEventInfo info;
	TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));

	iServSession.GetEvent(KRMDebugSecurity0FileName,status,infoPtr);

	// Wait for notification of the addprocess hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// Check this was the right kind of event
	test(info.iEventType == EEventsAddProcess);

	const TInt uid3offset = 2;

	// Get UID3 for current process
	TUint32 Uid3 = process.Type()[uid3offset].iUid;

	// Check correct UID3 is returned from the driver
    test(info.iAddProcessInfo.iUid3 == Uid3);

    // Check correct creator ID for test application is returned from the driver
    test(info.iAddProcessInfo.iCreatorThreadId == creatorThreadId);

	// Kill the process, as we don't need it anymore
	process.Kill(KErrNone);

	// Wait for the remove process event
	iServSession.GetEvent(KRMDebugSecurity0FileName,status,infoPtr);

	// Wait for notification of the remove process hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// Check this was the right kind of event
	test(info.iEventType == EEventsRemoveProcess);

	test(KErrNone == iServSession.SetEventAction(KRMDebugSecurity0FileName,EEventsRemoveProcess, EActionIgnore));

	test(KErrNone == iServSession.SetEventAction(KRMDebugSecurity0FileName,EEventsAddProcess, EActionIgnore));

	test(KErrNone == iServSession.DetachExecutable(KRMDebugSecurity0FileName));

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-0736
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Checks that process break points can be set, and that they can co-exist alongside thread breakpoints
//! @SYMTestActions     Checks that process break points can be set, and that they can co-exist alongside thread breakpoints
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
void CRunModeAgent::TestProcessBreakPoints(void)
	{
	test.Next(_L("TestProcessBreakPoints\n"));

	// check that process breakpoints are supported
	TTag tag = GetTag(ETagHeaderIdBreakpoints, EBreakpointProcess);
	test(tag.iValue);

	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	// Try to set the breakpoint
	TBreakId breakId;
	TUint32 address = (TUint32)(&RMDebug_BranchTst1);
	RProcess process;
	TProcessId processId = process.Id();
	process.Close();

	test(KErrNone == iServSession.SetProcessBreak(breakId, processId, address, EArmMode));
	test(KErrAlreadyExists == iServSession.SetBreak(breakId, iThreadID, address, EArmMode));
	test(KErrAlreadyExists == iServSession.SetBreak(breakId, iThreadID, address, EThumbMode));
	test(KErrAlreadyExists == iServSession.SetProcessBreak(breakId, processId, address, EArmMode));
	test(KErrAlreadyExists == iServSession.SetProcessBreak(breakId, processId, address, EThumbMode));
	test(KErrNone == iServSession.ClearBreak(breakId));

	test(KErrNone == iServSession.SetBreak(breakId, iThreadID, address, EArmMode));
	test(KErrAlreadyExists == iServSession.SetProcessBreak(breakId, processId, address, EArmMode));
	test(KErrAlreadyExists == iServSession.SetProcessBreak(breakId, processId, address, EThumbMode));
	test(KErrNone == iServSession.ClearBreak(breakId));

	test(KErrNone == iServSession.ResumeThread(iThreadID));

	test(KErrNone == iServSession.DetachExecutable(iFileName));
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBase-T-RMDEBUG2-1309
//! @SYMTestType
//! @SYMPREQ            PREQ1426
//! @SYMTestCaseDesc    Checks that in the case of multiple low priority events (user traces in this case) we can still receive higher
//!				priority events should the buffer reach a critical level
//! @SYMTestActions     Run to first breakpoint in our test code. Then multiple trace events are issued. We should still be able to hit
//!				the second breakpoint
//!
//! @SYMTestExpectedResults KErrNone.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestMultipleTraceEvents(void)
	{
	test.Next(_L("TestMultipleTraceEvents\n"));

	//attach to target debug process
	test(KErrNone == iServSession.AttachExecutable(iFileName, EFalse));

	//and suspend the thread
	test(KErrNone == iServSession.SuspendThread(iThreadID));

	//register interest in BP's & trace events and trace ignored events
	test(KErrNone == iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionSuspend));
	test(KErrNone == iServSession.SetEventAction(iFileName,EEventsUserTrace, EActionContinue));
	test(KErrNone == iServSession.SetEventAction(iFileName,EEventsUserTracesLost, EActionContinue));

	// Try to set the breakpoints
	TBreakId armBreakId;
	TBreakId armBreakId2;
	TUint32 address = (TUint32)(&RMDebug_BranchTst1);
	TUint32 address2 = (TUint32)(&RMDebug_StepTest_Non_PC_Modifying);

	test(KErrNone == iServSession.SetBreak(armBreakId,iThreadID,address,EArmMode));
	test(KErrNone == iServSession.SetBreak(armBreakId2,iThreadID,address2,EArmMode));

	//set the target thread to execute the trace test function
	test(KErrNone == SwitchTestFunction(EMultipleTraceCalls));

	// Continue the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));

	// wait for the breakpoint to be hit
	TEventInfo info;
	static TRequestStatus status;

	TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));
	iServSession.GetEvent(iFileName,status,infoPtr);

	// Wait for notification of the 1st breakpoint hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// info should now be filled with the details
	test(info.iEventType == EEventsBreakPoint);
	test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address);
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);

	// Continue the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));

	//Now we try to hit the second breakpoint. This will occur after a number of trace calls. If we hit this breakpoint it
	//means many trace calls are not preventing us hitting breakpoints.
	iServSession.GetEvent(iFileName,status,infoPtr);

	// Wait for notification of the 2nd breakpoint hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	TBool receivedTracesLost = EFalse;

	while(info.iEventType == EEventsUserTrace || info.iEventType == EEventsUserTracesLost)
		{
		//ensure we get told traces are being thrown away - we generate enough to flood the buffer
		if(info.iEventType == EEventsUserTracesLost)
			{
			receivedTracesLost = ETrue;

			// Now stop the target thread from generating trace events
			test(KErrNone == SwitchTestFunction(EDoNothing));
			}
		else
			{
			// Its EEventsUserTrace, so delay us in getting the next event so that it will be more 
			// likely to get a EEventsUserTracesLost next time. 
			// This is important on SMP since the platform can process lots of events, and thus
			// withouth the delay it is difficult for this test to reproduce the abnormal situation of 
			// lost trace packets
			User::After(200000);
			}

		iServSession.GetEvent(iFileName,status,infoPtr);

		// Wait for notification of the 2nd breakpoint hit event
		User::WaitForRequest(status);
		test(status==KErrNone);
		}

	//make sure we got told traces were lost
	test(receivedTracesLost != EFalse);

	// info should now be filled with the details of our breakpoint.
	test(info.iEventType == EEventsBreakPoint);
	test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address2);
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);

	//dont care for breakpoints or trace events no more
	test(KErrNone == iServSession.SetEventAction(iFileName,EEventsBreakPoint, EActionIgnore));
	test(KErrNone == iServSession.SetEventAction(iFileName,EEventsUserTrace, EActionIgnore));
	test(KErrNone == iServSession.SetEventAction(iFileName,EEventsUserTracesLost, EActionIgnore));

	//clear the breaks we set
	test(KErrNone == iServSession.ClearBreak(armBreakId));
	test(KErrNone == iServSession.ClearBreak(armBreakId2));

	// Continue the thread
	test(KErrNone == iServSession.ResumeThread(iThreadID));

	//attach to target debug process
	test(KErrNone == iServSession.DetachExecutable(iFileName));

	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID KBase-T-RMDEBUG2-2441
//! @SYMTestType
//! @SYMPREQ PREQ1426
//! @SYMTestCaseDesc Test clearing of a process breakpoint once the process has been killed.
//! @SYMTestActions Creates a new process then tries to set a process breakpoint and then kills the process which should clear the previously set breakpoint. Then repeat the step once again.
//! @SYMTestExpectedResults KErrNone
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//----------------------------------------------------------------------------------------------

void CRunModeAgent::TestProcessKillBreakpoint(void)
	{
	test.Next(_L("TestProcessKillBreakpoint\n"));
	//SID retrieved, used in Define/Attach of the property
	iMySid.iUid = RProcess().SecureId();

	static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);

	//define a property to pass on the address from the other process we would try to debug
	test ( KErrNone == RProperty::Define(iMySid, EMyPropertyInteger, RProperty::EInt, KAllowAllPolicy, KAllowAllPolicy));
	    //define a global semaphore to synchronise with debuggable process publishing the property
	test ( KErrNone == iAddressGlobSem.CreateGlobal(_L("RMDebugGlobSem"), 0) );

	DoTestProcessKillBreakpoint();
	// called once again
	// to check if we can set the breakpoint once again after the process gets killed
	DoTestProcessKillBreakpoint();

	//delete the property
	test ( KErrNone == RProperty::Delete(iMySid, EMyPropertyInteger));
	//close the semaphore
	iAddressGlobSem.Close();
	}

void CRunModeAgent::DoTestProcessKillBreakpoint()
	{
	test.Printf(_L("\nDoTestProcessKillBreakpoint\n"));

	TInt err = KErrNone;

	// check that killing a process is supported
	TTag tag = GetTag(ETagHeaderIdKillObjects, EFunctionalityKillProcess);
	test(tag.iValue);
	// check that killing a thread is not supported
	tag = GetTag(ETagHeaderIdKillObjects, EFunctionalityKillThread);
	test(!tag.iValue);

	// attach first!
	test ( KErrNone == iServSession.AttachExecutable(KRMDebugTestApplication, EFalse/*  Active */));

	// first launch a debuggable process
	RProcess processDebug;
	test ( KErrNone == LaunchProcess(processDebug, KRMDebugTestApplication(),ESpinForeverWithBreakPoint, 0, 0));

	// try to find the process in the list
	_LIT(KRMDebugAppName, "t_rmdebug_app");
	TBool found = ProcessExists(KRMDebugAppName);
	test (found);

	//search for the main thread created
   _LIT(KThreadWildCard, "t_rmdebug_app*");
	TProcessId processDebugId = processDebug.Id();
	TThreadId threadDebugId;

   	TFindThread find(KThreadWildCard);
	TFullName name;
	found = EFalse;
	while(find.Next(name)==KErrNone && !found)
		{
		RThread thread;
		err = thread.Open(find);
       	if (err == KErrNone)
			{
			RProcess process;
			thread.Process(process);
			if (((TUint32)process.Id() == processDebugId))
				{
				TFullName fullname = thread.FullName();
				test.Printf(_L("Match Found Name %lS Process ID%ld Thread Id %ld"), &fullname, process.Id().Id(), thread.Id().Id());
				found = ETrue;
				threadDebugId = thread.Id();
				}
			process.Close();
			}
			thread.Close();
   		}

	test (found); //check if we actually found the thread we want to debug

	//get the value(property) for the breakpoint address for the process to debug
	TInt address;
	RProperty integerProperty;
	test ( KErrNone == integerProperty.Attach(iMySid, EMyPropertyInteger, EOwnerThread));

	//waiting on semaphore to be sure that the property is set
	iAddressGlobSem.Wait();

	test ( KErrNone == integerProperty.Get(address));
	integerProperty.Close();

	test.Printf(_L("Address retrieved to set breakpoint 0x%8x"), address);

	//suspend the thread before we set a breakpoint
	test ( KErrNone == iServSession.SuspendThread(threadDebugId));

	//set a process breakpoint
	TBreakId breakId;
	test(KErrNone == iServSession.SetProcessBreak(breakId, processDebugId, address, EArmMode));

	test(KErrNone ==iServSession.SetEventAction(KRMDebugTestApplication,EEventsProcessBreakPoint, EActionContinue));

	//resume the thread now
	test(KErrNone == iServSession.ResumeThread(threadDebugId));

	// wait for the breakpoint to be hit
	static TRequestStatus status;
	TEventInfo info;
	TPtr8 infoPtr((TUint8*)&info,0,sizeof(TEventInfo));
	iServSession.GetEvent(KRMDebugTestApplication,status,infoPtr);
	// Wait for notification of the breakpoint hit event
	User::WaitForRequest(status);
	test(status==KErrNone);

	// info should now be filled with the details
	test(info.iEventType ==  EEventsProcessBreakPoint);
	test(info.iThreadBreakPointInfo.iRmdArmExcInfo.iR15 == address);
	test(info.iProcessIdValid);
	test(info.iThreadIdValid);

	// Not interested in breakpoint events any more
	test(KErrNone == iServSession.SetEventAction(KRMDebugTestApplication, EEventsProcessBreakPoint, EActionIgnore));

	// program now running, so try to kill it which should clear all the breakpoints
	test(KErrNone == iServSession.KillProcess(processDebugId, 0  /* kill reason */ ));

	processDebug.Close();
	User::After(2000000);	// should die within two seconds.

	// can we still find it? Should be gone
	found = ProcessExists(KRMDebugAppName);
	test (!found);

	// release the program again
	test(KErrNone == iServSession.DetachExecutable(KRMDebugTestApplication));

	}

void CRunModeAgent::HelpTestSecurityAttachDetachExecutable(const TDesC& aProcessName, TBool aExpectSuccess)
	{
	RProcess process;
	TInt err = process.Create(aProcessName, KNullDesC, EOwnerProcess);
	test (err == KErrNone);

	// rendezvous with process
	TRequestStatus status;
	process.Rendezvous(status);

	// start the test program
	process.Resume();
	User::WaitForRequest(status);
	test(status==KErrNone);

	// attach to the program (passively)
	err = iServSession.AttachExecutable(aProcessName, EFalse);

	if( gUseDelay ) User::After(500000);

	// Do we expect to successfully attach
	if (aExpectSuccess)
	{
		// Yes
		test(KErrNone == err);

		// Now detach again
		test(KErrNone == iServSession.DetachExecutable(aProcessName));
		if( gUseDelay ) User::After(500000);
	}
	else
	{
		// No
		test(KErrPermissionDenied == err);

		// Just to be sure, try active attachment
		test(KErrPermissionDenied == iServSession.AttachExecutable(aProcessName, ETrue));
		if( gUseDelay ) User::After(500000);
	}

	// Kill the process, as we don't need it anymore
	process.Kill(KErrNone);
	if( gUseDelay ) User::After(500000);
	}

void CRunModeAgent::ReportPerformance(void)
//
// Reports performance metrics from all the tests
//
	{
	test.Printf(_L("\nPerformance\n"));
	test.Printf(_L("========================\n"));

	// Memory
	test.Printf(_L("Memory read: %d KBytes/sec\n"),iMemoryReadKbytesPerSecond);
	test.Printf(_L("Memory write: %d KBytes/sec\n"),iMemoryWriteKbytesPerSecond);

	// Registers
	// to do

	// events
	// to do

	// Breakpoints
	test.Printf(_L("Breakpoint set/clear: %d/sec\n"),iBreakpointsPerSecond);
	test.Printf(_L("Maximum number of breakpoints: %d\n"),iMaxBreakpoints);

	// Stepping
	test.Printf(_L("Stepping speed: %d/sec\n"),iStepsPerSecond);

	// Runtime
	TInt ticks = HelpGetTestTicks();
	test (ticks != 0);

	TInt nkTicksPerSecond = HelpTicksPerSecond();
	test (nkTicksPerSecond != 0);

	test.Printf(_L("Total test runtime: %d seconds\n"),ticks/nkTicksPerSecond);

	// Final sizes of executables/rom/ram etc
	// to do

	test.Printf(_L("\n"));
	}

/**
 * Helper code for the stepping tests. Sets a breakpoint in a running thread.
 * It suspends the thread, sets the breakpoint, and resumes the thread.
 *
 * @param aBreakId - Reference to a TBreakId which will be set when the breakpoint is set
 * @param aThreadId - The thread id for which we should set the breakpoint.
 * @param aBreakAddress - The address to set the breakpoint
 * @param aMode - The architecture of the breakpoint to be set (ARM/Thumb/Thumb2EE)
 * @return KErrNone if successful. One of the other system wide error codes otherwise.
 */
TInt CRunModeAgent::HelpTestStepSetBreak(TBreakId& aBreakId, TThreadId aThreadId, const TUint32 aBreakAddress, TArchitectureMode aMode, TBool aThreadSpecific, TProcessId aProcessId)
	{
	TInt err = KErrNone;

	// Suspend the thread
	err = iServSession.SuspendThread(aThreadId);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStepSetBreak - Failed to suspend thread\n"));
		return err;
		}

	// Set the breakpoint
	err = aThreadSpecific
		? iServSession.SetBreak(aBreakId,aThreadId,aBreakAddress,aMode)
		: iServSession.SetProcessBreak(aBreakId, aProcessId, aBreakAddress, aMode);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStepSetBreak - Failed to set breakpoint\n"));
		return err;
		}

	// Continue the thread
	err = iServSession.ResumeThread(aThreadId);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStepSetBreak - Failed to resume thread\n"));
		return err;
		}

	return KErrNone;
	}

/**
 * Helper code for the stepping tests. Clears a breakpoint in a running thread.
 * It suspends the thread, clears the breakpoint, and resumes the thread.
 *
 * @param aBreakId - Reference to a TBreakId which will be set when the breakpoint is set
 * @return KErrNone if successful. One of the other system wide error codes otherwise.
 */
TInt CRunModeAgent::HelpTestStepClearBreak(TBreakId aBreakId, const TThreadId aThreadId, TBool aThreadSpecific)
	{
	TInt err = KErrNone;

	// Find out what thread id we need to suspend
	TThreadId threadId;
	TProcessId processId;
	TUint32 address;
	TArchitectureMode mode;

	err = aThreadSpecific
		? iServSession.BreakInfo(aBreakId, threadId, address, mode)
		: iServSession.ProcessBreakInfo(aBreakId, processId, address, mode);
	if (err != KErrNone )
		{
		test.Printf(_L("HelpTestStepClearBreak - failed to obtain information for breakpoint\n"));
		return err;
		}
	if(aThreadSpecific && aThreadId != threadId)
		{
		test.Printf(_L("HelpTestStepClearBreak - mismatched thread Ids\n"));
		return KErrGeneral;
		}

	// Suspend the thread
	err = iServSession.SuspendThread(aThreadId);
	if (!(err == KErrNone || err == KErrAlreadyExists))
		{
		test.Printf(_L("HelpTestStepClearBreak - failed to suspend thread\n"));
		return err;
		}

	// Clear the breakpoint
	err = iServSession.ClearBreak(aBreakId);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStepClearBreak - failed to clear breakpoint\n"));
		return err;
		}

	// Continue the thread
	err = iServSession.ResumeThread(aThreadId);
	if (!(err == KErrNone || err == KErrNotFound))
		{
		test.Printf(_L("HelpTestStepClearBreak - failed to resume thread\n"));
		return err;
		}

	return KErrNone;
	}

/**
 * Helper code for the stepping tests. Waits for a previously set breakpoint to be hit.
 *
 * @param aProcessName - The name of the process in which the breakpoint is set. E.g. z:\sys\bin\app.exe
 * @param aEventInfo - The event information block which is filled in when the breakpoint is hit.
 * @return KErrNone if successful. One of the other system wide error codes otherwise.
 */
TInt CRunModeAgent::HelpTestStepWaitForBreak(const TDesC& aProcessName, TEventInfo& aEventInfo)
	{
	static TRequestStatus status;

	TPtr8 infoPtr((TUint8*)&aEventInfo,0,sizeof(TEventInfo));

	iServSession.GetEvent(aProcessName,status,infoPtr);

	// Wait for notification of the breakpoint hit event
	User::WaitForRequest(status);
	if (status == KErrNone)
		{
		return KErrNone;
		}
	else
		{
		return KErrGeneral;
		}
	}

/**
 * Helper code for the stepping tests. Reads the current target PC for a given thread.
 *
 * @param aThreadId - Thread id for which to read the current target PC.
 * @param aPc - Reference to a TUint32 which will be set to the current target PC.
 * @return KErrNone if successful. One of the other system wide error codes otherwise.
 */
TInt CRunModeAgent::HelpTestStepReadPC(TThreadId aThreadId, TUint32& aPC)
	{
	TInt err = KErrNone;

	//create buffer containing PC register ID
	RBuf8 pcId;
	err = pcId.Create(sizeof(TRegisterInfo));
	if (err != KErrNone)
		{
		return err;
		}

	TRegisterInfo reg1 = (TRegisterInfo)0x00000f00;
	pcId.Append(reinterpret_cast<const TUint8*>(&reg1), sizeof(TRegisterInfo));

	//create buffer containing desired PC value
	TPtr8 pcValue((TUint8*)&aPC,4,4);

	//create buffer for PC flag value
	RBuf8 pcFlag;
	err = pcFlag.Create(sizeof(TUint8));

	//read the new PC value
	err = iServSession.ReadRegisters(aThreadId, pcId, pcValue, pcFlag);
	if (err != KErrNone)
		{
		//delete temporary buffers
		pcId.Close();
		pcFlag.Close();
		return err;
		}

	//get the flag and check the PC value was read ok
	TRegisterFlag flag = ENotSupported;
	err = GetFlag(pcFlag, 0, flag);
	if (err != KErrNone)
		{
		//delete temporary buffers
		pcId.Close();
		pcFlag.Close();
		return err;
		}

	if (flag == EValid)
		{
		//delete temporary buffers
		pcId.Close();
		pcFlag.Close();
		return KErrNone;
		}
	else
		{
		//delete temporary buffers
		pcId.Close();
		pcFlag.Close();
		return err;
		}
	}

/**
 * Helper code for the stepping tests. Single steps a given thread from aStartAddress to aEndAddress. Note
 * that it reaches aStartAddress by setting a breakpoint at that address and waiting until it is hit.
 *
 * @param aThreadId - Thread id for which to read the current target PC.
 * @param aStartAddress - The target address at which stepping will start.
 * @param aEndAddress - The target address at which stepping will end.
 * @param aMode - The architecture of the breakpoint which must be set at the start address (ARM/Thumb/Thumb2EE).
 * @return KErrNone if successful. One of the other system wide error codes otherwise.
 */
TInt CRunModeAgent::HelpTestStep(TThreadId aThreadId, TUint32 aStartAddress, TUint32 aEndAddress, TArchitectureMode aMode, TUint aNumSteps, TBool aThreadSpecific, TProcessId aProcessId)
	{
	TInt err = KErrNone;

	// Ensure that the supplied addresses are word/half-word aligned as appropriate.
	if (aMode == EArmMode)
		{
		// ARM breakpoints must be word-aligned (2 lsb must be zero)
		aStartAddress &= 0xFFFFFFFC;
		aEndAddress &= 0xFFFFFFFC;
		}
	else if (aMode == EThumbMode)
		{
		// Thumb breakpoints must be half-word aligned (lsb must be zero)
		aStartAddress &= 0xFFFFFFFE;
		aEndAddress	 &= 0xFFFFFFFE;
		}
	else if (aMode == EThumb2EEMode)
	{
		// Thumb2EE breakpoints are not currently supported
		return KErrNotSupported;
	}

	// Set breakpoint at the start address
	TBreakId tempBreakId;
	TEventInfo info;

	err = HelpTestStepSetBreak(tempBreakId,aThreadId,aStartAddress,aMode,aThreadSpecific,aProcessId);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - Failed to set breakpoint at aStartAddress 0x%08x\n"),aStartAddress);
		return err;
		}

	// wait for the breakpoint to be hit
	err = HelpTestStepWaitForBreak(iFileName,info);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - Failed to hit the breakpoint at aStartAddress 0x%08x\n"),aStartAddress);
		return err;
		}

	// Check the PC == aStartAddress
	TUint32 pc = 0;
	err = HelpTestStepReadPC(aThreadId,pc);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - Failed to read the PC after hitting breakpoint at aStartAddress 0x%08x\n"),aStartAddress);
		return err;
		}

	if (pc != aStartAddress)
		{
		test.Printf(_L("HelpTestStep - Incorrect PC value after hitting breakpoint (expected 0x%08x actual 0x%08x)\n"),aStartAddress,pc);
		return KErrGeneral;
		}

	err = iServSession.Step(aThreadId,aNumSteps);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - Failed to do step from 0x%08x to 0x%08x\n"),aStartAddress,aEndAddress,aNumSteps);
		return err;
		}

	// only one 'completed step' event in the buffer.
	err = HelpTestStepWaitForBreak(iFileName,info);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - Could not read breakpoint event info after stepping"));
		return err;
		}
	// end

	// Check PC == aEndAddress
	err = HelpTestStepReadPC(aThreadId,pc);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - failed read the PC after stepping\n"));
		return err;
		}
	if (pc != aEndAddress)
		{
		test.Printf(_L("HelpTestStep - Incorrect PC value after stepping (expected 0x%08x actual 0x%08x)\n"),aEndAddress,pc);
		return KErrGeneral;
		}

	// Clear the breakpoint
	err = HelpTestStepClearBreak(tempBreakId, aThreadId, aThreadSpecific);
	if (err != KErrNone)
		{
		test.Printf(_L("HelpTestStep - failed to clear temporary breakpoint\n"));
		return err;
		}

	return KErrNone;
	}

/**
 * Helper code for the stepping tests. Returns the number of nanokernel ticks in one second.
 *
 * @return Number of nanokernel ticks. 0 if unsuccesful.
 */
TInt CRunModeAgent::HelpTicksPerSecond(void)
	{
	TInt nanokernel_tick_period;
	HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);

	ASSERT(nanokernel_tick_period != 0);

	static const TInt KOneMillion = 1000000;

	return KOneMillion/nanokernel_tick_period;
	}

/**
  Given aTestNumber runs the appropriate test inside heap markers

  @param aTestNumber test to run, corresponds to an entry in iTestArray

  @panic Panic if aTestNumber is not in valid range
  */
void CRunModeAgent::RunTest(TInt aTestNumber)
	{
	if( (aTestNumber<0) || (aTestNumber>=KMaxTests) )
		{
		User::Panic(_L("Test number out of range"), aTestNumber);
		}
	__UHEAP_MARK;
	(this->*(iTestArray[aTestNumber].iFunctionPtr))();
	__UHEAP_MARKEND;
	}

void CRunModeAgent::PrintVersion()
	{
	test.Printf(_L("\nt_rmdebug2.exe\nVersion: %S\n"), &(testVersion.Name()));
	test.Printf(_L("Press any key...\n"));
	test.Getch();
	}

void CRunModeAgent::PrintUsage()
	{
	test.Printf(_L("Invoke with arguments:\n"));
	test.Printf(_L("-r: run specified tests in reverse order\n"));
	test.Printf(_L("-h: display usage information\n"));
	test.Printf(_L("-v: display version\n"));
	test.Printf(_L("-d: use delays\n"));
	test.Printf(_L("<number>: test number to run, can specify more than one from the following list:\n"));
	test.Printf(_L("Press any key for list...\n"));
	test.Getch();
	// if there are too many of these they won't fit on the screen! Stick another Getch() in if there get too many
	for(TInt i=0; i<KMaxTests; i++)
		{
		test.Printf(_L("%2d: %S\n"), i, &(iTestArray[i].iFunctionName));
		}
	test.Printf(_L("Press any key...\n"));
	test.Getch();
	}

/**
  Parse the command line, see CRunModeAgent::PrintUsage for syntax
  */
void CRunModeAgent::ParseCommandLineL(TUint32& aMode, RArray<TInt>& aTests)
	{
	// get the length of the command line arguments
	TInt argc = User::CommandLineLength();

	// allocate a buffer for the command line arguments and extract the data to it
	HBufC* commandLine = HBufC::NewLC(argc);
	TPtr commandLineBuffer = commandLine->Des();
	User::CommandLine(commandLineBuffer);

	// reset mode
	aMode = (TTestMode)0;

	// create a lexer and read through the command line
	TLex lex(*commandLine);
	while (!lex.Eos())
		{
		// expecting the first character to be a '-'
		if (lex.Get() == '-')
			{
			TChar arg = lex.Get();
			switch (arg)
				{
				case 'v':
					//print out the help
					aMode |= EModeVersion;
					break;
				case 'h':
					//print out the help
					aMode |= EModeHelp;
					break;
				case 'r':
					//store the fact that we want to run in reverse
					aMode |= EModeReverse;
					break;
				case 'd':
					//store the fact that we want to run in reverse
					gUseDelay = EFalse;
					RDebug::Printf("Not using delays");
					break;
				default:
					// unknown argument so leave
					User::Leave(KErrArgument);
				}
			}
		else
			{
			lex.UnGet();
			TInt testNumber;
			User::LeaveIfError(lex.Val(testNumber));
			if( (testNumber<0) || (testNumber>=KMaxTests) )
				{
				User::Leave(KErrArgument);
				}
			aTests.AppendL(testNumber);
			}
		lex.SkipSpace();
		}
	// if no tests specified then run them all
	if(aTests.Count() == 0)
		{
		aMode |= EModeAll;
		}

	// do clean up
	CleanupStack::PopAndDestroy(commandLine);
	}

void CRunModeAgent::ClientAppL()
//
// Performs each test in turn
//
	{
	test.Start(_L("ClientAppL"));

	RArray<TInt> testsToRun;
	TUint32 testMode = 0;
	ParseCommandLineL(testMode, testsToRun);

	//if help or version mode specified then just print out the relevant stuff and quit
	if((testMode & EModeHelp) || (testMode & EModeVersion))
		{
		if(testMode & EModeHelp)
			{
			PrintUsage();
			}
		if(testMode & EModeVersion)
			{
			PrintVersion();
			}
		test.End();
		return;
		}

	if(testMode & EModeAll)
		{
		for(TInt i=0; i<KMaxTests; i++)
			{
			testsToRun.AppendL(i);
			}
		}

	// if EModeReverse specified then reverse the array elements
	TInt numberOfTests = testsToRun.Count();
	if(testMode & EModeReverse)
		{
		for(TInt i=0; i<(numberOfTests>>1); i++)
			{
			TInt temp = testsToRun[i];
			testsToRun[i] = testsToRun[numberOfTests - (i+1)];
			testsToRun[numberOfTests - (i+1)] = temp;
			}
		}

	__UHEAP_MARK;
	SetupAndAttachToDSS();
	__UHEAP_MARKEND;

	HelpStartTestTimer();
	for(TInt i=0; i<numberOfTests; i++)
		{
		RunTest(testsToRun[i]);
		if( gUseDelay ) User::After(500000);
		}
	testsToRun.Close();

	HelpStopTestTimer();

	ReportPerformance();

	test.End();
	}

/**
  Fill the test array with pointers to each test.
  */
void CRunModeAgent::FillArray()
	{
	iTestArray[0].iFunctionPtr = &CRunModeAgent::TestDriverSecurity;
	iTestArray[0].iFunctionName = _L("TestDriverSecurity");
	iTestArray[1].iFunctionPtr = &CRunModeAgent::TestDllUsage;
	iTestArray[1].iFunctionName = _L("TestDllUsage");
	iTestArray[2].iFunctionPtr = &CRunModeAgent::TestSecurity;
	iTestArray[2].iFunctionName = _L("TestSecurity");
	iTestArray[3].iFunctionPtr = &CRunModeAgent::TestAttachExecutable;
	iTestArray[3].iFunctionName = _L("TestAttachExecutable");
	iTestArray[4].iFunctionPtr = &CRunModeAgent::TestGetExecutablesList;
	iTestArray[4].iFunctionName = _L("TestGetExecutablesList");
	iTestArray[5].iFunctionPtr = &CRunModeAgent::TestGetProcessList;
	iTestArray[5].iFunctionName = _L("TestGetProcessList");
	iTestArray[6].iFunctionPtr = &CRunModeAgent::TestGetXipLibrariesList;
	iTestArray[6].iFunctionName = _L("TestGetXipLibrariesList");
	iTestArray[7].iFunctionPtr = &CRunModeAgent::TestGetThreadList;
	iTestArray[7].iFunctionName = _L("TestGetThreadList");
	iTestArray[8].iFunctionPtr = &CRunModeAgent::TestGetCodeSegsList;
	iTestArray[8].iFunctionName = _L("TestGetCodeSegsList");
	iTestArray[9].iFunctionPtr = &CRunModeAgent::TestGetListInvalidData;
	iTestArray[9].iFunctionName = _L("TestGetListInvalidData");
	iTestArray[10].iFunctionPtr = &CRunModeAgent::TestMemoryAccess;
	iTestArray[10].iFunctionName = _L("TestMemoryAccess");
	iTestArray[11].iFunctionPtr = &CRunModeAgent::TestDebugFunctionality;
	iTestArray[11].iFunctionName = _L("TestDebugFunctionality");
	iTestArray[12].iFunctionPtr = &CRunModeAgent::TestSuspendResume;
	iTestArray[12].iFunctionName = _L("TestSuspendResume");
	iTestArray[13].iFunctionPtr = &CRunModeAgent::TestBreakPoints;
	iTestArray[13].iFunctionName = _L("TestBreakPoints");
	iTestArray[14].iFunctionPtr = &CRunModeAgent::TestModifyBreak;
	iTestArray[14].iFunctionName = _L("TestModifyBreak");
	iTestArray[15].iFunctionPtr = &CRunModeAgent::TestBreakInfo;
	iTestArray[15].iFunctionName = _L("TestBreakInfo");
	iTestArray[16].iFunctionPtr = &CRunModeAgent::TestRunToBreak;
	iTestArray[16].iFunctionName = _L("TestRunToBreak");
	iTestArray[17].iFunctionPtr = &CRunModeAgent::TestBreakPointsInLoop;
	iTestArray[17].iFunctionName = _L("TestBreakPointsInLoop");
	iTestArray[18].iFunctionPtr = &CRunModeAgent::TestRegisterAccess;
	iTestArray[18].iFunctionName = _L("TestRegisterAccess");
	iTestArray[19].iFunctionPtr = &CRunModeAgent::TestStep;
	iTestArray[19].iFunctionName = _L("TestStep");
	iTestArray[20].iFunctionPtr = &CRunModeAgent::TestDemandPaging;
	iTestArray[20].iFunctionName = _L("TestDemandPaging");
	iTestArray[21].iFunctionPtr = &CRunModeAgent::TestEventsForExternalProcess;
	iTestArray[21].iFunctionName = _L("TestEventsForExternalProcess");
	iTestArray[22].iFunctionPtr = &CRunModeAgent::TestEvents;
	iTestArray[22].iFunctionName = _L("TestEvents");
	iTestArray[23].iFunctionPtr = &CRunModeAgent::TestKillProcess;
	iTestArray[23].iFunctionName = _L("TestKillProcess");
	iTestArray[24].iFunctionPtr = &CRunModeAgent::TestProcessBreakPoints;
	iTestArray[24].iFunctionName = _L("TestProcessBreakPoints");
	iTestArray[25].iFunctionPtr = &CRunModeAgent::TestMultipleTraceEvents;
	iTestArray[25].iFunctionName = _L("TestMultipleTraceEvents");
	iTestArray[26].iFunctionPtr = &CRunModeAgent::TestAddRemoveProcessEvents;
	iTestArray[26].iFunctionName = _L("TestAddRemoveProcessEvents");
	iTestArray[27].iFunctionPtr = &CRunModeAgent::TestCrashFlash;
	iTestArray[27].iFunctionName = _L("TestCrashFlash");
	iTestArray[28].iFunctionPtr = &CRunModeAgent::TestProcessKillBreakpoint;
	iTestArray[28].iFunctionName = _L("TestProcessKillBreakpoint");
	};

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
   	test.Title();
   RunModeAgent = CRunModeAgent::NewL();
   if (RunModeAgent != NULL)
       {
        __UHEAP_MARK;
	    TRAP(ret,RunModeAgent->ClientAppL());
	    __UHEAP_MARKEND;

	    delete RunModeAgent;
       }

	delete trap;

	return ret;
	}

/**
Helper function to get the aOffset'th value from aFlags

@param aFlags descriptor containing TRegisterFlag type flags
@param aOffset index of flag value to extract from aFlags
@param aFlagValue the flag value if function returned successfully

@return KErrNone if value was read successfully, KErrTooBig if aOffset is
        greater than aFlags.Length()
*/
TInt CRunModeAgent::GetFlag(const TDes8& aFlags, const TUint aOffset, TRegisterFlag &aFlagValue) const
	{
	//get pointer to data
	const TUint8 *ptr = aFlags.Ptr();

	//check aOffset is valid
	TUint length = aFlags.Length();
	if(aOffset >= length)
		return KErrTooBig;

	//get flag value
	aFlagValue = (TRegisterFlag)ptr[aOffset];
	return KErrNone;
	}

/**
  Helper function to set the value of FunctionChooser in the target debug thread.

  @param aTestFunction TTestFunction enum to set FunctionChooser to

  @return KErrNone if the value was set correctly, or one of the other system wide error codes
  */
TInt CRunModeAgent::SwitchTestFunction(TTestFunction aTestFunction)
	{
	//suspend the target thread
	TInt suspendError = iServSession.SuspendThread(iThreadID);
	if(! ( (suspendError == KErrNone) || (suspendError == KErrAlreadyExists) ) )
		{
		//the thread is not suspended so exit
		return suspendError;
		}

	//get the address of FunctionChooser
	TUint32 functionChooserAddress = (TUint32)&FunctionChooser;
	//put the new value for FunctionChooser into a descriptor
	TPtr8 functionBuf((TUint8*)&aTestFunction, sizeof(TTestFunction), sizeof(TTestFunction));
	//write the new value into the target thread
	TInt writeError = iServSession.WriteMemory(iThreadID, functionChooserAddress, sizeof(TTestFunction), functionBuf, EAccess32, EEndLE8);

	if(KErrNone == suspendError)
		{
		//if this function suspended the target thread then we need to resume it
		TInt resumeError = iServSession.ResumeThread(iThreadID);
		if(KErrNone != resumeError)
			{
			//resuming failed so return the error
			return resumeError;
			}
		}

	//suspending and resuming was successful so return the error code from the WriteMemory call
	return writeError;
	}

/**
  Launch a separate process to debug.

  @param aProcess the RProcess object to use to create the process
  @param aFileName file name of the executable to create the process from
  @param aFunctionType function that the target process should call on execution
  @param aDelay delay before the new process should call the function represented by aFunctionType
  @param aExtraThreads number of extra threads to create in the child process

  @return KErrNone on success, or one of the other system wide error codes
  */
TInt CRunModeAgent::LaunchProcess(RProcess& aProcess, const TDesC& aFileName, TDebugFunctionType aFunctionType, TUint32 aDelay, TUint32 aExtraThreads)
	{
	// at the moment we support two arguments, this number might have to be increased to support arguments
	const TUint KMaxCommandLineLength = 32;

	// create a command line buffer
	RBuf commandLine;
	commandLine.Create(KMaxCommandLineLength);

	// append the command line arguments to the buffer
	_LIT(KFArg, "-f");
	commandLine.Append(KFArg());
	commandLine.AppendNum(aFunctionType);

	_LIT(KSpace, " ");
	commandLine.Append(KSpace());

	_LIT(KDArg, "-d");
	commandLine.Append(KDArg());
	commandLine.AppendNum(aDelay);

	commandLine.Append(KSpace());

	_LIT(KEArg, "-e");
	commandLine.Append(KEArg());
	commandLine.AppendNum(aExtraThreads);

	// create the new process, matching on file name only, not specifying uid values
	TInt err = aProcess.Create(aFileName, commandLine);	// owned by the process

	// check that there was no error raised
	if(err != KErrNone)
		{
		commandLine.Close();
		return err;
		}

	TRequestStatus status = KRequestPending;
	aProcess.Rendezvous(status);

	commandLine.Close();	// after target thread starts

	if(KRequestPending != status.Int())
		{
		// startup failed so kill the process
		aProcess.Kill(KErrNone);
		return status.Int();
		}
	else
		{
		// start up succeeded so resume the process
		aProcess.Resume();
		User::WaitForRequest(status);
		if(KErrNone != status.Int())
			{
			aProcess.Kill(KErrNone);
			}
		return status.Int();
		}
	}

/**
  Helper function to read a tag header from a debug functionality block

  @param aDebugFunctionalityBlock block to read header from
  @param aTagHdrId header type to find

  @return pointer to the header, or NULL if not available
  */
TTagHeader* CRunModeAgent::GetTagHdr(const TDesC8& aDebugFunctionalityBlock, const TTagHeaderId aTagHdrId) const
	{
	TUint8* ptr = (TUint8*) aDebugFunctionalityBlock.Ptr();
	TUint8* blockEnd = ptr + aDebugFunctionalityBlock.Size();

	while(ptr < blockEnd)
		{
		TTagHeader* header = (TTagHeader*)ptr;
		if(header->iTagHdrId == aTagHdrId)
			{
			return header;
			}
		ptr += sizeof(TTagHeader) + (header->iNumTags * sizeof(TTag));
		}
	return NULL;
	}

/**
  Helper function to read a tag from a debug functionality block

  @param aTagHdr pointer to a tag header in a debug functionality block
  @param aElement element to return from the header's data

  @return pointer to the tag, or NULL if not available
  */
TTag* CRunModeAgent::GetTag(const TTagHeader* aTagHdr, const TInt aElement) const
	{
	TUint8* ptr = (TUint8*)aTagHdr + sizeof(TTagHeader);
	TUint8* blockEnd = ptr + (aTagHdr->iNumTags * sizeof(TTag));

	while(ptr < blockEnd)
		{
		TTag* tag = (TTag*)ptr;
		if(tag->iTagId == aElement)
			{
			return tag;
			}
		ptr += sizeof(TTag);
		}
	return NULL;
	}

TTag CRunModeAgent::GetTag(const TTagHeaderId aTagHdrId, const TInt aElement)
	{
	TUint32 bufsize = 0;	// Safe default size

	// Get functionality block size
	test(KErrNone == iServSession.GetDebugFunctionalityBufSize(&bufsize));

	// Ensure we have a finite buffer size
	test(bufsize!=0);

	// Allocate space for the functionality data
	HBufC8* dftext = HBufC8::NewLC(bufsize);

	// create an empty TPtr8 refering to dftext
	TPtr8 dftextPtr(dftext->Des());

	// Get the functionality block
	test(KErrNone == iServSession.GetDebugFunctionality(dftextPtr));

	// read a value from the data to check it has come through as expected
	TTagHeader* header = GetTagHdr(dftext->Des(), aTagHdrId);
	test(header != NULL);
	TTag* tag = GetTag(header, aElement);
	test(tag != NULL);

	TTag tagToReturn = *tag;

	// Remove our temporary buffer
	CleanupStack::PopAndDestroy(dftext);

	return tagToReturn;
	}

/**
  Helper function which returns a Boolean indicating with a process with the
  specified name is currently running.

  @param aProcessName - Name of the process to find
  @return ETrue if found, EFalse otherwise
  */
TBool CRunModeAgent::ProcessExists(const TDesC& aProcessName)
	{
	TInt    err=KErrNone;
	TBool	found = FALSE;

_LIT(KWildCard,"*");

	TFindProcess find(KWildCard);
	TFullName name;
	while(find.Next(name)==KErrNone)
		{
		RProcess process;
		err = process.Open(find);
		if (err == KErrNone)
			{
			if (name.Find(aProcessName) != KErrNotFound)
				{
					found = TRUE;
				}
			process.Close();
			}
	   }

	return found;
	}
