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
// f32test\demandpaging\t_pagestress.cpp
// Demand Paging Stress Tests
// t_pagestress.exe basically attempts to cause large ammounts of paging by calling
// functions (256) which are alligned on a page boundary from multiple threads.
// There are mulitple versions of t_pagestress installed on a test system to test rom
// and code paging from various media types.
// Usage:
// t_pagestress and t_pagestress_rom
// Common command lines:
// t_pagestress lowmem
// debug - switch on debugging information
// silent - no output to the screen or serial port
// check - check the allignments
// single - run the tests in a single thread
// multiple <numThreads> - run the tests in multiple threads where <numThreads>
// interleave - force thread interleaving
// prio - each thread reschedules in between each function call, causes lots of context changes
// media - perform media access during the tests, very stressful
// lowmem - low memory tests
// forward - patern in which to execute function calls 
// backward - patern in which to execute function calls 			
// random - patern in which to execute function calls 			
// all - patern in which to execute function calls (forward, backward and random)
// inst - for debugging a parameter passed to a spawned exe to give it an id.
// iters <count> - the number of times to loop (a '-' means run forever)
// t_pagestress causes a large ammount of paging by repeatedly calling 
// 256 functions which have been aligned on page boundaries from 
// multiple threads.
// 1  - a single thread calling all functions
// 2  - Multiple threads calling all functions
// 3  - Multiple threads calling all functions with a priority change 
// after each function call
// 4  - Multiple threads calling all functions with background 
// media activity
// 5  - Multiple threads calling all functions with media activity and 
// a priority change after each function call
// 6  - Multiple threads calling all functions with process interleave
// 7  - Multiple threads calling all functions with process interleave 
// and a priority change after each function call
// 8  - Multiple threads calling all functions with process interleave 
// media acess and a priority change after each function call
// 9  - Multiple threads calling all functions with low available memory
// 10 - Multiple threads calling all functions with low available memory,
// starting with initial free ram.
// 
//

//! @SYMTestCaseID			KBASE-T_PAGESTRESS-0327
//! @SYMTestType			UT
//! @SYMPREQ				PREQ1110
//! @SYMTestCaseDesc		Demand Paging Stress Tests
//! @SYMTestActions			0  - Check the alignment of all functions
//! @SYMTestExpectedResults All tests should pass.
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented

#include <e32test.h>
RTest test(_L("T_PAGESTRESS"));

#include <e32rom.h>
#include <e32svr.h>
#include <u32hal.h>
#include <f32file.h>
#include <f32dbg.h>
#include <e32msgqueue.h>
#include <e32math.h>

#include "testdefs.h"

#ifdef __X86__
#define TEST_ON_UNPAGED
#endif


/* The following line will cause t_pagestress.h to declare an array of function
 * pointers to  page boundary aligned functions that we can use in this test...
 */
#define TPS_DECLARE_ARRAY
#include "t_pagestress.h"

TBool   	TestDebug					= EFalse;
TBool		TestSilent					= EFalse;
TBool		TestExit					= EFalse;

TBool   	TestCheck					= EFalse;
TBool   	TestSingle					= EFalse;
TBool   	TestMultiple				= EFalse;
TBool   	TestForever					= EFalse;
TInt		TestMaxLoops				= 20;
TInt		TestMultipleThreadCount		= 50;
TInt		TestInstanceId				= 0;

#define TEST_INTERLEAVE_PRIO		EPriorityMore//EPriorityRealTime //23 // KNandThreadPriority - 1
TBool		TestInterleave				= EFalse;
TBool		TestWeAreTheTestBase		= EFalse;
TBool		TestBootedFromMmc			= EFalse;
TInt		DriveNumber=-1;   // Parameter - Which drive?  -1 = autodetect.
#define TEST_NONE		0x0
#define TEST_FORWARD	0x2
#define TEST_BACKWARD	0x4
#define TEST_RANDOM		0x8
#define TEST_ALL		(TEST_RANDOM | TEST_BACKWARD | TEST_FORWARD)
TUint32		TestWhichTests				= TEST_ALL;
TBuf<32>	TestNameBuffer;
TBool		TestPrioChange				= ETrue;
TBool		TestStopMedia				= EFalse;
TBool		TestMediaAccess				= EFalse;
#define TEST_LM_NUM_FREE	0
#define TEST_LM_BLOCKSIZE	1
#define TEST_LM_BLOCKS_FREE	4
TBool		TestLowMem					= EFalse;
RPageStressTestLdd Ldd;
TInt		TestPageSize				= 4096;
RSemaphore	TestMultiSem;
RMsgQueue<TBuf <64> >	TestMsgQueue;
TBool		TestIsDemandPaged = ETrue;


#define TEST_NEXT(__args) \
	if (!TestSilent)\
		test.Next __args;

#define DBGS_PRINT(__args)\
	if (!TestSilent)\
		test.Printf __args ;

#define DBGD_PRINT(__args)\
	if (TestDebug)\
		test.Printf __args ;\

#define DEBUG_PRINT(__args)\
if (!TestSilent)\
	{\
	if (aMsgQueue && aBuffer && aTheSem)\
		{\
		aBuffer->Zero();\
		aBuffer->Format __args ;\
		aTheSem->Wait();\
		aMsgQueue->SendBlocking(*aBuffer);\
		aTheSem->Signal();\
		}\
	else\
		{\
		test.Printf __args ;\
		}\
	}

#define RUNTEST(__test, __error)\
	if (!TestSilent)\
		test(__test == __error);\
	else\
		__test;

#define RUNTEST1(__test)\
	if (!TestSilent)\
		test(__test);

#define DEBUG_PRINT1(__args)\
if (TestDebug)\
	{\
	DEBUG_PRINT(__args)\
	}

//
// CheckAlignments
//
// The following functions wanders through the function pointer array
// declared in t_pagestress.h and ensures that the alignment has worked, 
// (a good start!) and then calls each of the functions in turn to 
// ensure that they work, simple first sanity check test.
//

TInt CheckAlignments()
	{
	// now it's time to play with the array of function pointers...
	TInt  seed = 1;
	TUint32 index = 0;
	TInt ret = KErrNone;

	while (index < (PAGESTRESS_FUNC_COUNT - 1))
		{
		DBGD_PRINT((_L("\nAddress (%u) 0x%x difference to next %u\n"), index, (TUint32)PagestressFuncPtrs[index], (TUint32)PagestressFuncPtrs[index + 1] - (TUint32)PagestressFuncPtrs[index]));	
		if ((((TUint32)PagestressFuncPtrs[index + 1] - (TUint32)PagestressFuncPtrs[0]) % 4096) != 0)
			{
			DBGS_PRINT((_L("\nError! Allignment for %u is not offset of 4096 from index 0 0x%x 0x%x\n"), index, (TUint32)PagestressFuncPtrs[index + 1], (TUint32)PagestressFuncPtrs[0]));	
			ret = KErrGeneral;
			}
		//seed = PagestressFuncPtrs[index](seed, index);
		seed = CallTestFunc(seed, index, index);
		index ++;
		}
	return ret;
	}

//
// RunThreadForward
//
// Walk through the function pointer array (forwards) calling each function
//

void RunThreadForward()
	{
	TInt	seed = 1;
	TUint32 index = 0;
	RThread thisThread;

	while (index < PAGESTRESS_FUNC_COUNT)
		{
		if (TestPrioChange)
			{
			TThreadPriority originalThreadPriority = thisThread.Priority();
			thisThread.SetPriority(EPriorityLess);
			User::AfterHighRes(0);
			thisThread.SetPriority(originalThreadPriority);
			}
		//seed = PagestressFuncPtrs[index](seed, index);
		seed = CallTestFunc(seed, index, index);
		index ++;
		}
	}

//
// RunThreadBackward
//
// Walk through the function pointer array (backwards) calling each function
//

void RunThreadBackward()
	{
	TInt	seed = 1;
	TInt	index = PAGESTRESS_FUNC_COUNT;
	RThread thisThread;

	while (index > 0)
		{
		if (TestPrioChange)
			{
			TThreadPriority originalThreadPriority = thisThread.Priority();
			thisThread.SetPriority(EPriorityLess);
			User::AfterHighRes(0);
			thisThread.SetPriority(originalThreadPriority);
			}
		index --;
		//seed = PagestressFuncPtrs[index](seed, index);
		seed = CallTestFunc(seed, index, index);
		}
	}

//
// RunThreadRandom
//
// Walk through the function pointer array in a random order a number of times calling each function
//

void RunThreadRandom()
	{
	TInt	seed = 1;
	TInt	index = 0;
	TInt	randNum;
	RThread thisThread;

	while (index < (TInt)PAGESTRESS_FUNC_COUNT)
		{
		if (TestPrioChange)
			{
			TThreadPriority originalThreadPriority = thisThread.Priority();
			thisThread.SetPriority(EPriorityLess);
			User::AfterHighRes(0);
			thisThread.SetPriority(originalThreadPriority);
			}
		randNum = Math::Random();
		randNum %= PAGESTRESS_FUNC_COUNT;
		//seed = PagestressFuncPtrs[randNum](seed, randNum);
		seed = CallTestFunc(seed, randNum, randNum);
		index ++;
		}
	}


//
// PerformTestThread
//
// This is the function that actually does the work.
// It is complicated a little because test.Printf can only be called from the first thread that calls it 
// so if we are using multiple threads we need to use a message queue to pass the debug info from the
// child threads back to the parent for the parent to then call printf.
//
//

LOCAL_C void PerformTestThread(TInt					  aThreadIndex, 
							   RMsgQueue<TBuf <64> > *aMsgQueue = NULL, 
							   TBuf<64>				 *aBuffer = NULL,
							   RSemaphore			 *aTheSem = NULL)
	{
	TUint start = User::TickCount();

	DEBUG_PRINT1((_L("%S : thread Starting %d\n"), &TestNameBuffer, aThreadIndex));
	
	// now select how we do the test...
	TInt	iterIndex;

	if (TEST_ALL == (TestWhichTests & TEST_ALL))
		{
		#define LOCAL_ORDER_INDEX1	6
		#define LOCAL_ORDER_INDEX2	3
		TInt	order[LOCAL_ORDER_INDEX1][LOCAL_ORDER_INDEX2] = {	{TEST_FORWARD, TEST_BACKWARD,TEST_RANDOM},
																	{TEST_FORWARD, TEST_RANDOM,  TEST_BACKWARD},
																	{TEST_BACKWARD,TEST_FORWARD, TEST_RANDOM},
																	{TEST_BACKWARD,TEST_RANDOM,  TEST_FORWARD},
																	{TEST_RANDOM,  TEST_FORWARD, TEST_BACKWARD},
																	{TEST_RANDOM,  TEST_BACKWARD,TEST_FORWARD}};
		TInt	whichOrder = 0;

		for (iterIndex = 0; iterIndex < TestMaxLoops; iterIndex ++)
			{
			TInt    selOrder = ((aThreadIndex + 1) * (iterIndex + 1)) % LOCAL_ORDER_INDEX1;
			for (whichOrder = 0; whichOrder < LOCAL_ORDER_INDEX2; whichOrder ++)
				{
				switch (order[selOrder][whichOrder])
					{
						case TEST_FORWARD:
						DEBUG_PRINT1((_L("%S : %d Iter %d Forward\n"), &TestNameBuffer, aThreadIndex, iterIndex));
						RunThreadForward();
						break;

						case TEST_BACKWARD:
						DEBUG_PRINT1((_L("%S : %d Iter %d Backward\n"), &TestNameBuffer, aThreadIndex, iterIndex));
						RunThreadBackward();
						break;

						case TEST_RANDOM:
						DEBUG_PRINT1((_L("%S : %d Iter %d Random\n"), &TestNameBuffer, aThreadIndex, iterIndex));
						RunThreadRandom();
						break;
						
						default: // this is really an error.
						break;
					}
				}
			}
		}
	else
		{
		if (TestWhichTests & TEST_FORWARD)
			{
			for (iterIndex = 0; iterIndex < TestMaxLoops; iterIndex ++)
				{
				DEBUG_PRINT1((_L("%S : %d Iter %d Forward\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				RunThreadForward();
				}
			}
			
		if (TestWhichTests & TEST_BACKWARD)
			{
			for (iterIndex = 0; iterIndex < TestMaxLoops; iterIndex ++)
				{
				DEBUG_PRINT1((_L("%S : %d Iter %d Backward\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				RunThreadBackward();
				}
			}

		if (TestWhichTests & TEST_RANDOM)
			{
			for (iterIndex = 0; iterIndex < TestMaxLoops; iterIndex ++)
				{
				DEBUG_PRINT1((_L("%S : %d Iter %d Random\n"), &TestNameBuffer, aThreadIndex, iterIndex));
				RunThreadRandom();
				}
			}
		}
	DEBUG_PRINT1((_L("%S : thread Exiting %d (tickcount %u)\n"), &TestNameBuffer, aThreadIndex, User::TickCount() - start));
	}


//
// MultipleTestThread
//
// Thread function, one created for each thread in a multiple thread test.
//

LOCAL_C TInt MultipleTestThread(TAny* aUseTb)
	{
	TBuf<64>					localBuffer;

	if (TestInterleave)	
		{
		RThread				thisThread;
		thisThread.SetPriority((TThreadPriority) TEST_INTERLEAVE_PRIO);
		}

	PerformTestThread((TInt) aUseTb, &TestMsgQueue, &localBuffer, &TestMultiSem);
	
	return KErrNone;
	}

//
// DoSingleTest
// 
// Perform the single thread test, spawning a number of threads.
//

LOCAL_C void DoSingleTest()
	{
	PerformTestThread((TInt) TestInstanceId);
	}

//
// FindDriveNumber
// 
// Find the first read write drive.
//

TInt FindDriveNumber(RFs fs)
	{
	TDriveInfo driveInfo;
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
		TInt r = fs.Drive(driveInfo, drvNum);
		if (r >= 0)
			{
			if (driveInfo.iType == EMediaHardDisk)
				return (drvNum);
			}
		}
	return -1;
	}

//
// FindFsNANDDrive
//
// Find the NAND drive
//

static TInt FindFsNANDDrive(RFs& aFs)
	{
	TDriveList driveList;
	TDriveInfo driveInfo;
	TInt r=aFs.DriveList(driveList);
    if (r == KErrNone)
		{
		for (TInt drvNum= (DriveNumber<0)?0:DriveNumber; drvNum<KMaxDrives; ++drvNum)
			{
			if(!driveList[drvNum])
				continue;   //-- skip unexisting drive

			if (aFs.Drive(driveInfo, drvNum) == KErrNone)
				{
				if(driveInfo.iMediaAtt&KMediaAttPageable)
					{
					TBool readOnly = driveInfo.iMediaAtt & KMediaAttWriteProtected;		// skip ROFS partitions
					if(!readOnly)
						{
						if ((drvNum==DriveNumber) || (DriveNumber<0))		// only test if running on this drive
							{
							return (drvNum);
							}
						}
					}
				}
			}
		}
	return -1;
	}

//
// FindMMCDriveNumber
// 
// Find the first read write drive.
//

TInt FindMMCDriveNumber(RFs& aFs)
	{
	TDriveInfo driveInfo;
	for (TInt drvNum=0; drvNum<KMaxDrives; ++drvNum)
		{
		TInt r = aFs.Drive(driveInfo, drvNum);
		if (r >= 0)
			{
			if (driveInfo.iType == EMediaHardDisk)
				return (drvNum);
			}
		}
	return -1; 
	}

//
// PerformRomAndFileSystemAccess
// 
// Access the rom and dump it out to one of the writeable partitions...
// really just to make the media server a little busy during the test.
//

TInt PerformRomAndFileSystemAccessThread(RMsgQueue<TBuf <64> > *aMsgQueue = NULL, 
										 TBuf<64>			   *aBuffer = NULL,
										 RSemaphore			   *aTheSem = NULL)
	{
	TUint maxBytes = KMaxTUint;
	TInt startTime = User::TickCount();

	RFs fs;
	RFile file;
	if (KErrNone != fs.Connect())
		{
		DEBUG_PRINT(_L("PerformRomAndFileSystemAccessThread : Can't connect to the FS\n"));
		return KErrGeneral;
		}

	// get info about the ROM...
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	TUint8* start;
	TUint8* end;
	if(romHeader->iPageableRomStart)
		{
		start = (TUint8*)romHeader + romHeader->iPageableRomStart;
		end = start + romHeader->iPageableRomSize;
		}
	else
		{
		start = (TUint8*)romHeader;
		end = start + romHeader->iUncompressedSize;
		}
	if (end <= start)
		return KErrGeneral;

	// read all ROM pages in a random order...and write out to file in ROFs, 
	TUint size = end - start - TestPageSize;
	if(size > maxBytes)
		size = maxBytes;

	TUint32 random=1;
	TPtrC8 rom;
	TUint8 *theAddr;

	TInt		drvNum = TestBootedFromMmc ? FindMMCDriveNumber(fs) : FindFsNANDDrive(fs);
	TBuf<32>	filename = _L("d:\\Pageldrtst.tmp");
	if (drvNum >= 0)
		{
		filename[0] = 'a' + drvNum;
		DEBUG_PRINT((_L("%S : Filename %S\n"), &TestNameBuffer, &filename));
		}
	else
		DEBUG_PRINT((_L("PerformRomAndFileSystemAccessThread : error getting drive num\n")));

	for(TInt i=size/(TestPageSize); i>0; --i)
		{
		DEBUG_PRINT1((_L("%S : Opening the file\n"), &TestNameBuffer));
		if (KErrNone != file.Replace(fs, filename, EFileWrite))
			{
			DEBUG_PRINT((_L("%S : Opening the file Failed!\n"), &TestNameBuffer));
			}

		random = random*69069+1;
		theAddr = (TUint8*)(start+((TInt64(random)*TInt64(size))>>32));
		if (theAddr + TestPageSize > end)
			{
			DEBUG_PRINT((_L("%S : address is past the end 0x%x / 0x%x\n"), &TestNameBuffer, (TInt)theAddr, (TInt)end));
			}
		rom.Set(theAddr,TestPageSize);
		DEBUG_PRINT1((_L("%S : Writing the file\n"), &TestNameBuffer));
		TInt ret = file.Write(rom);
		if (ret != KErrNone)
			{
			DEBUG_PRINT1((_L("%S : Write returned error %d\n"), &TestNameBuffer, ret));
			}
		DEBUG_PRINT1((_L("%S : Closing the file\n"), &TestNameBuffer));
		file.Close();

		DEBUG_PRINT1((_L("%S : Deleting the file\n"), &TestNameBuffer));
		ret = fs.Delete(filename);
		if (KErrNone != ret)
			{
			DEBUG_PRINT((_L("%S : Delete returned error %d\n"), &TestNameBuffer, ret));
			}
		if (TestStopMedia)
			break;
		}
	fs.Close();
	DEBUG_PRINT1((_L("Done in %d ticks\n"), User::TickCount() - startTime));
	return KErrNone;
	}


//
// PerformRomAndFileSystemAccess
//
// Thread function, kicks off the file system access.
//

LOCAL_C TInt PerformRomAndFileSystemAccess(TAny* )
	{
	TBuf<64>					localBuffer;

	PerformRomAndFileSystemAccessThread(&TestMsgQueue, &localBuffer, &TestMultiSem);
	
	return KErrNone;
	}

//
// DoMultipleTest
// 
// Perform the multiple thread test, spawning a number of threads.
// It is complicated a little because test.Printf can only be called from the first thread that calls it 
// so if we are using multiple threads we need to use a message queue to pass the debug info from the
// child threads back to the parent for the parent to then call printf.
//
#define DOTEST(__operation, __condition)\
	if (aLowMem) \
		{\
		__operation;\
		while (!__condition)\
			{\
			Ldd.DoReleaseSomeRam(TEST_LM_BLOCKS_FREE);\
			__operation;\
			}\
		RUNTEST1(__condition);\
		}\
	else\
		{\
		__operation;\
		RUNTEST1(__condition);\
		}

void DoMultipleTest(TBool aLowMem = EFalse)
	{
	TInt			 index;

	RThread			*pTheThreads  = (RThread *)User::AllocZ(sizeof(RThread) * TestMultipleThreadCount);
	TInt			*pThreadInUse = (TInt *)User::AllocZ(sizeof(TInt) * TestMultipleThreadCount);

	TRequestStatus	mediaStatus;
	RThread			mediaThread;
	
	TInt ret;
	DOTEST((ret = TestMsgQueue.CreateLocal(TestMultipleThreadCount * 10, EOwnerProcess)),
	       (KErrNone == ret));

	DOTEST((ret = TestMultiSem.CreateLocal(1)),
	       (KErrNone == ret));

	// make sure we have a priority higher than that of the threads we spawn...
	RThread thisThread;
	TThreadPriority savedThreadPriority = thisThread.Priority();
	const TThreadPriority KMainThreadPriority = EPriorityMuchMore;
	__ASSERT_COMPILE(KMainThreadPriority>TEST_INTERLEAVE_PRIO);
	thisThread.SetPriority(KMainThreadPriority);

	if (TestMediaAccess)
		{
		TestStopMedia = EFalse;
		mediaThread.Create(_L(""),PerformRomAndFileSystemAccess,KDefaultStackSize,NULL,NULL);
		mediaThread.Logon(mediaStatus);
		RUNTEST1(mediaStatus == KRequestPending);
		mediaThread.Resume();
		}

	// spawn some processes to call the functions....
	for (index = 0; index < TestMultipleThreadCount; index++)
		{
		DBGD_PRINT((_L("%S : Starting thread.%d!\n"), &TestNameBuffer, index));
		DOTEST((ret = pTheThreads[index].Create(_L(""),MultipleTestThread,KDefaultStackSize,NULL,(TAny*) index)),
		       (ret == KErrNone));
		pTheThreads[index].Resume();
		pThreadInUse[index] = 1;
		}
	
	// now process any messages sent from the child threads.
	TBool		anyUsed = ETrue;
	TBuf<64>	localBuffer;

	while(anyUsed)
		{
		anyUsed = EFalse;
		// check the message queue and call printf if we get a message.
		while (KErrNone == TestMsgQueue.Receive(localBuffer))
			{
			DBGS_PRINT((localBuffer));
			}

		// walk through the thread list to check which are still alive.
		for (index = 0; index < TestMultipleThreadCount; index++)
			{
			if (pThreadInUse[index])
				{
				if (pTheThreads[index].ExitType() != EExitPending)
					{
					if (pTheThreads[index].ExitType() == EExitPanic)
						{
						DBGS_PRINT((_L("%S : Thread Panic'd  %d...\n"), &TestNameBuffer, index));	
						}
					pThreadInUse[index] = EFalse;
					pTheThreads[index].Close();
					}
				else
					{
					anyUsed = ETrue;
					}
				}
			}
		User::AfterHighRes(50000);
		}

	if (TestMediaAccess)
		{
		TestStopMedia = ETrue;
		DBGD_PRINT((_L("%S : Waiting for media thread to exit...\n"), &TestNameBuffer));	
		User::WaitForRequest(mediaStatus);
		mediaThread.Close();
		}

	TestMsgQueue.Close();
	TestMultiSem.Close();

	// cleanup the resources and exit.
	User::Free(pTheThreads);
	User::Free(pThreadInUse);

	thisThread.SetPriority(savedThreadPriority);
	}


//
// ParseCommandLine 
//
// read the arguments passed from the command line and set global variables to 
// control the tests.
//

TBool ParseCommandLine()
	{
	TBuf<256> args;
	User::CommandLine(args);
	TLex	lex(args);
	TBool	retVal = ETrue;
	
	// initially test for arguments, the parse them, if not apply some sensible defaults.
	TBool	foundArgs = EFalse;	
	
	FOREVER
		{
		TPtrC  token=lex.NextToken();
		if(token.Length()!=0)
			{
			if ((token == _L("help")) || (token == _L("-h")) || (token == _L("-?")))
				{
				DBGS_PRINT((_L("\nUsage:  %S [debug] [check] [single | multiple <numThreads>]  [forward | backward | random | all] [iters <iters>] [media] [lowmem] [interleave]\n'-' indicated infinity.\n\n"), &TestNameBuffer));
				test.Getch();
				}
			else if (token == _L("debug"))
				{
				if (!TestSilent)
					{
					TestDebug = ETrue;
					TestPrioChange = ETrue;
					}
				}
			else if (token == _L("silent"))
				{
				TestSilent = ETrue;
				TestDebug = EFalse;
				}
			else if (token == _L("check"))
				{
				TestCheck = ETrue;
				}
			else if (token == _L("single"))
				{
				TestSingle = ETrue;
				}
			else if (token == _L("multiple"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (lexv.Val(value)==KErrNone)
					{
					if ((value <= 0) || (value > 100))
						{
						TestMultipleThreadCount = 10;
						}
					else
						{
						TestMultipleThreadCount = value;
						}
					}
				else
					{
					DBGS_PRINT((_L("Bad value for thread count '%S' was ignored.\n"), &val));
					retVal = EFalse;
					break;
					}
				TestMultiple = ETrue;
				}
			else if (token == _L("prio"))
				{
				TestPrioChange = !TestPrioChange;
				}
			else if (token == _L("lowmem"))
				{
				TestLowMem = ETrue;
				}
			else if (token == _L("media"))
				{
				TestMediaAccess = ETrue;
				}
			else if (token == _L("forward"))
				{
				TestWhichTests = TEST_FORWARD;
				}
			else if (token == _L("backward"))
				{
				TestWhichTests = TEST_BACKWARD;
				}
			else if (token == _L("random"))
				{
				TestWhichTests = TEST_RANDOM;
				}
			else if (token == _L("all"))
				{
				TestWhichTests = TEST_ALL;
				}
			else  if (token == _L("iters"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (val==_L("-"))
					{
					TestForever = ETrue;
					TestMaxLoops = KMaxTInt;
					}
				else
					{
					if (lexv.Val(value)==KErrNone)
						{
						TestMaxLoops = value;
						}
					else
						{
						DBGS_PRINT((_L("Bad value for thread count '%S' was ignored.\n"), &val));
						retVal = EFalse;
						break;
						}
					}
				}
			else  if (token == _L("interleave"))
				{
				TestInterleave = ETrue;
				}
			else  if (token == _L("inst"))
				{
				TPtrC val=lex.NextToken();
				TLex lexv(val);
				TInt value;

				if (lexv.Val(value)==KErrNone)
					{
					TestInstanceId = value;
					}
				}
			else
				{
				if ((foundArgs == EFalse) && (token.Length() == 1))
					{
					// Single letter argument...only run on MMC drive
					if (token.CompareF(_L("d")) == 0)
						{
						break;
						}
					else
						{
						if (!TestSilent)
							{
							test.Title();
							test.Start(_L("Skipping non drive 'd' - Test Exiting."));
							test.End();
							}
						foundArgs = ETrue;
						TestExit = ETrue;
						break;
						}
					}
				DBGS_PRINT((_L("Unknown argument '%S' was ignored.\n"), &token));
				break;
				}
			foundArgs = ETrue;
			}
		else
			{
			break;
			}
		}
	if (!foundArgs)
		{
		retVal = EFalse;
		}
	return retVal;
	}

//
// AreWeTheTestBase
//
// Test whether we are the root of the tests.
//

void AreWeTheTestBase(void)
	{
	if (!TestSilent)
		{
		TFileName  filename(RProcess().FileName());

		TParse	myParse;
		myParse.Set(filename, NULL, NULL);
		TestNameBuffer.Zero();
		TestNameBuffer.Append(myParse.Name());
		TestNameBuffer.Append(_L(".exe"));

		TestWeAreTheTestBase = !TestNameBuffer.Compare(_L("t_pagestress.exe"));

		RFs fs;
		if (KErrNone != fs.Connect())
			{
			TEntry  anEntry;
			TInt retVal = fs.Entry(_L("z:\\test\\mmcdemandpaginge32tests.bat"), anEntry);
			if (retVal == KErrNone)
				{
				TestBootedFromMmc = ETrue;
				}
			else
				{
				TestBootedFromMmc = EFalse;
				}
			fs.Close();
			}

		}
	else
		{
		TestNameBuffer.Zero();
		TestNameBuffer.Append(_L("t_pagestress.exe"));
		}
	}

//
// PerformAutoTest
//
// Perform the autotest
//
void PerformAutoTest()
	{
	SVMCacheInfo  tempPages;

	if (TestIsDemandPaged)
		{
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		DBGS_PRINT((_L("PerformAutoTest : Start cache info: iMinSize %d iMaxSize %d iCurrentSize %d iMaxFreeSize %d\n"),
					 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize));
		}
	TestInterleave = EFalse;
	TestPrioChange = EFalse;
	TestMediaAccess = EFalse;

#if defined __ARMCC__ || defined __X86__
	// Currently we only build aligned DLLs on ARMV5 and X86 builds.
	TEST_NEXT((_L("Alignment Check.")));
	RUNTEST1(CheckAlignments() == KErrNone);
#endif

	TestMaxLoops = 2;
	TestWhichTests = TEST_RANDOM;

	TEST_NEXT((_L("Single thread all.")));
	DoSingleTest();

	TEST_NEXT((_L("Multiple threads all.")));
	DoMultipleTest();
	
	TestPrioChange = ETrue;
	TEST_NEXT((_L("Multiple threads all with prio.")));
	DoMultipleTest();

	TestPrioChange = EFalse;
	TestMediaAccess = ETrue;
	TEST_NEXT((_L("Multiple threads all with media activity.")));
	DoMultipleTest();

	TestPrioChange = ETrue;
	TestMediaAccess = ETrue;
	TEST_NEXT((_L("Multiple threads all with media activity and prio.")));
	DoMultipleTest();

	TestInterleave = ETrue;
	TestPrioChange = EFalse;
	TestMediaAccess = EFalse;
	
	TEST_NEXT((_L("Multiple threads random with interleave.")));
	DoMultipleTest();

	TestPrioChange = ETrue;
	TEST_NEXT((_L("Multiple threads random with interleave and prio.")));
	DoMultipleTest();

	TestMediaAccess = ETrue;
	TEST_NEXT((_L("Multiple threads random with media interleave and prio.")));
	DoMultipleTest();

	if (TestIsDemandPaged)
		{
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		DBGS_PRINT((_L("PerformAutoTest : End cache info: iMinSize %d iMaxSize %d iCurrentSize %d iMaxFreeSize %d\n"),
					 tempPages.iMinSize, tempPages.iMaxSize, tempPages.iCurrentSize ,tempPages.iMaxFreeSize));
		}
	TestInterleave = EFalse;
	TestPrioChange = EFalse;
	TestMediaAccess = EFalse;
	}

//
// DoLowMemTest
//
// Low Memory Test
//

void DoLowMemTest()
	{
	TInt r = User::LoadLogicalDevice(KPageStressTestLddName);
	RUNTEST1(r==KErrNone || r==KErrAlreadyExists);
	RUNTEST(Ldd.Open(),KErrNone);
	
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));

	if (TestIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		TInt	minSize = 8 * 4096;
		TInt	maxSize = 256 * 4096;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	// First load some pages onto the page cache 
	TestMaxLoops = 1;
	TestWhichTests = TEST_RANDOM;
	DoSingleTest();

	Ldd.DoConsumeRamSetup(TEST_LM_NUM_FREE, TEST_LM_BLOCKSIZE);
	TEST_NEXT((_L("Single thread with Low memory.")));
	TestMultipleThreadCount	= 25;
	TestInterleave = EFalse;
	TestMaxLoops = 20;
	TestPrioChange = EFalse;
	TestMediaAccess = EFalse;
	TestWhichTests = TEST_ALL;
	
	DoSingleTest();
	
	Ldd.DoConsumeRamFinish();

	TEST_NEXT((_L("Multiple thread with Low memory.")));
	// First load some pages onto the page cache 
	TestMaxLoops = 1;
	TestWhichTests = TEST_RANDOM;
	DoSingleTest();

	Ldd.DoConsumeRamSetup(TEST_LM_NUM_FREE, TEST_LM_BLOCKSIZE);
	
	TestWhichTests = TEST_ALL;
	TestMaxLoops = 10;
	TestMultipleThreadCount	= 25;
	DoMultipleTest(ETrue);

	Ldd.DoConsumeRamFinish();

	TEST_NEXT((_L("Multiple thread with Low memory, with starting free ram.")));
	// First load some pages onto the page cache 
	TestMaxLoops = 1;
	TestWhichTests = TEST_RANDOM;
	DoSingleTest();

	Ldd.DoConsumeRamSetup(32, TEST_LM_BLOCKSIZE);
	
	TestWhichTests = TEST_ALL;
	TestMaxLoops = 10;
	TestMultipleThreadCount	= 25;
	DoMultipleTest(ETrue);

	Ldd.DoConsumeRamFinish();
	
	TEST_NEXT((_L("Close test driver")));
	Ldd.Close();
	RUNTEST(User::FreeLogicalDevice(KPageStressTestLddName), KErrNone);
	if (TestIsDemandPaged)
		{
		TInt minSize = tempPages.iMinSize;
		TInt maxSize = tempPages.iMaxSize;
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}
	}

//
// E32Main
//
// Main entry point.
//

TInt E32Main()
	{
#ifndef TEST_ON_UNPAGED
	TRomHeader* romHeader = (TRomHeader*)UserSvr::RomHeaderAddress();
	if(!romHeader->iPageableRomStart)
		{
		TestIsDemandPaged = EFalse;
		}
#endif
	TUint start = User::TickCount();

	TBool parseResult = ParseCommandLine();

	if (TestExit)
		{
		return KErrNone;
		}

	AreWeTheTestBase();

	TInt  minSize = 8 * 4096;
	TInt  maxSize = 64 * 4096;
	SVMCacheInfo  tempPages;
	memset(&tempPages, 0, sizeof(tempPages));
	if (TestIsDemandPaged)
		{
		// get the old cache info
		UserSvr::HalFunction(EHalGroupVM,EVMHalGetCacheSize,&tempPages,0);
		// set the cache to our test value
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	// get the page size.
	UserSvr::HalFunction(EHalGroupKernel,EKernelHalPageSizeInBytes,&TestPageSize,0);

	if (!TestSilent)
		{
		test.Title();
		test.Start(_L("Demand Paging stress tests..."));
		test.Printf(_L("%S\n"), &TestNameBuffer);
		}

	if (parseResult)
		{
		if (!TestSilent)
			{
			extern TInt *CheckLdmiaInstr(void);
			test.Printf(_L("%S : CheckLdmiaInstr\n"), &TestNameBuffer);
			TInt   *theAddr = CheckLdmiaInstr();
			test.Printf(_L("%S : CheckLdmiaInstr complete 0x%x...\n"), &TestNameBuffer, (TInt)theAddr);
			}
		if (TestCheck)
			{
			CheckAlignments();
			}
		if (TestLowMem)
			{
			DoLowMemTest();
			}
		if (TestSingle)
			{
			DoSingleTest();
			}
		if (TestMultiple)
			{
			DoMultipleTest();
			}
		}
	else
		{
		PerformAutoTest();

		DoLowMemTest();

		if (TestWeAreTheTestBase)
			{
			RProcess		theProcess;
			TRequestStatus	status;

			TInt retVal = theProcess.Create(_L("t_pagestress_rom.exe"),_L(""));
			if (retVal != KErrNotFound)
				{
				RUNTEST1(retVal == KErrNone);
				theProcess.Logon(status);
				RUNTEST1(status == KRequestPending);
				theProcess.Resume();
				User::WaitForRequest(status);
				if (theProcess.ExitType() != EExitPending)
					{
					RUNTEST1(theProcess.ExitType() != EExitPanic);
					}
				}
			}
		}

	if (TestIsDemandPaged)
		{
		minSize = tempPages.iMinSize;
		maxSize = tempPages.iMaxSize;
		// put the cache back to the the original values.
		UserSvr::HalFunction(EHalGroupVM,EVMHalSetCacheSize,(TAny*)minSize,(TAny*)maxSize);
		}

	if (!TestSilent)
		{
		test.Printf(_L("%S : Complete (%u ticks)\n"), &TestNameBuffer, User::TickCount() - start);	
		test.End();
		}
	return 0;
	}


