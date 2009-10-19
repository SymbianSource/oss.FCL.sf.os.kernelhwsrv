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
// f32test\bench\t_notify_perf_util.cpp
// 
//

#include "t_notify_perf.h"
#include "t_server.h"

void DoKillThreadsL()
    {
    TInt count = 0;
    while (count < gNotiThreads.Count())
        {
        gNotiThreads[count].Kill(KErrGeneral);
        count++;
        }
    gFileThread.Kill(KErrGeneral);
    }

// Safe way of checking in sub threads, it leaves when check fails, Main thread will catch the error and exit.
// if use test() directly in sub-threads, the threads may hang if check fails
void SafeTestL(TBool aResult, TInt aId, TInt aLine, TText* aFile)
    {
    if (!aResult)
        {
        if (aId >= 0)
            {
            RDebug::Print(_L("NotificationThread%02d: Failed check in %s at line %d"), aId, aFile, aLine);
            }
        else if (aId == KNoThreadId)
            {
            RDebug::Print(_L("Failed check in %s at line %d"), aFile, aLine);
            }
        CTestExecutor::KillAllTestThreads();
        }
    }

void SafeTestL(TInt aResult, TInt aExpected, TInt aId, TInt aLine, TText* aFile)
    {
    if (aResult != aExpected)
        {
        if (aId >= 0)
            {
            RDebug::Print(_L("NotificationThread%02d: Failed check in %s at line %d, expected %d, got %d"), aId, aFile, aLine, aExpected, aResult);
            }
        else if (aId == KNoThreadId)
            {
            RDebug::Print(_L("Failed check in %s at line %d, expected %d, got %d"), aFile, aLine, aExpected, aResult);
            }
        CTestExecutor::KillAllTestThreads();
        }
    }

void SetTestPaths()
    {
    gTestPath.FillZ();
    gLogFilePath.FillZ();
    
    gTestPath.Append(gDriveToTest);
    gTestPath.Append(_L(":\\F32-TST\\T_Notify_Perf\\"));
    
#ifndef __WINSCW__
    gLogFilePath.Append(gDriveToTest);
#else
    gLogFilePath.Append((TChar)'C');  //If emulator lets stick it on C: (\epoc32\wisncw\c\)
#endif
    if (gPerfMeasure)
        gLogFilePath.Append(_L(":\\F32-TST\\NPTestLog\\"));
    else
        gLogFilePath.Append(_L(":\\F32-TST\\Temp\\"));
    }

// Mapping from file operations to notification types
TUint OpNotifyMapping(TUint16& aOption, TInt aOperation)
    {
    if (aOption & EEnhanced)
        {
        switch(aOperation)
            {       
            case EOpCreate:
            case EOpCreateDir:
                return TFsNotification::ECreate;
            case EOpReplace:
            case EOpRename:
            case EOpRenameDir:
                return TFsNotification::ERename;
            case EOpChgAttr:
                return TFsNotification::EAttribute;
            case EOpWrite:
            case EOpResize:
            case EOpManyChanges:
            case EOpManyFiles:
                return TFsNotification::EFileChange;
            case EOpDelete:
            case EOpDeleteDir:
                return TFsNotification::EDelete;
            case EOpMixed:
                return (TUint) (TFsNotification::EAllOps & (~TFsNotification::EOverflow));
            default:
                return (TUint) TFsNotification::EAllOps;
            }
        }
    else if (aOption & EOriginal)
        {
        switch(aOperation)
            {   
            case EOpCreate:
            case EOpReplace:
            case EOpRename:
                return ENotifyFile;
            case EOpChgAttr:
            case EOpResize:
                return ENotifyAttributes;
            case EOpWrite:
            case EOpManyChanges:
            case EOpManyFiles:
                return ENotifyWrite;
            case EOpDelete:
                return ENotifyEntry;
            case EOpCreateDir:
            case EOpRenameDir:
            case EOpDeleteDir:
                return ENotifyDir;
            case EOpMixed:
            default:
                return ENotifyAll;
            }
        }
    else if (aOption & EPlugin)
        {
        switch(aOperation)
            {
            case EOpCreate:
                return EMdsFileCreated;
            case EOpReplace:
                return EMdsFileReplaced;
            case EOpRename:
                return EMdsFileRenamed;
            case EOpDelete:
                return EMdsFileDeleted;
            case EOpRenameDir:
                return EMdsDirRenamed;
            default:
                // All other operations are not testable
                return EMdsFileUnknown;
            }
        }
    return 0;
    }

// generate file names for testing
void FileNameGen(TFileName& aName, TInt aNum, TBool aIsFile = ETrue)
    {
    aName.FillZ();
    aName.Copy(gTestPath);
    if (aIsFile)
        aName.AppendFormat(_L("%04d.tst"), aNum);
    else
        aName.AppendFormat(_L("DIR%04d\\"), aNum);
    }

void ClearTestPathL()
    {
    RDebug::Print(_L("Clearing test path..."));
    RFs fs;
    User::LeaveIfError(fs.Connect());
    CFileMan* fm = CFileMan::NewL(fs);
    TInt r = fm->RmDir(gTestPath);
    test(r==KErrNone || r==KErrPathNotFound || r==KErrNotFound);
    r = fs.MkDirAll(gTestPath);
    test(r==KErrNone || r==KErrAlreadyExists);
    
    delete fm;
    fs.Close();
    }

void DeleteLogFilesL()
    {
    RDebug::Print(_L("Clearing test logs if exist..."));
    RFs fs;
    User::LeaveIfError(fs.Connect());
    CFileMan* fm = CFileMan::NewL(fs);
    
    TFileName logFiles;
    logFiles.Copy(gLogFilePath);
    logFiles.Append('*');
    logFiles.Append(gLogPostFix);
    
    TInt r = fm->Delete(logFiles);
    test(r==KErrNone || r==KErrPathNotFound || r==KErrNotFound);
    if (r != KErrNone)
        {
        r = fs.MkDirAll(gLogFilePath);
        test(r==KErrNone || r==KErrAlreadyExists);
        }
    
    delete fm;
    fs.Close();
    }

// Copy log files from test drive to MMC
void CopyLogFilesL()
    {
    RFs fs;
    User::LeaveIfError(fs.Connect());
    CFileMan* fm = CFileMan::NewL(fs);
    
    TFileName path;
    path.Append(_L("D:\\NPTLogs\\"));
    TInt r = fs.MkDirAll(path);
    test(r == KErrNone || r == KErrAlreadyExists);
    fm->Copy(gLogFilePath, path);
    
    delete fm;
    fs.Close();
    }

// compare the name of two entries
TBool CompareEntryName(const TEntry& aEntry1, const TEntry& aEntry2)
    {
    return (aEntry1.iName.Compare(aEntry2.iName) == 0);
    }

// start file operations
void DoFileOperationL(TThreadParam* aParam)
    {
    CFileOperator fileOperator(aParam->iSetting, *(aParam->iLoggerArray), aParam->iSmphFT, aParam->iSmphNT);
    fileOperator.DoChangesL();
    }

// start monitoring notification
void DoNotificationOperationL(TThreadParam* aParam)
    {
    CActiveScheduler* sch = new(ELeave) CActiveScheduler(); 
    CleanupStack::PushL(sch);
    CActiveScheduler::Install(sch);
    
    CNotifyOperator notifyOperator(aParam->iSetting, aParam->iSmphFT, aParam->iSmphNT, aParam->iLogger);
    aParam->iSmphFT->Signal();
    notifyOperator.StartOperationL();
    
    CleanupStack::PopAndDestroy();
    }

// entry function of file operaton thread
TInt FileOperationThread(TAny* aParam)
    {
    CTrapCleanup* cleanup;
    cleanup = CTrapCleanup::New();
    
    TRAPD(r, DoFileOperationL(static_cast<TThreadParam*>(aParam)));

    delete cleanup;
    
    return r;
    }

// entry function of notification thread
TInt NotificationOperationThread(TAny* aParam)
    {
    CTrapCleanup* cleanup;
    cleanup = CTrapCleanup::New();
    
    TRAPD(r, DoNotificationOperationL(static_cast<TThreadParam*>(aParam)));
    
    delete cleanup;
    
    return r;
    }

TInt KillerThread(TAny*)
    {
    CTrapCleanup* cleanup;
    cleanup = CTrapCleanup::New();
    
    TRAPD(r, DoKillThreadsL());
    
    delete cleanup;
    
    return r;
    }
