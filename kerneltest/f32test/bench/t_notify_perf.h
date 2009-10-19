// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\bench\t_notify_perf.h
// 
//

#include <f32file.h>
#include <f32file_private.h>
#include <e32test.h>
#include <e32svr.h>
#include <hal.h>
#include <e32math.h>
#include <e32std.h>

// File operation made in test path to trigger notifications
enum TNotifyPerfTestOperations
	{
	// File Operations
	EOpEnd,			// indicates that a series of operations ended
	EOpCreate,
	EOpReplace,
	EOpChgAttr,
	EOpRename,
	EOpWrite,
	EOpResize,
	EOpDelete,
	EOpManyChanges,	// Large number changes to a single file 
	EOpManyFiles,	// Small changes to large number of files
	
	// Directory Operations
	EOpCreateDir,
	EOpRenameDir,
	EOpDeleteDir,
	
	// Mixed Operations
	EOpMixed		// A series of mixed operations involving both Files and Dirs
	};
	
enum TTestOptions
	{
	//////////////////////////////////////////////////////
	// Lowest 4 bit reserved for notification treads ID //
	//////////////////////////////////////////////////////
	
	// Test with a lot of filters for enhanced notification - 1st bit
	// set - on; unset - off
	EBigFilter	= 0x0010,
	
	// Whether perform full directory scan and save the file changes to a list - 2nd bit
	// set - perform; unset - not perform
	EReportChg	= 0x0020, 
	
	// Whether use big buffer for enhanced notification- 3rd bit
	// Set - big buffer(no overflow); unset - small(could have overflow)
	EBigBuffer	= 0x0040,
	
	// For multi clients test. Enhanced Notification Only!
	// Mode 1: set a variety of different notifications, same on each clients - 4th bit
	EMultiNoti1	= 0x0080,
	
	// For multi clients test. Enhanced Notification Only!
	// Mode 2: set a variety of different notifications, in which some are different on each clients, 
	// and some are same on each clients, only support upto 4 clients - 5th bit
	EMultiNoti2	= 0x0100,
	
	ENotPerfTestReserved6	= 0x0200, 
	ENotPerfTestReserved7	= 0x0400, 
	ENotPerfTestReserved8	= 0x0800, 
	
	// Notification type - 13th - 16th bits
	ENoNotify	= 0x1000,	// Test without notification
	EEnhanced	= 0x2000,	// Using enhanced notification APIs
	EOriginal	= 0x4000,	// Using original notification APIs
	EPlugin		= 0x8000,	// Using Nokia plug-in for notification
	};

// Note for Plugin Test
// the plugin test can only be run manually because the plugin is not available in KHS code base
// to run the test:
// 1. enable the MACRO above in the mmp
// 2. get a S60 environment
// 3. copy the MdsFileServerPlugin.ptx from the release(\Winscw or \Armv5 depend on what kind of test you want to run) 
//    directory of S60 to the equivalent folder of release directory of your epoc32
// 4. when build a rom, include the MdsFileServerPlugin.ptx file in the rom
// 5. then you can run
enum TMdsFSPOperation
    {
    EMdsFSPOpEnable,
    EMdsFSPOpDisable,
    EMdsFSPOpRegisterNotification,
    EMdsFSPOpAddNotificationPath,
    EMdsFSPOpRemoveNotificationPath,
    EMdsFSPOpAddIgnorePath,
    EMdsFSPOpRemoveIgnorePath,
    EMdsFSPOpNotificationCancel,
    };

class TMdsFSPStatus
    {
public:
    TInt iFileEventType;
    TInt iDriveNumber;
    TFileName iFileName;
    TFileName iNewFileName;
    TUid iProcessId;
    };

enum TMdsFileEventType
    {
    EMdsFileCreated,
    EMdsFileRenamed,
    EMdsFileModified,
    EMdsFileReplaced,
    EMdsFileDeleted,
    EMdsDriveFormatted,
    EMdsFileUnknown,
    EMdsDirRenamed
    };

typedef TPckgBuf<TMdsFSPStatus> TMdsFSPStatusPckg;

const TInt KMdsFSPluginPosition = 0x200071CD;
_LIT(KPluginName, "MdsFileServerPlugin");

// the list of operations to be conducted during a test case
const TUint KDefaultOpList[] = {EOpCreate, EOpReplace, EOpChgAttr, EOpRename, EOpWrite, EOpResize, EOpDelete, EOpEnd};
const TUint KDefaultOpListDir[] = {EOpCreateDir, EOpRenameDir, EOpDeleteDir, EOpEnd};

const TUint KManyChangesOpList[] = {EOpManyChanges, EOpManyChanges, EOpManyChanges, EOpManyChanges, EOpManyChanges, EOpEnd};
const TUint KManyFilesOpList[] = {EOpManyFiles, EOpManyFiles, EOpManyFiles, EOpManyFiles, EOpManyFiles, EOpEnd};
const TUint KMixedOpTestList[] = {EOpMixed, EOpMixed, EOpMixed, EOpMixed, EOpMixed, EOpEnd};

const TUint16 KNotifyOptionMask = 0xF000;
const TUint16 KNotifyTreadIdMask = 0x000F;

// default time scale for timer
const TUint KDefaultTimeScale = 1000; // 1ms
const TInt KMaxHeapSize = 0x1000000;

// used by SafeCheck
const TInt KNoThreadId = -1;

// a Controllor of whether measure time and write loggs;
extern TBool gPerfMeasure;

extern TFileName gTestPath;
extern TFileName gLogFilePath;

// Threads handles
extern RArray<RThread> gNotiThreads;
extern RThread gFileThread;

extern TBuf<50> gLogPostFix;

//-------------------------------------------------------------

class CTimerLogger;

// a wrapper for test settings
class TTestSetting
	{

public:
	TTestSetting();
	TTestSetting(TInt aNumFiles, TInt aNumCli, TUint16 aOpt, const TUint* aOpList);
	inline void Reset();
	
public:
	TInt iNumFiles;
	TInt iNumCli;
	TUint16 iOption;
	const TUint* iOperationList;
	
	};

// a wrapper of parameters for the main thread to pass into notification thread or file operation thread
struct TThreadParam
	{
	TTestSetting iSetting;
	RSemaphore* iSmphFT;	// Semophore used by File Thread for waiting for signals from Notification Threads
	RSemaphore* iSmphNT;	// Semophore used by Notification Threads for waiting for signals from File Thread
	CTimerLogger* iLogger;	// Logger used by Notification Threads;
	RPointerArray<CTimerLogger>* iLoggerArray;	// a pointer to an array of pointers to CTimmerLoggger
	};

// This is the controller of the plugin, it's a simplified copy of the plugin engine used in S60 internally
class CMdsPluginControl : public RPlugin
    {   
public:
    inline void RegisterNotification( TMdsFSPStatusPckg& aMdsFSPStatus, TRequestStatus& aStat);
    inline void AddNotificationPath( const TDesC& aPath );
    inline void RemoveNotificationPath( const TDesC& aPath );
    inline TInt Enable();
    inline TInt Disable();
    inline void NotificationCancel();
    };

// timer class, also responsible for writing logs
class CTimerLogger : public CBase
	{
	
public:	
	static CTimerLogger* NewL(const TFileName& aLogFile);
	~CTimerLogger();
	inline TInt MeasureStart();
	inline TInt MeasureEnd();
	inline TBool Timing();
	TInt Log(const TDesC& aDes, TBool aIsLine = ETrue);
	TInt LogAndPrint(const TDesC& aDes, TBool aIsLine = ETrue);
	TInt LogSettingDescription(const TInt aNumFile, const TInt aNumCli, const TUint16 aOption, TBool aNumOpVaries = EFalse);
	TInt LogTestStepTime(TUint aOp, TInt aNum);

private:
	CTimerLogger();
	void ConstructL(const TFileName& aLogFile);
	
private:
	TBool iTiming;
	TUint32 iTickNumber;
	TUint iTimeScale;
	TInt iTickPeriod;
	TFileName iLogFile;
	RFs iFs;
	};

// the conductor of test cases
class CTestExecutor : public CBase
	{

public:
	CTestExecutor(TTestSetting& aSetting);
	~CTestExecutor();
	
	inline void SetTestSetting(TTestSetting& aSetting);
	inline void LogDescription();
	
	void RunTestCaseL();
	static void KillAllTestThreads();
	
private:
	TTestSetting iTestSetting;
	
	};

// This class performs file operations
class CFileOperator : public CBase
	{
	
public:	
	CFileOperator(const TTestSetting& aSetting, RPointerArray<CTimerLogger>& aLoggerArray, RSemaphore* aSmphFT, RSemaphore* aSmphNT);
	~CFileOperator();
	void DoChangesL();
	
private:
	void DoCreateL();
	void DoReplaceL();
	void DoChangeAttL();
	void DoRenameL();
	void DoWriteL();
	void DoResizeL();
	void DoDeleteL();
	void DoCreateDirL();
	void DoRenameDirL();
	void DoDeleteDirL();
	void DoMixedOperationsL();
	
	void DoManyChangesOnSingleFileL();
	void DoSmallChangesOnManyFilesL();
	
	void MesureStartsAll();
	void WaitForSignalsAll();

	void TestStepPrepare(TUint aOp);
	void TestStepFinish(TUint aOp);
	
private:
	// test case will use the number iFirstFile and iNumFiles to generate test file names.
	// For example: 
	// if iFirstFile = 0 and iNumFiles = 100, the test files will be 0000.tst, 0001.tst ... 0099.tst
	// if iFirstFile = 21 and iNumFiles = 200, the test files will be 0021.tst, 0022.tst ... 0220.tst
	//
	// When doing rename or replace test, the new names for test will be the biggest existing file 
	// number + 1 to the number + iNumFiles.
	// As a result, in the case of if iFirstFile = 0 and iNumFiles = 100:
	// The exsting files should be 0000.tst ... 0099.tst
	// Rename or Replace test will use the new names 0100.tst ... 0199.tst to replace or rename the
	// existing files.
	TInt iFirstFile;
	
	// Number of files for operating.
	// Note: in "Large number changes to a single file" case, this indicates the number of changes made on the single file.
	TInt iNumFiles;
	TInt iNumCli;
	TUint16 iOption;
	const TUint* iCurrentOp;
	
	RPointerArray<CTimerLogger> iLoggers;
	RSemaphore* iSmphS;		// Use for signaling Notification threads
	RSemaphore* iSmphW;		// Use for waiting signal from Notification threads
	
	RFs iFs;
	};

// an operator to monitor notification watcher and test stopper
class CNotifyOperator : public CBase
	{
	
public:
	CNotifyOperator(const TTestSetting& aSetting, RSemaphore* aSmphFT, RSemaphore* aSmphNT, CTimerLogger* aLogger);
	~CNotifyOperator();
	
	void StartOperationL();
	
private:
	void LogNotificationNumbers(TInt aNumNoti, TInt aNumIter, TInt aNumOverflow);
	void LogTestResult(TInt aCounter, TInt aMeanCounter, TInt aOFCounter);
	void TestChangeReport(TInt aNumChanges);
	
private:
	TInt iNumFiles;
	TUint16 iOption;
	const TUint* iCurrentOp;
	
	CTimerLogger* iLogger;
	RSemaphore* iSmphS;	// Use for signaling file operation thread
	RSemaphore* iSmphW;	// Use for waiting signal from file operation thread
	};

// this class is responsible for handling notifications
class CNotifyWatcher : public CActive
	{
friend class CNotifyOperator;

public:
	static CNotifyWatcher* NewL(TInt aNumFiles, TUint16 aOption, TUint aCurrentOp, CTimerLogger* aLogger);
	~CNotifyWatcher();
	
	void DoCancel();
	void RunL();

	void RequestNotification();
	
	void Reset(TUint aOp);
	
private:
	CNotifyWatcher(TInt aNumFiles, TUint16 aOption, TUint aCurrentOp, CTimerLogger* aLogger);
	void ConstructL();

	void FullDirectoryScanL(RArray<TEntry>& aArray);
	void MakeChangeRecordL(RArray<TEntry>& aArray);
	TBool CompareEntry(const TEntry& aEntry1, const TEntry& aEntry2);
	
	void AddLotsOfFilters();
	
	void RequestNotificationEnhanced();
	void RequestNotificationOriginal();
	
	void HandleNotification(TBool aLastTime);
	
	void HandleNotificationEnhanced(TBool aLastTime);
	void HandleNotificationOriginal(TBool aLastTime);
	
	void ResetMdsFSPStatus();
	void RequestNotificationPlugin();
	void HandleNotificationPlugin(TBool aLastTime);
	
private:
	TInt iCounter;
	TInt iMeanCounter;
	TInt iOverflowCounter;
	
	TUint iCurrentOp;
	TInt iNumFiles;
	TUint16 iOption;

	RArray<TEntry> iEntries;
	RArray<TFileName> iRecords;	// this is the output we produce
	
	CTimerLogger* iLogger;
	
	CFsNotify* iNotify;
	  
	TMdsFSPStatusPckg iPluginStatusPkg;
    CMdsPluginControl iPlugin;	
	
	RFs iFs;
	};

// An AO aims for stopping the Active Scheduler when test finish
class CTestStopper : public CActive
	{
	
public:
	CTestStopper();
	~CTestStopper();
	
	void DoCancel();
	void RunL();
	
	void StartWaitingForFile();
	
private:
	TFileName iTestEndFile;
	RFs iFs;
	};

#include "t_notify_perf.inl"
