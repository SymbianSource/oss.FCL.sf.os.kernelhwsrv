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
// e32test/defrag/perf/t_perf.cpp
// t_pagemove loads and opens the logical device driver ("D_PAGEMOVE.LDD"). 
// Following this, it requests that the driver attempt to move various kinds of pages
// directly amd notes time taken for each of the moves.
// Platforms/Drives/Compatibility:
// Hardware only. No defrag support on emulator. 
// 2  -  Test the performance of moving user chunk pages
// 3  -  Test the performance of moving dll pages
// 4  -  Test the performance of moving kernel stack pages
// 
//

//! @SYMTestCaseID			KBASE-T_DEFRAGPERF-0601
//! @SYMTestType			CT
//! @SYMPREQ				PREQ308
//! @SYMTestCaseDesc		Testing performace of page moving
//! @SYMTestActions			1  -  Test the performance of moving code chunk pages
//! @SYMTestExpectedResults Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//--------------------------------------------------------------------------------------------------
#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32std.h>
#include <e32std_private.h>

#include "..\d_pagemove.h"
#include "..\..\mmu\mmudetect.h"
#include "t_perf.h"


LOCAL_D RTest test(_L("T_DEFRAGPERF"));

const TPtrC KLddFileName=_L("D_PAGEMOVE.LDD");

const TUint KRepetitions=50;
const TUint KPageMoves = 2000;	// default to moving a page 2000 times


TInt gPageSize;

#define DEFRAG_CHUNK_MAX (4 * 1024 * 1024)
#define DEFRAG_CHUNK_MIN (1 * 1024 * 1024)

#define MIN_PROCS 10
#define MAX_PROCS 50
#define PROC_INCREMENTS 10

_LIT(BaseFileName,   "t_defragtestperf");
_LIT(BaseFileNameExt, ".exe");

LOCAL_C void TestUserDataMove(RPageMove& pagemove)
{
	RChunk chunk;
	TInt size; 
	TInt r = KErrGeneral;

	DefragLatency timer;
	timer.CalibrateTimer(test);
	 
	size = DEFRAG_CHUNK_MAX; 

	while (size > DEFRAG_CHUNK_MIN) 
		{
		/* Create a local chunk */
		r = chunk.CreateLocal(size,size);
		if (r == KErrNone)
			{
			TEST_PRINTF(_L("Created %dMB chunk\n"), size/(1024 * 1024));
			break;
			}
		size = size / 2;
		}

	test(r == KErrNone);

	/* Initialise the Chunk */
	TUint8* base = (TUint8 *)chunk.Base();	
	for (TUint i = 0; i < size/sizeof(*base); i++) 
		{
		base[i] = i;
		}


	TEST_PRINTF(_L("Starting to move chunk pages (%x %d) ...\n"), chunk.Base(), size);
	TUint j = 0;
	for (; j < KRepetitions; j++) 
		{
		TInt size2 = size;
		base = (TUint8 *)chunk.Base();	
		timer.StartTimer();
		while (size2)
			{
			test_KErrNone(pagemove.TryMovingUserPage(base));
			base += gPageSize;
			size2 -= gPageSize;
			}

		test(timer.StopTimer(test) > 0);
		}

	DTime_t average, max, min, delay;
	TEST_PRINTF(_L("Finished moving chunk pages\n"));
	average = timer.GetResult(max, min, delay);
	test.Printf(_L("Fast counter ticks to move %d chunk pages: Av %d Min %d Max %d (Overhead %d)\n"), size/gPageSize, average, min, max, delay);
	test.Printf(_L("Average of %d ticks to move one page\n\n"), average / (size/gPageSize));

	base = (TUint8 *)chunk.Base();	
	for (TUint i = 0; i < size/sizeof(*base); i++) 
		{
		test_Equal((TUint8)i, base[i]);
		}
}
	
TInt CreateProc(RProcess& aProcess, TInt aIndex)
	{
	TFileName filename; 
	TRequestStatus status;
	TBuf<64> command;

	filename.Format(_L("%S%S"), &BaseFileName, &BaseFileNameExt);		
	TInt r=aProcess.Create(filename, command);
	
	if (r!=KErrNone)
		{
		test.Printf(_L("Process %d could not be loaded!\n"), aIndex);
		return r;
		}

	aProcess.Rendezvous(status);
	aProcess.Resume();
	User::WaitForRequest(status);
	return KErrNone; 
	}

void KillAllProcs(RProcess *aProcess, TInt aNum, TInt aError)
	{
	TInt i;
	for (i = 0; i < aNum; i++) 
		{
		aProcess[i].Kill(aError);
		}
	}

TInt TestDLL(int aNumProcess, RPageMove& aPageMove)
	{
	RProcess *process;
	TInt retVal;

	process = new RProcess[aNumProcess];
	
	if (process == NULL)
		return KErrGeneral;

	TEST_PRINTF(_L("Starting %d processes\n"), aNumProcess);
	for (TInt i = 0; i < aNumProcess; i++) 
		{
		retVal = CreateProc(process[i], i);
		if (retVal != KErrNone)
			{
			// More Cleanup ? 
			KillAllProcs(process, i, KErrNone);
			delete[] process;
			return retVal;
			}
		}

	/* Now Map the DLL to this process and Run the tests */
	DllDefrag dll;
	TEST_PRINTF(_L("Main Proc Loading The DLL ... \n"));
	retVal = dll.LoadTheLib(0, test);
	
	if (retVal != KErrNone)
		{
		KillAllProcs(process, aNumProcess, retVal);
		test.Printf(_L("Error (%d) Loading The DLLs!\n"), retVal);
		delete[] process;
		return KErrGeneral;
		}

	retVal = dll.TestDLLPerformance(0, aPageMove, test);
	KillAllProcs(process, aNumProcess, KErrNone);
	delete[] process;

	return retVal;
	}

/*  
 * Is this portable .. Hell No, so dont ask.
 */
#define isNum(a) ((a) >= '0' && (a) <= '9')

TInt atoi(const TBuf<64>& buf, TInt pos)
	{
	TInt i = pos;
	TInt num = 0;
	TInt len = User::CommandLineLength();
	while(i < len)
	{
		TInt8 ch = buf[i++];
		if (!isNum(ch)) 
			{
			test.Printf(_L("Invalid Argument!\n"));
			return KErrGeneral;
			}
		num = (num * 10) + (ch - '0');
	}
	return num;
	}

void TestCodeChunkMoving(RPageMove aPageMove)
	{
	_LIT(ELOCL_DEFAULT, "");
	_LIT(ELOCLUS, "T_LOCLUS_RAM");

	// Change locale and move the locale page repeatedly
	// On multiple memmodel this is the easiest way to move CodeChunk pages.


	// Setup the RAM loaded US Locale DLL
	test_KErrNone(UserSvr::ChangeLocale(ELOCLUS));
	TLocale locale;
	locale.Refresh();

	
	// Now get a pointer to some data in the DLL. This will be used to move a
	// page from the dll 
	SLocaleLanguage localeLanguage;
	TPckg<SLocaleLanguage> localeLanguageBuf(localeLanguage);
	TInt r = RProperty::Get(KUidSystemCategory, KLocaleLanguageKey, localeLanguageBuf);
	test_KErrNone(r);
	TAny* localeAddr = (TAny *) localeLanguage.iDateSuffixTable;
	test(localeAddr != NULL);
	

	// Now we have a RAM loaded locale page determine the performance
	TEST_PRINTF(_L("Start moving a RAM loaded locale page\n"));
	// Setup the timer
	DefragLatency timer;
	timer.CalibrateTimer(test);
	TUint reps = 0;
	for (; reps < KRepetitions; reps++)
		{
		timer.StartTimer();
		TUint i = 0;
		for (; i < KPageMoves; i++)
			{
			test_KErrNone(aPageMove.TryMovingLocaleDll(localeAddr));
			}
		test_Compare(timer.StopTimer(test), >, 0);
		}
	DTime_t max, min, delay;
	TUint average = timer.GetResult(max, min, delay);
	test.Printf(_L("Fast counter ticks to move code chunk page %d times: Av %d Min %d Max %d (Overhead %d)\n"), KPageMoves, average, min, max, delay);
	test.Printf(_L("Average of %d ticks to move one page\n\n"), average/ KPageMoves);
	
	
	// Reset the Locale to the default
	r=UserSvr::ChangeLocale(ELOCL_DEFAULT);	
	test(r==KErrNone);
	locale.Refresh();
	}

GLDEF_C TInt E32Main()
    {
	test.Title();	
	TInt procs = 1;
	_LIT(KStart,"-procs=");
	const TInt offset = KStart().Length();

	TBuf<64> cmd;
	User::CommandLine(cmd);

	if (!HaveMMU())
		{
		test.Printf(_L("This test requires an MMU\n"));
		return KErrNone;
		}
	test.Start(_L("Load test LDD"));
	TInt r=User::LoadLogicalDevice(KLddFileName);
	test(r==KErrNone || r==KErrAlreadyExists);

	r=UserHal::PageSizeInBytes(gPageSize);
	test_KErrNone(r);

	RPageMove pagemove;
	test.Next(_L("Open test LDD"));
	r=pagemove.Open();
	test_KErrNone(r);

	DefragLatency timer;
	timer.CalibrateTimer(test);
	test.Printf(_L("\nFast Counter Frequency = %dHz \n\n"), timer.iFastCounterFreq);

	
	test.Next(_L("Test performance of moving code chunk pages"));
	TestCodeChunkMoving(pagemove);

	/* 
	 * Test User Data.
	 * Create a large chunk ~ 4MB (> L2 Cache) and move pages from that chunk
	 */
	test.Next(_L("Test performance of moving user chunk pages"));
	TestUserDataMove(pagemove);

	/* Test DLL Move */
	test.Next(_L("Test performance of moving dll pages"));	
	TInt pos = cmd.FindF(KStart);
	if (pos >= 0) 
		{
		pos += offset;
		procs = atoi(cmd, pos);
		}

	if (procs < MIN_PROCS)
		procs = MIN_PROCS;
	else if (procs > MAX_PROCS)
		procs = MAX_PROCS;

	for (TInt i = 1; ; )
		{ 
		test.Printf(_L("Testing with %d Processes mapping a DLL\n"), i);
		TestDLL(i, pagemove);

		if (i >= procs)
			break;

		if (i + PROC_INCREMENTS >= procs)
			i = procs;
		else
			i += PROC_INCREMENTS;
		}

	if ((MemModelAttributes()&EMemModelTypeMask) != EMemModelTypeFlexible)
		{
		test.Next(_L("Test performance of moving kernel stack pages"));
		r = pagemove.TestKernelDataMovePerformance();
		test_KErrNone(r);
		}

	test.Next(_L("Close test LDD"));

	pagemove.Close();
	User::FreeLogicalDevice(KLddFileName);
	test.End();
	return(KErrNone);
    }
