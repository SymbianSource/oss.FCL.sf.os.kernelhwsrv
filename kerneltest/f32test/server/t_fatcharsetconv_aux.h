// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\server\T_FatCharSetConv_Aux.h
// 
//

#ifndef T_FATCHARSETCONV_AUX_H
#define T_FATCHARSETCONV_AUX_H
#define __E32TEST_EXTENSION__
#include <f32file.h>
#include <e32test.h>
#include <hal.h>
#include "t_server.h"
#include "fat_utils.h"

using namespace Fat_Test_Utils;
class TTestLogFailureData;
extern TTestLogFailureData gLogFailureData;
extern RRawDisk TheDisk;
extern TFatBootSector gBootSector;
extern TBool gIOTesting;
extern RFs TheFs;
extern RFile TheFile;
extern RDir TheDir;
extern TFileName gSessionPath;
extern TInt gAllocFailOff;
extern TInt gAllocFailOn;
extern TChar gDriveToTest;
extern TFileName gFileName;
extern TBool gAutoTest;


enum TTCType
	{ 
	EUnitaryTest,
	EBinaryTest,
	ESymbianFATSpecific
	};

extern TTCType gTCType;
extern TUint gTCId;

// function declarations
class TTestSwitches;
class TTestParamAll;
struct TTestCaseUnitaryBasic;

// Constants
const TInt KFileName350=350;
const TInt KFileName50=50;
const TInt KFileName20=20;

// Literals
// Code Pages
_LIT(KTestLocale, "t_tlocl_cp932.dll");

//Log file specific
_LIT(KLogFileName,"C:\\Failure");
_LIT(KExeLogFileName,"C:\\ExecutionFailure");
_LIT(KVerLogFileName,"C:\\VerificationFailure");
_LIT(KExtension,".txt");
_LIT(KComma,",");
_LIT(KUnderScore,"_");
_LIT(KNewLine,"\n");

// FileSystem specific
_LIT(KFAT,"Fat");
_LIT(KWin32,"Win32");

// test case type
_LIT(KUnitary,"Unitary");
_LIT(KBinary,"Binary");
_LIT(KSymbianFATSpecific,"SymbianFATSpecific");

// test case phases
_LIT(KExecution,"Execution");
_LIT(KVerification,"Verification");

// os names
_LIT(KWindows,"Windows");
_LIT(KSymbian,"Symbian");
_LIT(KNone,"None");

// log file header
_LIT(KLogFileHeader,"IOT,TestCase,FailedDuring,Id,Path,FileSystem,ExeOsName,ExeDrive,VerOsName,VerDrive,API,Line,FunctionName,File");

// general purpose
_LIT(KYes,"Yes");
_LIT(KNo,"No");

// function names
_LIT(KDoAllBasicUnitaryTestsL,"DoAllBasicUnitaryTestsL");
_LIT(KDoAllBasicBinaryTestsL,"DoAllBasicBinaryTestsL");
_LIT(KTestLeadingE5Handling,"TestLeadingE5Handling");
_LIT(KTestFileLengthMax,"TestFileLengthMax");
_LIT(KTestFileLengthExceedMax,"TestFileLengthExceedMax");
_LIT(KTestVFATCase1,"TestVFATCase1");
_LIT(KTestVFATCase2,"TestVFATCase2");
_LIT(KTestVFATCase3,"TestVFATCase3");
_LIT(KTestIllegalCharsWithDll,"TestIllegalCharsWithDll");
_LIT(KTestIllegalCharsWithoutDLL,"TestIllegalCharsWithoutDLL");
_LIT(KCheckDisk,"CheckDisk");
_LIT(KLogTestFailureData,"LogTestFailureData");
_LIT(KDataGenerationL,"DataGenerationL");
_LIT(KDeletePathAfterTest,"DeletePathAfterTest");
_LIT(KDataExecutionL,"DataExecutionL");
_LIT(KDataVerificationL,"DataVerificationL");
_LIT(KSearchTestCaseByArrayIdx,"SearchTestCaseByArrayIdx");
_LIT(KScanTestDrive,"ScanTestDrive");
_LIT(KTestCompatibility,"TestCompatibility");

// api names
_LIT(KGetShortName,"EGetShortName");
_LIT(KGetShortNameWithDLL,"EGetShortNameWithDLL");
_LIT(KGetShortNameWithoutDLL,"EGetShortNameWithoutDLL");
_LIT(KCreateFile,"ECreateFile");
_LIT(KIsValidName,"EIsValidName");
_LIT(KRenameFile,"ERenameFile");
_LIT(KReadFileSection,"EReadFileSection");
_LIT(KDeleteFile,"EDeleteFile");
_LIT(KOpenDir,"EOpenDir");
_LIT(KReadDir,"EReadDir");
_LIT(KRemoveDir,"ERemoveDir");
_LIT(KIsFileInRom,"EIsFileInRom");
_LIT(KReplaceFile,"EReplaceFile");
_LIT(KOperateOnFileNames,"EOperateOnFileNames");
_LIT(KFileModify,"EFileModify");
_LIT(KFileAttributes,"EFileAttributes");
_LIT(KRFsEntry,"ERFsEntry)");
_LIT(KRFsReplace,"ERFsReplace");
_LIT(KRFsRename,"ERFsRename");
_LIT(KGetDir,"EGetDir");
_LIT(KWriteToFile,"EWriteToFile");
_LIT(KReadFromFile,"EReadFromFile");
_LIT(KMkDir,"EMkDir");
_LIT(KMkDirAll,"EMkDirAll");
_LIT(KFileTemp,"EFileTemp");
_LIT(KLongShortConversion,"ELongShortConversion");

#ifdef LOG_FAILURE_DATA
#define testAndLog(flag)													\
	{																		\
	if(flag == EFalse)														\
		{																	\
		TInt line = __LINE__;												\
		test.Printf(_L("\nFAILURE on LINE: %d"),line);						\
		LogTestFailureData(gTCType, failedOnBuf, gTCId, tcUniquePath, line);	\
		}																	\
	}
#else
#define testAndLog(flag)													\
	{																		\
	test(flag);																\
	}
#endif


// forward declaration
struct TDirSetupFiles;

///////////////////////////////////////////////////////////////////////////////
//	Files Setup Pattern Definitions
///////////////////////////////////////////////////////////////////////////////
static TPtrC gDirPatterns[] = 
	{	
		
	// ******************************ALPHABETIC-CHARACTERS***************************************************

	_L("ABC(DE).TX"),				//0 MAJOR <8 ,EXT <3
	_L("ABC(DE).TXTTXT"),			//1 MAJOR <8 ,EXT >3
	_L("ABC(DE).TXT"),				//2 MAJOR <8 ,EXT =3
		
	_L("ABCDEF(GH).TX"),			//3 MAJOR >8 ,EXT <3
	_L("ABCDEF(GH).TXTTXT"),		//4 MAJOR >8 ,EXT >3
	_L("ABCDEF(GH).TXT"),			//5 MAJOR >8 ,EXT =3
			
	_L("ABC(DEF).TX"),				//6 MAJOR =8 ,EXT <3
	_L("ABC(DEF).TXTTXT"),			//7 MAJOR =8 ,EXT >3
	_L("ABC(DEF).TXT"),				//8 MAJOR =8 ,EXT =3
		
	// ******************************UNICODE-CHARACTERS*******************************************************	

	_L("\x65B0\x65B0.\x65B0"),						//9 MAJOR <8  ,EXT <3
	_L("\x65B0\x6587.\x65B0\x6587"),				//10 MAJOR <8 ,EXT 3>6
	_L("\x65B0\x65B0.\x65B0\x65B0\x65B0"),			//11 MAJOR <8 ,EXT =6
	_L("\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0"),	//12 MAJOR >8 ,EXT >6
		
	_L("\x65B0\x65B0\x65B0\x65B0.\x65B0"),						//13 MAJOR =8 ,EXT <3
	_L("\x65B0\x65B0\x65B0\x6587.\x65B0\x6587"),				//14 MAJOR =8 ,EXT 3>6
	_L("\x65B0\x65B0\x65B0\x4EF6.\x65B0\x65B0\x65B0"),			//15 MAJOR =8 ,EXT =6
	_L("\x65B0\x4EF6\x65B0\x6587.\x65B0\x4EF6\x65B0\x6587"),	//16 MAJOR =8 ,EXT >6
		
	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0"),				//17 MAJOR 8<16 ,EXT <3
	_L("\x65B0\x4EF6\x65B0\x65B0\x6587.\x65B0\x6587"),				//18 MAJOR 8<16 ,EXT >3
	_L("\x65B0\x6587\x65B0\x4EF6\x65B0.\x65B0\x65B0\x65B0"),		//19 MAJOR 8<16 ,EXT =6
	_L("\x65B0\x6587\x6587\x6587\x4EF6.\x65B0\x65B0\x65B0\x65B0"),	//20 MAJOR 8<16 ,EXT >6
		
	_L("\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6"),						//21 MAJOR =16 ,EXT <3
	_L("\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587"),				//22 MAJOR =16 ,EXT 3>6
	_L("\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0"),				//23 MAJOR =16 ,EXT =6
	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0"),	//24 MAJOR =16 ,EXT >6
		
	_L("\x6587\x6587\x6587\x6587\x65B0\x65B0\x65B0\x65B0\x4EF6\x4EF6.\x65B0"),				//25 MAJOR >16 ,EXT <3
	_L("\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x6587"),		//26 MAJOR >16 ,EXT 3>6
	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.\x65B0\x65B0\x65B0"),		//27 MAJOR >16 ,EXT =6
	_L("\x65B0\x65B0\x4EF6\x65B0\x65B0\x65B0\x4EF6\x65B0\x65B0.\x65B0\x65B0\x65B0\x65B0"),	//28 MAJOR >16 ,EXT >6

	// ******************************MIXED-CHARACTERS*********************************************************************

	_L("\x65B0(A).\x65B0"),							//29 MAJOR <8 ,EXT <3
	_L("\x65B0(A).A\x65B0"),						//30 MAJOR <8 ,EXT =3
	_L("\x65B0(A).A\x65B0\x6587"),					//31 MAJOR <8 ,EXT 3<6
	_L("\x65B0(A).AB\x65B0\x65B0"),					//32 MAJOR <8 ,EXT <6
	_L("\x65B0(A).AB\x65B0\x65B0\x65B0\x65B0"),		//33 MAJOR <8 ,EXT >6
							
	_L("\x65B0\x65B0(AB).\x65B0"),						//34 MAJOR =8 ,EXT <3
	_L("(AB)\x65B0\x65B0.A\x65B0"),						//35 MAJOR =8 ,EXT =3
	_L("\x65B0(AB)\x65B0.A\x65B0\x6587"),				//36 MAJOR =8 ,EXT 3<6
	_L("\x65B0(\x65B0)AB.AB\x65B0\x65B0"),				//37 MAJOR =8 ,EXT <6
	_L("\x65B0\x65B0(CD).AB\x65B0\x65B0\x65B0\x65B0"),	//38 MAJOR =8 ,EXT >6

	_L("\x65B0\x65B0\x65B0\x65B0(AB).\x65B0"),							//39 MAJOR 8<16 ,EXT <3
	_L("\x65B0\x65B0\x65B0\x65B0(AB).A\x65B0"),							//40 MAJOR 8<16 ,EXT =3
	_L("AB\x65B0\x6587\x65B0\x65B0.A\x65B0\x6587"),						//41 MAJOR 8<16 ,EXT 3<6
	_L("CD\x65B0\x6587\x65B0\x65B0.AB\x65B0\x65B0"),					//42 MAJOR 8<16 ,EXT <6
	_L("\x65B0\x6587(\x65B0\x65B0).AB\x65B0\x65B0\x65B0\x65B0"),		//43 MAJOR 8<16 ,EXT >6

	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0"),						//44 MAJOR =16 ,EXT <3
	_L("(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0"),						//45 MAJOR =16  ,EXT =3
	_L("\x65B0\x65B0\x65B0(AB)\x65B0\x65B0\x65B0.A\x65B0\x6587"),				//46 MAJOR =16  ,EXT 3<6
	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).AB\x65B0\x65B0"),				//47 MAJOR =16  ,EXT <6
	_L("\x65B0\x65B0(\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0"),	//48 MAJOR =16  ,EXT >6
		
	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(AB).\x65B0"),						//49 MAJOR >16 ,EXT <3
	_L("(AB)\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0.A\x65B0"),						//50 MAJOR >16 ,EXT =3
	_L("\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0\x65B0(CD).A\x65B0\x6587"),				//51 MAJOR >16 ,EXT 3<6
	_L("\x65B0\x65B0\x65B0(\x65B0\x65B0\x65B0)CD\x65B0\x65B0.AB\x65B0\x65B0"),				//52 MAJOR >16 ,EXT <6
	_L("CD\x65B0\x65B0(\x65B0\x65B0)\x65B0\x65B0\x65B0\x65B0.AB\x65B0\x65B0\x65B0\x65B0"),	//53 MAJOR >16 ,EXT >6

	// ****************************************************************************************************************************	

	_L("TESTDIR\\"),			//54
	_L("TESTDIR\\DIR1\\"),		//55	
	_L("TESTDIR\\DIR2\\"),		//56	
	_L("TESTDIR\\DIR3\\"),		//57
	_L("TESTDIR\\DIR4\\"),		//58	

	_L("TESTDIR_FS\\"),			//59	
	_L("TESTDIR_FS\\DIR3\\"),	//60		
	_L("TESTDIR_FS\\DIR1\\"),	//61		
	_L("TESTDIR_FS\\DIR2\\"),	//62		

	_L("TESTDIR_FS\\DIR3\\FILE03.TXT"),		//63
	_L("TESTDIR_FS\\DIR1\\FILE01.TXT"),		//64	
	_L("TESTDIR_FS\\DIR2\\FILE02.TXT"),		//65
	
	_L("TESTALPHA.TXT"), 		//66
	_L("TESTALPHA.TXTTXT"), 	//67
	
	_L("REPLACED.TXT"),		//68
	_L("REPLACED.TX"),		//69
	_L("REPLACED.TXTTXT"),	//70
	
	_L("XYZ\\"),			//71
	_L("TESTRENAME.DAT"),	//72
	
	_L("\x65B0\x65B0\x65B0\\"), 	//73
	_L("\x65B0\x6587\x4EF6\x4EF6\x6587\x65B0.\x65B0\x4EF6"),  //74
	
	_L("AB(\x65B0\x6587)CD\\"),				//75
	_L("AB\x65B0\x6587(CDEF).\x4EF6(AB)"),	//76
	
	
	_L("\x65B0\x6587\x65B0\x4EF6.TXT"),				//77
	_L("\x65B0\x6587\x65B0\x4EF6(A).TXT"),			//78
	_L("\x65B0\x6587\x65B0\x4EF6(B).TXT"),			//79
	_L("\x65B0\x6587\x65B0\x4EF6(C).TXT"),			//80
	
	_L("\x65B0.TXT"),							//81
	_L("\x65B0\x6587.TXTTXT"),					//82
	_L("\x65B0\x6587\x4EF6.TX"),				//83	
	
	_L("ABCDE.\x65B0\x65B0"),					//84
	_L("ABCDEFG.\x65B0\x65B0\x65B0"),			//85
	_L("ABCD.\x65B0T"),							//86
	_L("ABCDE.T\x65B0"),						//87
	
	_L("\x222F\x2F3A\x3C3E\x7C00.TXT"),			//88
	_L("\x235B\x245C\x255D\x265E.TXT"),			//89

	_L("\x65B0\x6587\\"),											//90
	_L("\x65B0\x6587\\\x65B0\x6587\x65B0\\"),						//91	
	_L("\x65B0\x6587\\\x65B0\x6587\x65B0\x6587\\"),					//92	
	_L("\x65B0\x6587\\\x65B0\x6587\x65B0\x6587\x65B0\\"),			//93
	_L("\x65B0\x6587\\\x65B0\x6587\x65B0\x6587\x65B0\x6587\\"),		//94	

	_L("\x65B0\x6587\x4EF6\\"),										//95	
	_L("\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\\"),					//96		
	_L("\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\\"),			//97		
	_L("\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\x6587\\"),		//98		

	_L("\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\\\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6\x65B0\x4EF6.\x4EF6"),						//99
	_L("\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\\\x4EF6\x4EF6\x65B0\x65B0\x65B0\x65B0\x65B0\x6587.\x65B0\x6587"),			//100	
	_L("\x65B0\x6587\x4EF6\\\x65B0\x4EF6\x65B0\x4EF6\x6587\\\x65B0\x65B0\x65B0\x4EF6\x65B0\x4EF6\x4EF6.\x65B0\x4EF6\x65B0"),	//101
	
	_L("\x65B0(A)\x6587\\"),												//102
	_L("\x65B0(A)\x6587\\\x65B0(AB)\x65B0\\"),								//103	
	_L("\x65B0(A)\x6587\\\x65B0(ABCD)\x65B0\\"),							//104	
	_L("\x65B0(A)\x6587\\\x65B0\x65B0(ABCDEF)\x65B0\x65B0\\"),				//105
	_L("\x65B0(A)\x6587\\\x65B0\x65B0\x65B0(ABGH)\x65B0\x65B0\x65B0\\"),	//106	

	_L("\x65B0\x6587(AB)\\"),										//107	
	_L("\x65B0\x6587(AB)\\\x65B0(A)\x65B0\\"),						//108		
	_L("\x65B0\x6587(AB)\\\x65B0\x4EF6(AB)\x4EF6\\"),				//109		
	_L("\x65B0\x6587(AB)\\\x65B0\x4EF6(ABCDEF)\x4EF6\x6587\\"),		//110		

	_L("\x65B0\x6587(AB)\\\x65B0(A)\x65B0\\\x65B0\x4EF6\x65B0(AB)\x4EF6\x65B0\x4EF6.\x4EF6"),							//111
	_L("\x65B0\x6587(AB)\\\x65B0\x4EF6(AB)\x4EF6\\\x4EF6\x4EF6(ABC)\x65B0\x65B0\x6587.\x65B0\x6587"),					//112	
	_L("\x65B0\x6587(AB)\\\x65B0\x4EF6(ABCDEF)\x4EF6\x6587\\\x65B0\x65B0\x65B0(CD)\x4EF6\x4EF6.\x65B0\x4EF6\x65B0"),	//113
	};

#define EOB 	-1			// 'End Of Block' tag
#define CON 	(EOB - 1)	// 'Continue' tag
#define LAST 	130 		// Last item's tag
//...

// Predefined dir data blocks for testing data setup:
#define ALL			{0,CON,LAST,EOB}		// Block includes all items

#define EMPTY		{EOB}					// Empty block

#define BLOCK01		{0,CON,8,EOB}
#define BLOCK02		{9,CON,12,EOB}
#define BLOCK03		{13,CON,16,EOB}
#define BLOCK04		{17,CON,20,EOB}
#define BLOCK05		{21,CON,24,EOB}
#define BLOCK06		{25,CON,28,EOB}
#define BLOCK07		{29,CON,33,EOB}
#define BLOCK08		{34,CON,38,EOB}
#define BLOCK09		{39,CON,43,EOB}
#define BLOCK10		{44,CON,48,EOB}
#define BLOCK11		{49,CON,53,EOB}
#define BLOCK12		{54,CON,58,EOB}
#define BLOCK13		{59,CON,62,EOB}
#define BLOCK14		{63,CON,65,EOB}
#define BLOCK15		{77,CON,80,EOB}
#define BLOCK16		{81,CON,87,EOB}
#define BLOCK17		{90,CON,94,EOB}
#define BLOCK18		{95,CON,98,EOB}
#define BLOCK19		{99,CON,101,EOB}
#define BLOCK20		{102,CON,106,EOB}
#define BLOCK21		{107,CON,110,EOB}
#define BLOCK22		{111,CON,113,EOB}




///////////////////////////////////////////////////////////////////////////////
//	Data structures for building test case parameters
///////////////////////////////////////////////////////////////////////////////
enum TTestingAPI
	{ 
	EGetShortName,
	EGetShortNameWithDLL,
	EGetShortNameWithoutDLL,
	ECreateFile,
	EIsValidName,
	ERenameFile,
	EReadFileSection,
	EDeleteFile,
	EOpenDir,
	EReadDir,
	ERemoveDir,
	EIsFileInRom,
	EReplaceFile,
	EOperateOnFileNames,
	EFileModify,
	EFileAttributes,
	ERFsEntry,
	ERFsReplace,
	ERFsRename,
	EGetDir,
	EWriteToFile,
	EReadFromFile,
	EMkDir,
	EMkDirAll,
	EFileTemp,
	ELongShortConversion,
	};

struct TDirSetupFiles
	{
	TInt iAddingBlock[25];
	TInt iDeductBlock[25];
	};
// Basic Testing Parameters
struct TTestParamBasic
	{
	TUint 			iTestCaseID;
	TTestingAPI		iAPI;
	TUint 			iSwitch;
	TInt			iSyncReturn;
	TInt			iAsyncReturn;
	TInt			iAsyncStatus;
	};
	

// Basic Presettings Parameters
struct TTestParamBasicPrs
	{
	TChar* 			iDrvChar;
	TText16*		iCmdPath;		
	TText16*		iPrsPath;
	TDirSetupFiles 	iPrsFiles;
	TText16*		iCmpPath;
	TDirSetupFiles 	iCmpFiles;
	
	};

// Presettings with file open mode
struct TTestParamPrsFileOpen
	{
	TTestParamBasicPrs 	iBasicPrs;
	TBool				iFileOpen;
	};

// Presettings with file attribs and modified time
struct TTestParamPrsAttribsTime
	{
	TTestParamBasicPrs 	iBasicPrs;
	TUint				iAttribsSet;
	TTime				iTime;
	};


///////////////////////////////////////////////////////////
//	Test case data structures
///////////////////////////////////////////////////////////
struct TTestCaseUnitaryBasic
	{
	TTestParamBasic 	iBasic;
	TTestParamBasicPrs 	iSrcPrsBasic;
	};

struct TTestCaseBinaryBasic
	{
	TTestParamBasic 	iBasic;
	TTestParamBasicPrs 	iSrcPrsBasic;
	TTestParamBasicPrs 	iTrgPrsBasic;
	};	

struct TTestCaseUnitaryAttribTime
	{
	TTestParamBasic 			iBasic;
	TUint						iSetAttribs;
	TUint						iClearAttribs;
	TTestParamPrsAttribsTime	iSrcPrsAT;
	};

struct TTestCaseBinaryAttribTime
	{
	TTestParamBasic 			iBasic;
	TTestParamPrsAttribsTime	iSrcPrsAT;
	TTestParamPrsAttribsTime	iTrgPrsAT;
	};

////////////////////////////////////////////////////////////
//	An interface between test framework and test cases
////////////////////////////////////////////////////////////
class TTestParamAll
	{
public:
	TUint 			iTestCaseID;
	TTestingAPI		iAPI;
	TFileName		iAPIName;
	TUint 			iSwitch;
	TInt			iSyncReturn;
	TInt			iAsyncReturn;
	TInt			iAsyncStatus;

	TBool			iIsWithDLL;
	
	TUint			iSetAttribs;
	TUint			iClearAttribs;
	TTime			iSetModified;

	TChar* 			iSrcDrvChar;

	TFileName		iSrcCmdPath;
	TFileName		iSrcPrsPath;
	TFileName		iTrgCmdPath;

	TDirSetupFiles 	iSrcPrsFiles;
	TUint			iSrcPrsAttribs;
	TTime			iSrcPrsTime;
	TBool			iSrcPrsFileOpen;


	TChar* 			iTrgDrvChar;

	};

class TTestSwitches
	{
public:
	
	TBool 		iExeOnSymbian;
	TBool		iVerOnSymbian;
	TBool		iExeOnWindows;
	TBool		iVerOnWindows;
	TChar		iExeDriveChar;
	TChar		iVerDriveChar;
	TInt		iExeDriveNum;
	TInt		iVerDriveNum;
	TBuf<20>	iMountedFSName;
	};

class TTestLogFailureData
	{
public:

	TFileName		iTCTypeName; // unitary/binary/Special
	TFileName		iTCFailureOn; // test phase data execution/data verification
	TUint 			iTCId;
	TFileName 		iTCUniquePath; // complete path with DLL/without DLL with Synch/Asynch etc
	TBuf<20>		iFSName;
	TFileName		iExeOsName; // OS name
	TFileName		iVerOsName; // OS name
	TChar			iExeDrive;
	TChar			iVerDrive;
	TFileName		iAPIName;
	TInt			iLineNum;
	TFileName		iFuncName;
	TBuf8<50>		iFileName;
	};


void LogTestFailureData(TTCType tcType, TFileName failedOn, TUint tcId, TFileName tcUniquePath,TInt line);
void ClearTCLogData();
void InitLogData();
void CheckDisk();
void GetBootInfo();
void ReadBootSector(TFatBootSector& aBootSector);
void QuickFormat();
TInt SearchTestCaseByArrayIdx(TUint aIdx, const TTestCaseUnitaryBasic aBasicBinaryTestCaseGroup[], TTestParamAll& aTestCaseFound, TBool aIsWithDLL);
TInt SearchTestCaseByArrayIdx(TUint aIdx, const TTestCaseBinaryBasic aBasicBinaryTestCaseGroup[], TTestParamAll& aTestCaseFound, TBool aIsWithDLL);
void Help();
void ClearSwitches(TTestSwitches& aSwitches);
void ParseCommandArguments(TTestSwitches& aSwitches);
void InitialiseL();
void RmDir(const TDesC& aDirName);
void Cleanup();
void ParsingDirDataBlock(const TInt aDataBlock[], RArray<TInt>& aDirDataArray);
void CheckIfIOTesting(TTestSwitches& aSwitches);
/*template <class C>
TInt controlIo(RFs &fs, TInt drv, TInt fkn, C &c);*/
void MakeFile(const TDesC& aFileName,const TUidType& aUidType,const TDesC8& aFileContents);
void MakeFile(const TDesC& aFileName,const TDesC8& aFileContents);
void MakeFile(const TDesC& aFileName,TInt anAttributes);
void MakeFile(const TDesC& aFileName);
TInt CurrentDrive(TChar aDriveChar);
TInt CurrentDrive();
void CreateTestDirectory(const TDesC& aSessionPath);
void ReportCheckDiskFailure(TInt aRet);
void MakeDir(const TDesC& aDirName);
void Format(TInt aDrive);

//Function declarations
void RmDir(const TDesC& aDirName);
TBool CompareL(const TDesC& aDir1,const TDesC& aDir2);
void SetupDirectories(TBool aCreateFiles, TFileName* aDestOtherDrive);
void InitialiseL();
void Cleanup();
void CleanupFileHandles();
void ParsingDirDataBlock(TInt aDataBlock[], RArray<TInt>& aFinalDirDataArray);
void SetupDir(TDesC& aPath, TDirSetupFiles aDirSetupData);
void SetupDirFiles(const TDesC& aPath, const TDirSetupFiles& aDirFiles);
void PrintDir(const TDesC& aPath, const TChar& aDrv);
void DoSymbianSpecificCases();
inline TBool isFAT(const TDesC& aFSName)
	{
	return (aFSName.Compare(KFAT) == 0);
	}
inline TBool isWin32(const TDesC& aFSName)
	{
	return (aFSName.Compare(KWin32) == 0);
	}

//functions that are required by both the .cpp files.
GLREF_C void TestLeadingE5Handling();
GLREF_C void TestFileLengthMax();
GLREF_C void TestFileLengthExceedMax();
GLREF_C void TestIllegalCharsWithoutDLL();
GLREF_C void TestIllegalCharsWithDll();
GLREF_C void TestVFATCase1();
GLREF_C void TestVFATCase2();
GLREF_C void TestVFATCase3();

GLREF_C void GetBootInfo();
GLREF_C void QuickFormat();
GLREF_C void ReadBootSector(TFatBootSector& aBootSector);



#endif /*T_FATCHARSETCONV_AUX_H*/
