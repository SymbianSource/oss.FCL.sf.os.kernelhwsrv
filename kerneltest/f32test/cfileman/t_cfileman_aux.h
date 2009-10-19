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
// f32test\cfileman\t_cfileman_aux.h
// 
//

#ifndef T_CFILEMAN_AUX_H
#define T_CFILEMAN_AUX_H

#include <f32file.h>
#include <e32test.h>
#include <hal.h>
#include "t_server.h"

void RmDir(const TDesC& aDirName);
TBool CompareL(const TDesC& aDir1,const TDesC& aDir2);
void SetupDirectories(TBool aCreateFiles, TFileName* aDestOtherDrive);
void InitialiseL();
void Cleanup();
void CleanupFileHandles();

struct TDirSetupFiles;

void ParsingDirDataBlock(TInt aDataBlock[], RArray<TInt>& aFinalDirDataArray);
void SetupDir(TDesC& aPath, TDirSetupFiles aDirSetupData);
void SetupDirFiles(const TDesC& aPath, const TDirSetupFiles& aDirFiles);

void PrintDir(const TDesC& aPath, const TChar& aDrv);

///////////////////////////////////////////////////////////////////////////////
//	Files Setup Pattern Definitions
///////////////////////////////////////////////////////////////////////////////
static TPtrC gDirPatterns[] = 
	{
	_L("FILE1.TXT"),			// 0
	_L("FILE2.TXT"),			// 1
	_L("FILE01.TXT"),			// 2
	_L("FILE02.TXT"),			// 3
	_L("FILE.TXT"),				// 4
	_L("OTHER.TXT"),			// 5
	_L("FILE.DAT"),				// 6
	_L("FILE"),					// 7
	
	_L("DIR1\\"),				// 8
	
	_L("DIR1\\FILE1.TXT"),		// 9
	_L("DIR1\\FILE2.TXT"),		// 10
	_L("DIR1\\FILE01.TXT"),		// 11
	_L("DIR1\\FILE02.TXT"),		// 12
	_L("DIR1\\FILE.TXT"),		// 13
	_L("DIR1\\OTHER.TXT"),		// 14
	_L("DIR1\\FILE.DAT"),		// 15
	_L("DIR1\\FILE"),			// 16
	
	_L("DIR2\\"),				// 17
	
	_L("DIR2\\FILE1.TXT"),		// 18
	_L("DIR2\\FILE2.TXT"),		// 19
	_L("DIR2\\FILE01.TXT"),		// 20
	_L("DIR2\\FILE02.TXT"),		// 21
	_L("DIR2\\FILE.TXT"),		// 22
	_L("DIR2\\OTHER.TXT"),		// 23
	_L("DIR2\\FILE.DAT"),		// 24
	_L("DIR2\\FILE"),			// 25
	
	
	_L("DIR1\\DIR11\\"),		// 26
	
	_L("DIR1\\DIR11\\FILE1.TXT"),	// 27
	_L("DIR1\\DIR11\\FILE2.TXT"),	// 28
	_L("DIR1\\DIR11\\FILE01.TXT"),	// 29
	_L("DIR1\\DIR11\\FILE02.TXT"),	// 30
	_L("DIR1\\DIR11\\FILE.TXT"),	// 31
	_L("DIR1\\DIR11\\OTHER.TXT"),	// 32
	_L("DIR1\\DIR11\\FILE.DAT"),	// 33
	_L("DIR1\\DIR11\\FILE"),		// 34
	
		
	_L("DIR1\\DIR12\\"),		// 35
	
	_L("DIR1\\DIR12\\FILE1.TXT"),	// 36
	_L("DIR1\\DIR12\\FILE2.TXT"),	// 37
	_L("DIR1\\DIR12\\FILE01.TXT"),	// 38
	_L("DIR1\\DIR12\\FILE02.TXT"),	// 39
	_L("DIR1\\DIR12\\FILE.TXT"),	// 40
	_L("DIR1\\DIR12\\OTHER.TXT"),	// 41
	_L("DIR1\\DIR12\\FILE.DAT"),	// 42
	_L("DIR1\\DIR12\\FILE"),		// 43
	
	//Special
	_L("RENAMED.TXT"),				//44
	_L("RENAMED1.TXT"),				//45
	_L("RENAMED2.TXT"),				//46
	_L("RENAMED01.TXT"),			//47
	_L("RENAMED02.TXT"),			//48
	_L("RENAMED.DAT"),				//49
	_L("RENAMED"),					//50
	
	_L("FILE1.REN"),			//51
	_L("FILE2.REN"),			//52
	_L("FILE01.REN"),			//53
	_L("FILE02.REN"),			//54
	_L("FILE.REN"),				//55
	_L("OTHER.REN"),			//56
	_L("RENAMED.REN"),			//57
	_L("RENAMED1.REN"),			//58
	_L("RENAMED2.REN"),			//59
	_L("RENAMED01.REN"),		//60
	_L("RENAMED02.REN"),		//61
	_L("abcEtxt"),				//62
	_L("DIR1\\RENAMED.TXT"),	//63
	_L("DIR1\\RENAMED1.TXT"),	//64
	_L("DIR1\\RENAMED2.TXT"),	//65
	_L("DIR1\\RENAMED01.TXT"),	//66
	_L("DIR1\\RENAMED02.TXT"),	//67
	_L("DIR1\\DIR11\\RENAMED.TXT"),	//68
	_L("DIR1\\DIR11\\RENAMED1.TXT"),	//69
	_L("DIR1\\DIR11\\RENAMED2.TXT"),	//70
	_L("DIR1\\DIR11\\RENAMED01.TXT"),	//71
	_L("DIR1\\DIR11\\RENAMED02.TXT"),	//72
	_L("DIR1\\DIR12\\RENAMED.TXT"),	//73
	_L("DIR1\\DIR12\\RENAMED1.TXT"),	//74
	_L("DIR1\\DIR12\\RENAMED2.TXT"),	//75
	_L("DIR1\\DIR12\\RENAMED01.TXT"),	//76
	_L("DIR1\\DIR12\\RENAMED02.TXT"),	//77
	_L("DIR2\\RENAMED.TXT"),	//78
	_L("DIR2\\RENAMED1.TXT"),	//79
	_L("DIR2\\RENAMED2.TXT"),	//80
	_L("DIR2\\RENAMED01.TXT"),	//81
	_L("DIR2\\RENAMED02.TXT"),	//82
	_L("DIR1\\FILE1.REN"),		//83
	_L("DIR1\\FILE2.REN"),		//84
	_L("DIR1\\FILE01.REN"),		//85
	_L("DIR1\\FILE02.REN"),		//86
	_L("DIR1\\FILE.REN"),		//87
	_L("DIR1\\OTHER.REN"),		//88
	_L("DIR1\\RENAMED.REN"),	//89
	_L("DIR1\\RENAMED1.REN"),	//90
	_L("DIR1\\RENAMED2.REN"),	//91
	_L("DIR1\\RENAMED01.REN"),	//92
	_L("DIR1\\RENAMED02.REN"),	//93
	_L("DIR1\\DIR11\\FILE1.REN"),		//94
	_L("DIR1\\DIR11\\FILE2.REN"),		//95
	_L("DIR1\\DIR11\\FILE01.REN"),		//96
	_L("DIR1\\DIR11\\FILE02.REN"),		//97
	_L("DIR1\\DIR11\\FILE.REN"),		//98
	_L("DIR1\\DIR11\\OTHER.REN"),		//99
	_L("DIR1\\DIR11\\RENAMED.REN"),		//100
	_L("DIR1\\DIR11\\RENAMED1.REN"),	//101
	_L("DIR1\\DIR11\\RENAMED2.REN"),	//102
	_L("DIR1\\DIR11\\RENAMED01.REN"),	//103
	_L("DIR1\\DIR11\\RENAMED02.REN"),	//104
	_L("DIR1\\DIR12\\FILE1.REN"),		//105
	_L("DIR1\\DIR12\\FILE2.REN"),		//106
	_L("DIR1\\DIR12\\FILE01.REN"),		//107
	_L("DIR1\\DIR12\\FILE02.REN"),		//108
	_L("DIR1\\DIR12\\FILE.REN"),		//109
	_L("DIR1\\DIR12\\OTHER.REN"),		//110
	_L("DIR1\\DIR12\\RENAMED.REN"),		//111
	_L("DIR1\\DIR12\\RENAMED1.REN"),	//112
	_L("DIR1\\DIR12\\RENAMED2.REN"),	//113
	_L("DIR1\\DIR12\\RENAMED01.REN"),	//114
	_L("DIR1\\DIR12\\RENAMED02.REN"),	//115
	_L("DIR2\\FILE1.REN"),		//116
	_L("DIR2\\FILE2.REN"),		//117
	_L("DIR2\\FILE01.REN"),		//118
	_L("DIR2\\FILE02.REN"),		//119
	_L("DIR2\\FILE.REN"),		//120
	_L("DIR2\\OTHER.REN"),		//121
	_L("DIR2\\RENAMED.REN"),	//122
	_L("DIR2\\RENAMED1.REN"),	//123
	_L("DIR2\\RENAMED2.REN"),	//124
	_L("DIR2\\RENAMED01.REN"),	//125
	_L("DIR2\\RENAMED02.REN"),	//126
	_L("DIR1\\RENAMED.DAT"),	//127
	_L("DIR1\\DIR11\\RENAMED.DAT"),	//128
	_L("DIR1\\DIR12\\RENAMED.DAT"),	//129
	_L("DIR2\\RENAMED.DAT"),	//130
	
	};

#define EOB 	-1			// 'End Of Block' tag
#define CON 	(EOB - 1)	// 'Continue' tag
#define LAST 	130 		// Last item's tag
//...

// Predefined dir data blocks for testing data setup:
#define ALL			{0,CON,LAST,EOB}		// Block includes all items

#define EMPTY		{EOB}					// Empty block

#define BLOCK01		{0,CON,7,EOB}
#define BLOCK02		{0,CON,4,EOB}
#define BLOCK03		{0,CON,5,EOB}
#define BLOCK04		{0,CON,7,9,CON,16,18,CON,25,27,CON,34,36,CON,43,EOB}// Src\\*
#define BLOCK05		{0,CON,4,9,CON,13,18,CON,22,27,CON,31,36,CON,40,EOB}// Src\\FILE*.TXT
#define BLOCK06		{0,1,9,10,18,19,27,28,36,37,EOB}// Src\\FILE?.TXT
#define BLOCK07		{0,CON,5,9,CON,14,18,CON,23,27,CON,32,36,CON,41,EOB}//Src\\*.TXT
#define BLOCK08		{4,6,13,15,22,24,31,33,40,42,55,EOB}//Src\\FILE.*
#define BLOCK09		{8,CON,16,26,CON,43,EOB}
#define BLOCK10     {26,CON,34,EOB}
#define BLOCK11		{9,CON,16,26,CON,43,EOB}
#define BLOCK12		{9,CON,16,EOB}
#define BLOCK13		{0,CON,3,EOB}
#define BLOCK14 	{0,1,EOB}
#define BLOCK15		{0,1,8,9,10,17,18,19,26,27,28,35,36,37,EOB}
#define BLOCK16		{5,8,14,17,23,26,32,35,41,EOB}
#define BLOCK17		{3,8,12,17,21,26,30,35,39,EOB}
#define BLOCK18		{8,CON,16,EOB}
#define BLOCK19		{51,CON,61,EOB}
#define BLOCK20		{0,CON,5,44,CON,48,EOB}
#define BLOCK21		{0,CON,25,35,CON,61,EOB}
#define BLOCK22		{9,CON,63,EOB}
#define BLOCK23		{63,CON,71}
#define BLOCK24		{0,CON,7,44,CON,62,EOB}
#define BLOCK25		{0,CON,7,9,CON,16,18,CON,25,27,CON,34,36,CON,62,EOB}
#define BLOCK26		{0,CON,5,9,CON,14,18,CON,23,27,CON,32,36,CON,41,44,CON,48,EOB}
#define BLOCK27		{4,6,13,15,22,24,31,33,40,42,55,EOB}
#define BLOCK28		{8,CON,43,EOB}
#define BLOCK29		{0,CON,5,44,CON,48,EOB}
#define BLOCK30		{44,8,63,17,68,26,73,35,78,EOB}
#define BLOCK31	    {44,CON,48,63,CON,82,EOB}
#define BLOCK32		{51,CON,61,83,CON,126,EOB}
#define BLOCK33		{44,49,57,89,100,111,122,127,CON,130,EOB}
#define BLOCK36     {0,CON,5,9,CON,14,18,CON,23,27,CON,32,36,CON,41,44,CON,48,63,CON,82,EOB}
#define BLOCK37     {4,6,13,15,22,24,31,33,40,42,55,87,98,109,120,EOB}					
#define BLOCK39		{8,CON,16,26,CON,43,63,CON,77,83,CON,115,127,CON,129,EOB}
#define BLOCK45		{51,CON,61,83,CON,126,EOB}
#define BLOCK46		{44,49,57,63,68,73,78,89,100,111,122,127,128,129,130,EOB}
#define BLOCK47		{44,49,55,57,63,68,73,78,87,89,98,100,109,111,120,122,127,128,129,130,EOB}

#define BLOCK34		{0,CON,5,9,CON,14,18,CON,23,27,CON,32,36,CON,41,44,CON,48,63,CON,82,EOB}
#define BLOCK35		{4,6,13,15,22,24,31,33,40,42,55,87,98,109,120,EOB} 
#define BLOCK40 	{44,49,57,63,68,73,78,89,100,111,122,127,CON,130,EOB} 
#define BLOCK41		{8,CON,16,26,CON,43,63,CON,77,83,CON,115,127,CON,129,EOB}
#define BLOCK42		{0,CON,7,44,CON,49,51,CON,61,EOB}
#define BLOCK43 	{9,CON,16,26,CON,43,63,CON,77,83,CON,115,127,CON,129,EOB}

///////////////////////////////////////////////////////////////////////////////
//	Data structures for building test case parameters
///////////////////////////////////////////////////////////////////////////////
enum TTestingAPI
	{ 
	ECFMDelete,
	ECFMMove,
	ECFMRmDir,
	ECFMCopy,
	ECFMRename,
	ECFMCopyHandle,
	ECFMAttribs,
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

struct TTestCaseUnitaryFileOpen
	{
	TTestParamBasic 		iBasic;
	TTestParamPrsFileOpen	iSrcPrsFO;
	};

struct TTestCaseBinaryFileOpen
	{
	TTestParamBasic 		iBasic;
	TTestParamPrsFileOpen	iSrcPrsFO;
	TTestParamPrsFileOpen	iTrgPrsFO;
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
	TUint 			iSwitch;
	TInt			iSyncReturn;
	TInt			iAsyncReturn;
	TInt			iAsyncStatus;

	TUint			iSetAttribs;
	TUint			iClearAttribs;
	TTime			iSetModified;

	TChar* 			iSrcDrvChar;
	TPtrC			iSrcCmdPath;

	TPtrC			iSrcPrsPath;
	TDirSetupFiles 	iSrcPrsFiles;
	TUint			iSrcPrsAttribs;
	TTime			iSrcPrsTime;
	TBool			iSrcPrsFileOpen;

	TPtrC			iSrcCmpPath;
	TDirSetupFiles 	iSrcCmpFiles;
	TUint			iSrcCmpAttribs;
	TTime			iSrcCmpTime;
	TBool			iSrcCmpFileOpen;

	TChar* 			iTrgDrvChar;
	TPtrC			iTrgCmdPath;

	TPtrC			iTrgPrsPath;
	TDirSetupFiles 	iTrgPrsFiles;
	TUint			iTrgPrsAttribs;
	TTime			iTrgPrsTime;
	TBool			iTrgPrsFileOpen;

	TPtrC			iTrgCmpPath;
	TDirSetupFiles 	iTrgCmpFiles;
	TUint			iTrgCmpAttribs;
	TTime			iTrgCmpTime;
	TBool			iTrgCmpFileOpen;
	};
#endif /*T_CFILEMAN_AUX_H*/
