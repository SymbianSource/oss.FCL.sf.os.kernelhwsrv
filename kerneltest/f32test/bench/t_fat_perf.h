/**
* Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* File Name:		f32test/bench/t_fat_perf.h
* Include file for t_fat_perf.cpp (PREQ 1885).
* 
*
*/



//Include Files
#include <f32file.h>
#include <e32test.h>
#include <hal.h>
#include "t_server.h"
#include "fat_utils.h"
#include "f32_test_utils.h" 

using namespace F32_Test_Utils;
using namespace Fat_Test_Utils;

extern TInt gTestCaseNo;

extern TFileName 	gPath;
extern TFileName 	gFileNameBase;
extern TInt 		gFileNo;
extern TInt		    gTestCase;
extern TInt			gCacheClear;
extern TInt         gZeroPadFileNumberForFixedLengthFileNames;

//////////////////////////////////////////////////////////////////////////////
// Interfaces

// File Operation Modes
enum TFileOperationMode
	{
	EFATPerfFileOpen,
	EFATPerfFileCreate,
	EFATPerfFileReplace,
	EFATPerfFileDelete,
	EFATPerfFileWrite,
	EFATPerfFileRead,
	};

// File Creation Options
enum TNamingConOrRan
	{
	EConsecutive,
	ERandom,
	};

// Standards
enum TNamingUniOrAsc
	{
	EAscii,
	EUnicode,
	};

// Test function names
enum TTestFunction
	{
	EFATPerfSetup,
	EFATPerfCreate,
	EFATPerfOpen,
	EFATPerfDelete,
	EFATPerfRead,
	EFATPerfWrite,
	EFATPerfDirCacheInfo,
	};
	
// For DirCache info printing 
// These enum values are based on enum TControlIO defined in \f32\sfat\common_constants.h
enum TDirCacheControlIOMap
	{
	ETestDumpFATDirCache = 15,	///<15
	ETestFATDirCacheInfo = 16,	///<16
	};

// File Naming schemes
class TNamingSchemeParam
	{
public:
    TNamingSchemeParam();
    
public:
	TNamingConOrRan	iConOrRan;
	TNamingUniOrAsc	iUniOrAsc;
	TBool iZeroPadFileNumber; // Only applies to consecutive file name generation
	TFileName 	iFileNameBase;
	TUint	iMaxFileNameLength;	// Applies only to random name generation
	TUint	iMinStringLength;
	};

class CMeasureAndLogUnit;

// File Operation Parameters
struct TFileOpUnitParam
	{
	TFileName 	iDirName;
	TNamingSchemeParam iNamingSchemeParam;
	TFileOperationMode	iFileOpMode;
	CMeasureAndLogUnit* iMLUnitPtr;
	};

// Directory Unit Parameters
struct TDirUnitParam
	{
	TUint		iPriority;
	TFileName	iDirName;
	TUint		iRuns;
	TUint		iFilesPerRun;
	TUint		iSampleInterval;
	TNamingSchemeParam		iNamingScheme;
	TFileOperationMode		iFileOpMode;
	};

struct	TMLUnitParam
	{
	TInt	iID;
	};

struct TMLParam
	{
	TFileName	iDirName;
	TFileName	iFileName;
	TNamingSchemeParam iNamingScheme;
	TUint	iCurFileNo;
	TFileOperationMode	iFileOpMode;
	};

/////////////////////////////////////////////////////////////////////////////////////////////
// Module definition


// Measurement and Log Unit
class CMeasureAndLogUnit : CBase
	{
public:
	static CMeasureAndLogUnit* NewLC(const TMLUnitParam& aParam);
	static CMeasureAndLogUnit* NewL(const TMLUnitParam& aParam);
	~CMeasureAndLogUnit();
	TInt	MeasureStart();
	TInt	MeasureEnd();
	TInt	Log(const TFileName& aDirName, const TFileName& aFileName, TUint aCurrentFileNo, TUint aCurrentFilePos);
private:
	CMeasureAndLogUnit();
	void ConstructL(const TMLUnitParam& aParam);
	TInt	DoMeasureStart();
	TInt	DoMeasureEnd();
	TInt	DoLog(const TFileName& aFileName, TUint aCurrentFileNo, TUint aCurrentFilePos);
	
private:
	TInt	iID;
	TUint32	iStartStatus;
	TUint32	iEndStatus;
	TInt	iFreq;
	TReal	iScale;		
	TUint	iLogItemNo;
	};

// File Operation Unit
class CFileOperationUnit : CBase
	{
public:
	static CFileOperationUnit* NewLC(const TFileOpUnitParam& aParam);
	static CFileOperationUnit* NewL(const TFileOpUnitParam& aParam);
	~CFileOperationUnit();
	
	TInt 	Run(const TFileName& aDirName, const TFileName& aFileName, TBool aIsTakingMeasurement, TUint aCurFileNo, TUint aCurFilePos);
	void	SetMLUnit(CMeasureAndLogUnit* aMLUnit);

private:
	CFileOperationUnit();	
	void ConstructL(const TFileOpUnitParam& aParam);

private:
	TFileName 	iDirName;
	TNamingSchemeParam iNamingSchemeParam;
	TFileOperationMode	iFileOpMode;
	CMeasureAndLogUnit* iMLUnitPtr;
	RFs					iRFs;
	TBool				iDirCreated;
	};


class CDirUnit : public CBase
	{
public:
	static CDirUnit* NewLC(const TDirUnitParam& aParam, const TChar aDriveChar);
	static CDirUnit* NewL(const TDirUnitParam& aParam, const TChar aDriveChar);
	~CDirUnit();
	TInt Run(const TInt aCurPriority);
	void SetMLUnit(CMeasureAndLogUnit* aMLUnit);
	TInt Priority();
	const TFileName& Name();

private:
	CDirUnit();
	void ConstructL(const TDirUnitParam& aParam, const TChar aDriveChar);

	TInt 	GenerateFileName(TFileName& aFileName); // uses iNameGen
	TBool 	CheckMeasurementTaking();		     	// uses currentFileNo, 
												    // totalFileNo, 
												    // samplingInterval
	TInt 	DoFileOperation();
	TFileName GenerateRandomString(const TUint aMinStringLength, const TUint aMaxStringLength, const TNamingUniOrAsc aUniOrAsc);
	TBool FileNameIsUnique(const TFileName& aFileName);
private:
	TInt					iPriority;
	TFileName				iDirName;
	TUint					iRuns;
	TUint					iCurrentRunNo;
	TUint					iFilesPerRun;
	TUint					iCurrentFileNo;
	TUint					iTotalFileNo;
	TUint					iSampleInterval;
	
	TUint                   iNumDigitsInTotalFileNo;    // The number of digits iTotalFileNo has.
                                                        // Used to zero pad the file number if iZeroPadFileNumberForFixedWidthFileNames is ETrue
	
	TNamingConOrRan	iConOrRan;                 // Consecutive or random
	TNamingUniOrAsc	iUniOrAsc;                 // ASCII or Unicode
	TBool iZeroPadFileNumberForFixedLengthFileNames;
	TFileName 			iFileNameBase;
	TUint				iMaxFileNameLength;   // Applies only to random name generation
	TUint				iMinStringLength;     // Applies only to random name generation
	
	CFileOperationUnit* 	iFileOpUnit;
	CMeasureAndLogUnit*	iMLUnitPtr;
	
	};

//Execution Unit
class CExecutionUnit : public CBase
	{
public:
	static CExecutionUnit* NewLC(CMeasureAndLogUnit* aMLUnit, const TChar aDriveChar);
	static CExecutionUnit* NewL(CMeasureAndLogUnit* aMLUnit, const TChar aDriveChar);

	~CExecutionUnit();

	TInt 	AddDirUnitL(const TDirUnitParam& aParam);
	TInt	RecalculateCurrentPrioirty();
	TInt 	Run();

	///For Debug
	TUint ForDebug_AddrOfDirUnit(TUint aDirUnitIter);
	TUint ForDebug_AddrOfDirUnitArray();

private:
	CExecutionUnit();
	void ConstructL(CMeasureAndLogUnit* aMLUnit, const TChar aDriveChar);

private:
	// Array of CDirUnit
	RPointerArray<CDirUnit>	iDirUnitArray;
	
	// Logging Unit
	CMeasureAndLogUnit*	iMLUnitPtr;	// no ownership
	
	// Current priority
	TInt 	iCurPriority;
	
	TChar	iDriveChar;
	
	}; 
	
/*-- EOF--*/
	
	
