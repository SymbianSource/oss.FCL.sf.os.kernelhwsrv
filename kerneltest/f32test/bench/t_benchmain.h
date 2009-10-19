// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\bench\t_benchmain.h
// 
//


#include "t_select.h"
#include <f32file.h>
#include "f32_test_utils.h" 

using namespace F32_Test_Utils;

#if !defined(__T_BENCHSTD_H__)
#define __T_BENCHSTD_H__

#define FailIfError(r) \
	{ \
	if (r != KErrNone) \
		{ \
		test.Printf(_L("Return code == %d\n"), r); \
		test(EFalse); \
		} \
	}

enum TSelectedTest
    {
	ELocalDriveTest, EFindEntryTest, EFileSeekTest
	};

	
void CallTestsL();
void CreateTestDirectory(const TDesC& aTestPath);
void DeleteTestDirectory();
void SetSessionPath(const TDesC& aPathName);
void ReportCheckDiskFailure(TInt aRet);
void CheckDisk();
void CheckEntry(const TDesC& aName,TUint anAttributes,const TTime& aModified);
void PrintResultTime( TInt aPosX, TInt aPosY, TInt aValue) ;
void PrintResult( TInt aPosX, TInt aPosY, TInt aValue);
void PrintHeaders(TInt aType, TPtrC16 aTitle ); 
void PrintResultS( TInt aPosX, TInt aPosY, TDes16& aValue); 
void InitializeDrive(CSelectionBox* aSelector);
TInt ValidateDriveSelection(TDriveUnit aDrive,TSelectedTest aTest);
void FormatFat(TDriveUnit aDrive);

void FileNamesGeneration(TDes16& aBuffer, TInt aLong, TInt aPos,TInt ext);
TInt Validate(TAny* aSelector);
TInt CreateDirWithNFiles(TInt aN, TInt aType);
TInt TestFileCreate(TAny* aSelector);


GLREF_D RTest test;
GLREF_D RFs TheFs;
GLREF_D TFileName gSessionPath;
GLREF_D TFileName gExeFileName;
GLREF_D TInt gAllocFailOff;
GLREF_D TInt gAllocFailOn;
GLREF_D TInt64 gSeed;
GLREF_D TChar gDriveToTest;
GLREF_D TInt gFilesLimit; 	
GLREF_D TInt gTypes;  		
GLREF_D TInt gMode; 	
GLREF_D TInt gFormat; 
GLREF_D TInt gMinutes; 	
GLREF_D TInt gFileSize; 

GLREF_D TInt gTestHarness;
GLREF_D TInt gTestCase;	
GLREF_D TInt gTimeUnit;

const TInt KMaxFiles  = 10000 ; 
const TInt KMaxTypes  = 3 ; 
const TInt KOneK = 1024;


_LIT(KDirMultipleName, "dir%d_%d\\");
_LIT(KCommonFile,"LAST.TXT");

#if defined(_DEBUG)
#define SetAllocFailure(a) SetAllocFailure(a)
#else
#define SetAllocFailure(a) IsRomAddress(NULL)
#endif

#endif
