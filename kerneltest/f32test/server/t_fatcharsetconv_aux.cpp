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
// f32test\server\t_fatcharsetconv_aux.cpp
// 
//

#include <f32dbg.h>
#include "t_fatcharsetconv_aux.h"

CFileMan* gFileMan = NULL;
RPointerArray<RFile>* gFileHandles = NULL;
TTestLogFailureData gLogFailureData;
RRawDisk TheDisk;
TFatBootSector gBootSector;
TBool gIOTesting;
TBool gAutoTest; // is BTB test

RFs TheFs;
RFile TheFile;
RDir TheDir;

TFileName gSessionPath;
TInt gAllocFailOff=KAllocFailureOff;
TInt gAllocFailOn=KAllocFailureOff;
TChar gDriveToTest;
TFileName gFileName;

TTCType	gTCType;
TUint	gTCId;

GLDEF_C void Format(TInt aDrive)
//
// Format current drive
//
	{
	test.Next(_L("Format"));
	TBuf<4> driveBuf=_L("?:\\");
	driveBuf[0]=(TText)(aDrive+'A');
	RFormat format;
	TInt count;
	TInt r=format.Open(TheFs,driveBuf,EQuickFormat,count);
	test_KErrNone(r);
	while(count)
		{
		TInt r=format.Next(count);
		test_KErrNone(r);
		}
	format.Close();
	}

void MakeDir(const TDesC& aDirName)
//
// Make a directory
//
	{
	TInt r=TheFs.MkDirAll(aDirName);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	}


void ReportCheckDiskFailure(TInt aRet)
//
// Report the failure of checkdisk
//
	{
	test.Printf(_L("CHECKDISK FAILED: "));
	switch(aRet)
		{
	case 1:	test.Printf(_L("File cluster chain contains a bad value (<2 or >maxCluster)\n")); break;
	case 2:	test.Printf(_L("Two files are linked to the same cluster\n")); break;
	case 3:	test.Printf(_L("Unallocated cluster contains a value != 0\n"));	break;
	case 4:	test.Printf(_L("Size of file != number of clusters in chain\n")); break;
	default: test.Printf(_L("Undefined Error value %d\n"),aRet);
		}
	test(EFalse);
	}

void CreateTestDirectory(const TDesC& aSessionPath)
//
// Create directory for test
//
	{
	TParsePtrC path(aSessionPath);
	test(path.DrivePresent()==EFalse);
	TInt r=TheFs.SetSessionPath(aSessionPath);
	test_KErrNone(r);
	r=TheFs.SessionPath(gSessionPath);
	test_KErrNone(r);
	r=TheFs.MkDirAll(gSessionPath);
	test_Value(r, r == KErrNone || r==KErrAlreadyExists);
	}

TInt CurrentDrive(TChar aDriveChar)
//
// Return the current drive number
//
	{
	TInt driveNum;
	TInt r = TheFs.CharToDrive(aDriveChar,driveNum);
	test_KErrNone(r);
	gDriveToTest = gSessionPath[0] = (TText)aDriveChar;
	return(driveNum);
	}

TInt CurrentDrive()
//
// Return the current drive number
//
	{
	TInt driveNum;
	TInt r = TheFs.CharToDrive(gSessionPath[0],driveNum);
	test_KErrNone(r);
	return(driveNum);
	}

void MakeFile(const TDesC& aFileName,const TUidType& aUidType,const TDesC8& aFileContents)
//
// Make a file and write uid and data
//
	{
	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		test_KErrNone(r);
		}
	TCheckedUid checkedUid(aUidType);
	TPtrC8 uidData((TUint8*)&checkedUid,sizeof(TCheckedUid));
	r=file.Write(uidData);
	test_KErrNone(r);
	r=file.Write(aFileContents);
	test_KErrNone(r);
	file.Close();
	}

void MakeFile(const TDesC& aFileName,const TDesC8& aFileContents)
//
// Make a file and write something in it
//
	{
	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	if(r!=KErrNone && r!=KErrPathNotFound)
		{
		test.Printf(_L("ERROR: r=%d"),r);
		test(EFalse);
		}
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		test_KErrNone(r);
		}
	r=file.Write(aFileContents);
	test_KErrNone(r);
	file.Close();
	}

void MakeFile(const TDesC& aFileName,TInt anAttributes)
//
// Make a file and write something in it
//
	{
	RFile file;
	TInt r=file.Replace(TheFs,aFileName,0);
	test_Value(r, r == KErrNone || r==KErrPathNotFound);
	if (r==KErrPathNotFound)
		{
		r=TheFs.MkDirAll(aFileName);
		test_KErrNone(r);
		r=file.Replace(TheFs,aFileName,0);
		test_KErrNone(r);
		}
	file.Close();
	r=TheFs.SetAtt(aFileName,anAttributes,0);
	test_KErrNone(r);
	}

void MakeFile(const TDesC& aFileName)
//
// Make a file
//
	{
	MakeFile(aFileName,_L8(""));
	}

#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
void QuickFormat()
    {
    FormatFatDrive(TheFs, CurrentDrive(), ETrue);
    }

void ReadBootSector(TFatBootSector& aBootSector)
	{
    TInt r = ReadBootSector(TheFs, CurrentDrive(), KBootSectorNum<<KDefaultSectorLog2, aBootSector);
    test_KErrNone(r);

    if(!aBootSector.IsValid())
        {
        test.Printf(_L("Wrong bootsector! Dump:\n"));
        aBootSector.PrintDebugInfo();
        test(0);
        }
	}

void GetBootInfo()
	{
	QuickFormat();
	ReadBootSector(gBootSector);
	}
#endif

void CheckIfIOTesting(TTestSwitches& aSwitches)
	{
	if( ((aSwitches.iExeOnSymbian || aSwitches.iExeOnWindows) && !(aSwitches.iVerOnSymbian || aSwitches.iVerOnWindows))
	  ||((aSwitches.iVerOnSymbian || aSwitches.iVerOnWindows) && !(aSwitches.iExeOnSymbian || aSwitches.iExeOnWindows))
	  )
		{
		gIOTesting = ETrue;
		}
	}

void CheckDisk()
	{
	TInt r=TheFs.CheckDisk(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported)
		ReportCheckDiskFailure(r);
	r = TheFs.ScanDrive(gSessionPath);
	if (r!=KErrNone && r!=KErrNotSupported && r!=KErrInUse)
		ReportCheckDiskFailure(r);
	}

void InitLogData()
	{
	gLogFailureData.iExeOsName = KNone;
	gLogFailureData.iVerOsName = KNone;
	gLogFailureData.iExeDrive  = 'A';
	gLogFailureData.iVerDrive  = 'A';
	}

void ClearTCLogData()
	{
	gLogFailureData.iTCTypeName.SetLength(0);
	gLogFailureData.iTCFailureOn.SetLength(0);
	gLogFailureData.iTCId = 0;
	gLogFailureData.iTCUniquePath.SetLength(0);
	gLogFailureData.iAPIName.SetLength(0);
	gLogFailureData.iLineNum = 0;
	// gLogFailureData.iFileName.SetLength(0);
	}

/*
 * Search test cases by the index of the array of test case group, overloaded version for basic unitary cases.
 * @param 	aIdx		the test case index in search
 * @param	aBasicUnitaryTestCaseGroup		the input test group, should always be gBasicUnitaryTestCases[]
 * @param	aTestCaseFound		contains params of the test case found by the test case Id.
 * @return	KErrNone	if only one test case on the id is found
 * 			KErrNotFound	if no test case is found
 */
TInt SearchTestCaseByArrayIdx(TUint aIdx, const TTestCaseUnitaryBasic aBasicUnitaryTestCaseGroup[],
					 TTestParamAll& aTestCaseFound, TBool aIsWithDLL=EFalse)
	{
	if (aBasicUnitaryTestCaseGroup[aIdx].iBasic.iTestCaseID != 0)
		{
		aTestCaseFound.iTestCaseID 	= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iTestCaseID;
		aTestCaseFound.iAPI 		= aBasicUnitaryTestCaseGroup[aIdx].iBasic.iAPI;

		aTestCaseFound.iSrcDrvChar	= aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iDrvChar;
		aTestCaseFound.iSrcCmdPath.Copy(aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmdPath);
		aTestCaseFound.iSrcPrsPath.Copy(aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsPath);
		aTestCaseFound.iSrcPrsFiles = aBasicUnitaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsFiles;

		aTestCaseFound.iIsWithDLL = aIsWithDLL;

		// To make every test case uniquely indentified for interOP
		// update the test case id  and its path correspondingly
		TBuf<15> tempName = _L("_");
		if(aTestCaseFound.iIsWithDLL)
			tempName.Append(_L("DLL"));
		else
			tempName.Append(_L("NDLL"));

		TInt idx = aTestCaseFound.iSrcCmdPath.Find(_L("\\Src\\"));
		aTestCaseFound.iSrcCmdPath.Insert(idx, tempName);
		aTestCaseFound.iSrcPrsPath.Insert(idx, tempName);
		}
	else
		return KErrNotFound;

	return KErrNone;
	}
/*
 * Search test cases by the index of the array of test case group, overloaded version for basic binary cases.
 * @param 	aIdx		the test case index in search
 * @param	aBasicUnitaryTestCaseGroup		the input test group, should always be gBasicBinaryTestCases[]
 * @param	aTestCaseFound		contains params of the test case found by the test case Id.
 * @return	KErrNone	if only one test case on the id is found
 * 			KErrNotFound	if no test case is found
 */
TInt SearchTestCaseByArrayIdx(TUint aIdx, const TTestCaseBinaryBasic aBasicBinaryTestCaseGroup[],
					TTestParamAll& aTestCaseFound, TBool aIsWithDLL=EFalse)
	{
	if (aBasicBinaryTestCaseGroup[aIdx].iBasic.iTestCaseID != 0)
		{
		aTestCaseFound.iTestCaseID 	= aBasicBinaryTestCaseGroup[aIdx].iBasic.iTestCaseID;
		aTestCaseFound.iAPI 		= aBasicBinaryTestCaseGroup[aIdx].iBasic.iAPI;
	
		aTestCaseFound.iSrcDrvChar	= aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iDrvChar;
		aTestCaseFound.iSrcCmdPath.Copy(aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iCmdPath);
		aTestCaseFound.iSrcPrsPath.Copy(aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsPath);
		aTestCaseFound.iSrcPrsFiles = aBasicBinaryTestCaseGroup[aIdx].iSrcPrsBasic.iPrsFiles;

		aTestCaseFound.iTrgDrvChar	= aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iDrvChar;
		aTestCaseFound.iTrgCmdPath.Copy(aBasicBinaryTestCaseGroup[aIdx].iTrgPrsBasic.iCmdPath);

		aTestCaseFound.iIsWithDLL = aIsWithDLL;
		// To make every test case uniquely indentified for interOP
		// update the test case id  and its path correspondingly
		TBuf<15> tempBuf;
		TInt idx = aTestCaseFound.iSrcCmdPath.Find(_L("\\T_FCSC\\")) + 8 /* Len of \\T_FCSC\\ */;
		TInt i = 0;
		while(aTestCaseFound.iSrcCmdPath[idx] != '\\')
			{
			tempBuf.SetLength(i+1);
			tempBuf[i++] = aTestCaseFound.iSrcCmdPath[idx++]; 
			}
		tempBuf.Append(_L("_"));
		if(aTestCaseFound.iIsWithDLL)
			tempBuf.Append(_L("DLL"));
		else
			tempBuf.Append(_L("NDLL"));
		
		TInt len = 0;
		idx = aTestCaseFound.iSrcCmdPath.Find(_L("\\T_FCSC\\")) + 8;
		while(aTestCaseFound.iSrcCmdPath[idx] != '\\')
			{
			len++;
			aTestCaseFound.iSrcCmdPath[idx++]; 
			}
		aTestCaseFound.iSrcCmdPath.Replace(idx-len, len, tempBuf);

		len =0;
		idx = aTestCaseFound.iSrcPrsPath.Find(_L("\\T_FCSC\\")) + 8;
		while(aTestCaseFound.iSrcPrsPath[idx] != '\\')
			{
			len++;
			aTestCaseFound.iSrcCmdPath[idx++]; 
			}
		aTestCaseFound.iSrcPrsPath.Replace(idx-len, len, tempBuf);

		len =0;
		idx = aTestCaseFound.iTrgCmdPath.Find(_L("\\T_FCSC\\")) + 8;
		while(aTestCaseFound.iTrgCmdPath[idx] != '\\')
			{
			len++;
			aTestCaseFound.iTrgCmdPath[idx++]; 
			}
		aTestCaseFound.iTrgCmdPath.Replace(idx-len, len, tempBuf);
		}
	else
		{
		return KErrNotFound;
		}
	return KErrNone;
	}

void Help()
	{
	RDebug::Print(_L("t_fatcharsetconv [-x {s,w}] [-d {dt}][-v {sw}]"));
	RDebug::Print(_L("\t-x :	for executing tests"));
	RDebug::Print(_L("\t\t	s or S for execution on Symbian"));
	RDebug::Print(_L("\t\t	w or W for execution on Windows"));
	RDebug::Print(_L("\t-d:		drive to test for execution/verification"));
	RDebug::Print(_L("\t\t	This test runs on FAT or Win32 file systems only"));
	RDebug::Print(_L("\t\t	d or D for test drive on Symbian (FAT file system)"));
	RDebug::Print(_L("\t\t	Any FAT/Win32 file system drive on emulator i.e. T, X etc"));
	RDebug::Print(_L("\t-v :	for validating tests"));
	RDebug::Print(_L("\t\t	s or S for verification on symbian"));
	RDebug::Print(_L("\t\t	w or W for verification on Windows"));
	}

void ClearSwitches(TTestSwitches& aSwitches)
	{
	// clear the switches
	aSwitches.iExeOnSymbian = EFalse;
	aSwitches.iVerOnSymbian = EFalse;
	aSwitches.iExeOnWindows = EFalse;
	aSwitches.iVerOnWindows = EFalse;
	aSwitches.iExeDriveChar =' ';
	aSwitches.iVerDriveChar =' ';
	aSwitches.iExeDriveNum =0;
	aSwitches.iVerDriveNum =0;
	aSwitches.iMountedFSName.SetLength(0);
	}


void ParseCommandArguments(TTestSwitches& aSwitches)
	{
	TBuf<256> cmd;
	User::CommandLine(cmd);
	RDebug::Print(_L("Command Line : %S"), &cmd);
	TChar testDrive = 'C';
		
	InitLogData();
	ClearSwitches(aSwitches);

	TFileName currentFile=RProcess().FileName();
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	if (token.MatchF(currentFile)==0)
		{
		token.Set(lex.NextToken());
		}
	if (token.Length()==0)
		{
		testDrive = 'C'; // default drives
		gAutoTest = ETrue;
		}
	else if((token[0] >='A' && token[0]<='Z') || (token[0] >='a' && token[0]<='z'))
		{
		testDrive = token[0];
		gAutoTest = ETrue;
		}
	else
		{
		while (!lex.Eos())
			{
			if (token.Length()==0)
				{
				continue;	// ignore trailing whitespace
				}
			if (token==_L("-x") || token==_L("-X"))
				{
				token.Set(lex.NextToken());
				if((token==_L("s")) || (token==_L("S")))
					{
					aSwitches.iExeOnSymbian = ETrue;
					gLogFailureData.iExeOsName = KSymbian;
					}
				else if((token==_L("w")) || (token==_L("W")))
					{
					aSwitches.iExeOnWindows = ETrue;
					gLogFailureData.iExeOsName = KWindows;
					}
				token.Set(lex.NextToken());
				continue;
				}
			if (token==_L("-d") || token==_L("-D"))
				{
				token.Set(lex.NextToken());
				testDrive = token[0];
				token.Set(lex.NextToken());
				continue;
				}
			if (token==_L("-v") || token==_L("-V"))
				{
				token.Set(lex.NextToken());
				if((token==_L("s")) || (token==_L("S")))
					{
					aSwitches.iVerOnSymbian = ETrue;
					gLogFailureData.iVerOsName = KSymbian;
					}
				else if((token==_L("w")) || (token==_L("W")))
					{
					aSwitches.iVerOnWindows = ETrue;
					gLogFailureData.iVerOsName = KWindows;
					}
				token.Set(lex.NextToken());
				continue;
				}
			RDebug::Print(_L("Unknown option %S"), &token);
			Help();
			return;
			}
		}

	CheckIfIOTesting(aSwitches);

	if(gIOTesting)
		{
		gAutoTest = EFalse;
		}
	else
		{
		gAutoTest = ETrue;
		}

	testDrive.UpperCase();
	if (gAutoTest)
		{
#if defined (__WINS__)
		//execution phase
		aSwitches.iExeOnWindows = ETrue;
		aSwitches.iExeDriveChar = testDrive;
		aSwitches.iExeDriveNum = CurrentDrive(aSwitches.iExeDriveChar);
		gLogFailureData.iExeDrive = aSwitches.iExeDriveChar;
		gLogFailureData.iExeOsName = KWindows;
		//verification phase
		aSwitches.iVerOnWindows = ETrue;
		aSwitches.iVerDriveChar = testDrive;
		aSwitches.iVerDriveNum = CurrentDrive(aSwitches.iVerDriveChar);
		gLogFailureData.iVerDrive = aSwitches.iVerDriveChar;
		gLogFailureData.iVerOsName = KWindows;
#else	
		//execution phase
		aSwitches.iExeOnSymbian = ETrue;
		aSwitches.iExeDriveChar = testDrive;
		aSwitches.iExeDriveNum = CurrentDrive(aSwitches.iExeDriveChar);
		gLogFailureData.iExeDrive = aSwitches.iExeDriveChar;
		gLogFailureData.iExeOsName = KSymbian;
		//verification phase	
		aSwitches.iVerOnSymbian = ETrue;
		aSwitches.iVerDriveChar = testDrive;
		aSwitches.iVerDriveNum = CurrentDrive(aSwitches.iVerDriveChar);
		gLogFailureData.iVerDrive = aSwitches.iVerDriveChar;
		gLogFailureData.iVerOsName = KSymbian;
#endif
		}

	if(aSwitches.iExeOnWindows || aSwitches.iExeOnSymbian)
		{
		aSwitches.iExeDriveChar = testDrive;
		aSwitches.iExeDriveNum = CurrentDrive(aSwitches.iExeDriveChar);
		gLogFailureData.iExeDrive = aSwitches.iExeDriveChar;
		}
	if(aSwitches.iVerOnWindows || aSwitches.iVerOnSymbian)
		{
		aSwitches.iVerDriveChar = testDrive;
		aSwitches.iVerDriveNum = CurrentDrive(aSwitches.iVerDriveChar);
		gLogFailureData.iVerDrive = aSwitches.iVerDriveChar;
		}
	}

void InitialiseL()
	{
	gFileMan=CFileMan::NewL(TheFs);
	}

void RmDir(const TDesC& aDirName)
	{
	TFileName filename_dir = aDirName;
	TInt r = 0;
	r = TheFs.SetAtt(filename_dir, 0, KEntryAttReadOnly);
	test_KErrNone(r);
	r=gFileMan->RmDir(filename_dir);
	test_Value(r, r == KErrNone || r==KErrNotFound || r==KErrPathNotFound || r==KErrInUse);
	}

// Cleanup test variables
void Cleanup()
	{
	delete gFileMan;
	}

/**
    Parsing Dir Data Block
    @param  aDataBlock		data block in TInt[] for parsing	
    @param  aDirDataArray	returning dir data array after parsing

    @panic 					if data setup error
*/
void ParsingDirDataBlock(const TInt aDataBlock[], RArray<TInt>& aDirDataArray)
	{
	TInt err = KErrNone;
	aDirDataArray.Reset();

	if (aDataBlock[0] == EOB)
		{
		return;
		}

	TInt i = 1;
	FOREVER
		{
		TInt lastItem = aDataBlock[i-1];
		TInt currentItem = aDataBlock[i];
		
		// check currentItem
		if (currentItem == EOB)
			{
			if (lastItem == CON || lastItem > LAST)
			//check last
				{
				test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
				test(EFalse);
				}
			else
			// passed, insert last, break
				{
				err = aDirDataArray.InsertInOrder(lastItem);
				if (err != KErrNone && err != KErrAlreadyExists)
					{
					test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
					test(EFalse);
					}
				break;
				}
			}
		else if (currentItem == CON)
		// if current == CON
			{
			if (lastItem == CON || lastItem >= LAST)
			// check last item
				{
				test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
				test(EFalse);
				}
			else // last < LAST, last != CON
				{
				// check next item
				TInt nextItem = aDataBlock[i+1];
				if (nextItem <= 0 || nextItem > LAST || lastItem >= nextItem)
					{
					test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
					test(EFalse);
					}
				else
					{
					// all valid
					for (TInt j = lastItem; j < nextItem; j++)
						{
						err = aDirDataArray.InsertInOrder(j);
						if (err != KErrNone && err != KErrAlreadyExists)
							{
							test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
							test(EFalse);
							}
						}
					}
				}
			i++;
			}
		else if (0 <= currentItem && currentItem <= LAST)
		// if current == normal item
			{
			if (lastItem == CON)
				{
				i++;
				continue;
				}
			else if (lastItem >= LAST)
			// check last item
				{
				test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
				test(EFalse);
				}
			else
			// passed, insert last
				{
				err = aDirDataArray.InsertInOrder(lastItem);
				if (err != KErrNone && err != KErrAlreadyExists)
					{
					test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
					test(EFalse);
					}
				}
			i++;
			}
			else	// invalid input
			{
			test.Printf(_L("ERROR<SetupDir>: wrong dir data setup! err=%d\n"), err);
			test(EFalse);
			}
		}
	}

/**
    Setup dir structure for testing and verifying functional results
    @param	datastr			data structure to setup directory
    @param  iOperation   	Operation to be performed 
    @param  SrcDrive		Source drive
    @param	Targetdrive		Target drive input
    @panic					if data structure definition is incorrect
*/
void SetupDirFiles(const TDesC& aPath, const TDirSetupFiles& aDirFiles)
	{
	TFileName path = aPath;
	if (path.Length() == 0)
		{
		test.Printf(_L("ERROR<SetupDirFiles()>: Zero length src path!\n"));
		test(EFalse);
		}
	
	MakeDir(path);
	
	RArray<TInt> addBlockDataArray;
	RArray<TInt> deductBlockDataArray;
	
	ParsingDirDataBlock(aDirFiles.iAddingBlock, addBlockDataArray);
	ParsingDirDataBlock(aDirFiles.iDeductBlock, deductBlockDataArray);
	
	if (addBlockDataArray.Count() == 0)
	// empty dir setup
		{
		return;
		}
	for (TInt i = 0; i < deductBlockDataArray.Count(); ++i)
		{
		TInt idxToDelete = addBlockDataArray.FindInOrder(deductBlockDataArray[i]);
		if (idxToDelete >= 0)
			{
			addBlockDataArray.Remove(idxToDelete);
			}
		else if (idxToDelete == KErrNotFound)
			{
			continue;
			}
		else
			{
			test.Printf(_L("ERROR<<SetupDir>>: wrong dir data setup! err=%d\n"), idxToDelete);
			test(EFalse);
			}
		}
	if (addBlockDataArray.Count() > 0)
		{
		for (TInt i = 0; i < addBlockDataArray.Count(); ++i)
			{
			TInt idx = addBlockDataArray[i];
			path = aPath;
			path += gDirPatterns[idx];
			if (path[path.Length() - 1] == '\\')
				{
				MakeDir(path);
				}
			else
				{
				MakeFile(path, _L8("blahblah"));
				}
			}
		}
	
	addBlockDataArray.Reset();
	deductBlockDataArray.Reset();
	}

void LogTestFailureData(TTCType tcType, TFileName failedOn, 
					TUint tcId, TFileName tcUniquePath, TInt line)
	{
	if(tcType == EUnitaryTest)
		gLogFailureData.iTCTypeName = KUnitary;
	else if(tcType == EBinaryTest)
		gLogFailureData.iTCTypeName = KBinary;
	gLogFailureData.iTCFailureOn = failedOn;
	gLogFailureData.iTCId = tcId;
	gLogFailureData.iTCUniquePath.Copy(tcUniquePath);
	gLogFailureData.iLineNum = line;
	gLogFailureData.iFileName.Copy(gFileName);

	RFile file;
	TBuf8<256>	tempBuf;

	TFileName logFileName;
	if(gIOTesting)
		{
		if(failedOn == KExecution)
			{
			logFileName.Append(KExeLogFileName);
			}
		else
			{
			logFileName.Append(KVerLogFileName);
			}
		}
	else
		{
		logFileName.Append(KLogFileName);
		}

	logFileName.Append(KUnderScore);
	logFileName.Append(gDriveToTest);
	logFileName.Append(KExtension);

	TInt r = file.Create(TheFs, logFileName, EFileRead|EFileWrite);
	test_Value(r, r == KErrNone || r == KErrAlreadyExists);

	if (r == KErrNone)
		{
		tempBuf.Append(KLogFileHeader);
		file.Write(tempBuf);
		}

	if (r == KErrAlreadyExists)
		{
		r = file.Open(TheFs, logFileName, EFileRead|EFileWrite);
		test_KErrNone(r);
		TInt start = 0;
		r=file.Seek(ESeekEnd,start);
		test_KErrNone(r);
		}
	
	tempBuf.SetLength(0);
	tempBuf.Append(KNewLine);
	if(gIOTesting)
		tempBuf.Append(KYes);
	else
		tempBuf.Append(KNo);
	tempBuf.Append(KComma);
	file.Write(tempBuf);

	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iTCTypeName);
	tempBuf.Append(KComma);
	file.Write(tempBuf);

	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iTCFailureOn);
	tempBuf.Append(KComma);
	file.Write(tempBuf);

	tempBuf.SetLength(0);
	tempBuf.AppendNum(gLogFailureData.iTCId);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
		
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iTCUniquePath);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
			
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iFSName);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
	
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iExeOsName);
	tempBuf.Append(KComma);
	file.Write(tempBuf);

	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iExeDrive);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
	
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iVerOsName);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
			
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iVerDrive);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
			
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iAPIName);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
	
	tempBuf.SetLength(0);
	tempBuf.AppendNum(gLogFailureData.iLineNum);
	tempBuf.Append(KComma);
	file.Write(tempBuf);
	
	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iFuncName);
	tempBuf.Append(KComma);
	file.Write(tempBuf);

	tempBuf.SetLength(0);
	tempBuf.Append(gLogFailureData.iFileName);
	file.Write(tempBuf);

	file.Close();
	}
