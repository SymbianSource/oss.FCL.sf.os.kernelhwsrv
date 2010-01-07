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
// e32test\smp_demo\smp_demo.cpp
// Demonstration for SMP
// 
//

#include <e32test.h>
#include <u32hal.h>
#include <e32svr.h>
#include <f32file.h>
#include <hal.h>
#include <e32math.h>

RTest test(_L("SMP_DEMO"));

#define DEBUG_PRINT(__args)		test.Printf __args ;

TBool   TestThreadsExit = EFalse;
_LIT(KTestBlank, "");


//#define LOCK_TYPE	RMutex
//#define LOCK_TYPE_CREATE_PARAM	
#define LOCK_TYPE	RFastLock
#define LOCK_TYPE_CREATE_PARAM	
//#define LOCK_TYPE	RSemaphore
//#define LOCK_TYPE_CREATE_PARAM	0

#define MAX_THREADS		8		
#define MAX_CHAPTERS	28

TUint8		TestGuess[MAX_THREADS];
TInt		TestGuessReady[MAX_THREADS];
LOCK_TYPE	TestGuessLock[MAX_THREADS];

TInt		TestGuessChanged[MAX_THREADS];
LOCK_TYPE	TestChangedLock[MAX_THREADS];
TUint8		TestNext[MAX_THREADS];

TUint		TestGuessMisses[MAX_THREADS];
TUint		TestGuessCorrect[MAX_THREADS];
TUint		TestGuessIncorrect[MAX_THREADS];
TUint		TestGuessCollision[MAX_THREADS];
TInt		TestCpuCount = 0;

TBool		TestUseMathRandom = ETrue;
TBool		TestSingleCpu = EFalse;
TBool		TestDualCpu = EFalse;
TBool		TestNoPrint = EFalse;
TBool		TestSingleThread = EFalse;
TBool		TestUseAffinity = ETrue;

TInt LoadChapter(TInt chapterIndex, HBufC8 **aChapterPtrPtr)
	{
	RFile file;
	RFs fs;
	if (KErrNone != fs.Connect())
		{
		DEBUG_PRINT(_L("LoadChapter : Can't connect to the FS\n"));
		return KErrGeneral;
		}

	TBuf<32>	filename;
	filename.Format(_L("z:\\Test\\war_and_peace_ch%d.txt"), chapterIndex);

	TInt ret = file.Open(fs,filename,EFileRead);
	if (ret == KErrNone)
		{
		TInt fileSize = 0;
		ret = file.Size(fileSize);
		if (ret == KErrNone)
			{
			HBufC8 *theBuf = HBufC8::New(fileSize + 10);
			if (theBuf != NULL)
				{
				TPtr8 des2=theBuf->Des();
				ret = file.Read((TInt)0, des2,fileSize);
				if (ret == KErrNone)
					{
					*aChapterPtrPtr = theBuf;
					}
				else
					{
					DEBUG_PRINT((_L("LoadChapter : Read Failed for %S of %d\n"), &filename, fileSize));
					}
				}
			else
				{
				DEBUG_PRINT((_L("LoadChapter : Buffer Alloc Failed for %S\n"), &filename));
				}
			}
		else
			{
			DEBUG_PRINT((_L("LoadChapter : Size Failed for %S\n"), &filename));
			}
		file.Close();
		}
	else
		{
		DEBUG_PRINT((_L("LoadChapter : Open Failed for %S\n"), &filename));
		}
	
	return ret;
	}	

TInt SetCpuAffinity(TInt aThreadId)
	{
	if (TestUseAffinity)
		{
		TUint32 cpu;

		if (TestCpuCount == 4)
			cpu = (TUint32)(aThreadId % 3) + 1;
		else if (TestCpuCount == 2)
			cpu = (TUint32)1;
		else
			cpu = 0;

		TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)cpu, 0);
		test(r==KErrNone);	
		return r;
		}
	return KErrNone;
	}


LOCAL_C TInt DemoThread(TAny* aUseTb)
	{
	TInt	threadId = (TInt)aUseTb;
	
	SetCpuAffinity(threadId);
	User::After(100);

	TestGuessChanged[threadId] = EFalse;
	TestGuessReady[threadId] = EFalse;
	TestGuessMisses[threadId] = 0;
	TestGuessCorrect[threadId] = 0;
	TestGuessIncorrect[threadId] = 0;
	TestGuessCollision[threadId] = 0;

	TUint8	guess = 0;
	TUint8	nextChar = TestNext[threadId];
	TBool	correct = EFalse;

	while (!TestThreadsExit)
		{
		correct = EFalse;

		if (TestUseMathRandom)
			guess = (TUint8)Math::Random();
		else
			guess ++;

		if (TestGuessChanged[threadId])
			{
			TestChangedLock[threadId].Wait();
			nextChar = TestNext[threadId];
			TestGuessChanged[threadId] = EFalse;
			TestChangedLock[threadId].Signal();			
			}
		correct = (nextChar == guess);

		if (correct)
			{
			if (TestGuessReady[threadId] == EFalse)
				{
				TestGuessLock[threadId].Wait();
				TestGuess[threadId] = guess;
				TestGuessReady[threadId] = ETrue;
				TestGuessLock[threadId].Signal();
				TestGuessCorrect[threadId] ++;
				}
			else
				{
				TestGuessMisses[threadId] ++;
				}
			}
		else
			{
			TestGuessIncorrect[threadId] ++;
			}
		if (TestCpuCount == 1)
			{
			User::After(0);
			}
		}
	return KErrNone;
	}

TInt NumberOfCpus()
	{
	TInt r = UserSvr::HalFunction(EHalGroupKernel, EKernelHalNumLogicalCpus, 0, 0);
	test(r>0);
	return r;
	}

TInt ParseArgs(void)
	{
	TBuf<256> args;
	User::CommandLine(args);
	TLex	lex(args);

	FOREVER
		{
		TPtrC  token=lex.NextToken();
		if(token.Length()!=0)
			{
			if (token == _L("unbound"))
				{
				TestUseMathRandom = EFalse;
				}
			if (token == _L("single"))
				{
				TestSingleCpu = ETrue;
				}
			if (token == _L("dual"))
				{
				TestDualCpu = ETrue;
				}
			if (token == _L("silent"))
				{
				TestNoPrint = ETrue;
				}
			if (token == _L("onethread"))
				{
				TestSingleCpu = ETrue;
				TestSingleThread = ETrue;
				}
			if (token == _L("help"))
				{
				test.Printf(_L("smp_demo: unbound | single | onethread | silent | dual | noaffinity | help \n"));
				return -1;
				}
			if (token == _L("noaffinity"))
				{
				TestUseAffinity = EFalse;
				}
			}
		else
			{
			break;
			}
		}
		return KErrNone;
	}

TInt E32Main()
	{
	test.Title();
	test.Start(_L("SMP Demonstration guessing War and Peace...."));
	
	if (ParseArgs() != KErrNone)
		{
		test.Getch();
		test.End();
		return KErrNone;
		}

	TUint   start = User::TickCount();
	TInt	tickPeriod = 0;
	HAL::Get(HAL::ESystemTickPeriod, tickPeriod);

	if (TestSingleCpu)
		{
		TestCpuCount = 1;
		}
	else if (TestDualCpu)
		{
		TestCpuCount = 2;
		}
	else
		{
		TestCpuCount = NumberOfCpus();
		}
	
	DEBUG_PRINT((_L("CPU Count %d\n"), TestCpuCount));

	TRequestStatus	theStatus[MAX_THREADS];
	RThread			theThreads[MAX_THREADS];
	TBool			threadInUse[MAX_THREADS];

	TInt	index;
	TInt	maxChapters = MAX_CHAPTERS;

	if (TestUseMathRandom)
		{
		maxChapters = 2;
		}

	TInt	maxIndex = TestCpuCount - 1;
	if (maxIndex == 0)
		{
		maxChapters = 2;
		maxIndex = 1;
		}
	else if ((maxIndex == 1) && (TestUseMathRandom))
		{
		maxChapters = 4;
		}

	TInt	ret;
	TUint32 cpu = 0;

	if (TestUseAffinity)
		{
		UserSvr::HalFunction(EHalGroupKernel, EKernelHalLockThreadToCpu, (TAny *)cpu, 0);
		}

	if (TestSingleThread)
		{
		TInt	chapterIndex;
		TUint8	guess = 0;
		
		maxChapters = MAX_CHAPTERS;

		TRequestStatus keyStatus;
		CConsoleBase*  console=test.Console();
		
		console->Read(keyStatus);

		for (chapterIndex = 0; chapterIndex < maxChapters; chapterIndex ++)
			{
			HBufC8 *chapterPtr = NULL;
			ret = LoadChapter(chapterIndex + 1, &chapterPtr);
			if ((ret != KErrNone) || (chapterPtr == NULL))
				{
				DEBUG_PRINT((_L("E32Main: LoadChapter failed %d\n"), ret));
				}
			else
				{
				TPtr8			theDes = chapterPtr->Des();
				TUint8		   *pData = (TUint8 *)theDes.Ptr();
				TInt			dataLength = chapterPtr->Length();


				while (dataLength > 0)
					{
					if (TestUseMathRandom)
						guess = (TUint8)Math::Random();
					else
						guess ++;

 					if (*pData == guess)
						{
						pData ++;
						dataLength --;
						if (!TestNoPrint)
							{
							test.Printf(_L("%c"), (TUint8)guess);
							}
						}
					if (keyStatus != KRequestPending)
						{
						if (console->KeyCode() == EKeyEscape)
							{
							TestThreadsExit = ETrue;
							break;
							}
						console->Read(keyStatus);
						}
					}
				// clean up
				delete chapterPtr;
				test.Printf(_L("\n\n"));
				if (TestThreadsExit)
					{
					break;
					}
				}
			}
		console->ReadCancel();
		test.Printf(_L("Finished after %d chapters!\n"),chapterIndex);
		}
	else
		{
		for (index = 0; index < maxIndex; index ++)
			{
			TestGuessLock[index].CreateLocal(LOCK_TYPE_CREATE_PARAM);
			TestChangedLock[index].CreateLocal(LOCK_TYPE_CREATE_PARAM);
			ret = theThreads[index].Create(KTestBlank,DemoThread,KDefaultStackSize,NULL,(TAny*) index);
			if (ret == KErrNone)
				{
				theThreads[index].Logon(theStatus[index]);
				if (theStatus[index] != KRequestPending)
					{
					DEBUG_PRINT((_L("E32Main: !KRequestPending %d\n"), theStatus[index].Int() ));
					}	
				theThreads[index].Resume();
				threadInUse[index] = ETrue;
				DEBUG_PRINT((_L("E32Main: starting thread %d %d\n"), index, index % TestCpuCount));
				}
			else
				{
				DEBUG_PRINT((_L("E32Main: Create thread failed %d\n"), ret));
				return KErrGeneral;
				}
			}

		TInt	chapterIndex;
		TInt    index2;

		TRequestStatus keyStatus;
		CConsoleBase*  console=test.Console();
		
		console->Read(keyStatus);

		for (chapterIndex = 0; chapterIndex < maxChapters; chapterIndex ++)
			{
			HBufC8 *chapterPtr = NULL;
			ret = LoadChapter(chapterIndex + 1, &chapterPtr);
			if ((ret != KErrNone) || (chapterPtr == NULL))
				{
				DEBUG_PRINT((_L("E32Main: LoadChapter failed %d\n"), ret));
				}
			else
				{
				TPtr8			theDes = chapterPtr->Des();
				TUint8		   *pData = (TUint8 *)theDes.Ptr();
				TInt			dataLength = chapterPtr->Length();
				for (index2 = 0; index2 < maxIndex; index2 ++)
					{
					TestChangedLock[index2].Wait();
					TestGuessChanged[index2] = ETrue;
					TestNext[index2] = (TUint8)*pData;
					TestChangedLock[index2].Signal();
					}
				// where the real code goes!!
				TUint8	guess = 0;
				TBool	wasReady = EFalse;
				while (dataLength > 0)
					{
					for (index = 0; index < maxIndex; index ++)
						{
						wasReady = EFalse;
						if (TestGuessReady[index])
							{
							wasReady = ETrue;
							TestGuessLock[index].Wait();
							guess = (TUint8)TestGuess[index];
							TestGuessReady[index] = EFalse;
							TestGuessLock[index].Signal();
							}
						if (wasReady)
							{
							if (*pData == guess)
								{
								pData ++;
								dataLength --;
								for (index2 = 0; index2 < maxIndex; index2 ++)
									{
									TestChangedLock[index2].Wait();
									TestNext[index2] = (TUint8)*pData;
									TestGuessChanged[index2] = ETrue;
									TestChangedLock[index2].Signal();
									}
								if (!TestNoPrint)
									{
									test.Printf(_L("%c"), (TUint8)guess);
									}
								}
							else
								{
								TestGuessCollision[index] ++;
								}
							}
						if (TestCpuCount == 1)
							{
							User::After(0);
							}
						}
					if (keyStatus != KRequestPending)
						{
						if (console->KeyCode() == EKeyEscape)
							{
							TestThreadsExit = ETrue;
							break;
							}
						console->Read(keyStatus);
						}
					}
				// clean up
				delete chapterPtr;
				test.Printf(_L("\n\n"));
				if (TestThreadsExit)
					{
					break;
					}
				}
			}

		console->ReadCancel();
		
		test.Printf(_L("Finished after %d chapters!\n"),chapterIndex);
		for (index = 0; index < maxIndex; index ++)
			{
			test.Printf(_L("Thread %d stalls %u correct %u incorrect %u collision %u\n"), index, TestGuessMisses[index],TestGuessCorrect[index],TestGuessIncorrect[index], TestGuessCollision[index]);
			}
			
		// real code ends!!
		TestThreadsExit = ETrue;
		
		TBool		anyUsed = ETrue;

		while(anyUsed)
			{
			anyUsed = EFalse;

			for (index = 0; index < maxIndex; index++)
				{
				if (threadInUse[index])
					{
					if (theThreads[index].ExitType() != EExitPending)
						{
						threadInUse[index] = EFalse;
						TestGuessLock[index].Close();
						TestChangedLock[index].Close();
						}
					else
						{
						anyUsed = ETrue;
						}
					}
				}
			}
		}		
	TUint time = TUint((TUint64)(User::TickCount()-start)*(TUint64)tickPeriod/(TUint64)1000000);
	test.Printf(_L("Complete in %u seconds\n"), time);	
	test.Getch();
	test.End();
	return KErrNone;
	}

