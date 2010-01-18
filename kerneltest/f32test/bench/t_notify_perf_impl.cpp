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
// f32test\bench\t_notify_perf_impl.cpp
// 
//

#include "t_notify_perf.h"
#include "t_server.h"

extern void FileNameGen(TFileName& aName, TInt aNum, TBool aIsFile = ETrue);
extern TInt FileOperationThread(TAny* aSetting);
extern TInt NotificationOperationThread(TAny* aSetting);
extern TInt KillerThread(TAny*);
extern TUint OpNotifyMapping(TUint16& aOption, TInt aOperation);
extern TBool CompareEntryName(const TEntry& aEntry1, const TEntry& aEntry2);
extern void SafeTestL(TBool aResult, TInt aId, TInt aLine, TText* aFile);
extern void SafeTestL(TInt aResult, TInt aExpected, TInt aId, TInt aLine, TText* aFile);

const TInt KNotificationHeaderSize = (sizeof(TUint16)*2)+(sizeof(TUint));
const TInt KMinNotificationBufferSize = 2*KNotificationHeaderSize + 2*KMaxFileName;

TBool gPerfMeasure;
TFileName gTestPath;
TFileName gLogFilePath;
RArray<RThread> gNotiThreads;
RThread gFileThread;
TBuf<50> gLogPostFix;

// Prints out the filename
#define ExpandMe(X)  L ## X
#define Expand(X)    ExpandMe(X)

// Safe test, so that sub-threads are not hanging after check fail
#define SAFETEST0(a) SafeTestL(a,KNoThreadId,__LINE__,(TText*)Expand("t_notify_perf_impl.cpp"))
#define SAFETEST1(a,b) SafeTestL(a,b,__LINE__,(TText*)Expand("t_notify_perf_impl.cpp"))
#define SAFETEST2(a,b,c) SafeTestL(a,b,c,__LINE__,(TText*)Expand("t_notify_perf_impl.cpp"))


TTestSetting::TTestSetting()
: iNumFiles(0), iNumCli(0), iOption(0), iOperationList(NULL)
	{
	}

TTestSetting::TTestSetting(TInt aNumFiles, TInt aNumCli, TUint16 aOpt, const TUint* aOpList)
: iNumFiles(aNumFiles), iNumCli(aNumCli), iOption(aOpt), iOperationList(aOpList)
	{
	}

//===========================================================

CTimerLogger* CTimerLogger::NewL(const TFileName& aLogFile)
	{
	CTimerLogger* self = new(ELeave) CTimerLogger();
	CleanupStack::PushL(self);
	self->ConstructL(aLogFile);
	CleanupStack::Pop(self);
	return self;
	}

void CTimerLogger::ConstructL(const TFileName& aLogFile)
	{
	User::LeaveIfError(HAL::Get(HALData::ENanoTickPeriod, iTickPeriod));
	iLogFile.Copy(aLogFile);
	}

CTimerLogger::CTimerLogger()
:  iTiming(EFalse), iTickNumber(0), iTimeScale(KDefaultTimeScale)
	{
	iFs.Connect();
	}

CTimerLogger::~CTimerLogger()
	{
	iFs.Close();
	}

// Write Logs
TInt CTimerLogger::Log(const TDesC& aDes, TBool aIsLine)
	{
	RFile file;
	iFs.Connect();
	
	TInt err = file.Open(iFs,iLogFile,EFileShareExclusive|EFileWrite);
	SAFETEST0(err == KErrNone || err == KErrNotFound || err == KErrPathNotFound);
	
	if (err != KErrNone)
		{
		err = iFs.MkDirAll(iLogFile);
		SAFETEST0(err == KErrNone || err == KErrAlreadyExists);
		err = file.Create(iFs,iLogFile,EFileShareExclusive|EFileWrite);
		SAFETEST0(err == KErrNone);
		}
	
	TBuf8<240> data;
	data.Copy(aDes);
	if (aIsLine)
		{
		data.Append(_L8("\r\n"));
		}
		
	TInt offset = 0;
	err = file.Seek(ESeekEnd, offset);
	SAFETEST0(err == KErrNone);
	err = file.Write(data);
	SAFETEST0(err == KErrNone);

	file.Close();
	iFs.Close();
	return err;
	}

// Write Logs and also print the line written in the console
TInt CTimerLogger::LogAndPrint(const TDesC& aDes, TBool aIsLine)
	{
	TInt err = KErrNone;
	RDebug::Print(aDes);
	if (gPerfMeasure)
	    err = Log(aDes, aIsLine);
	return err;
	}

// Write and print the time result
TInt CTimerLogger::LogTestStepTime(TUint aOp, TInt aNum)
	{
	if (iTiming)
		return KErrGeneral;
	
	TBuf<100> buf;
	switch (aOp)
		{
		case EOpCreate:
			buf.Append(_L("Create - "));
			break;
		case EOpReplace:
			buf.Append(_L("Replace - "));
			break;
		case EOpChgAttr:
			buf.Append(_L("Change Attribute - "));
			break;
		case EOpRename:
			buf.Append(_L("Rename - "));
			break;
		case EOpWrite:
			buf.Append(_L("Write - "));
			break;
		case EOpResize:
			buf.Append(_L("Resize - "));
			break;
		case EOpDelete:
			buf.Append(_L("Delete - "));
			break;
		case EOpManyChanges:
			buf.AppendFormat(_L("%d Changes on Single File - "), aNum);
			break;
		case EOpManyFiles:
			buf.AppendFormat(_L("Small Changes on %d Files - "), aNum);
			break;
		case EOpCreateDir:
			buf.Append(_L("Create(dir) - "));
			break;
		case EOpRenameDir:
			buf.Append(_L("Rename(dir) - "));
			break;
		case EOpDeleteDir:
			buf.Append(_L("Delete(dir) - "));
			break;
		case EOpMixed:
			buf.AppendFormat(_L("%d Mixed Operations - "), aNum*18);
		default:
			break;
		}
	
	TReal time = (static_cast<TReal>(iTickNumber) * iTickPeriod) / iTimeScale;
	buf.AppendFormat(_L("time: %d ms"), static_cast<TInt>(time));
	return LogAndPrint(buf);
	}

// write and print the test case discription
TInt CTimerLogger::LogSettingDescription(const TInt aNumFile, const TInt aNumCli, const TUint16 aOption, TBool aNumOpVaries)
	{	
	LogAndPrint(_L("===================================================="));
	
	TBuf<120> buf;
	buf.Append(_L("Test: "));
	if (!aNumOpVaries)
		{
		buf.AppendFormat(_L("%d files/directories, %d Clients, "), aNumFile, aNumCli);
		}
	
	// Notification Type
	switch(aOption & KNotifyOptionMask)
		{		
		case ENoNotify:
			buf.Append(_L("No Notification"));
			break;
		case EEnhanced:
			buf.Append(_L("Enhanced"));
			break;
		case EOriginal:
			buf.Append(_L("Original"));
			break;
		case EPlugin:
			buf.Append(_L("Nokia Plug-in"));
			break;
		default:
			return KErrArgument;
		}
	
	if (aOption & EBigFilter)
		{
		buf.Append(_L(", 100 Filters"));
		}
	
	if (aOption & EReportChg)
		{
		buf.Append(_L(", Change Reporting"));
		}
	else
		{
		buf.Append(_L(", Not Report Changes"));
		}
	
	if (aOption & EEnhanced)
		{
		if (aOption & EBigBuffer)
			{
			buf.Append(_L(", Big Buffer"));
			} 
		else
			{
			buf.Append(_L(", Small Buffer"));
			}
		
		if (aOption & EMultiNoti1)
			{
			buf.Append(_L(", Multi Notification Mode 1"));
			}
		else if (aOption & EMultiNoti2)
			{
			buf.Append(_L(", Multi Notification Mode 2"));
			}
		}

	LogAndPrint(buf);
	LogAndPrint(_L("----------------------------------------------------"));
	
	return KErrNone;
	}

//===========================================================

CTestExecutor::CTestExecutor(TTestSetting& aSetting)
: iTestSetting(aSetting)
	{
	}

CTestExecutor::~CTestExecutor()
	{
	}

void CTestExecutor::KillAllTestThreads()
    {
    RThread killer;
    killer.Create(_L("KillerThread"), KillerThread, KDefaultStackSize, KMinHeapSize, KMaxHeapSize, NULL); 
    killer.Resume();
   
    TRequestStatus status;
    killer.Logon(status);
    User::WaitForRequest(status);
    TInt err = killer.ExitReason();
    test(err == KErrNone);
    }

// start run a test case
void CTestExecutor::RunTestCaseL()
	{
	RSemaphore smphF;
	smphF.CreateLocal(0);
	RSemaphore smphN;
	smphN.CreateLocal(0);
		
	RArray<RThread> notiThreads; // list of handles of notification threads
	RPointerArray<CTimerLogger> loggerList;
	
	TUint16 count = 0;
	TUint16 option = iTestSetting.iOption;
	while (count < iTestSetting.iNumCli)
		{
		test(count < 16);
		iTestSetting.iOption = (TUint16)(option + count); // Put Thread ID in option
		
		TThreadParam param;
		param.iSetting = iTestSetting;
		param.iSmphFT = &smphF;
		param.iSmphNT = &smphN;
			
		TFileName logName;
		logName.FillZ();
		
		if (gPerfMeasure)
		    {
            logName.Append(gLogFilePath);
            if (iTestSetting.iNumCli == 1)
                logName.Append(_L("SingleClient"));
            else
                logName.AppendFormat(_L("MultiClient%02d"), count);
            
            logName.Append(gLogPostFix);
		    }
		
		CTimerLogger* logger = CTimerLogger::NewL(logName);
		CleanupStack::PushL(logger);
		
		param.iLogger = logger;
		param.iLoggerArray = NULL;
		
		TUint operation = *iTestSetting.iOperationList;
		TBool numFilesVaries = EFalse; 
		
		if (operation == EOpManyFiles || operation == EOpManyChanges || operation == EOpMixed)
			{
			numFilesVaries = ETrue;
			}
		logger->LogSettingDescription(iTestSetting.iNumFiles, iTestSetting.iNumCli, iTestSetting.iOption, numFilesVaries);
		
		loggerList.AppendL(logger);
		
		TBuf<20> threadName;
		threadName.AppendFormat(_L("NotificationThread%02d"), count);

		RThread notifyOp;
		notifyOp.Create(threadName, NotificationOperationThread, KDefaultStackSize, KMinHeapSize, KMaxHeapSize, &param);
		
		notiThreads.AppendL(notifyOp);
		
		notifyOp.Resume();
		
		smphF.Wait();	// Wait for the parameters being properly passed
		
		CleanupStack::Pop(logger);
		count++;
		}
	
	gNotiThreads = notiThreads;
	
	if (iTestSetting.iNumCli == 0)	// no notification
		{
		TFileName logName;
		logName.Append(gLogFilePath);
		logName.Append(_L("SingleClient"));
		logName.Append(gLogPostFix);
	
		CTimerLogger* logger = CTimerLogger::NewL(logName);
		CleanupStack::PushL(logger);
		
		logger->LogSettingDescription(iTestSetting.iNumFiles, iTestSetting.iNumCli, iTestSetting.iOption);
		
		loggerList.AppendL(logger);
		CleanupStack::Pop(logger);
		}
	
	TThreadParam paramFileOp;
	paramFileOp.iSetting = iTestSetting;
	paramFileOp.iSmphFT = &smphF;
	paramFileOp.iSmphNT = &smphN;
	paramFileOp.iLogger = NULL;
	paramFileOp.iLoggerArray = &loggerList;
	
	RThread fileOp;
	fileOp.Create(_L("FileOperationThread"), FileOperationThread, KDefaultStackSize, KMinHeapSize, KMaxHeapSize, &paramFileOp);	
	gFileThread = fileOp;
	
	fileOp.Resume();
	
	TInt err;
	
	TRequestStatus status;
	fileOp.Logon(status);
	User::WaitForRequest(status);
	err = fileOp.ExitReason();
	test(err == KErrNone);
		
	count = 0;
	while(count < notiThreads.Count())
		{
		notiThreads[count].Logon(status);
		User::WaitForRequest(status);
		err = notiThreads[count].ExitReason();
		test(err == KErrNone);
		count++;
		}
	
	CLOSE_AND_WAIT(fileOp);
	
	count = 0;
	while(count < notiThreads.Count())
		{
		RThread thread = notiThreads[count];
		CLOSE_AND_WAIT(thread);
		count++;
		}
	
	for (TInt i = 0; i < loggerList.Count(); i++)
		{
		loggerList[i]->LogAndPrint(_L("===================================================="));
		}
	
	smphN.Close();
	smphF.Close();
	loggerList.ResetAndDestroy();
	loggerList.Close();
	notiThreads.Reset();
	notiThreads.Close();
	}

//===========================================================

CFileOperator::CFileOperator(const TTestSetting& aSetting, RPointerArray<CTimerLogger>& aLoggerArray, RSemaphore* aSmphFT, RSemaphore* aSmphNT)
: iFirstFile(0)
	{
	iNumFiles = aSetting.iNumFiles;
	iOption = aSetting.iOption;
	iCurrentOp = aSetting.iOperationList;
	iNumCli = aSetting.iNumCli;
	
	iLoggers = aLoggerArray; 
	iSmphS = aSmphNT;
	iSmphW = aSmphFT;

	iFs.Connect();
	}

CFileOperator::~CFileOperator()
	{
	iFs.Close();
	}

// starts measuring for all the notification thread
// the loggers in iLogger are shared by Notification thread
void CFileOperator::MesureStartsAll()
	{
	TInt i = 0;
	while (i < iNumCli)
		{
		iLoggers[i]->MeasureStart();
		i++;
		}
	}

// Wait for all notification threads to signal
void CFileOperator::WaitForSignalsAll()
	{
	TInt i = 0;
	while (i < iNumCli)
		{
		iSmphW->Wait();
		i++;
		}
	}

// prepare each test step
void CFileOperator::TestStepPrepare(TUint aOp)
	{
	RDebug::Print(_L("Preparing for the next test step..."));
	switch (aOp)
		{
		case EOpReplace:
			{
			User::After(1500000); // Wait for 1.5 sec so that the new files have different modified time
			RFile file;
			TInt count = iFirstFile + iNumFiles;
			while (count < (iFirstFile + (iNumFiles * 2)))
				{
				TFileName nextFile;
				FileNameGen(nextFile, count);
				file.Create(iFs, nextFile, EFileRead);
				file.Close();
				count++;
				}
			break;
			}
		case EOpManyChanges:
			{
			RFile file;
			TFileName nextFile;
			FileNameGen(nextFile, 9999);
			file.Create(iFs, nextFile, EFileRead);
			file.Close();
			break;
			}
		case EOpManyFiles:
			{
			CDir* list;
			iFs.GetDir(gTestPath, KEntryAttMaskSupported, ESortNone, list);
			TInt count = list->Count();
			delete list;
			
			RFile file;
			
			for (TInt i = 0; i < iNumFiles - count; i++)
				{
				TFileName nextFile;
				FileNameGen(nextFile, count + i);
				file.Create(iFs, nextFile, EFileRead);
				file.Close();
				}
			
			break;
			}
		// No preparation for other operations
		default:
			break;
		}
	RDebug::Print(_L("Preparation done..."));
	}

// Finish each test step, do some clearing or test setting modification
void CFileOperator::TestStepFinish(TUint aOp)
	{
	RDebug::Print(_L("Finishing the test step..."));
	switch (aOp)
		{
		case EOpRenameDir:
		case EOpRename:
			{
			iFirstFile += iNumFiles;
			break;
			}
		case EOpDelete:
		case EOpDeleteDir:
			{
			iFirstFile = 0;
			break;
			}
		case EOpManyChanges:
			{
			TFileName fileName;
			FileNameGen(fileName, 9999);
			iFs.Delete(fileName);
			iNumFiles += 1000;
			break;
			}
		case EOpManyFiles:
			{
			iNumFiles += 1000;
			break;
			}
	    case EOpMixed:
	        {
	        iNumFiles += 50;
	        }
	    // no clearing for other operations
		default:
			break;
		}
	RDebug::Print(_L("***** Test step finished *****"));
	RDebug::Print(_L("\n"));
	}

// perform the file operations
void CFileOperator::DoChangesL()
	{
	while(*iCurrentOp != EOpEnd)
		{
		TestStepPrepare(*iCurrentOp);
		
		if (iOption & ENoNotify)
			{
			iLoggers[0]->MeasureStart();
			}
		else
			{
			iSmphS->Signal(iNumCli);	// Signal notification threads that preparation is done
			WaitForSignalsAll();	// Wait for notification threads to finish requesting notifications 
			MesureStartsAll();
			}
		
		RDebug::Print(_L("Start Timing and File operations..."));	
		switch(*iCurrentOp)
			{
			case EOpCreate:
				DoCreateL(); 
				break;
			case EOpReplace:
				DoReplaceL();
				break;
			case EOpChgAttr:
				DoChangeAttL();
				break;
			case EOpRename:
				DoRenameL();
				break;
			case EOpWrite:
				DoWriteL(); 
				break;
			case EOpResize:
				DoResizeL(); 
				break;
			case EOpDelete:
				DoDeleteL();
				break;
			case EOpManyChanges:
				DoManyChangesOnSingleFileL();
				break;
			case EOpManyFiles:
				DoSmallChangesOnManyFilesL();
				break;
			case EOpCreateDir:
				DoCreateDirL();
				break;
			case EOpRenameDir:
				DoRenameDirL();
				break;
			case EOpDeleteDir:
				DoDeleteDirL();
				break;
			case EOpMixed:
				DoMixedOperationsL();
				break;
			default: 
				User::Leave(KErrArgument);
			}
		RDebug::Print(_L("File Operation Ended..."));
		
		if (iOption & ENoNotify)
			{
			RDebug::Print(_L("Timing ended..."));
			iLoggers[0]->MeasureEnd();
			iLoggers[0]->LogTestStepTime(*iCurrentOp, iNumFiles);
			}
		else 
			{
			RFile file;
			TFileName endFile(gLogFilePath);
			endFile.Append(_L("test.end"));
			TInt r = file.Replace(iFs, endFile, EFileWrite);
			SAFETEST0(r == KErrNone);
			file.Close();
	
			WaitForSignalsAll();	// Wait for notification threads to receive all the notifications 
			}
		
		TestStepFinish(*iCurrentOp);
			
		iCurrentOp++;
		}
	}

void CFileOperator::DoCreateL()
	{
	RFile file;
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count);

		TInt r = file.Create(iFs, nextFile, EFileRead);
		SAFETEST0(r == KErrNone);
		file.Close();	
		count++;
		}
	}

void CFileOperator::DoReplaceL()
	{
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName newFile;
		TFileName oldFile;
		FileNameGen(newFile, count);
		FileNameGen(oldFile, count + iNumFiles);
		
		iFs.Replace(oldFile, newFile);
		count++;
		}
	}

void CFileOperator::DoChangeAttL()
	{
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count);
		iFs.SetAtt(nextFile, KEntryAttNormal, KEntryAttArchive);
		count++;
		}
	}

void CFileOperator::DoRenameL()
	{
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName newFile;
		TFileName oldFile;
		FileNameGen(oldFile, count);
		FileNameGen(newFile, count + iNumFiles);
		
		iFs.Rename(oldFile, newFile);
		count++;
		}
	}

void CFileOperator::DoWriteL()
	{
	RFile file;
	TInt count = iFirstFile;
	_LIT8(KData, "Some text for writing test of enhanced notification performance test.");
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count);
		file.Open(iFs, nextFile, EFileWrite);
		file.Write(KData);
		// Flush file to ensure notification is received
		file.Flush();
		file.Close();	
		count++;
		}
	}

void CFileOperator::DoResizeL()
	{
	RFile file;
	TInt count = iFirstFile;

	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count);
		file.Open(iFs, nextFile, EFileWrite);
		TInt size;
		file.Size(size);
		file.SetSize(size+10);
		file.Close();	
		count++;
		}
	}

void CFileOperator::DoDeleteL()
	{
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count);
		iFs.Delete(nextFile);
		count++;
		}
	}

void CFileOperator::DoCreateDirL()
	{
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count, EFalse);
		iFs.MkDir(nextFile);
		count++;
		}
	}

void CFileOperator::DoRenameDirL()
	{
	TInt count = iFirstFile;
	while (count < iFirstFile + iNumFiles)
		{
		TFileName newFile;
		TFileName oldFile;
		FileNameGen(oldFile, count, EFalse);
		FileNameGen(newFile, count + iNumFiles, EFalse);
		
		iFs.Rename(oldFile, newFile);
		count++;
		}
	}

void CFileOperator::DoDeleteDirL()
	{
	TInt count = iFirstFile;
	
	while (count < iFirstFile + iNumFiles)
		{
		TFileName nextFile;
		FileNameGen(nextFile, count, EFalse);
		iFs.RmDir(nextFile);
		count++;
		}
	}

void CFileOperator::DoManyChangesOnSingleFileL()
	{
	TFileName fileName;
	FileNameGen(fileName, 9999);
	
	RFile file;
	_LIT8(KData, "a");
	
	for(TInt i = 0; i < iNumFiles; i++)
		{
		TInt offset = 0;
		file.Open(iFs, fileName, EFileWrite);
		file.Seek(ESeekEnd, offset);
		file.Write(KData);
		file.Flush();
		file.Close();
		}
	}

void CFileOperator::DoSmallChangesOnManyFilesL()
	{
	RFile file;
    _LIT8(KData, "a");
	
	for(TInt i = 0; i < iNumFiles; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i);
		file.Open(iFs, nextFile, EFileWrite);
		file.Write(KData);
		file.Flush();
		file.Close();	
		}
	}

void CFileOperator::DoMixedOperationsL()
	{
	// will perform 18*iNumFiles operations
	RFile file;
	TInt firstFile = iFirstFile;
	TInt lastFile = iFirstFile + (2 * iNumFiles);
	TInt firstDir = iFirstFile;
	TInt lastDir = iFirstFile + iNumFiles;
	
	_LIT8(KData, "Some text.");

	TInt i;
	// Create Files - 2*iNumFiles Ops
	// we create 2*iNumFiles files here so that we can ensure that at least iNumfiles ops are performed after the replace step
	for (i = firstFile; i < lastFile; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i);
		file.Create(iFs, nextFile, EFileRead);
		file.Close();	
		}

	// Create Directories - iNumFiles Ops
	for (i = firstDir; i < lastDir; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i, EFalse);
		iFs.MkDir(nextFile);
		}
	
	// Write - 2*iNumFiles Ops
	for (i = firstFile; i < lastFile; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i);
		file.Open(iFs, nextFile, EFileWrite);
		file.Write(KData);
		file.Flush();
		file.Close();
		}

	// Resize - 2*iNumFiles Ops
	for (i = firstFile; i < lastFile; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i);
		file.Open(iFs, nextFile, EFileWrite);
		TInt size;
		file.Size(size);
		file.SetSize(size+10);
		file.Close();
		}
	
	// Replace Files - iNumFiles Ops
	for (i = firstFile; i < firstFile + iNumFiles; i++)
		{
		TFileName newFile;
		TFileName oldFile;
		FileNameGen(oldFile, i);
		FileNameGen(newFile, i + iNumFiles);
		iFs.Replace(oldFile, newFile);
		}
	firstFile += iNumFiles;
	
	// Rename Files - iNumFiles Ops
	for (i = firstFile; i < lastFile; i++)
		{
		TFileName newFile;
		TFileName oldFile;
		FileNameGen(newFile, i - iNumFiles);
		FileNameGen(oldFile, i);	
		iFs.Rename(oldFile, newFile);
		}
	firstFile -= iNumFiles;
	lastFile -= iNumFiles;
	
	// Delete Dirs - iNumFiles Ops
	for (i = firstDir; i < lastDir; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i, EFalse);
		iFs.RmDir(nextFile);
		}
	
	// Delete File - iNumFiles Ops
	for (i = firstFile; i < lastFile; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i);
		iFs.Delete(nextFile);
		}

	// All-in-one - 7*iNumFiles Ops
	for (i = firstFile; i < lastFile; i++)
		{
		TFileName nextFile;
		FileNameGen(nextFile, i);
		TFileName nextDir;
		FileNameGen(nextDir, i, EFalse);
		
		iFs.MkDir(nextDir);
		
		file.Create(iFs, nextFile, EFileWrite);
		file.Write(KData);
		file.Flush();
		TInt size;
		file.Size(size);
		file.SetSize(size+10);
		file.Close();
		
		TFileName newName;
		FileNameGen(newName, i + iNumFiles);
		iFs.Rename(nextFile, newName);
		
		iFs.Delete(newName);
		iFs.RmDir(nextDir);
		}
	}

//==========================================================

CNotifyOperator::CNotifyOperator(const TTestSetting& aSetting, RSemaphore* aSmphFT, RSemaphore* aSmphNT, CTimerLogger* aLogger)
	{
	iNumFiles = aSetting.iNumFiles;
	iOption = aSetting.iOption;
	iCurrentOp = aSetting.iOperationList;
	iLogger = aLogger;
	iSmphS = aSmphFT;
	iSmphW = aSmphNT;
	}

CNotifyOperator::~CNotifyOperator()
	{
	}

// start operations in notification thread
// this is the main function called in the thread,
// it creates notification watcher, requests notification, and check test result
void CNotifyOperator::StartOperationL()
	{
	CNotifyWatcher* notifyWatcher = CNotifyWatcher::NewL(iNumFiles, iOption, *iCurrentOp, iLogger);
	CleanupStack::PushL(notifyWatcher);
	CTestStopper* stopper = new(ELeave) CTestStopper();
	CleanupStack::PushL(stopper);
	
	CActiveScheduler::Add(notifyWatcher);
	CActiveScheduler::Add(stopper);
	
	while (*iCurrentOp != EOpEnd)
		{
		iSmphW->Wait();	// wait for file thread finishing preparation for the current file operation.
		
		notifyWatcher->FullDirectoryScanL(notifyWatcher->iEntries);
		notifyWatcher->RequestNotification();
		stopper->StartWaitingForFile();
		
		iSmphS->Signal();	// Signal file thread that the notifications are requested, ready to receive
		
		CActiveScheduler::Start();
		////////////////////////////////
		// receiving notifications... //
		////////////////////////////////
		
		notifyWatcher->HandleNotification(ETrue); // handle for the last time
		
		LogTestResult(notifyWatcher->iCounter, notifyWatcher->iMeanCounter, notifyWatcher->iOverflowCounter);
		
		if(iOption & EReportChg)
		    {
			TestChangeReport(notifyWatcher->iRecords.Count());
		    }
		
		if (!gPerfMeasure)
		    {
		    TInt id = iOption & KNotifyTreadIdMask;
            if (iOption & EMultiNoti2)
                {
                if ( *iCurrentOp == EOpCreate ||
                    (id % 4 == 0 && (*iCurrentOp == EOpRename || *iCurrentOp == EOpReplace)) ||
                    (id % 4 == 1 && *iCurrentOp == EOpChgAttr) || 
                    (id % 4 == 2 && (*iCurrentOp == EOpResize || *iCurrentOp == EOpWrite)) ||
                    (id % 4 == 3 && *iCurrentOp == EOpDelete))
                    {
                    SAFETEST2(notifyWatcher->iCounter, iNumFiles, id);
                    }
                }
            else
                {
                SAFETEST2(notifyWatcher->iCounter, iNumFiles, id);
                }
		    }
		
		iSmphS->Signal();	// Signal file thread that all notifications are received
		
		iCurrentOp++;
		
		notifyWatcher->Reset(*iCurrentOp);
		if ((*iCurrentOp == EOpManyFiles) || (*iCurrentOp == EOpManyChanges))
			{
			iNumFiles += 1000;
			}
		else if (*iCurrentOp == EOpMixed)
		    {
		    iNumFiles += 50;
		    }
		}

	CleanupStack::PopAndDestroy(2);
	
	}

// Write log and print the notification numbers using iLogger
void CNotifyOperator::LogNotificationNumbers(TInt aNumNoti, TInt aNumIter, TInt aNumOverflow)
	{
	TBuf<100> buf;
	
	if (iOption & EEnhanced)
		{	
		if (aNumOverflow > 0)
			{
			buf.AppendFormat(_L("Buffer overflow: %d overflow notifications received."), aNumOverflow, aNumNoti);
			iLogger->LogAndPrint(buf);
			buf.Zero();
			}
		
		TReal mean = static_cast<TReal>(aNumNoti)/aNumIter;
		buf.AppendFormat(_L("%.2f mean enhanced notifications per iteration."), mean);	
		iLogger->LogAndPrint(buf);
		buf.Zero();
		}
	
	buf.AppendFormat(_L("%d notifications received overall."), aNumNoti);
	iLogger->LogAndPrint(buf);
	}

// Write log and print the test reault
void CNotifyOperator::LogTestResult(TInt aCounter, TInt aMeanCounter, TInt aOFCounter)
    {
    if (iOption & EMultiNoti2)
        {
        TInt id = iOption & KNotifyTreadIdMask;
        if ( *iCurrentOp == EOpCreate || 
            (id % 4 == 0 && (*iCurrentOp == EOpRename || *iCurrentOp == EOpReplace)) || 
            (id % 4 == 1 && *iCurrentOp == EOpChgAttr) || 
            (id % 4 == 2 && (*iCurrentOp == EOpWrite || *iCurrentOp == EOpResize)) ||
            (id % 4 == 3 && *iCurrentOp == EOpDelete))
            {
            iLogger->LogTestStepTime(*iCurrentOp, iNumFiles);
            LogNotificationNumbers(aCounter, aMeanCounter, aOFCounter);
            }
        else 
            iLogger->LogAndPrint(_L("File operation not tested - time: 0 ms"));
        }
    else if (iOption & EPlugin)
        {
        if (*iCurrentOp == EOpChgAttr || *iCurrentOp == EOpWrite || *iCurrentOp == EOpResize)
            iLogger->LogAndPrint(_L("File operation not tested - time: 0 ms"));
        else
            {
            iLogger->LogTestStepTime(*iCurrentOp, iNumFiles);
            LogNotificationNumbers(aCounter, aMeanCounter, aOFCounter);
            } 
        }
    else
        {
        iLogger->LogTestStepTime(*iCurrentOp, iNumFiles);
        LogNotificationNumbers(aCounter, aMeanCounter, aOFCounter);
        }
    }

// When change report enabled, check the number of changes we detected
void CNotifyOperator::TestChangeReport(TInt aNumChanges)
    {
    TBuf<100> buf;
    buf.AppendFormat(_L("%d file changes detected.\r\n"), aNumChanges);
    iLogger->LogAndPrint(buf);
    
    TInt id = iOption & KNotifyTreadIdMask;
    if (iOption & EEnhanced)
        {
        if (iOption & EMultiNoti2)
            {
            if ( *iCurrentOp == EOpCreate ||
                (id % 4 == 0 && (*iCurrentOp == EOpRename || *iCurrentOp == EOpReplace)) ||
                (id % 4 == 1 && *iCurrentOp == EOpChgAttr) || 
                (id % 4 == 2 && (*iCurrentOp == EOpResize || *iCurrentOp == EOpWrite)) ||
                (id % 4 == 3 && *iCurrentOp == EOpDelete))
                {
                SAFETEST2(aNumChanges, iNumFiles, id);
                }
            }
        else if (!(iOption & EBigBuffer))
            {
            // not testing this case, because the number of file changes detected 
            // could varies depend on when notifications are received
            }
        else if (*iCurrentOp == EOpMixed)
            {
            SAFETEST2(aNumChanges, (18*iNumFiles), id); // On cached drives, the number of write notification could vary.
            }
        else
            {
            SAFETEST2(aNumChanges, iNumFiles, id);
            }
        }
    else if (iOption & EOriginal)
        {
        if ((*iCurrentOp == EOpManyChanges) || (*iCurrentOp == EOpMixed))
            {
            // not testing this case, because the number of file changes detected 
            // could varies depend on when notifications are received
            }
        else if ((*iCurrentOp == EOpReplace) || (*iCurrentOp == EOpRename) || (*iCurrentOp == EOpRenameDir))
            {
            SAFETEST2(aNumChanges, (2*iNumFiles), id);
            }
        else
            {
            SAFETEST2(aNumChanges, iNumFiles, id);
            }
        }
    else if (iOption & EPlugin)
        {
        if (*iCurrentOp == EOpCreate || *iCurrentOp == EOpReplace || *iCurrentOp == EOpRename || 
                *iCurrentOp == EOpDelete || *iCurrentOp == EOpRenameDir)
            {
            SAFETEST2(aNumChanges, iNumFiles, id);
            }
        else if (*iCurrentOp == EOpMixed)
            {
            SAFETEST2(aNumChanges, (8*iNumFiles), id); // only part of operations can be notified
            }
        // Other cases are not testable.
        }
    }

//===========================================================
	
CNotifyWatcher::CNotifyWatcher(TInt aNumFiles, TUint16 aOption, TUint aCurrentOp, CTimerLogger* aLogger) 
: CActive(CActive::EPriorityStandard), iCounter(0), iMeanCounter(0), iOverflowCounter(0), iCurrentOp(aCurrentOp),
iNumFiles(aNumFiles), iOption(aOption), iEntries(aNumFiles), iRecords(aNumFiles), iLogger(aLogger)
	{
	}

CNotifyWatcher::~CNotifyWatcher()
	{
	Cancel();
	delete iNotify;

    iFs.DismountPlugin(KPluginName);
    iFs.RemovePlugin(KPluginName);
	
	iFs.Close();
	iEntries.Close();
	iRecords.Close();
	}

CNotifyWatcher* CNotifyWatcher::NewL(TInt aNumFiles, TUint16 aOption, TUint aCurrentOp, CTimerLogger* aLogger)
	{
	CNotifyWatcher* self = new(ELeave) CNotifyWatcher(aNumFiles, aOption, aCurrentOp, aLogger);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CNotifyWatcher::ConstructL()
	{
	iFs.Connect();
	
	if (iOption & EEnhanced)
		{
		TInt bufferSize;
		if(iOption & EBigBuffer)
			{
			bufferSize = KMinNotificationBufferSize * iNumFiles;
			if (iCurrentOp == EOpMixed)
				bufferSize = bufferSize * 18;
			}
		else
			bufferSize = KMinNotificationBufferSize;
		
		iNotify = CFsNotify::NewL(iFs, bufferSize);
		}
	else
		iNotify = NULL;

	if (iOption& EPlugin)
		{
		TInt r = iFs.AddPlugin(KPluginName);
		test(r == KErrNone || r == KErrAlreadyExists);
		if (r != KErrAlreadyExists)
		    {
		    r = iFs.MountPlugin(KPluginName);
		    test(r == KErrNone);
		    }
		}
	
	FullDirectoryScanL(iEntries);
	iRecords.Reset();
	}

// reset certain members so that we can do next test step
void CNotifyWatcher::Reset(TUint aOp)
	{
	iCurrentOp = aOp;
	iRecords.Reset();
	iCounter = 0;
	iOverflowCounter = 0;
	iMeanCounter = 0;
	if ((aOp == EOpManyFiles) || (aOp == EOpManyChanges))
		iNumFiles += 1000;
	}

// perform a full dir scan
void CNotifyWatcher::FullDirectoryScanL(RArray<TEntry>& aArray)
	{
	aArray.Reset();
	
	CDir* list;
	iFs.GetDir(gTestPath, KEntryAttMaskSupported, ESortByName, list);
	
	if (!list)
	    return;
	
	CleanupStack::PushL(list);
	
	for (TInt i = 0; i < list->Count(); i++)
		{
		TEntry en ((*list)[i]);
		aArray.AppendL(en);
		}
	
	CleanupStack::PopAndDestroy();
	}

// find out what has changed in test path, and save the changes in iRecord
void CNotifyWatcher::MakeChangeRecordL(RArray<TEntry>& aArray)
	{
	TInt num = aArray.Count();
	TInt r,i;
	for (i = 0; i < num; i++)
		{
		TInt index = iEntries.Find(aArray[i], CompareEntryName);
		SAFETEST1((index == KErrNotFound || index >= 0), (iOption & KNotifyTreadIdMask));
		
		TFileName name(aArray[i].iName);
		
		if (index == KErrNotFound)
			{
			r = iRecords.Append(name);
			SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
			}
		else
			{
			if (!CompareEntry(iEntries[index], aArray[i]))
				{
				r = iRecords.Append(name);
				SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
				}
			iEntries.Remove(index);
			}
		}
	
	num = iEntries.Count();
	
	for (i = 0; i < num; i++)
		{
		TFileName name(iEntries[i].iName);
		r = iRecords.Append(name);
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		}
	
	iEntries.Reset();
	num = aArray.Count();
	for (i = 0; i < num; i++)
		{
		TEntry en (aArray[i]);
		iEntries.AppendL(en);
		}
	}


TBool CNotifyWatcher::CompareEntry(const TEntry& aEntry1, const TEntry& aEntry2)
	{
	// we don't compare name, because names are compared by CompareEntryName() already
	// we don't check attributes when creating files, because dir scan sometimes returns file
	// entries before attributes being flushed, and we don't care about it in this test case.
	// we also don't check the modified time for all the test cases expect replacing test, 
	// because dir scan sometimes returns file entries before time being flushed, 
	// and we don't care about it as well.
	// For replacing test, we check modification time, because it's the way we distinguish the old and new files.
	if (iCurrentOp == EOpCreate)
	    {
	    if ((aEntry1.iSize == aEntry2.iSize) && (aEntry1.iType == aEntry2.iType))
            return ETrue;
	    }
	else if (iCurrentOp == EOpReplace || iCurrentOp == EOpManyFiles || iCurrentOp == EOpManyChanges || iCurrentOp == EOpMixed)
	    {
        if ((aEntry1.iAtt == aEntry2.iAtt) && (aEntry1.iModified == aEntry2.iModified) 
                && (aEntry1.iSize == aEntry2.iSize) && (aEntry1.iType == aEntry2.iType))
            return ETrue;
	    }
	else
	    {
        if ((aEntry1.iAtt == aEntry2.iAtt) && (aEntry1.iSize == aEntry2.iSize) 
                && (aEntry1.iType == aEntry2.iType))
            return ETrue;
	    }
	
	return EFalse;
	}

// add 100 filters for enhanced notification test case
void CNotifyWatcher::AddLotsOfFilters()
	{
	for (TInt i = 0; i < 100; i++)
		{
		TFileName path;
		path.Copy(gTestPath);
		path.Append('*');
		TFileName file;
		file.AppendFormat(_L("*.%3d"), i);
		TInt r = iNotify->AddNotification((TUint)TFsNotification::EAllOps, gTestPath, file);
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		}
	}

void CNotifyWatcher::RequestNotification()
	{
	switch (iOption & KNotifyOptionMask)
		{
		case EEnhanced:
			RequestNotificationEnhanced();
			break;
		case EOriginal:
			RequestNotificationOriginal();
			break;
		case EPlugin:
			RequestNotificationPlugin();
			break;
		case ENoNotify:
		default:
			return;
		}
	}

// request notification using enhanced notification
void CNotifyWatcher::RequestNotificationEnhanced()
	{
	if (iOption & EBigFilter)
		{
		AddLotsOfFilters();
		}
	
	TFileName path;
	path.Copy(gTestPath);
	path.Append('*');
	TBuf<3> file = _L("*");
	
	if (iOption & EMultiNoti1)
		{
		TInt r = iNotify->AddNotification(TFsNotification::ECreate, path, file); // Create & replace
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		r = iNotify->AddNotification(TFsNotification::ERename, path, file);	// Change Attribute
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		r = iNotify->AddNotification(TFsNotification::EAttribute, path, file); // Rename
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		r = iNotify->AddNotification(TFsNotification::EFileChange, path, file); // Write & Setsize
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		r = iNotify->AddNotification(TFsNotification::EDelete, path, file); // Delete
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		}
	else if (iOption & EMultiNoti2)
		{
		TInt r = iNotify->AddNotification(TFsNotification::ECreate, path, file); // Create & replace
		TInt id = iOption & KNotifyTreadIdMask;
		
		switch (id%4)
			{
			case 0:
				r = iNotify->AddNotification(TFsNotification::ERename, path, file);	// Change Attribute
				SAFETEST2(r, KErrNone, id);
				break;
			case 1:
				r = iNotify->AddNotification(TFsNotification::EAttribute, path, file); // Rename
				SAFETEST2(r, KErrNone, id);
				break;
			case 2:
				r = iNotify->AddNotification(TFsNotification::EFileChange, path, file); // Write & Setsize
				SAFETEST2(r, KErrNone, id);
				break;
			case 3:
				r = iNotify->AddNotification(TFsNotification::EDelete, path, file); // Delete
				SAFETEST2(r, KErrNone, id);
				break;
			default:
				break;
			}
		}
	else
		{
		TInt r = iNotify->AddNotification((TUint)TFsNotification::EAllOps, path, file);
		SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
		}
	
	TInt r = iNotify->RequestNotifications(iStatus);
	SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
	
	SetActive();
	}

// request notification using original notification
void CNotifyWatcher::RequestNotificationOriginal()
	{
	iFs.NotifyChange(ENotifyAll, iStatus, gTestPath);
	SetActive();
	}

void CNotifyWatcher::HandleNotification(TBool aLastTime)
	{
	switch (iOption & KNotifyOptionMask)
		{
		case EEnhanced:
			HandleNotificationEnhanced(aLastTime);
			break;
		case EOriginal:
			HandleNotificationOriginal(aLastTime);
			break;
		case EPlugin:
			HandleNotificationPlugin(aLastTime);
			break;
		case ENoNotify:
		default:
			return;
		}
	
	if(aLastTime)
		{
		iLogger->MeasureEnd();
		RDebug::Print(_L("Timing ended..."));
		Cancel();
		}
	}

// handle enhanced notification
void CNotifyWatcher::HandleNotificationEnhanced(TBool aLastTime)
	{
	TInt num = iCounter;
	iMeanCounter++;
	
	TFsNotification::TFsNotificationType type;
	const TFsNotification* notification;
	
	while((notification = iNotify->NextNotification())!= NULL)
		{	
		iCounter++;
		type = notification->NotificationType();
		if (iOption & EBigBuffer)
			{
			SAFETEST1((type & OpNotifyMapping(iOption, iCurrentOp)), (iOption & KNotifyTreadIdMask));
			}
		else
			{
			SAFETEST1((type & (OpNotifyMapping(iOption, iCurrentOp) | TFsNotification::EOverflow)), (iOption & KNotifyTreadIdMask));
			}
		
      if(iOption & EReportChg)
            {
            if (type & TFsNotification::EOverflow)
                {
                iOverflowCounter++;
                RArray<TEntry> newEntries;
                FullDirectoryScanL(newEntries);
                MakeChangeRecordL(newEntries);
                newEntries.Close();
                }
            else
                {
                TPtrC ptr;
                notification->Path(ptr);
                TFileName name;
                name.Copy(ptr);
                TInt r = iRecords.Append(name);
                SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
                }
            }
		}
	
	if (aLastTime)
		{
		if (num == iCounter) // no new notification so this iteration doesn't count
		    {
		    iMeanCounter--;
		    }
		return;
		}
	
	iNotify->RequestNotifications(iStatus);
	SetActive();
	}

// handle original notification
void CNotifyWatcher::HandleNotificationOriginal(TBool aLastTime)
	{
	iCounter++;
	
	if (iOption & EReportChg)
		{
		RArray<TEntry> newEntries;
		FullDirectoryScanL(newEntries);
		MakeChangeRecordL(newEntries);
		newEntries.Close();
		}
	
	if (aLastTime)
	    {
	    iCounter--; // the last time this function is called is not a notification.
		return;
	    }

	RequestNotificationOriginal();
	}

// reset the iPluginStatusPkg, so that we can request notification again
void CNotifyWatcher::ResetMdsFSPStatus()
    {
    TMdsFSPStatus& status = iPluginStatusPkg();

    status.iDriveNumber = 0;
    status.iFileEventType = EMdsFileUnknown;
    status.iFileName.Zero();
    status.iNewFileName.Zero();
    status.iProcessId = TUid::Null();
    }

// request notification using nokia MDS plugin
void CNotifyWatcher::RequestNotificationPlugin()
    {
    TInt r = iPlugin.Open(iFs, KMdsFSPluginPosition);
    SAFETEST2(r, KErrNone, (iOption & KNotifyTreadIdMask));
    iPlugin.AddNotificationPath(gTestPath);
    
    iPlugin.Enable();  
    ResetMdsFSPStatus();
    iPlugin.RegisterNotification(iPluginStatusPkg, iStatus);
    SetActive();
    }

// handle notifications from plugin
void CNotifyWatcher::HandleNotificationPlugin(TBool aLastTime)
	{
	if (aLastTime)
	    return;

	if (iOption & EReportChg)
	    {
	    TMdsFSPStatus& status = iPluginStatusPkg();
	    TFileName name;
	    name.Copy(status.iFileName);
	    iRecords.Append(name);
	    if (iCurrentOp != EOpMixed)
	        {
	        TInt type = status.iFileEventType;
	        SAFETEST1(((TUint)type == OpNotifyMapping(iOption, iCurrentOp)), (iOption & KNotifyTreadIdMask));
	        }
	    }
	
    iCounter++;
    ResetMdsFSPStatus();
    iPlugin.RegisterNotification(iPluginStatusPkg, iStatus);
    SetActive();
	}

// cancel request
void CNotifyWatcher::DoCancel()
	{
	switch (iOption & KNotifyOptionMask)
		{
		case EEnhanced:
			iNotify->CancelNotifications(iStatus);
			iNotify->RemoveNotifications();
			// not breaking here as the Enhanced may change to Original if Overflow
		case EOriginal:
			iFs.NotifyChangeCancel(iStatus);
			break;
		case EPlugin:
            iPlugin.Disable();
		    iPlugin.NotificationCancel();
		    iPlugin.Close();
			break;
		case ENoNotify:
		default:
			return;
		}
	}

void CNotifyWatcher::RunL()
	{	
	HandleNotification(EFalse);
	}

//========================================================================

CTestStopper::CTestStopper()
: CActive(EPriorityLow)
	{
	iFs.Connect();
	iTestEndFile.Append(gLogFilePath);
	iTestEndFile.Append(_L("test.End"));
	}

CTestStopper::~CTestStopper()
	{
	Cancel();
	iFs.Close();
	}

void CTestStopper::DoCancel()
	{
	iFs.NotifyChangeCancel(iStatus);
	}

// stop the scheduler since the test is done
void CTestStopper::RunL()
	{
	CActiveScheduler::Stop();
	iFs.Delete(iTestEndFile);
	Cancel();
	}

// start waiting for "test.end" file
void CTestStopper::StartWaitingForFile()
	{
	iFs.NotifyChange(ENotifyAll,iStatus,iTestEndFile);
	SetActive();
	}
