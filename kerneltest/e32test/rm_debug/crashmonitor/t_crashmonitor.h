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
// System Crash Monitor Shared Library tests
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef CRASH_MONITOR
#define CRASH_MONITOR

#include <e32base.h>
#include <e32cons.h>
#include <e32test.h>
#include <e32cmn.h>
#include <scmdatatypes.h>
#include <scmbytestreamutil.h>
#include <hal.h>

using namespace Debug;

LOCAL_D RTest test(_L("T_CRASHMONITOR_LIB"));

//number of test functions that we have
const TInt KMaxTests = 25;
const TUint KBufLen =  52224; // 0.5 meg buffer

class CSCMLibraryClient;

// Create a pointer to function type
typedef void (CSCMLibraryClient::*testFunction)();

class TFunctionData
	{
		
public:
	TFunctionData()
	: iFunctionPtr(NULL)
	{	
	}

	testFunction iFunctionPtr;
	TBuf<40> iFunctionName;
	};

//
// class CSCMLibraryClient
//
// The basic SCM Library Client
//
class CSCMLibraryClient : public CBase
	{
public:
	static CSCMLibraryClient* NewL();
	~CSCMLibraryClient();
	void ClientAppL();

private:
	CSCMLibraryClient();
	void ConstructL();

	TInt TestStartup();
	TInt TestShutdown();

	void FillArray();
	void PrintUsage();
	void PrintVersion();

	enum TTestMode 
		{
		//run all the tests
		EModeAll = 1<<0,
		//run the specified tests in reverse order
		EModeReverse = 1<<1,
		//print out help
		EModeHelp = 1<<2,
		//print out help
		EModeVersion = 1<<3
		};
	
	//Tests
	void TestCheckSum();

	void TestLockDataSerialization();
	void TestOffsetsHeaderSerialization();
	void TestInfoHeaderSerialization();
	void TestRawData();
	void TestProcessData();
	void TestThreadData();
	void TestThreadStack();
	void TestRegisterValue();
	void TestRegisterSet();
	
	void TestMemoryDump();
	void TestCodeSegmentSet();
	void TestCodeSegment();
	void TestTraceDump();
	void TestVariantSpecificData();
	void TestRomHeaderData();
	void TestSCMLockData();

	void RunTest(TInt aTestNumber);
	void ParseCommandLineL(TUint32& aMode, RArray<TInt>& aTests);
	void HelpStartTestTimer(void) { iStartTick = User::NTickCount(); iStopTick = 0; };
	void HelpStopTestTimer(void) { iStopTick = User::NTickCount(); };
	TInt HelpGetTestTicks(void) { return (iStopTick - iStartTick); };
	TInt HelpTicksPerSecond();
	void ReportPerformance();

private:
	TBool ChecksumHelper(TScmChecksum& aChecksum, TUint aBlocksize, TUint8* aBuffer, TUint aBufferLen);
	void DoWrite(MByteStreamSerializable& aObjectToWrite, TInt aPositon = 0);

	
private:	
	TFunctionData iTestArray[KMaxTests];
	
	// Timing information
	TInt iStartTick;
	TInt iStopTick;	
	
	TUint8		iBuffer[KBufLen]; 
	
	TByteStreamWriter iNonCachedWriter;	
	TCachedByteStreamWriter iCachedWriter;
	
	TByteStreamWriter* iWriter;
	TByteStreamReader iReader;
	};

#endif // CRASH_MONITOR
