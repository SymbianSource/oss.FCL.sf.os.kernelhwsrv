// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// File Name:		f32test/bench/t_fat_perf.cpp
// This file contains implementation for the functions called 
// by test cases from the command line to test FAT Performance on
// large number of files (PREQ 1885).
// 
//

//Include files
#include <e32test.h>
#include <f32file.h>
#include <f32dbg.h>
#include "t_server.h"
#include "t_fat_perf.h"



LOCAL_C void ClearCache(TInt); 

RTest test(_L("T_FAT_PERF"));

TFileName 	gPath;
TFileName 	gFileNameBase;
TInt 		gFileNo = 0;
TInt		gTestCase = -1;
TInt		gCacheClear = 0;
TInt        gZeroPadFileNumberForFixedLengthFileNames = 0;


//-----------------------------------------------------------------------------

TNamingSchemeParam::TNamingSchemeParam() 
    : iConOrRan(EConsecutive), iUniOrAsc(EAscii), iZeroPadFileNumber(EFalse), iMaxFileNameLength(0), iMinStringLength(0)
    {
    iFileNameBase = _L("");
    }

//-----------------------------------------------------------------------------
/**
  	This function creates the files in the following directories
  	INITIAL CONDITION: FILL FOLLOWING SUB-DIRS WITH 1600 FILES
	"\\DIR1\\"...2000 files ("ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1 ~ 2000.TXT")
	"\\DIR1\\DIR11\\"...1600 files ("ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1 ~ 1600.TXT")
	"\\DIR1\\DIR11\\DIR111\\"...1610 files ("ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1 ~ 1600.TXT") + "ANOTHERLONGFILENAME_1~10.TXT"
	"\\DIR1\\DIR12RAN\\"...1600 files ("...random string (31~34)....TXT")
	"\\DIR2\\"...1600 files ("ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_1 ~ 1600.TXT")
 */
//-----------------------------------------------------------------------------

LOCAL_C void DoTestCaseSetup()
	{
	test.Next(_L(" 'Test Setup' - 00"));

	TMLUnitParam mlParam = 
		{
		12345	// iID
		};
	CMeasureAndLogUnit* measureAndLogUnit = CMeasureAndLogUnit::NewLC(mlParam);
	CExecutionUnit* execUnit = CExecutionUnit::NewLC(measureAndLogUnit, gDriveToTest);

	TInt curDriveNum;
	TInt err = TheFs.CharToDrive(gDriveToTest, curDriveNum);
	test(KErrNone == err);
	FormatFatDrive(TheFs,curDriveNum, EQuickFormat, NULL, EFalse);

/*-----------------------------------------------------------------------------
 * Setup for "\\DIR1\\"
 *-----------------------------------------------------------------------------*/

   	TNamingSchemeParam namingScheme;
	namingScheme.iConOrRan 			       = EConsecutive;
	namingScheme.iUniOrAsc 			       = EAscii;
	namingScheme.iZeroPadFileNumber   = EFalse;
	namingScheme.iFileNameBase 		       = _L("ABCD1ABCD2ABCD3ABCD4ABCD5ABCD_");
	namingScheme.iMaxFileNameLength        = 0;
	namingScheme.iMinStringLength 	       = 0;
	
	TDirUnitParam dirParam1;
	dirParam1.iPriority 			= 1;
	dirParam1.iDirName 				= _L("?:\\DIR1\\");
	dirParam1.iRuns 				= 1;
	dirParam1.iFilesPerRun 			= 2000;
	dirParam1.iSampleInterval 		= 0;
	dirParam1.iNamingScheme 		= namingScheme;
	dirParam1.iFileOpMode 			= EFATPerfFileCreate;
	
	execUnit->AddDirUnitL(dirParam1);
	
/*-----------------------------------------------------------------------------
 * Setup for "\\DIR1\\DIR11\\"
 *-----------------------------------------------------------------------------*/

	TDirUnitParam dirParam11		= dirParam1;
	dirParam11.iDirName 			= _L("?:\\DIR1\\DIR11\\");
	dirParam11.iFilesPerRun			= 1600;
	dirParam11.iSampleInterval		= 0;

	execUnit->AddDirUnitL(dirParam11);
	
/*-----------------------------------------------------------------------------
 * Setup for "\\DIR1\\DIR11\\DIR111\\"
 *-----------------------------------------------------------------------------*/

	TDirUnitParam dirParam111		= dirParam1;
	dirParam111.iDirName 			= 	_L("?:\\DIR1\\DIR11\\DIR111\\");
	dirParam111.iFilesPerRun		= 1600;
	dirParam111.iSampleInterval		= 0;

	execUnit->AddDirUnitL(dirParam111);

/*-----------------------------------------------------------------------------
 * Test setup for "\\DIR1\\DIR11\\DIR111\\" with "ANOTHERLONGFILENAME_" file
 *-----------------------------------------------------------------------------*/

	TNamingSchemeParam namingScheme111A = namingScheme;
	namingScheme111A.iFileNameBase 		= _L("ANOTHERLONGFILENAME_");

	TDirUnitParam dirParam111A		= dirParam1;
	dirParam111A.iDirName 			= 	_L("?:\\DIR1\\DIR11\\DIR111\\");
	dirParam111A.iFilesPerRun		= 10;
	dirParam111A.iSampleInterval	= 0;
	dirParam111A.iNamingScheme 		= namingScheme111A;
	
	execUnit->AddDirUnitL(dirParam111A);

/*-----------------------------------------------------------------------------
 * Setup for "\\DIR1\\DIR12RAN\\"
 *-----------------------------------------------------------------------------*/

	TNamingSchemeParam namingScheme12 = namingScheme;
	namingScheme12.iConOrRan 			   = ERandom;
	namingScheme12.iZeroPadFileNumber = EFalse;
	namingScheme12.iFileNameBase 		   = _L("");
	namingScheme12.iMaxFileNameLength 	   = 34;
	namingScheme12.iMinStringLength 	   = 31;

	TDirUnitParam dirParam12		= dirParam1;
	dirParam12.iDirName 			= _L("?:\\DIR1\\DIR12RAN\\");
	dirParam12.iFilesPerRun			= 1600;
	dirParam12.iSampleInterval		= 0;
	dirParam12.iNamingScheme 		= namingScheme12;
	
	execUnit->AddDirUnitL(dirParam12);

/*-----------------------------------------------------------------------------
 * Setup for "\\DIR2\\"
 *-----------------------------------------------------------------------------*/

	TDirUnitParam dirParam2		= dirParam1;
	dirParam2.iDirName 			= _L("?:\\DIR2\\");
	dirParam2.iFilesPerRun		= 1600;
	dirParam2.iSampleInterval	= 0;
	dirParam2.iNamingScheme 	= namingScheme;
	
	execUnit->AddDirUnitL(dirParam2);

	execUnit->Run();
	CleanupStack::PopAndDestroy(2);
	}

//-----------------------------------------------------------------------------
/**
This function creates a files in the valid directory path
*/
//-----------------------------------------------------------------------------
LOCAL_C void DoTestCaseCreateFile()
	{
	test.Next(_L(" 'Create File' - 01"));

	TMLUnitParam mlParam = 
		{
		12345	// iID
		};
	CMeasureAndLogUnit* measureAndLogUnit = CMeasureAndLogUnit::NewLC(mlParam);
	CExecutionUnit* execUnit = CExecutionUnit::NewLC(measureAndLogUnit, gDriveToTest);

	ClearCache(gCacheClear);
	
	TNamingSchemeParam namingScheme;
	namingScheme.iZeroPadFileNumber = 	gZeroPadFileNumberForFixedLengthFileNames;
	
	TDirUnitParam dirParam1;
	dirParam1.iPriority 			= 1;
	dirParam1.iDirName 				= _L("");
	dirParam1.iRuns 				= 1;
	dirParam1.iFilesPerRun 			= 0;
	dirParam1.iSampleInterval 		= 1;
	dirParam1.iNamingScheme 		= namingScheme;
	dirParam1.iFileOpMode 			= EFATPerfFileCreate;
	
		
	TFileName path = _L("?:\\");
	path.Append(gPath);
	if (path[path.Length() - 1] != '\\')
		{
		path.Append('\\');
		}
	
	dirParam1.iDirName = path;
	dirParam1.iFilesPerRun = gFileNo;
	dirParam1.iNamingScheme.iFileNameBase = gFileNameBase;

	execUnit->AddDirUnitL(dirParam1);
	execUnit->Run();

	CleanupStack::PopAndDestroy(2);
	
	}

//-----------------------------------------------------------------------------
/**
This function opens file in the valid directoy path
*/
//-----------------------------------------------------------------------------

LOCAL_C void DoTestCaseOpenFile()
	{
	test.Next(_L(" 'Open File' - 02"));

	TMLUnitParam mlParam = 
		{
		12345	// iID
		};
	CMeasureAndLogUnit* measureAndLogUnit = CMeasureAndLogUnit::NewLC(mlParam);
	CExecutionUnit* execUnit = CExecutionUnit::NewLC(measureAndLogUnit, gDriveToTest);

	ClearCache(gCacheClear);
	
	TNamingSchemeParam namingScheme;
	namingScheme.iZeroPadFileNumber = gZeroPadFileNumberForFixedLengthFileNames;
	
	TDirUnitParam dirParam1;
	dirParam1.iPriority 			= 1;
	dirParam1.iDirName 				= _L("");
	dirParam1.iRuns 				= 1;
	dirParam1.iFilesPerRun 			= 0;
	dirParam1.iSampleInterval 		= 1;
	dirParam1.iNamingScheme 		= namingScheme;
	dirParam1.iFileOpMode 			= EFATPerfFileOpen;
	
	
	TFileName path = _L("?:\\");
	path.Append(gPath);
	if (path[path.Length() - 1] != '\\')
		{
		path.Append('\\');
		}
	
	dirParam1.iDirName = path;
	dirParam1.iFilesPerRun = gFileNo;
	dirParam1.iNamingScheme.iFileNameBase = gFileNameBase;

	execUnit->AddDirUnitL(dirParam1);
	execUnit->Run();

	CleanupStack::PopAndDestroy(2);
	
	}

//-----------------------------------------------------------------------------
/**
This function deletes the files in the valid directory path
*/
//-----------------------------------------------------------------------------

LOCAL_C void DoTestCaseDeleteFile()
	{
	test.Next(_L(" 'Delete File' - 03"));

	TMLUnitParam mlParam = 
		{
		12345	// iID
		};
	CMeasureAndLogUnit* measureAndLogUnit = CMeasureAndLogUnit::NewLC(mlParam);
	CExecutionUnit* execUnit = CExecutionUnit::NewLC(measureAndLogUnit, gDriveToTest);

	ClearCache(gCacheClear);
	
	TNamingSchemeParam namingScheme;
	namingScheme.iZeroPadFileNumber = gZeroPadFileNumberForFixedLengthFileNames;
	
	TDirUnitParam dirParam1;
	dirParam1.iPriority 			= 1;
	dirParam1.iDirName 				= _L("");
	dirParam1.iRuns 				= 1;
	dirParam1.iFilesPerRun 			= 0;
	dirParam1.iSampleInterval 		= 1;
	dirParam1.iNamingScheme 		= namingScheme;
	dirParam1.iFileOpMode 			= EFATPerfFileDelete;
	
	TFileName path = _L("?:\\");
	path.Append(gPath);
	if (path[path.Length() - 1] != '\\')
		{
		path.Append('\\');
		}
	
	dirParam1.iDirName = path;
	dirParam1.iFilesPerRun = gFileNo;
	dirParam1.iNamingScheme.iFileNameBase = gFileNameBase;

	execUnit->AddDirUnitL(dirParam1);
	execUnit->Run();

	CleanupStack::PopAndDestroy(2);
	
	}

//-----------------------------------------------------------------------------
/**
PREQ1885  - optional test case 
This function writes data into the files in the valid directory path
*/
//-----------------------------------------------------------------------------

LOCAL_C void DoTestCaseWriteFile()
	{
	test.Next(_L(" 'Write File'  - 04 - Write 4KB of Data"));

	TMLUnitParam mlParam = 
		{
		12345	// iID
		};
	CMeasureAndLogUnit* measureAndLogUnit = CMeasureAndLogUnit::NewLC(mlParam);
	CExecutionUnit* execUnit = CExecutionUnit::NewLC(measureAndLogUnit, gDriveToTest);

	ClearCache(gCacheClear);
	
	TNamingSchemeParam namingScheme;
	namingScheme.iZeroPadFileNumber = gZeroPadFileNumberForFixedLengthFileNames;

	TDirUnitParam dirParam1;
	dirParam1.iPriority 			= 1;
	dirParam1.iDirName 				= _L("");
	dirParam1.iRuns 				= 1;
	dirParam1.iFilesPerRun 			= 0;
	dirParam1.iSampleInterval 		= 1;
	dirParam1.iNamingScheme 		= namingScheme;
	dirParam1.iFileOpMode 			= EFATPerfFileWrite;
	
	TFileName path = _L("?:\\");
	path.Append(gPath);
	if (path[path.Length() - 1] != '\\')
		{
		path.Append('\\');
		}
	
	dirParam1.iDirName = path;
	dirParam1.iFilesPerRun = gFileNo;
	dirParam1.iNamingScheme.iFileNameBase = gFileNameBase;

	execUnit->AddDirUnitL(dirParam1);
	execUnit->Run();

	CleanupStack::PopAndDestroy(2);
	
	}

//-----------------------------------------------------------------------------
/**
PREQ1885  - optional test case  
This function Reads data from the files in the valid directory path
*/
//-----------------------------------------------------------------------------

LOCAL_C void DoTestCaseReadFile()
	{
	test.Next(_L(" 'Read File' - 05 Read 4KB of data"));

	TMLUnitParam mlParam = 
		{
		12345	// iID
		};
	CMeasureAndLogUnit* measureAndLogUnit = CMeasureAndLogUnit::NewLC(mlParam);
	CExecutionUnit* execUnit = CExecutionUnit::NewLC(measureAndLogUnit, gDriveToTest);

	ClearCache(gCacheClear);
		
	TNamingSchemeParam namingScheme;
    namingScheme.iZeroPadFileNumber = gZeroPadFileNumberForFixedLengthFileNames;
    
	TDirUnitParam dirParam1;
	dirParam1.iPriority 			= 1;
	dirParam1.iDirName 				= _L("");
	dirParam1.iRuns 				= 1;
	dirParam1.iFilesPerRun 			= 0;
	dirParam1.iSampleInterval 		= 1;
	dirParam1.iNamingScheme 		= namingScheme;
	dirParam1.iFileOpMode 			= EFATPerfFileRead;
	

	TFileName path = _L("?:\\");
	path.Append(gPath);
	if (path[path.Length() - 1] != '\\')
		{
		path.Append('\\');
		}
	
	dirParam1.iDirName = path;
	dirParam1.iFilesPerRun = gFileNo;
	dirParam1.iNamingScheme.iFileNameBase = gFileNameBase;

	execUnit->AddDirUnitL(dirParam1);
	execUnit->Run();

	CleanupStack::PopAndDestroy(2);
	
	}

//-----------------------------------------------------------------------------
/**
This function Dumps the information about Directory Cache
*/
//
LOCAL_C void DoTestCaseDirCacheInfo()
	{
	ClearCache(gCacheClear);
	RDebug::Print(_L("Dumping DirCache Info - Only for DEBUG Mode \n"));
	#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
        TInt ret = TheFs.ControlIo(CurrentDrive(), ETestFATDirCacheInfo);    // For DirCache info
        if (ret != KErrNone)
            {
            RDebug::Print(_L("RFs::ControlIo() returned %d when attempting to dump cache info"), ret);
            }
        ret = TheFs.ControlIo(CurrentDrive(), ETestDumpFATDirCache);  // For Dumping DirCache contents
        if (ret != KErrNone)
            {
            RDebug::Print(_L("RFs::ControlIo() returned %d when attempting to dump cache contents"), ret);
            }
	#endif
	}

/* To clear the cache - remount drive */

LOCAL_C void ClearCache(TInt gCacheClear)
    {
    TInt rel = KErrNone;
    if (gCacheClear == 1)
        {
        // Remount the drive to clear the cache 
        rel = RemountFS (TheFs, CurrentDrive(), NULL);
        if (rel != KErrNone)
            {
            RDebug::Print(_L("<<Error>>: Remounting: %d\n"), rel);
            User::Leave(rel);
            }	
        }

    }


/* Function for command line arguments for the tests */
LOCAL_C void ParseMyCommandArguments()
	{
	TBuf<0x100> cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	TPtrC token=lex.NextToken();
	TFileName thisfile=RProcess().FileName();
	if (token.MatchF(thisfile)==0)
		{
		token.Set(lex.NextToken());
		}
	test.Printf(_L("CLP=%S"),&token);

	if(token.Length()!=0)		
		{
		gDriveToTest=token[0];
		gDriveToTest.UpperCase();
		}
	else
		{
		gDriveToTest='C';
		return;
		}

	while (!lex.Eos())
		{
		token.Set(lex.NextToken());
		if (token.Compare(_L("-c")) == 0 || token.Compare(_L("-C")) == 0)
			{
			token.Set(lex.NextToken());
			if (token.MatchF(_L("Setup"))==0)
				{
				
				gTestCase = EFATPerfSetup;
				
				}
			else if (token.MatchF(_L("Create"))==0)
				{
				
				gTestCase = EFATPerfCreate; 
				
				}
			
			else if (token.MatchF(_L("Open"))==0)
				{
				
				gTestCase = EFATPerfOpen;
				
				}
			
			else if (token.MatchF(_L("Delete"))==0)
			
				{
				
				gTestCase = EFATPerfDelete;
				
				}
			
			else if (token.MatchF(_L("Write"))==0)
				{

				gTestCase = EFATPerfWrite;
	
				}
			
			else if (token.MatchF(_L("Read"))==0)
		
				{
				
				gTestCase = EFATPerfRead;
		
				}	
			else if (token.MatchF(_L("Dumpinfo"))==0)
				{
				gTestCase = EFATPerfDirCacheInfo;
				}
			else
				{
				test.Printf(_L("Bad command syntax"));
				test(EFalse);
				}
			}
		
		if (token.Compare(_L("-p")) == 0 || token.Compare(_L("-P")) == 0)
			{
			token.Set(lex.NextToken());
			if (token.Length() != 0)
				{
				gPath = token;
				}
			else
				{
				test.Printf(_L("Bad command syntax"));
				test(EFalse);
				}
			}
		
		if (token.Compare(_L("-b")) == 0 || token.Compare(_L("-B")) == 0)
			{
			token.Set(lex.NextToken());
			if (token.Length() != 0)
				{
				gFileNameBase = token;
				}
			else
				{
				test.Printf(_L("Bad command syntax"));
				test(EFalse);
				}
			}
		
		if (token.Compare(_L("-n")) == 0 || token.Compare(_L("-N")) == 0)
			{
			token.Set(lex.NextToken());
			if (token.Length() != 0)
				{
				TLex tokenLex;
				tokenLex.Assign(token);
				TInt value;
				tokenLex.Val(value);
				gFileNo = value;
				}
			else
				{
				test.Printf(_L("Bad command syntax"));
				test(EFalse);
				}
			}
			
		if (token.Compare(_L("-m")) == 0 || token.Compare(_L("-M")) == 0)
			{
			token.Set(lex.NextToken());
			if (token.Length() != 0)
				{
				TLex tokenLex;
				tokenLex.Assign(token);
				TInt value;
				tokenLex.Val(value);
				gCacheClear = value;
				}
			else
				{
				test.Printf(_L("Bad command syntax"));
				test(EFalse);
				}
			}
		
        if (token.Compare(_L("-f")) == 0 || token.Compare(_L("-F")) == 0)
            {
            token.Set(lex.NextToken());
            if (token.Length() != 0)
                {
                TLex tokenLex;
                tokenLex.Assign(token);
                TInt value;
                tokenLex.Val(value);
                gZeroPadFileNumberForFixedLengthFileNames = value;
                }
            else
                {
                test.Printf(_L("Bad command syntax"));
                test(EFalse);
                }
            }
		}
	}


/* System Info */
LOCAL_C void GetSystemInfo()
	{
	test.Next(_L("Get System Info"));
	TFSName fsName;
	TInt drvNo = -1; 
	TInt err = TheFs.CharToDrive(gDriveToTest, drvNo);
	test(err == KErrNone);
	err = TheFs.FileSystemSubType(drvNo, fsName);
	test(err == KErrNone);
	test.Printf(_L("File System: [%S]\n"), &fsName);
	TVolumeIOParamInfo ioParam;
	err = TheFs.VolumeIOParam(drvNo, ioParam);
	test(err == KErrNone);
	test.Printf(_L("Sector Size: [%d Bytes]\n"), ioParam.iBlockSize);
	test.Printf(_L("Cluster Size: [%d Bytes]\n"), ioParam.iClusterSize);
	}

//-----------------------------------------------------------------------------
/**
Main Function to call specific test features
*/
//-----------------------------------------------------------------------------
void CallTestsL()
	{
	
	//Interpreting Command Line Arguments
	ParseMyCommandArguments(); 
	

	// Get Drive Information
	GetSystemInfo(); 
 	
 	
 	// Switch Case for Test Functions
 	switch(gTestCase)
		{
	
	    //Test Setup
		case EFATPerfSetup:    
			{
			DoTestCaseSetup();
			break;
			}
		
		//File Create
		case EFATPerfCreate:
			{
			DoTestCaseCreateFile();
			break;
			}
	  	
	  	// File Open 
		case EFATPerfOpen:
			{
			DoTestCaseOpenFile();
			break;
			}
		
		// File Delete
		case EFATPerfDelete:
			{
			DoTestCaseDeleteFile();
			break;
			}
		// File Write
		case EFATPerfWrite:
			{
			DoTestCaseWriteFile();
			break;
			}
		
		// File Read
		case EFATPerfRead:
			{
			DoTestCaseReadFile();
			break;
			}
		// Print / Dump DirCache Info
		case EFATPerfDirCacheInfo:
			{
			DoTestCaseDirCacheInfo();
			break;
			}
		
		default:
			break;
		}

	}

/*-- EOF--*/
