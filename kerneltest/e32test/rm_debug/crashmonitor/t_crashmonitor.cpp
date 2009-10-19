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
// Tests the functionality of the SCM Libraries
// 
//

#include "t_crashmonitor.h"

#include <crashlogwalker.h>

using namespace Debug;

/**
 * Test suite version
 */
const TVersion testVersion(1,1,0);


/**
 * Constructor
 */
CSCMLibraryClient::CSCMLibraryClient():
	iNonCachedWriter(iBuffer, EFalse),
	iCachedWriter(iBuffer, EFalse),
	iWriter(NULL),
	iReader(iBuffer)	
	{
	}

/**
 * First phase constructor
 */
CSCMLibraryClient* CSCMLibraryClient::NewL()
	{
	CSCMLibraryClient* self = new(ELeave) CSCMLibraryClient();
  	self->ConstructL();
	return self;
	}

/**
 * Destructor
 */
CSCMLibraryClient::~CSCMLibraryClient()
	{
	}

/**
 * ConstructL
 */
void CSCMLibraryClient::ConstructL()
	{}



/**
 * Prints usage of this test suite
 */
void CSCMLibraryClient::PrintUsage()
	{
	test.Printf(_L("Invoke with arguments:\n"));
	test.Printf(_L("-r: run specified tests in reverse order\n"));
	test.Printf(_L("-h: display usage information\n"));
	test.Printf(_L("-v: display version\n"));
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
 * Parses arguments from command line
 * @param aMode Argument mode
 * @param aTests Array of tests
 */
void CSCMLibraryClient::ParseCommandLineL(TUint32& aMode, RArray<TInt>& aTests)
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

/**
 * This will run all tests in the suite
 */
void CSCMLibraryClient::ClientAppL()
	{	
	
	FillArray();
	
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

	HelpStartTestTimer();

	// first run al tests with non cached writer
	iWriter = &iNonCachedWriter;
	for(TInt i=0; i<numberOfTests; i++)
		{
		RunTest(testsToRun[i]);
		}
	
	iWriter = &iCachedWriter;	
	for(TInt i=0; i<numberOfTests; i++)
		{
		RunTest(testsToRun[i]);
		}

	
	testsToRun.Close();

	HelpStopTestTimer();

	ReportPerformance();
	
	test.End();
	}

/**
 * This fills our array of test functions with function pointers to tests
 */
void CSCMLibraryClient::FillArray() 
	{
	
	//iTestArray[0] = new TFunctionData();
	iTestArray[0].iFunctionPtr = &CSCMLibraryClient::TestCheckSum;
	iTestArray[0].iFunctionName = _L("TestCheckSum");
	
	iTestArray[1].iFunctionPtr = &CSCMLibraryClient::TestLockDataSerialization;
	iTestArray[1].iFunctionName = _L("TestLockDataSerialization");
	
	iTestArray[2].iFunctionPtr = &CSCMLibraryClient::TestOffsetsHeaderSerialization;
	iTestArray[2].iFunctionName = _L("TestOffsetsHeaderSerialization");
	
	iTestArray[3].iFunctionPtr = &CSCMLibraryClient::TestInfoHeaderSerialization;
	iTestArray[3].iFunctionName = _L("TestInfoHeaderSerialization");
	
	iTestArray[4].iFunctionPtr = &CSCMLibraryClient::TestRawData;
	iTestArray[4].iFunctionName = _L("TestRawData");

	iTestArray[5].iFunctionPtr = &CSCMLibraryClient::TestProcessData;
	iTestArray[5].iFunctionName = _L("TestProcessData");
	
	iTestArray[6].iFunctionPtr = &CSCMLibraryClient::TestThreadData;
	iTestArray[6].iFunctionName = _L("TestThreadData");
	
	iTestArray[7].iFunctionPtr = &CSCMLibraryClient::TestThreadStack;
	iTestArray[7].iFunctionName = _L("TestThreadStack");
		
	iTestArray[8].iFunctionPtr = &CSCMLibraryClient::TestRegisterValue;
	iTestArray[8].iFunctionName = _L("TestRegisterValue");
		
	iTestArray[9].iFunctionPtr = &CSCMLibraryClient::TestRegisterSet;
	iTestArray[9].iFunctionName = _L("TestRegisterSet");

	iTestArray[10].iFunctionPtr = &CSCMLibraryClient::TestMemoryDump;
	iTestArray[10].iFunctionName = _L("TestMemoryDump");
	
	iTestArray[11].iFunctionPtr = &CSCMLibraryClient::TestCodeSegmentSet;
	iTestArray[11].iFunctionName = _L("TestCodeSegmentSet");
	
	iTestArray[12].iFunctionPtr = &CSCMLibraryClient::TestCodeSegment;
	iTestArray[12].iFunctionName = _L("TestCodeSegment");
	
	iTestArray[13].iFunctionPtr = &CSCMLibraryClient::TestTraceDump;
	iTestArray[13].iFunctionName = _L("TestTraceDump");
	
	iTestArray[14].iFunctionPtr = &CSCMLibraryClient::TestVariantSpecificData;
	iTestArray[14].iFunctionName = _L("TestVariantSpecificData");
	
	iTestArray[15].iFunctionPtr = &CSCMLibraryClient::TestRomHeaderData;
	iTestArray[15].iFunctionName = _L("TestRomHeaderData");
	
	iTestArray[16].iFunctionPtr = & CSCMLibraryClient::TestSCMLockData;
	iTestArray[16].iFunctionName = _L("TestSCMLockData");

	};

/**
 * Entry point for crash monitor tests
 */
GLDEF_C TInt E32Main()
	{
	TInt ret = KErrNone;
	
	CTrapCleanup* trap = CTrapCleanup::New();
	if (!trap)
		return KErrNoMemory;
	
   	test.Title();
   	CSCMLibraryClient* tester = CSCMLibraryClient::NewL();
   	if (tester != NULL)
       {
        __UHEAP_MARK;
	    TRAP(ret,tester->ClientAppL());
	    __UHEAP_MARKEND;

	   delete tester;
       }
       
	delete trap;
	return ret;
	}

/**
 * Runs a given test identified by argument
 * @param aTestNumber Test to run
 */
void CSCMLibraryClient::RunTest(TInt aTestNumber)
	{
	if( (aTestNumber<0) || (aTestNumber>=KMaxTests) )
		{
		User::Panic(_L("Test number out of range"), aTestNumber);
		}
	__UHEAP_MARK;
	
	if(iTestArray[aTestNumber].iFunctionPtr)
		{
		test.Printf(_L("pre-run test %d"), aTestNumber);
		(this->*(iTestArray[aTestNumber].iFunctionPtr))();
		test.Printf(_L("post-run test %d"), aTestNumber);
		}

	__UHEAP_MARKEND;
	}

/**
 * Prints the version of this test suite
 */
void CSCMLibraryClient::PrintVersion()
	{
	test.Printf(_L("\nt_crashmonitor_lib.exe\nVersion: %S\n"), &(testVersion.Name()));
	test.Printf(_L("Press any key...\n"));
	test.Getch();
	}
/**
 * Reports performance metrics from all the tests
 */
void CSCMLibraryClient::ReportPerformance(void)
	{
	test.Printf(_L("\nPerformance\n"));
	test.Printf(_L("========================\n"));

	
	// Runtime
	TInt ticks = HelpGetTestTicks();
#ifndef __WINS__
	test (ticks != 0);  
#endif
	TInt nkTicksPerSecond = HelpTicksPerSecond();

#ifndef __WINS__
	test (nkTicksPerSecond != 0);
#endif

	test.Printf(_L("Total test runtime: %d seconds\n"),ticks/nkTicksPerSecond);

	test.Printf(_L("\n"));
	}

/**
 * Returns the number of nanokernel ticks in one second
 * @return Number of nanokernel ticks. 0 if unsuccesful
 */
TInt CSCMLibraryClient::HelpTicksPerSecond(void)
	{
	TInt nanokernel_tick_period;
	HAL::Get(HAL::ENanoTickPeriod, nanokernel_tick_period);
	
	ASSERT(nanokernel_tick_period != 0);

	static const TInt KOneMillion = 1000000;

	return KOneMillion/nanokernel_tick_period;
	}

void CSCMLibraryClient::DoWrite(MByteStreamSerializable& aObjectToWrite, TInt aPosition)
	{
	if(iWriter == &iCachedWriter)
		{
		iCachedWriter.SetPosition(aPosition);
		test(aObjectToWrite.Serialize(iCachedWriter) == KErrNone);
		iCachedWriter.FlushCache();	
		}
	else if(iWriter == &iNonCachedWriter)
		{
		iNonCachedWriter.SetPosition(aPosition);		
		test(aObjectToWrite.Serialize(iNonCachedWriter) == KErrNone);	
		}
	else
		{
		// no writer
		test(EFalse);
		}
	}

// BASE granted test id's 2364 to 2394

//---------------------------------------------
// !@SYMTestCaseID KBASE-T_SCMLIB-2364
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TScmChecksum structure 
//! @SYMTestActions TScmChecksum serialized & then deserialized
//! @SYMTestExpectedResults TScmChecksum structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestCheckSum()
	{
	test.Next(_L("TestCheckSum\n"));
	
	TScmChecksum chksm1;
	chksm1.Reset();
	
	TInt blocksize1 = 123;
	TInt blocksize2 = 166;
	
	TScmChecksum chksm2;
	chksm2.Reset();
		
	test(ChecksumHelper(chksm1, blocksize1, iBuffer, KBufLen));
	test(ChecksumHelper(chksm2, blocksize2, iBuffer, KBufLen));
		
	test(chksm1  == chksm2);

	}

TBool CSCMLibraryClient::ChecksumHelper(TScmChecksum& aChecksum, TUint aBlocksize, TUint8* aBuffer, TUint aBufferLen) 
	{
	if( aBlocksize == 0 || aBufferLen == 0 )
		{
		return EFalse;
		}
	
	TInt remaining = aBufferLen;
	TInt pos = 0;
		
	while(remaining > aBlocksize)
		{		
		aChecksum.ChecksumBlock(aBuffer + pos, aBlocksize);
		pos += aBlocksize;
		remaining -= aBlocksize;
		}
	
	aChecksum.ChecksumBlock(aBuffer + pos, remaining);
	pos += remaining;
	
	return (pos == aBufferLen);
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2365
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TSCMLockData structure 
//! @SYMTestActions TSCMLockData serialized & then deserialized
//! @SYMTestExpectedResults TSCMLockData structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
void CSCMLibraryClient::TestLockDataSerialization()
	{	
	test.Next(_L("TestLockDataSerialization\n"));	

	TSCMLockData lockData1;
	
	//Arbitrary values
	lockData1.SetLockCount(10);
	lockData1.SetMutexHoldCount(17);
	lockData1.SetMutexThreadWaitCount(36);

	DoWrite(lockData1);
	
	iReader.SetPosition(0);
	//Test deserialisation works
	TSCMLockData lockData2;	
	test(lockData2.Deserialize(iReader) == KErrNone);
	
	//Test we got back the correct object
	test(lockData1 == lockData2);
	}

//---------------------------------------------
//! //! @SYMTestCaseID KBASE-T_SCMLIB-2366
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TCrashOffsetsHeader structure 
//! @SYMTestActions TCrashOffsetsHeader serialized & then deserialized
//! @SYMTestExpectedResults TCrashOffsetsHeader structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//--------------------------------------------- 
void CSCMLibraryClient::TestOffsetsHeaderSerialization()
	{
	test.Next(_L("TestOffsetsHeaderSer ialization\n"));	
	
	TCrashOffsetsHeader header1;
	header1.iCTFullRegOffset = 123; 
	header1.iCTUsrStkOffset = 456;
	header1.iCTSvrStkOffset = 789;
	header1.iCPMetaOffset = 1001;
	header1.iCTMetaOffset = 99;		
	header1.iCPCodeSegOffset = 1234;
	header1.iSysUsrStkOffset = 3456;
	header1.iSysSvrStkOffset = 255;
	header1.iSysUsrRegOffset = 999;
	header1.iSysSvrRegOffset = 2002;
	header1.iTLstOffset = 3003;
	header1.iPLstOffset = 4004;
	header1.iSysCodeSegOffset = 5005;
	header1.iExcStkOffset = 6006;
	header1.iTraceOffset = 1233;
	header1.iScmLocksOffset = 3421;
	header1.iKernelHeapOffset = 89;
	header1.iVarSpecInfOffset = 0;
	header1.iRomInfoOffset = 123;

	DoWrite(header1);
	
	iReader.SetPosition(0);
	TCrashOffsetsHeader header2;
	
	test(header2.Deserialize(iReader) == KErrNone);
	
	test(header2 == header1);
	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2367
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TCrashInfoHeader structure 
//! @SYMTestActions TCrashInfoHeader serialized & then deserialized
//! @SYMTestExpectedResults TCrashInfoHeader structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestInfoHeaderSerialization()
	{
	test.Next(_L("TestInfoHeaderSerialization\n"));	

	TCrashInfoHeader infoHeader1;
	infoHeader1.iLogSize = 5000;	
	infoHeader1.iFlashAlign = 4;
	infoHeader1.iCachedWriterSize = 16;
	infoHeader1.iPid = 1001;
	infoHeader1.iTid = 2002;
	infoHeader1.iExitType = 90;
	infoHeader1.iExitReason = 23;	
	infoHeader1.iExcCode = 8899;
	infoHeader1.iCrashTime = 12345;	
	infoHeader1.iCrashId = 23;
	infoHeader1.iFlashBlockSize = 256 * 1024;
	infoHeader1.iFlashPartitionSize = 1024 * 1024;
	
	TVersion ver(21,43,54);
	
	infoHeader1.iSCMDataTypesVersion = ver;

	DoWrite(infoHeader1);
	
	iReader.SetPosition(0);
	TCrashInfoHeader infoHeader2;
	
	test(infoHeader2.Deserialize(iReader) == KErrNone);
	
	test(infoHeader2 == infoHeader1);
		
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2368 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TRawData structure 
//! @SYMTestActions TRawData serialized & then deserialized
//! @SYMTestExpectedResults TRawData structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestRawData()
	{
	test.Next(_L("TestRawData\n"));
	
	const TInt KLen = 256;
	
	TUint8 data[KLen];
	
	for(TInt i=0;i<KLen;i++)
		{
		data[i] = ((i<<2) % KLen) + i;
		}
	
	TRawData rawData1;

	rawData1.iLength = KLen;
	rawData1.iData.Set(const_cast<TUint8*>(data), KLen, KLen);;

	DoWrite(rawData1);
	
	TPtr8 p(iBuffer, KBufLen, KBufLen);
	TCrashLogWalker walker(p);
	TInt pos = 0;
	TInt len = 0;
	TRawData* rawData2 = walker.GetRawDataTypeL(pos, len, p, 0);

	CleanupStack::PushL(rawData2);
	
	test(rawData1.iLength == rawData2->iLength);
	test(rawData1.iData.Compare(rawData2->iData) == 0);
	
	CleanupStack::PopAndDestroy(rawData2);
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2369 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TProcessData structure 
//! @SYMTestActions TProcessData serialized & then deserialized
//! @SYMTestExpectedResults TProcessData structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestProcessData()
	{
	test.Next(_L("TestProcessData\n"));
	
	TProcessData procData1;
	
	procData1.iPriority = 99;		
	procData1.iPid = MAKE_TINT64(34567,12345);
	
	
	procData1.iName.Zero();
	for(TInt i=0;i<KMaxProcessName;i++)
		{	
		procData1.iName.Append( TChar( (i%60) + 32 ));
		}
	
	procData1.iNamesize = KMaxProcessName;		

	DoWrite(procData1);
	
	TProcessData procData2;
	iReader.SetPosition(0);
	
	test(procData2.Deserialize(iReader) == KErrNone);
	test(procData1.iPriority == procData1.iPriority);	
	test(procData1.iPid == procData2.iPid);	
	test(procData1.iNamesize == procData2.iNamesize);
	test(procData1.iName.Compare(procData2.iName) == 0);	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2370 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TThreadData structure 
//! @SYMTestActions TThreadData serialized & then deserialized
//! @SYMTestExpectedResults TThreadData structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestThreadData()
	{
	test.Next(_L("TestThreadData\n"));
	
	TThreadData threadData1;

	threadData1.iPriority = 3455;
	threadData1.iTid = MAKE_TINT64(34998,18345);
	threadData1.iOwnerId = MAKE_TINT64(34448,48345);
	threadData1.iSvcSP = 67272;
	threadData1.iSvcStack = 888882;
	threadData1.iSvcStacksize = 4535;
	threadData1.iUsrSP = 892;
	threadData1.iUsrStack = 7727;
	threadData1.iUsrStacksize = 343;
	threadData1.iLastCpu = 12312;
	threadData1.iSvcHeap = 8738;
	threadData1.iSvcHeapSize = 4;
	
	threadData1.iNamesize = TThreadData::KMaxThreadName;
	
	threadData1.iName.Zero();
	for(TInt i=0;i<TThreadData::KMaxThreadName;i++)
		{	
		threadData1.iName.Append( TChar( (i%60) + 32 ));
		}


	DoWrite(threadData1);
	
	iReader.SetPosition(0);
	TThreadData threadData2;
	
	test(threadData2.Deserialize(iReader) == KErrNone);
	

	test(threadData1.iPriority == threadData2.iPriority);
	test(threadData1.iTid == threadData2.iTid);
	test(threadData1.iOwnerId == threadData2.iOwnerId);
	test(threadData1.iSvcSP == threadData2.iSvcSP);
	test(threadData1.iSvcStack == threadData2.iSvcStack);
	test(threadData1.iSvcStacksize == threadData2.iSvcStacksize);
	test(threadData1.iUsrSP == threadData2.iUsrSP);
	test(threadData1.iUsrStack == threadData2.iUsrStack);
	test(threadData1.iUsrStacksize == threadData2.iUsrStacksize);
	test(threadData1.iLastCpu == threadData2.iLastCpu);
	test(threadData1.iSvcHeap == threadData2.iSvcHeap);
	test(threadData1.iSvcHeapSize == threadData2.iSvcHeapSize);	
	test(threadData1.iNamesize == threadData2.iNamesize);
	test(threadData1.iName.Compare(threadData2.iName) == 0);	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2371 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TThreadStack structure 
//! @SYMTestActions TThreadStack serialized & then deserialized
//! @SYMTestExpectedResults TThreadStack structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestThreadStack()
	{
	test.Next(_L("TestThreadStack\n"));
	
	TThreadStack threadStack1;
	threadStack1.iStackType = TThreadStack::ESvrStack;
	threadStack1.iThreadId = MAKE_TINT64(774998,17345);
	
	DoWrite(threadStack1);

	iReader.SetPosition(0);
	TThreadStack threadStack2;
	test(threadStack2.Deserialize(iReader) == KErrNone);
	
	test(threadStack1.iStackType == threadStack2.iStackType);
	test(threadStack1.iThreadId == threadStack2.iThreadId);
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2372 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TRegisterValue structure 
//! @SYMTestActions TRegisterValue serialized & then deserialized
//! @SYMTestExpectedResults TRegisterValue structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestRegisterValue()
	{
	test.Next(_L("TestregisterValue\n"));
	TRegisterValue regValue1;
	
	regValue1.iOwnId = MAKE_TINT64(55498,58345);
	regValue1.iType = 3456;
	regValue1.iClass = 45;
	regValue1.iSubId = 5546;
	regValue1.iSize = 2;
	
	DoWrite(regValue1);
	
	iReader.SetPosition(0);
	TRegisterValue regValue2;
	test(regValue2.Deserialize(iReader) == KErrNone);
	
	test(regValue1.iOwnId == regValue2.iOwnId);
	test(regValue1.iType == regValue2.iType);
	test(regValue1.iClass == regValue2.iClass);
	test(regValue1.iSubId == regValue2.iSubId);
	test(regValue1.iSize == regValue2.iSize);	
	
	switch(regValue1.iSize)
		{
		case 0:
			test(regValue1.iValue8 == regValue2.iValue8);
			break;
		case 1:
			test(regValue1.iValue16 == regValue2.iValue16);
			break;
		case 2:
			test(regValue1.iValue32 == regValue2.iValue32);
			break;
		case 3:
			test(regValue1.iValue64 == regValue2.iValue64);
			break;
		default:
			test(EFalse);
			break;
		}	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2373 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TRegisterSet structure 
//! @SYMTestActions TRegisterSet serialized & then deserialized
//! @SYMTestExpectedResults TRegisterSet structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestRegisterSet()
	{
	test.Next(_L("TRegisterSet\n"));
	
	TRegisterSet set1;	
	set1.iNumRegisters = 3784;
	DoWrite(set1);
	
	iReader.SetPosition(0); 
	TRegisterSet set2;
	test(set2.Deserialize(iReader) == KErrNone);
		
	test(set1.iNumRegisters == set2.iNumRegisters);	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2374 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TMemoryDump structure 
//! @SYMTestActions TMemoryDump serialized & then deserialized
//! @SYMTestExpectedResults TMemoryDump structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestMemoryDump()
	{
	test.Next(_L("TestMemoryDump\n"));
	
	TMemoryDump memDump1;
	memDump1.iStartAddress = 0x23FACED0;
	memDump1.iPid = MAKE_TINT64(1234, 5678);
	memDump1.iLength = 999;
	
	DoWrite(memDump1);
	
	iReader.SetPosition(0); 
	TMemoryDump memDump2;
	test(memDump2.Deserialize(iReader) == KErrNone);

	test(memDump1.iStartAddress = memDump2.iStartAddress);
	test(memDump1.iPid = memDump2.iPid);
	test(memDump1.iLength = memDump2.iLength);
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2375 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TCodeSegmentSet structure 
//! @SYMTestActions TCodeSegmentSet serialized & then deserialized
//! @SYMTestExpectedResults TCodeSegmentSet structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestCodeSegmentSet()
	{
	test.Next(_L("TestCodeSegmentSet\n"));
	TCodeSegmentSet tss1;
		
	tss1.iNumSegs = 45;	
	tss1.iPid = MAKE_TINT64(28272,671717);
	
	DoWrite(tss1);
	
	iReader.SetPosition(0);
	TCodeSegmentSet tss2;
	test(tss2.Deserialize(iReader) == KErrNone);
	
	test(tss1.iNumSegs == tss2.iNumSegs);
	test(tss1.iPid == tss2.iPid);
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2376 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TCodeSegment structure 
//! @SYMTestActions TCodeSegment serialized & then deserialized
//! @SYMTestExpectedResults TCodeSegment structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestCodeSegment()
	{
	test.Next(_L("TestCodeSegment\n"));
	
	TCodeSegment cs1;
	cs1.iCodeSegType = EExeCodeSegType;	
	cs1.iCodeSegMemInfo.iCodeBase = 345;
	cs1.iCodeSegMemInfo.iCodeSize = 566;
	cs1.iCodeSegMemInfo.iConstDataBase = 776;
	cs1.iCodeSegMemInfo.iConstDataSize = 626267;
	cs1.iCodeSegMemInfo.iInitialisedDataBase = 873;
	cs1.iCodeSegMemInfo.iInitialisedDataSize = 52625;
	cs1.iCodeSegMemInfo.iUninitialisedDataBase = 3737;
	cs1.iCodeSegMemInfo.iUninitialisedDataSize = 53535;
	
	cs1.iName.Zero();
	for(TInt i=0;i<TCodeSegment::KMaxSegmentNameSize;i++)
		{
		cs1.iName.Append( TChar( (i%60) + 32 ));
		}

	cs1.iNameLength = TCodeSegment::KMaxSegmentNameSize;
	cs1.iXip = ETrue;
	
	DoWrite(cs1);
	
	iReader.SetPosition(0);	
	TCodeSegment cs2;
	test(cs2.Deserialize(iReader) == KErrNone);
		
	test(cs1.iCodeSegType == cs2.iCodeSegType);	
	test(cs1.iCodeSegMemInfo.iCodeBase == cs2.iCodeSegMemInfo.iCodeBase);
	test(cs1.iCodeSegMemInfo.iCodeSize == cs2.iCodeSegMemInfo.iCodeSize);
	test(cs1.iCodeSegMemInfo.iConstDataBase == cs2.iCodeSegMemInfo.iConstDataBase);
	test(cs1.iCodeSegMemInfo.iConstDataSize == cs2.iCodeSegMemInfo.iConstDataSize); 
	test(cs1.iCodeSegMemInfo.iInitialisedDataBase == cs2.iCodeSegMemInfo.iInitialisedDataBase);
	test(cs1.iCodeSegMemInfo.iInitialisedDataSize == cs2.iCodeSegMemInfo.iInitialisedDataSize);
	test(cs1.iCodeSegMemInfo.iUninitialisedDataBase == cs2.iCodeSegMemInfo.iUninitialisedDataBase);
	test(cs1.iCodeSegMemInfo.iUninitialisedDataSize == cs2.iCodeSegMemInfo.iUninitialisedDataSize);
		
	test(cs1.iNameLength == cs2.iNameLength);
	test(cs1.iName.Compare(cs2.iName) == 0);	
	test(cs1.iXip == cs2.iXip);
	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2377 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TTraceDump structure 
//! @SYMTestActions TTraceDump serialized & then deserialized
//! @SYMTestExpectedResults TTraceDump structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestTraceDump()
	{
	test.Next(_L("TestTraceDump\n"));
	
	TTraceDump td1;
	td1.iSizeOfMemory = 378282;
	td1.iNumberOfParts = 440;
	
	DoWrite(td1);
	
	iReader.SetPosition(0);

	TTraceDump td2;
	test(td2.Deserialize(iReader) == KErrNone);
	test(td1.iSizeOfMemory == td2.iSizeOfMemory);
	test(td1.iNumberOfParts == td2.iNumberOfParts);
	}
	

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2379 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TVariantSpecificData structure 
//! @SYMTestActions TVariantSpecificData serialized & then deserialized
//! @SYMTestExpectedResults TVariantSpecificData structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestVariantSpecificData()
	{
	test.Next(_L("TestVariantSpecificData\n"));		
	TVariantSpecificData vsd1;
	
	vsd1.iSize = 37372;
	DoWrite(vsd1);
	
	iReader.SetPosition(0);		
	TVariantSpecificData vsd2;
	test(vsd2.Deserialize(iReader) == KErrNone);	
	}

//---------------------------------------------
//! @SYMTestCaseID KBASE-T_SCMLIB-2380 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TRomHeaderData structure 
//! @SYMTestActions TRomHeaderData serialized & then deserialized
//! @SYMTestExpectedResults TRomHeaderData structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestRomHeaderData()
	{
	test.Next(_L("TestRomHeaderData\n"));
	
	TRomHeaderData rhd1;
	rhd1.iMajorVersion = 12;
	rhd1.iMinorVersion = 2;							
	rhd1.iBuildNumber = 7828;				
	rhd1.iTime = MAKE_TINT64(716171, 62672);
	
	DoWrite(rhd1);
	
	iReader.SetPosition(0);
	TRomHeaderData rhd2;
	test(rhd2.Deserialize(iReader) == KErrNone);
	test(rhd1.iMajorVersion == rhd2.iMajorVersion);
	test(rhd1.iMinorVersion == rhd2.iMinorVersion);							
	test(rhd1.iBuildNumber == rhd2.iBuildNumber);				
	test(rhd1.iTime == rhd2.iTime);		
	}

//---------------------------------------------
//! //! @SYMTestCaseID KBASE-T_SCMLIB-2381 
//! @SYMTestType
//! @SYMPREQ PREQ1700
//! @SYMTestCaseDesc Ensures we can serialise and deserialise the TSCMLockData structure 
//! @SYMTestActions TSCMLockData serialized & then deserialized
//! @SYMTestExpectedResults <> structure serialized / deserialized ok
//! @SYMTestPriority High
//! @SYMTestStatus Implemented
//---------------------------------------------
void CSCMLibraryClient::TestSCMLockData()
	{
	
	test.Next(_L("TestSCMLockData\n"));

	TSCMLockData ld1;	
	ld1.SetMutexHoldCount(3);
	ld1.SetLockCount(4);
	ld1.SetMutexThreadWaitCount(5);

	DoWrite(ld1);
	
	iReader.SetPosition(0);
	TSCMLockData ld2;
	test(ld2.Deserialize(iReader) == KErrNone);
	
	test(ld1.MutexHoldCount() == ld2.MutexHoldCount());
	test(ld1.MutexThreadWaitCount() == ld2.MutexThreadWaitCount());
	test(ld1.LockCount() == ld2.LockCount());
	test(ld1 == ld2);
	test(!(ld1 != ld2));
	}	
		
//eof

