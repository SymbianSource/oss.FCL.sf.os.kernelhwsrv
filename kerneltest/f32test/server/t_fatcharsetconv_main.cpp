// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\t_fatcharsetconv_main.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <e32svr.h>
//#include <hal.h>
#include <f32fsys.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_chlffs.h"
#include "fat_utils.h"
#include "t_fatcharsetconv_cases.h"


RTest test(_L("T_FatCharSetConv"));

using namespace Fat_Test_Utils;

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)

template <class C>
TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c)
	{
    TPtr8 ptrC((TUint8 *)&c, sizeof(C), sizeof(C));
    TInt r = fs.ControlIo(drv, fkn, ptrC);
    return r;
	}

/*
 * Presetting module, presets initial source, target and comparing direcotries.
 * @param	aParam	test param that contains all information about a test case
 */
void DataGenerationL(const TTestParamAll& aParam)
	{
	// Setup source files
	RBuf path;
	path.CreateL(aParam.iSrcPrsPath);
	
	path[0] = (TUint16)*aParam.iSrcDrvChar;
	if(path[0] == (TUint8)gDriveToTest)
		{
		SetupDirFiles(path, aParam.iSrcPrsFiles);
		}
	
	if (aParam.iAPI == EGetShortName || aParam.iAPI == EGetShortNameWithDLL || aParam.iAPI == EGetShortNameWithoutDLL || aParam.iAPI == ERFsReplace || aParam.iAPI == ERFsRename ||aParam.iAPI == ERenameFile )
		{
		path[0] = (TUint16)*aParam.iTrgDrvChar;
		}
	path.Close();
	CheckDisk();
	}

/*
 * Test execution module
 * @param	aParam	test param that contains all information about a test case
 * @panic	USER:84	if return codes do not match the expected values. 	
 */
void DataExecutionL(const TTestParamAll& aParam, const TTCType aTCType)
	{
	RBuf srcCmdFile;
	srcCmdFile.CreateL(aParam.iSrcCmdPath);
	RBuf trgCmdFile;
	trgCmdFile.CreateL(aParam.iTrgCmdPath);
	RBuf srcCmd;
	srcCmd.CreateL(aParam.iSrcPrsPath);
	
	if (srcCmdFile.Length() > 0)
		{
		srcCmdFile[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	else
		{
		// srcCmdFile= gSessionPath;
		srcCmdFile[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	if (srcCmd.Length() > 0)
		{
		srcCmd[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	else
		{
		// srcCmd= gSessionPath;
		srcCmd[0] = (TUint16)*aParam.iSrcDrvChar;
		}

	// logging for failure
	gTCType = aTCType;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(KExecution());
	gTCId = aParam.iTestCaseID;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(aParam.iSrcPrsPath);

	TInt r = KErrNone;

	switch(aParam.iAPI)
		{
		case EGetShortName:
		case EGetShortNameWithDLL:
		case EGetShortNameWithoutDLL:
			{
			__UHEAP_MARK;
			if(aParam.iAPI == EGetShortName )
				gLogFailureData.iAPIName = KGetShortName;
			else if(aParam.iAPI == EGetShortNameWithDLL )
				gLogFailureData.iAPIName = KGetShortNameWithDLL;
			else if(aParam.iAPI == EGetShortNameWithoutDLL )
				gLogFailureData.iAPIName = KGetShortNameWithoutDLL;

				if (trgCmdFile.Length() > 0)
					{
					trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
					}
				else
					{
					//trgCmdFile= gSessionPath;
					trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
					}

			TBuf<0x10> shortName;
			r=TheFs.GetShortName(srcCmdFile,shortName);
			testAndLog(r==KErrNone);
			srcCmd.ReAllocL(srcCmd.Length() + shortName.Length());
			srcCmd+= shortName;
			testAndLog(srcCmd==trgCmdFile);
			__UHEAP_MARKEND;
			}
			break;
		case ELongShortConversion:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KLongShortConversion;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}

			RBuf lgnFullPath;
			RBuf shnFullPath;
			TFileName longName;
			TBuf<0x10> shortName;
	
			r = TheFs.GetShortName(srcCmdFile, shortName);
			testAndLog(r==KErrNone);
	
			shnFullPath.CreateL(srcCmd);
			shnFullPath.ReAllocL(srcCmd.Length() + shortName.Length());
			shnFullPath.Append(shortName);
			r = shnFullPath.Compare(trgCmdFile);
			testAndLog(r==KErrNone);

			r = TheFs.GetLongName(shnFullPath, longName);
			testAndLog(r==KErrNone);
			
			lgnFullPath.CreateL(srcCmd);
			lgnFullPath.ReAllocL(srcCmd.Length() + longName.Length());
			lgnFullPath.Append(longName);
			r = lgnFullPath.Compare(srcCmdFile);
			testAndLog(r==KErrNone);
			
			lgnFullPath.Close();
			shnFullPath.Close();
			__UHEAP_MARKEND;
			}
			break;
		case ECreateFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName  = KCreateFile;
			r = TheFile.Create(TheFs,srcCmdFile,EFileWrite);
			testAndLog(r==KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EIsValidName:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KIsValidName;
			TText badChar;
			r=TheFs.IsValidName(srcCmdFile,badChar);
			testAndLog(r==(TInt)ETrue);
			r=TheFs.IsValidName(srcCmdFile);
			testAndLog(r==(TInt)ETrue);
			__UHEAP_MARKEND;
			}
			break;
		case EMkDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KMkDir;
			r=TheFs.MkDir(srcCmdFile);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;		
		case EMkDirAll:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KMkDirAll;
			r=TheFs.MkDirAll(srcCmdFile);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;		
		case ERenameFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRenameFile;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
				
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);
			r=TheFile.Rename(trgCmdFile);
			testAndLog(r==KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EReadFileSection:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReadFileSection;
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);
			
			TInt numWrite = 15;
			TBuf8<50> writeBuf = _L8("I am going to write some junk for testing purpose");
			TheFile.Write(writeBuf, numWrite);
   			r=TheFile.Flush();
   			testAndLog(r==KErrNone);
  			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EDeleteFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KDeleteFile;
			r=TheFs.Delete(srcCmdFile);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case EOpenDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KOpenDir;
			r=TheDir.Open(TheFs,srcCmdFile,KEntryAttMaskSupported);
			testAndLog(r==KErrNone);
			TheDir.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EReadDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReadDir;
			r=TheDir.Open(TheFs,srcCmdFile,KEntryAttMaskSupported);
			testAndLog(r==KErrNone);
			TheDir.Close();
			__UHEAP_MARKEND;
			}
			break;
		case ERemoveDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRemoveDir;
			r=TheFs.RmDir(srcCmdFile);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;		
		case EIsFileInRom:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KIsFileInRom;
			// The tested files are created on non-rom drives, which should not be found 
			//	on rom drives
			TUint8* ptr=TheFs.IsFileInRom(srcCmdFile);
			testAndLog(ptr==NULL);
			__UHEAP_MARKEND;
			}
			break;
		case EReplaceFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReplaceFile;
			r=TheFile.Replace(TheFs, srcCmdFile ,EFileWrite);
			testAndLog(r==KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EOperateOnFileNames:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KOperateOnFileNames;
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead);
			testAndLog(r == KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EFileModify:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KFileModify;
			TTime time;
			r=TheFs.Modified(srcCmdFile,time);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case EFileAttributes:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KFileAttributes;
			TUint att;
			r=TheFs.Att(srcCmdFile,att);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case ERFsEntry:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRFsEntry;
			TEntry entryDetail;
			r=TheFs.Entry(srcCmdFile, entryDetail);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case ERFsReplace:
			{
			__UHEAP_MARK;	
			gLogFailureData.iAPIName = KRFsReplace;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			r=TheFs.Replace(srcCmdFile,trgCmdFile);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case ERFsRename:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRFsRename;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				} 
			r=TheFs.Rename(srcCmdFile,trgCmdFile);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}	
			break;
		case EGetDir:	
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KGetDir;
			CDir *anEntryList;
			r=TheFs.GetDir(srcCmdFile,KEntryAttNormal,ESortByName,anEntryList);
			testAndLog(r==KErrNone);
			delete anEntryList;
			__UHEAP_MARKEND;
			}
			break;
		case EFileTemp:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KFileTemp;
			TFileName tempFileName;
			r=TheFile.Temp(TheFs,srcCmdFile,tempFileName,EFileWrite);
			testAndLog(r == KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EReadFromFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReadFromFile;
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);
			
			TBuf8<200> writeBuf = _L8("I am going to write something to the file and then read from the specified position to test an overload of RFIle::Read");
			TInt numWrite = 50;
   			TheFile.Write(writeBuf, numWrite);
   			testAndLog(r==KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}	
		break;
		case EWriteToFile:
			{
			__UHEAP_MARK;						
			gLogFailureData.iAPIName = KWriteToFile;

			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);

			TBuf8<200> writeBuf = _L8("I am going to write something to the file and then read from the specified position to test an overload of RFIle::Read");
			TInt numWrite = 50;
   			TheFile.Write(writeBuf, numWrite);
   			testAndLog(r==KErrNone);
   			r=TheFile.Flush();
   			testAndLog(r==KErrNone);
			TheFile.Close();
   			__UHEAP_MARKEND;
			}	
			
		break;
	 	default:
		break;

		}
//	test.Printf(_L("DataExecution::\tTest case %d passed\n"),aParam.iTestCaseID);

	failedOnBuf.Close();
	tcUniquePath.Close();

	srcCmdFile.Close();
	trgCmdFile.Close();
	srcCmd.Close();
	CheckDisk();
	}

void DataVerificationL(const TTestParamAll& aParam, const TTCType aTCType)
	{
	RBuf srcCmdFile;
	srcCmdFile.CreateL(aParam.iSrcCmdPath);
	RBuf trgCmdFile;
	trgCmdFile.CreateL(aParam.iTrgCmdPath);
	RBuf srcCmd;
	srcCmd.CreateL(aParam.iSrcPrsPath);

	if (srcCmdFile.Length() > 0)
		{
		srcCmdFile[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	else
		{
		//srcCmdFile= gSessionPath;
		srcCmdFile[0] = (TUint16)*aParam.iSrcDrvChar;
		}
		
	if (srcCmd.Length() > 0)
		{
		srcCmd[0] = (TUint16)*aParam.iSrcDrvChar;
		}
	else
		{
		//srcCmd= gSessionPath;
		srcCmd[0] = (TUint16)*aParam.iSrcDrvChar;
		}

	// logging for failure
	gTCType = aTCType;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(KVerification());
	gTCId = aParam.iTestCaseID;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(aParam.iSrcPrsPath);

	TInt r = KErrNone;
	switch(aParam.iAPI)
		{
		case EGetShortName:
		case EGetShortNameWithDLL:
		case EGetShortNameWithoutDLL:
			{
			__UHEAP_MARK;
			if(aParam.iAPI == EGetShortName )
				gLogFailureData.iAPIName = KGetShortName;
			else if(aParam.iAPI == EGetShortNameWithDLL )
				gLogFailureData.iAPIName = KGetShortNameWithDLL;
			else if(aParam.iAPI == EGetShortNameWithoutDLL )
				gLogFailureData.iAPIName = KGetShortNameWithoutDLL;

				if (trgCmdFile.Length() > 0)
					{
					trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
					}
				else
					{
					//trgCmdFile= gSessionPath;
					trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
					}

			TBuf<0x10> shortName;
			r=TheFs.GetShortName(srcCmdFile,shortName);
			testAndLog(r==KErrNone);
			if(r==KErrNone)
				{
				srcCmd.ReAllocL(srcCmd.Length() + shortName.Length());
				srcCmd+= shortName;
				testAndLog(srcCmd==trgCmdFile);
				}
			__UHEAP_MARKEND;
			}
			break;
		case ELongShortConversion:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KLongShortConversion;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}

			RBuf lgnFullPath;
			RBuf shnFullPath;
			TFileName longName;
			TBuf<0x10> shortName;
	
			r = TheFs.GetShortName(srcCmdFile, shortName);
			testAndLog(r==KErrNone);
	
			if(r==KErrNone)
				{
				shnFullPath.CreateL(srcCmd);
				shnFullPath.ReAllocL(srcCmd.Length() + shortName.Length());
				shnFullPath.Append(shortName);
				r = shnFullPath.Compare(trgCmdFile);
				testAndLog(r==KErrNone);

				r = TheFs.GetLongName(shnFullPath, longName);
				testAndLog(r==KErrNone);
				
				lgnFullPath.CreateL(srcCmd);
				lgnFullPath.ReAllocL(srcCmd.Length() + longName.Length());
				lgnFullPath.Append(longName);
				r = lgnFullPath.Compare(srcCmdFile);
				testAndLog(r==KErrNone);

				lgnFullPath.Close();
				shnFullPath.Close();
				}
			__UHEAP_MARKEND;
			}
			break;
		case ECreateFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KCreateFile;
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EIsValidName:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KIsValidName;
			TText badChar;
			r=TheFs.IsValidName(srcCmdFile,badChar);
			testAndLog(r);
			r=TheFs.IsValidName(srcCmdFile);
			testAndLog(r);
			__UHEAP_MARKEND;
			}
			break;
		case EMkDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KMkDir;
			r=TheDir.Open(TheFs,srcCmdFile,KEntryAttMaskSupported);
			testAndLog(r==KErrNone);
			TheDir.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EMkDirAll:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KMkDirAll;
			r=TheDir.Open(TheFs,srcCmdFile,KEntryAttMaskSupported);
			testAndLog(r==KErrNone);
			TheDir.Close();
			__UHEAP_MARKEND;
			}
			break;
		case ERenameFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRenameFile;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNotFound);
			// r = TheFile.Open(TheFs, trgCmdFile, EFileRead|EFileWrite);
			// testAndLog(r == KErrNone);
			// TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EReadFileSection:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReadFileSection;
			TBool isFileOpen = EFalse;
			r=TheFs.IsFileOpen(srcCmdFile,isFileOpen);
			testAndLog(r==KErrNone);
			if(isFileOpen)
				{
				TheFile.Close();
				}
			else
				{
				TInt numRead = 15;
				TBuf8<50> readBuf;
				r=TheFs.ReadFileSection(srcCmdFile, 0, readBuf, numRead);
				testAndLog(r==KErrNone);
				}
			__UHEAP_MARKEND;
			}
			break;
		case EDeleteFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KDeleteFile;
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNotFound);
			// TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EOpenDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KOpenDir;
			r=TheDir.Open(TheFs,srcCmdFile,KEntryAttMaskSupported);
			testAndLog(r==KErrNone);
			TheDir.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EReadDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReadDir;
			TEntry entry;
			TInt dirEntryCount = 0;
			r=TheDir.Open(TheFs,srcCmdFile,KEntryAttMaskSupported);
			testAndLog(r==KErrNone);
			FOREVER
				{
				r = TheDir.Read(entry);
				if(r == KErrEof)
					{
					break;
					}
				dirEntryCount++;
				}
			TheDir.Close();
			testAndLog(dirEntryCount == 4);
			__UHEAP_MARKEND;
			}
			break;
		case ERemoveDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRemoveDir;
			r = TheDir.Open(TheFs, srcCmdFile, KEntryAttMaskSupported);
			testAndLog(r == KErrPathNotFound);
			// TheDir.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EIsFileInRom:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KIsFileInRom;
			TUint8* ptr=TheFs.IsFileInRom(srcCmdFile);
			// file should not be in ROM
			testAndLog(ptr==NULL)
			__UHEAP_MARKEND;
			}
			break;
		case EReplaceFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReplaceFile;
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);
			TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case EOperateOnFileNames:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KOperateOnFileNames;
			
			TFileName realFileNameInFS;
			TFileName fullFileName;
			TFileName fileName;

			r = TheFile.Open(TheFs, srcCmdFile, EFileRead);
			testAndLog(r == KErrNone);

			if(r==KErrNone)
				{
				r=TheFile.FullName(fullFileName);
				testAndLog(r==KErrNone);
				testAndLog(fullFileName==srcCmdFile);
				
				TInt length = srcCmdFile.Length()-srcCmd.Length();
				
				r=TheFile.Name(fileName);
				testAndLog(r==KErrNone);
				
				TBuf<50> tempFileName;
				tempFileName = srcCmdFile.Right(length);
				testAndLog(fileName==tempFileName);
				testAndLog(length==fileName.Length());

				r=TheFs.RealName(srcCmdFile,realFileNameInFS);
				testAndLog(r==KErrNone);
				testAndLog(realFileNameInFS==srcCmdFile);
				TheFile.Close();
				}
			__UHEAP_MARKEND;
			}
			break;
		case EFileModify:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KFileModify;
			TTime tempTime;
			r=TheFs.Modified(srcCmdFile,tempTime);
			testAndLog(r==KErrNone);
			tempTime.HomeTime();
			r=TheFs.SetModified(srcCmdFile,tempTime);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case EFileAttributes:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KFileAttributes;
			TUint att;
			r=TheFs.Att(srcCmdFile,att);
			testAndLog(r==KErrNone);
			r=TheFs.SetAtt(srcCmdFile,KEntryAttHidden,0);
			testAndLog(r==KErrNone);
			r=TheFs.Att(srcCmdFile,att);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case ERFsEntry:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRFsEntry;
			TEntry entryDetail;
			r=TheFs.Entry(srcCmdFile,entryDetail);
			testAndLog(r==KErrNone);
			TTime time;
			time.HomeTime();
			r=TheFs.SetEntry(srcCmdFile, time, KEntryAttHidden, KEntryAttNormal);
			testAndLog(r==KErrNone);
			r=TheFs.Entry(srcCmdFile, entryDetail);
			testAndLog(r==KErrNone);
			__UHEAP_MARKEND;
			}
			break;
		case ERFsReplace:
			{
			__UHEAP_MARK;	
			gLogFailureData.iAPIName = KRFsReplace;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNotFound);
			// r = TheFile.Open(TheFs, trgCmdFile, EFileRead|EFileWrite);
			// testAndLog(r == KErrNone);
			// TheFile.Close();
			__UHEAP_MARKEND;
			}
			break;
		case ERFsRename:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KRFsRename;
			if (trgCmdFile.Length() > 0)
				{
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				}
			else
				{
				//trgCmdFile= gSessionPath;
				trgCmdFile[0] = (TUint16)*aParam.iTrgDrvChar;
				} 
			TInt len=srcCmdFile.Length();
			if(srcCmdFile[--len]=='\\')
				{
				r = TheDir.Open(TheFs, srcCmdFile, KEntryAttMaskSupported);
				testAndLog(r == KErrPathNotFound);
				// r = TheDir.Open(TheFs, trgCmdFile, KEntryAttMaskSupported);
				// testAndLog(r == KErrNone);			
				// TheDir.Close();
				}
			else
				{
				r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
				testAndLog(r == KErrNotFound);
				// r = TheFile.Open(TheFs, trgCmdFile, EFileRead|EFileWrite);
				// testAndLog(r == KErrNone);
				// TheFile.Close();
				}
			__UHEAP_MARKEND;			
			}
			break;
		case EGetDir:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KGetDir;
			CDir *anEntryList;
			r=TheFs.GetDir(srcCmdFile,KEntryAttNormal,ESortByName,anEntryList);
			testAndLog(r==KErrNone);
			delete anEntryList;
			__UHEAP_MARKEND;
			}
			break;
		case EFileTemp:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KFileTemp;
			TFileName tempFileName;
			r=TheFile.Temp(TheFs,srcCmdFile,tempFileName,EFileWrite);
			testAndLog(r == KErrNone);
			TheFile.Close();
			r = TheFile.Create(TheFs, tempFileName, EFileRead|EFileWrite);
			testAndLog(r == KErrAlreadyExists);
			__UHEAP_MARKEND;
			}			
			break;
		case EReadFromFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KReadFromFile;
			
			TInt start=0, readLen=15, readPos=10;
			TBuf8<200> readBuf;
			TInt numWritten = 50;

			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);

			if(r==KErrNone)
				{
				r=TheFile.Seek(ESeekStart,start);
		  		testAndLog(r==KErrNone);

				readBuf.SetLength(0);
				r=TheFile.Read(readBuf);
				testAndLog(r == KErrNone);
				testAndLog(numWritten == readBuf.Length());
				
				//An overload of RFile::Read(), read specified no.of bytes.
	   			r=TheFile.Seek(ESeekStart,start);
	  			testAndLog(r==KErrNone);
	  			
				readBuf.SetLength(0);
				TheFile.Read(readBuf,readLen);
				testAndLog(r == KErrNone);
				testAndLog(readLen == readBuf.Length());
				
				//An overload of RFile::Read(), read from the specified position.
				readBuf.SetLength(0);
				r=TheFile.Read(readPos,readBuf);
				testAndLog(r == KErrNone);
				testAndLog(numWritten-readPos == readBuf.Length());
				TheFile.Close();
				}
			__UHEAP_MARKEND;
			}	
			break;	
		case EWriteToFile:
			{
			__UHEAP_MARK;
			gLogFailureData.iAPIName = KWriteToFile;
			
			r = TheFile.Open(TheFs, srcCmdFile, EFileRead|EFileWrite);
			testAndLog(r == KErrNone);

			if(r==KErrNone)
				{
				TInt writeLen=20, writePos=10, start=0, readLen=0;
				TBuf8<200> writeBuf = _L8("I am going to write something to the file and then read from the specified position to test an overload of RFIle::Read");
				TInt numWritten = writeBuf.Length();
				TBuf8<200> readBuf;
				
	   			TheFile.Write(writeBuf);
	   			r=TheFile.Flush();
	   			testAndLog(r==KErrNone);
	   			
	   			r=TheFile.Seek(ESeekStart,start);
	  			testAndLog(r==KErrNone);
	  		
	   			readBuf.SetLength(0);
	   			r=TheFile.Read(readBuf);
	   			testAndLog(r==KErrNone);
	   			readLen = readBuf.Length();
	   			testAndLog(numWritten == readLen);
	   			
	   			r=TheFile.SetSize(0);
	   			testAndLog(r==KErrNone);
	   			
	   			//An overload of RFile::Write(), write specified no.of bytes.
	   			TheFile.Write(writeBuf,writeLen);
	   			r=TheFile.Flush();
	   			testAndLog(r==KErrNone);
	   			
	   			r=TheFile.Seek(ESeekStart,start);
	  			testAndLog(r==KErrNone);
				
				readBuf.SetLength(0);
	   			r=TheFile.Read(readBuf);
	   			testAndLog(r==KErrNone);
	   			readLen = readBuf.Length();
	   			testAndLog(writeLen==readLen);
	   			
				r=TheFile.SetSize(0);
	   			testAndLog(r==KErrNone);
	   			
	   			//An overload of RFile::Write(), write to a particular position.
	   			TBuf8<50> testBuf = _L8("Testing different variants of RFile::Write APIs");   		
	    		TheFile.Write(testBuf);
	    		r=TheFile.Flush();
	   			testAndLog(r==KErrNone);
				
	   			TheFile.Write(writePos,writeBuf);
	   			r=TheFile.Flush();
	   			testAndLog(r==KErrNone);
	   			
	      		r=TheFile.Seek(ESeekStart,start);
	  			testAndLog(r==KErrNone);
	  			
	  			readBuf.SetLength(0);
	   			r=TheFile.Read(readBuf);
	   			testAndLog(r==KErrNone);
	   			readLen = readBuf.Length();
	   			testAndLog(numWritten + writePos == readLen);
	   			
				r=TheFile.SetSize(0);
	   			testAndLog(r==KErrNone);
	   			
	   			//An overload of RFile::Write(), write to a particular position and specified no. of bytes.
	   			TInt size;
	    		TheFile.Write(testBuf);
	    		r=TheFile.Flush();
	   			testAndLog(r==KErrNone);
			
	   			TheFile.Write(writePos,writeBuf,writeLen);
	   			r=TheFile.Flush();
	   			testAndLog(r==KErrNone);
	   			
	   			r=TheFile.Seek(ESeekStart,start);
	  			testAndLog(r==KErrNone);
	  			
	  			readBuf.SetLength(0);
	   			r=TheFile.Read(readBuf);
	   			testAndLog(r==KErrNone);
	   			
	   			TInt newSize = testBuf.Length()-(writePos+writeLen);
	   			if(newSize < testBuf.Length())
	   				{
	   				size = testBuf.Length();
	   				}
	   			else
	   				size = newSize;
	   			readLen = readBuf.Length();
	   			testAndLog(readLen==size);

	   			r=TheFile.SetSize(0);
	   			testAndLog(r==KErrNone);

	   			TheFile.Close();
				}
   			__UHEAP_MARKEND;
			}
			break;
	 	default:
			break;
		}
//	test.Printf(_L("DataVerification::\tTest case %d passed\n"),aParam.iTestCaseID);

	failedOnBuf.Close();
	tcUniquePath.Close();

	srcCmdFile.Close();
	trgCmdFile.Close();
	srcCmd.Close();
	CheckDisk();
	}
	
void DeletePathAfterTest(const TTestParamAll& aParam)
	{
	TFileName path = aParam.iSrcPrsPath;
	if (path.Length() == 0)
		{
		test.Printf(_L("ERROR<SetupDirFiles()>: Zero length src path!\n"));
		test(EFalse);
		}

	path[0] = (TUint16)*aParam.iSrcDrvChar;
	TInt idx = path.Find(_L("Src\\"));
	path.Delete(idx,path.Length()-idx);
	RmDir(path);
	}


/*
 * Do all basic binary test cases defined in gBasicUnitaryTestCases[]
 */
void DoAllBasicUnitaryTestsL(const TTestCaseUnitaryBasic aBasicUnitaryTestCaseGroup[],
									TTestSwitches& aSwitches, TBool aIsWithDLL)
	{
	TTestParamAll* 	nextTestCase = new(ELeave) TTestParamAll();
	
	TInt i = 0;
	
	// Reset the Test Log Data
	ClearTCLogData();

	while(SearchTestCaseByArrayIdx(i, aBasicUnitaryTestCaseGroup,
							*nextTestCase, aIsWithDLL) == KErrNone)
		{
		TTime startTime;
		TTime endTime;
		startTime.HomeTime();
		if(aSwitches.iExeOnSymbian || aSwitches.iExeOnWindows)
			{
			DataGenerationL(*nextTestCase);
			DataExecutionL(*nextTestCase, EUnitaryTest);
			}
		if(aSwitches.iVerOnSymbian || aSwitches.iVerOnWindows)
			{
			DataVerificationL(*nextTestCase, EUnitaryTest);
			DeletePathAfterTest(*nextTestCase);
			}
			
		CheckDisk();
		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken(0);
		timeTaken = endTime.MicroSecondsFrom(startTime);

//		test.Printf(_L("Test Case Id : %d\n"),(*nextTestCase).iTestCaseID);
//		TInt time=0;
//		time=I64LOW(timeTaken.Int64()/1000);
//		test.Printf(_L("Time Taken by test case %d = %d mS \n"),nextTestCase.iTestCaseID,time);
		++i;
		}
	delete nextTestCase;
	}
		
/*
 * Do all basic binary test cases defined in gBasicBinaryTestCases[]
 */
void DoAllBasicBinaryTestsL(const TTestCaseBinaryBasic aBasicBinaryTestCaseGroup[],
							TTestSwitches& aSwitches, TBool aIsWithDLL)
	{
	TTestParamAll*	nextTestCase = new(ELeave) TTestParamAll();
	
	TInt i = 0;
	
	// Reset the Test Log Data
	ClearTCLogData();
	
	while (SearchTestCaseByArrayIdx(i, aBasicBinaryTestCaseGroup,
							*nextTestCase, aIsWithDLL) == KErrNone)
		{
		if (aIsWithDLL)
			{
			if(((aBasicBinaryTestCaseGroup[i].iBasic.iAPI==EGetShortNameWithoutDLL)))
				{
				++i;
				continue;
				}
			}
		else
			{
			if(((aBasicBinaryTestCaseGroup[i].iBasic.iAPI== EGetShortNameWithDLL) || (aBasicBinaryTestCaseGroup[i].iBasic.iAPI==ELongShortConversion)) )
				{
				++i;
				continue;
				}
			}
		TTime startTime;
		TTime endTime;
		startTime.HomeTime();

		if(aSwitches.iExeOnSymbian || aSwitches.iExeOnWindows) 
			{
			DataGenerationL(*nextTestCase);
			DataExecutionL(*nextTestCase, EBinaryTest);
			}
		if(aSwitches.iVerOnSymbian || aSwitches.iVerOnWindows)
			{
			DataVerificationL(*nextTestCase, EBinaryTest);
			DeletePathAfterTest(*nextTestCase);
			}

		endTime.HomeTime();
		TTimeIntervalMicroSeconds timeTaken(0);
		timeTaken = endTime.MicroSecondsFrom(startTime);

//		test.Printf(_L("Test Case Id : %d\n"),(*nextTestCase).iTestCaseID);
//		TInt time2=0;
//		time2=I64LOW(timeTaken.Int64()/1000);
//		test.Printf(_L("Time Taken by test id %d = %d mS \n"),nextTestCase.iTestCaseID,time2);
		++i;
		}

	delete nextTestCase;
	}

/*
 * Main testing control unit
 */
void TestMainWithDLLL(TTestSwitches& aSwitches)
	{
	// Enables codepage dll implementation of LocaleUtils functions for this test only
	TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
	test_KErrNone(r);
		
	test.Printf(_L("Load the Code Page DLL"));
	r = UserSvr::ChangeLocale(KTestLocale);
	test_KErrNone(r);
			
	test.Next(_L("Test Unitary APIs with only Sync Variant with DLL"));
	gLogFailureData.iFuncName = KDoAllBasicUnitaryTestsL;
	DoAllBasicUnitaryTestsL(gBasicUnitaryTestCases, aSwitches, ETrue);
	
	test.Next(_L("Test Binary Tests with DLL"));
	gLogFailureData.iFuncName = KDoAllBasicBinaryTestsL;
	DoAllBasicBinaryTestsL(gBasicBinaryTestCases, aSwitches, ETrue);
	
	// Disables codepage dll implementation of LocaleUtils functions for other base tests
	r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	test_KErrNone(r);
	}
	
void TestMainWithoutDLLL(TTestSwitches& aSwitches)
	{
	test.Next(_L("Test Unitary APIs with only Sync Variant without DLL"));

	// Disables codepage dll implementation of LocaleUtils functions
	TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
	test_KErrNone(r);

	gLogFailureData.iFuncName = KDoAllBasicUnitaryTestsL;
	DoAllBasicUnitaryTestsL(gBasicUnitaryTestCases, aSwitches, EFalse);
	
	test.Next(_L("Test Binary Tests without DLL"));
	gLogFailureData.iFuncName = KDoAllBasicBinaryTestsL;
	DoAllBasicBinaryTestsL(gBasicBinaryTestCases, aSwitches, EFalse);
	}

void TestCompatibility(const TTestSwitches& aSwitches)
	{
	test.Next(_L("test compatibility"));

	// logging for failure
	gTCType = ESymbianFATSpecific;
	RBuf failedOnBuf;
	failedOnBuf.CreateL(gLogFailureData.iFuncName);
	gTCId = 0;
	RBuf tcUniquePath;
	tcUniquePath.CreateL(KNone());

	// original file name in DBCS
	TFileName fn1 = _L("\\\x798F\x5C71\x96C5\x6CBB");
	if (aSwitches.iExeOnSymbian)
		{
		GetBootInfo();
		RFile file;
		TFileName fn = _L("\\ABCD");
		
		TInt r=file.Create(TheFs,fn,EFileRead);
		testAndLog(r==KErrNone);
		file.Close();

		//	Assume this file is the first entry in the root directory
		r=TheDisk.Open(TheFs,CurrentDrive());
		testAndLog(r==KErrNone);
		
	    //-- read the 1st dir entry, it should be a DOS entry 
	    const TInt posEntry1=gBootSector.RootDirStartSector() << KDefaultSectorLog2; //-- dir entry1 position
	    
	    TFatDirEntry fatEntry1;
		TPtr8 ptrEntry1((TUint8*)&fatEntry1,sizeof(TFatDirEntry));
	    testAndLog(TheDisk.Read(posEntry1, ptrEntry1)==KErrNone); 
	    testAndLog(!fatEntry1.IsVFatEntry());

	    // Manually modify the short name into unicode characters
	    // 	Unicode: 	0x(798F 5C71 96C5 6CBB)
	    //	Shift-JIS: 	0x(959F 8E52 89EB 8EA1)

	    TBuf8<8> unicodeSN = _L8("ABCD1234");
	    unicodeSN[0] = 0x95;
	    unicodeSN[1] = 0x9F;
	    unicodeSN[2] = 0x8E;
	    unicodeSN[3] = 0x52;
	    unicodeSN[4] = 0x89;
	    unicodeSN[5] = 0xEB;
	    unicodeSN[6] = 0x8E;
	    unicodeSN[7] = 0xA1;
	    
	    fatEntry1.SetName(unicodeSN);
	    testAndLog(TheDisk.Write(posEntry1, ptrEntry1)==KErrNone);
	    TheDisk.Close();
		}
	if (aSwitches.iVerOnSymbian)
		{
		// Access the file without the DLL loaded.
	    TInt r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
		testAndLog(r==KErrNone);
		
		TEntry entry;
		r = TheFs.Entry(fn1, entry);
		testAndLog(r==KErrNotFound);
		
	    // Access the file with the DLL loaded.
	    r = TheFs.ControlIo(CurrentDrive(), KControlIoEnableFatUtilityFunctions);
		testAndLog(r==KErrNone);
		r = UserSvr::ChangeLocale(KTestLocale);
		testAndLog(r==KErrNone);

		r = TheFs.Entry(fn1, entry);
		testAndLog(r==KErrNone);
		
	    r = TheFs.ControlIo(CurrentDrive(), KControlIoDisableFatUtilityFunctions);
		testAndLog(r==KErrNone);
		}
	failedOnBuf.Close();
	tcUniquePath.Close();
	}

#endif //defined(_DEBUG) || defined(_DEBUG_RELEASE)

/*
 * Initialise test 
*/
void CallTestsL(TTestSwitches& aSwitches)
	{
	test.Title();
	test.Start(_L("Starting T_FATCHARSETCONV tests"));
	(void) aSwitches;
#ifdef LOG_FAILURE_DATA
	test.Printf(_L("LOG_FAILURE_DATA: ON\n"));
#else
	test.Printf(_L("LOG_FAILURE_DATA: OFF\n"));
#endif

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	// Test only runs on Fat file systems
	TheFs.SessionPath(gSessionPath);
	TInt r = TheFs.FileSystemName(aSwitches.iMountedFSName, CurrentDrive());
	if (KErrNone == r)
		{
		// For automated testing this test should run on FAT file systems only.
		// For inter-operability testing on FAT and Win32 file systems.
		// if ((AUTO && FAT) || (IOT && (FAT || Win32))
		test.Printf(_L("Current File System: \"%S\"\n"), &aSwitches.iMountedFSName);
		if( (gAutoTest && isFAT(aSwitches.iMountedFSName)) ||
			(gIOTesting && (isFAT(aSwitches.iMountedFSName) || isWin32(aSwitches.iMountedFSName))))
			{
			// Log File System Name
			gLogFailureData.iFSName = aSwitches.iMountedFSName;
			
			// Store current file name
			TFileName fileName;
			TInt i=0;
			while(__FILE__[i]!='\0')
				{
				fileName.SetLength(i+1);
				fileName[i] = __FILE__[i++];
				}
			gFileName.Append(fileName);
			
			InitialiseL();
			// Special Cases	- Non-Symbian
			// Note: this case MUST be run as the first case as it performs formatting!!
			TestCompatibility(aSwitches);
			CreateTestDirectory(_L("\\F32-TST\\T_FATCHARSETCONV\\"));
			//Special Cases	- Only Symbian Specific
			if(!gIOTesting && aSwitches.iMountedFSName.Compare(KFAT) == 0)
				{
				DoSymbianSpecificCases();
				}

			test.Next(_L("Test without DLL loaded"));
			if(gAutoTest)
				{
				QuickFormat();
				}
			TestMainWithoutDLLL(aSwitches);
							
			test.Next(_L("Test with DLL loaded"));
			if(gAutoTest)
				{
				QuickFormat();
				}
			TestMainWithDLLL(aSwitches);
			Cleanup();
			}
		else
			{
			if(gAutoTest)
				test.Printf(_L("Test only runs on \"FAT\" File Systems !!!\n"));
			if(gIOTesting)
				test.Printf(_L("Test only runs on \"FAT\" and \"Win32\" File Systems !!!\n"));
			}
		}
	else
		{
		test.Printf(_L("Drive %C: is not ready!\n"), 'A'+CurrentDrive());
		test(EFalse);
		}
#else
	test.Printf(_L("Test only runs on DEBUG builds, see test logs of debug builds for details.\n"));
#endif  // _DEBUG) || _DEBUG_RELEASE
	test.End();
	}

LOCAL_C void PushLotsL()
//
// Expand the cleanup stack
//
	{
	TInt i;
	for(i=0;i<1000;i++)
		CleanupStack::PushL((CBase*)NULL);
	CleanupStack::Pop(1000);
	}


LOCAL_C void DoTests(TInt aDrive, TTestSwitches& aSwitches)
//
// Do testing on aDrive
//
	{
	gSessionPath=_L("?:\\F32-TST\\");
	TChar driveLetter;
	TInt r=TheFs.DriveToChar(aDrive,driveLetter);
	test_KErrNone(r);
	gSessionPath[0]=(TText)driveLetter;
	r=TheFs.SetSessionPath(gSessionPath);
	test_KErrNone(r);
	test.Printf(_L("gSessionPath = \"%S\"\n"), &gSessionPath);

// !!! Disable platform security tests until we get the new APIs
//	if(User::Capability() & KCapabilityRoot)
		CheckMountLFFS(TheFs,driveLetter);
	
	User::After(1000000);

//	Format(CurrentDrive());

	r=TheFs.MkDirAll(gSessionPath);
	if(r == KErrCorrupt)
		{
		Format(aDrive);
		r=TheFs.MkDirAll(gSessionPath);
		test_KErrNone(r);
		}
	if (r!=KErrNone && r!=KErrAlreadyExists)
		{
		test.Printf(_L("MkDirAll() r %d\n"),r);
		test(EFalse);
		}
	TheFs.ResourceCountMarkStart();
	TRAP(r,CallTestsL(aSwitches));
	if (r==KErrNone)
		TheFs.ResourceCountMarkEnd();
	else
		{
		test.Printf(_L("Error: Leave %d\n"),r);
		test(EFalse);
		}

	CheckDisk();
	TestingLFFS(EFalse);
	}

	
GLDEF_C TInt E32Main()
//
// Test with drive nearly full
//
    {
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();
	TRAPD(r,PushLotsL());
	__UHEAP_MARK;

	test.Title();
	test.Start(_L("Starting tests..."));

	gSessionPath=_L("?:\\F32-TST\\");
	TTestSwitches testSwitches;
	ParseCommandArguments(testSwitches);

	r=TheFs.Connect();
	test_KErrNone(r);
	TheFs.SetAllocFailure(gAllocFailOn);

	TTime timerC;
	timerC.HomeTime();

	TInt theDrive;
	r=TheFs.CharToDrive(gDriveToTest,theDrive);
	test_KErrNone(r);
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TPckgBuf<TIOCacheValues> pkgOrgValues;
	TIOCacheValues& orgValues=pkgOrgValues();
	r = controlIo(TheFs,theDrive, KControlIoCacheCount, orgValues);
	test_KErrNone(r);

	test.Printf(_L("\n"));
	test.Printf(_L("Requests on close queue at start=%d\n"),orgValues.iCloseCount);
	test.Printf(_L("Requests on free queue at start=%d\n"),orgValues.iFreeCount);
	test.Printf(_L("Requests dynamically allocated at start=%d\n"),orgValues.iAllocated);
	test.Printf(_L("Requests in total at start=%d\n"),orgValues.iTotalCount);
#endif

	DoTests(theDrive, testSwitches);

	TTime endTimeC;
	endTimeC.HomeTime();
	TTimeIntervalSeconds timeTakenC;
	r=endTimeC.SecondsFrom(timerC,timeTakenC);
	test_KErrNone(r);

	test.Printf(_L("Time taken for test = %d seconds\n"),timeTakenC.Int());
	TheFs.SetAllocFailure(gAllocFailOff);
	
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	TPckgBuf<TIOCacheValues> pkgValues;
	TIOCacheValues& values=pkgValues();
	r = controlIo(TheFs,theDrive, KControlIoCacheCount, values);
	test_KErrNone(r);
	
	test.Printf(_L("Requests on close queue at end=%d\n"),values.iCloseCount);
	test.Printf(_L("Requests on free queue at end=%d\n"),values.iFreeCount);
	test.Printf(_L("Requests dynamically allocated at end=%d\n"),values.iAllocated);
	test.Printf(_L("Requests in total at end=%d\n"),values.iTotalCount);
	
	test(orgValues.iCloseCount==values.iCloseCount);
	test(orgValues.iAllocated == values.iAllocated);
#endif

	TheFs.Close();
	test.End();
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return(KErrNone);
    }
