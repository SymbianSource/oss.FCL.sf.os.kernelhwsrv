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
// File Name:		f32test/bench/t_fat_perf_impl.cpp
// This file contains FRAMEWORK implementation for the tests to test
// the FAT Performance on large number of files (PREQ 1885).
// 
//

// Include Files
#include "t_fat_perf.h"
#include <e32math.h>
#include <hal.h>

const TInt gMeasurementScale = K1mSec;	// defines the scales of measurement 

CMeasureAndLogUnit::CMeasureAndLogUnit()
	{
     
	}

void CMeasureAndLogUnit::ConstructL(const TMLUnitParam& aParam)
	{
	iID = aParam.iID;
	User::LeaveIfError(HAL::Get(HALData::EFastCounterFrequency, iFreq));
	iScale = gMeasurementScale;
	iLogItemNo = 1;
	}

CMeasureAndLogUnit* CMeasureAndLogUnit::NewLC(const TMLUnitParam& aParam)
	{
	CMeasureAndLogUnit* self = new(ELeave) CMeasureAndLogUnit();
	CleanupStack::PushL(self);
	self->ConstructL(aParam);
	return self;
	}

CMeasureAndLogUnit* CMeasureAndLogUnit::NewL(const TMLUnitParam& aParam)
	{
	CMeasureAndLogUnit* self = CMeasureAndLogUnit::NewLC(aParam);
	CleanupStack::Pop(self);
	return self;
	}

CMeasureAndLogUnit::~CMeasureAndLogUnit()
	{

	}

TInt CMeasureAndLogUnit::MeasureStart()
	{
	iStartStatus = User::FastCounter();
	return KErrNone;
	}
TInt CMeasureAndLogUnit::MeasureEnd()
	{
	iEndStatus = User::FastCounter();
	return KErrNone;
	}

//Measurement and Log unit
TInt CMeasureAndLogUnit::Log(const TFileName& aDirName, const TFileName& aFileName, TUint aCurrentFileNo, TUint aCurrentFilePos)
	{
	TReal measure = iScale * ((TReal)(iEndStatus - iStartStatus)) / ((TReal) iFreq);
	TBuf<0x10> scale;
	if (iScale == K1mSec)  
		{
		scale = _L("millisecond");
		}
	else if (iScale == K1Sec) 
		{
		scale = _L("second");
		}
	else if (iScale == K1uSec) 
		{
		scale = _L("microsecond");
		}
	test.Printf(_L("[LOG]: \t%u \t\"%S\" \t%f \t%S \t\"%S\" \t%u \t%u\n"), 
					iLogItemNo, &aDirName, measure, &scale, &aFileName, aCurrentFileNo, aCurrentFilePos);

	iLogItemNo++;

	return KErrNone;
	}



CFileOperationUnit::CFileOperationUnit()
:iDirCreated(EFalse)
	{

	}

// File Operation Unit
void CFileOperationUnit::ConstructL(const TFileOpUnitParam& aParam)
	{
	iDirName = aParam.iDirName;
	iNamingSchemeParam = aParam.iNamingSchemeParam;
	iFileOpMode = aParam.iFileOpMode;
	iMLUnitPtr = aParam.iMLUnitPtr;

	TInt rel = iRFs.Connect();
	if (rel != KErrNone)
		{
		test.Printf(_L("<<Error>>: DIRUNIT: iRFs.Connect = %d\n"), rel);
		test(EFalse);
		}
	}


CFileOperationUnit* CFileOperationUnit::NewLC(const TFileOpUnitParam& aParam)
	{
	CFileOperationUnit* self = new(ELeave) CFileOperationUnit();
	CleanupStack::PushL(self);
	self->ConstructL(aParam);
	return self;
	}

CFileOperationUnit* CFileOperationUnit::NewL(const TFileOpUnitParam& aParam)
	{
	CFileOperationUnit* self = CFileOperationUnit::NewLC(aParam);
	CleanupStack::Pop(self);
	return self;
    }

CFileOperationUnit::~CFileOperationUnit()
	{
	iRFs.Close();
	}

void CFileOperationUnit::SetMLUnit(CMeasureAndLogUnit* aMLUnit)
	{
	iMLUnitPtr = aMLUnit;
	}


// File Operation unit - Test Functions 
TInt CFileOperationUnit::Run(const TFileName& aDirName, const TFileName& aFileName, TBool aIsTakingMeasurement, TUint aCurFileNo, TUint aCurFilePos)
	{
	TMLParam mlParam;
	RFile	rfile;
	TInt	rel = KErrNone;
	TInt	rel1 = KErrNone; 
	TInt	bufMaxLength = 4096;
	TInt	Ret1 = KErrNone; 
	TInt	Ret2 = KErrNone; 
	RBuf8 	buf; 
	RBuf8	databuf;

		
	if (aIsTakingMeasurement && iMLUnitPtr != NULL)
		{
		mlParam.iDirName = iDirName;
		mlParam.iFileName = aFileName;
		mlParam.iNamingScheme = iNamingSchemeParam;
		mlParam.iCurFileNo = aCurFileNo;
		mlParam.iFileOpMode = iFileOpMode;
		}
	else if (aIsTakingMeasurement && iMLUnitPtr == NULL)
		{
		test.Printf(_L("<<Error>>CZFileOperationUnit::Run(): no logging unit associated!!\n"));
		return KErrGeneral;
		}

	switch (iFileOpMode)
		{
		case EFATPerfFileReplace:		// Replace Operations
			{
			test.Printf(_L("CZFileOperationUnit::Run(): EZFileReplace\n"));
			break;
			}
		case EFATPerfFileCreate:
			{
			if (!iDirCreated && gTestCase != EFATPerfCreate)  // 'EFATPerfCreate' is  an enum defined in header file t_fat_perf.h
				{
				rel = iRFs.MkDirAll(aFileName);
				test.Printf(_L("MakeDirAll \"%S\" error: %d\n"), &aFileName, rel);					
				if (rel != KErrNone && rel != KErrAlreadyExists)
					{
					test.Printf(_L("<<Error>>: MakeDirAll \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
				iDirCreated = ETrue;
				}
			if (aIsTakingMeasurement)
				{
				iMLUnitPtr->MeasureStart();
				rel = rfile.Create(iRFs, aFileName, EFileShareAny);
				iMLUnitPtr->MeasureEnd();
				iMLUnitPtr->Log(aDirName, aFileName, aCurFileNo, aCurFilePos);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileCreating \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
				}
			else
				{
				rel = rfile.Create(iRFs, aFileName, EFileShareAny);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileCreating error: %d\n"), rel);
					return rel;
					}
				}
			break;
			}
	
		case EFATPerfFileOpen:      // Open Operations
			{
			if (aIsTakingMeasurement)
				{
				iMLUnitPtr->MeasureStart();
				rel = rfile.Open(iRFs, aFileName, EFileShareAny);
				iMLUnitPtr->MeasureEnd();
				iMLUnitPtr->Log(aDirName, aFileName, aCurFileNo, aCurFilePos);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileOpen \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
				}
			else
				{
				rel = rfile.Open(iRFs, aFileName, EFileShareAny);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileOpen error: %d\n"), rel);
					return rel;
					}
				}
			break;
			}
		
	
		case EFATPerfFileDelete:     // Delete Operations
			{
			
		
			if (aIsTakingMeasurement)
				{
				iMLUnitPtr->MeasureStart();
				rel = iRFs.Delete(aFileName);
				iMLUnitPtr->MeasureEnd();
				iMLUnitPtr->Log(aDirName, aFileName, aCurFileNo, aCurFilePos);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileDelete \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
															
				}
			else
				{
			
				rel = rfile.Open(iRFs, aFileName, EFileShareAny);
				if (rel != KErrNone)
				{
					test.Printf(_L("<<Error>>: FileOpen \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
				rfile.Close(); // file needs be closed before  deleting.
				
				rel = iRFs.Delete(aFileName);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileDelete \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
						
				}
			break;
			}
		
			
			
		case EFATPerfFileWrite:			//Write Operations
			{
			
			// creating buffer for Write operation 
			Ret2 = databuf.CreateMax(bufMaxLength);
	
			if (Ret2 != KErrNone)
				{
				test.Printf(_L("<<Error>>: Unable to create a buffer 'databuf': %d\n"), Ret2);
				return Ret2;
				}
			
			databuf.Fill('A', bufMaxLength);	
	
					
			if (aIsTakingMeasurement)
				{
				
				rel = rfile.Open(iRFs, aFileName, EFileShareAny|EFileWrite);
				iMLUnitPtr->MeasureStart();
				rel1 = rfile.Write(databuf);
				iMLUnitPtr->MeasureEnd();
				iMLUnitPtr->Log(aDirName, aFileName, aCurFileNo, aCurFilePos);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileOpen \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
			
				if (rel1 != KErrNone)
				{
				test.Printf(_L("<<Error>>: FileWrite \"%S\" error: %d\n"), &aFileName, rel1);
				return rel1;
				}
								
				}
			else
				{
			
				rel = rfile.Open(iRFs, aFileName, EFileShareAny|EFileWrite);
				rel1 = rfile.Write(databuf);
				
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileOpen error: %d\n"), rel);
					return rel;
					}
				
				if (rel1 != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileWrite error: %d\n"), rel1);
					return rel1;
					}
				}
			break;
			}
		
		
		
		case EFATPerfFileRead:			// Read Operations
			{
			
			// creating the buffer for Read operation 
			Ret1 = buf.CreateMax(bufMaxLength);	
			if (Ret1 != KErrNone)
				{
				test.Printf(_L("<<Error>>: Unable to create a buffer 'buf': %d\n"), Ret1);
				return Ret1;
				}
			
		
			if (aIsTakingMeasurement)
				{
				
				rel = rfile.Open(iRFs, aFileName, EFileShareAny|EFileRead);
				iMLUnitPtr->MeasureStart();
				rel1 = rfile.Read(buf);
				iMLUnitPtr->MeasureEnd();
				iMLUnitPtr->Log(aDirName, aFileName, aCurFileNo, aCurFilePos);
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileOpen \"%S\" error: %d\n"), &aFileName, rel);
					return rel;
					}
				if (rel1 != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileRead \"%S\" error: %d\n"), &aFileName, rel1);
					return rel1;
					}
				}
			else
				{
				rel = rfile.Open(iRFs, aFileName, EFileShareAny|EFileRead);
				rel1 = rfile.Read(buf);
				
				if (rel != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileOpen error: %d\n"), rel);
					return rel;
					}
				
				if (rel1 != KErrNone)
					{
					test.Printf(_L("<<Error>>: FileRead \"%S\" error: %d\n"), &aFileName, rel1);
					return rel1;
					}
			 	}
			break;
			}
		
		default:
			{
			// Error: KErrNotSupported!!
			test.Printf(_L("<<Error>>CZFileOperationUnit::Run(): KErrNotSupported!!\n"));
			return KErrNotSupported;
			}
		 }

	rfile.Close();
	buf.Close(); 
	databuf.Close();
	return KErrNone;
	}



CDirUnit::CDirUnit()
	{

	}

void CDirUnit::ConstructL(const TDirUnitParam& aParam, const TChar aDriveChar)
	{
	iPriority = aParam.iPriority;
	iDirName.Copy(aParam.iDirName);
	iDirName[0] = (TUint16)aDriveChar;  
	iRuns = aParam.iRuns;
	iCurrentRunNo = 1;
	iFilesPerRun = aParam.iFilesPerRun;
	iCurrentFileNo = 1;
	iTotalFileNo = iRuns * iFilesPerRun;
	iSampleInterval = aParam.iSampleInterval;
	
	
	iConOrRan = aParam.iNamingScheme.iConOrRan;
	iUniOrAsc = aParam.iNamingScheme.iUniOrAsc;
	iZeroPadFileNumberForFixedLengthFileNames = aParam.iNamingScheme.iZeroPadFileNumber;
	iFileNameBase = aParam.iNamingScheme.iFileNameBase;
	iMaxFileNameLength = aParam.iNamingScheme.iMaxFileNameLength;
	iMinStringLength = aParam.iNamingScheme.iMinStringLength;

	if (iZeroPadFileNumberForFixedLengthFileNames)
	    {
	    // Calculate how many digits the highest file postfix will have so that
	    // zero padding can be added.
	    TFileName fileNamePostFixBuffer;
	    fileNamePostFixBuffer.AppendNum(iTotalFileNo);
	    iNumDigitsInTotalFileNo = fileNamePostFixBuffer.Length();
	    }
	else
	    {
	    iNumDigitsInTotalFileNo = 0;
	    }
	
	TFileOpUnitParam fileOpParam;
	fileOpParam.iDirName = iDirName;
	fileOpParam.iNamingSchemeParam = aParam.iNamingScheme;
	fileOpParam.iFileOpMode = aParam.iFileOpMode;
	fileOpParam.iMLUnitPtr = iMLUnitPtr;
	CFileOperationUnit* fileOpUnit = CFileOperationUnit::NewL(fileOpParam);
	iFileOpUnit = fileOpUnit;
	}

CDirUnit* CDirUnit::NewLC(const TDirUnitParam& aParam, const TChar aDriveChar)
	{
	CDirUnit* self = new(ELeave) CDirUnit();
	CleanupStack::PushL(self);
	self->ConstructL(aParam, aDriveChar);
	return self;
	}

CDirUnit* CDirUnit::NewL(const TDirUnitParam& aParam, const TChar aDriveChar)
	{
	CDirUnit* self = CDirUnit::NewLC(aParam, aDriveChar);
	CleanupStack::Pop(self);
	return self;
	}

CDirUnit::~CDirUnit()
	{
	delete iFileOpUnit;
	}

TInt CDirUnit::Priority()
	{
	return iPriority;
	}

const TFileName&	CDirUnit::Name()
	{
	return iDirName;
	}


_LIT(KFileExtDefault,	".TXT");

//Character set for random file generation
static TText16 gFileNameCharPool[] =
	{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
//	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',	
	'-', '_', '^', '$', '~', '!', '#', '%', '&', '{', '}', '@', '(', ')', '\'',
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	};

const TUint KNumOfLegalAsciiChar = 51;


// Random String Generation for file names
TFileName CDirUnit::GenerateRandomString(const TUint aMinStringLength, const TUint aMaxStringLength, const TNamingUniOrAsc aUniOrAsc)
	{
	TInt strLen = -1;
	TFileName aFileName;
	
	if (aMinStringLength == aMaxStringLength && aMaxStringLength != 0)
		{
		strLen = aMinStringLength;
		}
	else if (aMaxStringLength > aMinStringLength)
		{
		do
			{
			strLen = aMinStringLength + ((TUint) (Math::Random())) % (aMaxStringLength - aMinStringLength);
			}
		while (strLen == 0);
		}
	else
		{
		test.Printf(_L("<<Random String Error>>: Bad String Length Setting!!\n"));
		}
	
	if (aUniOrAsc == EAscii)
		{
		TInt i;
		for (i = 0; i < strLen; ++i)
			{
			TInt nextCharIdx = (((TUint) (Math::Random())) % KNumOfLegalAsciiChar);
			aFileName.Append((TChar)gFileNameCharPool[nextCharIdx]);
			}
		}
	else if(aUniOrAsc == EUnicode)
		{
		test.Printf(_L("<<Random String Error>>: Unicode is not supported yet!!\n"));
		}
    else 
        {
        test(0);
        }

	return aFileName;
	}

TBool CDirUnit::FileNameIsUnique(const TFileName& /*aFileName*/)
	{

	return ETrue;

	}

// File Name Generation
TInt CDirUnit::GenerateFileName(TFileName& aFileName)
	{
	TBool fileNameIsUnique = EFalse;
	if (iRuns == iFilesPerRun && iFilesPerRun == 1)
		{
		aFileName.Zero();
		aFileName.Append(iDirName);
		aFileName.Append(iFileNameBase);
		aFileName.Append(KFileExtDefault);
		fileNameIsUnique = FileNameIsUnique(aFileName);
		if (fileNameIsUnique == EFalse)
			{
			test.Printf(_L("<<Error>>: File name is not unique!\n"));
			return KErrArgument;
			}
		return KErrNone;
		}
	
	if (iConOrRan == EConsecutive)
		{
		aFileName.Zero();
		aFileName.Append(iDirName);
		aFileName.Append(iFileNameBase);
		
		if (iZeroPadFileNumberForFixedLengthFileNames)
		    {
		    aFileName.AppendNumFixedWidth(iCurrentFileNo, EDecimal, iNumDigitsInTotalFileNo);
		    }
		else
		    {
		    aFileName.AppendNum(iCurrentFileNo);
		    }

		aFileName.Append(KFileExtDefault);
		fileNameIsUnique = FileNameIsUnique(aFileName);
		if (fileNameIsUnique == EFalse)
			{
			test.Printf(_L("<<Error>>: File name is not unique!\n"));
			return KErrArgument;
			}
		return KErrNone;
		}
	else if(iConOrRan == ERandom)
		{
		if (iMaxFileNameLength <= 0)
			{
			test.Printf(_L("<<Parameter Error>>: DIR: \"%S\"\n"), &iDirName);
			test.Printf(_L("<<Parameter Error>>: EZRandom && iMaxNameLength <= 0\n"));
			return KErrArgument;
			}

		do
			{
			aFileName.Zero();
			aFileName.Append(iDirName);
			aFileName.Append(iFileNameBase);
			TFileName randomString = GenerateRandomString(iMinStringLength, iMaxFileNameLength, iUniOrAsc);
			aFileName.Append(randomString);
			aFileName.Append(KFileExtDefault);
			fileNameIsUnique = FileNameIsUnique(aFileName);
			}
		while(fileNameIsUnique == EFalse);
		
		return KErrNone;
		}

	return KErrNone;
	}

void CDirUnit::SetMLUnit(CMeasureAndLogUnit* aMLUnitPtr)
	{
	iMLUnitPtr = aMLUnitPtr;
	iFileOpUnit->SetMLUnit(aMLUnitPtr);
	}

TBool CDirUnit::CheckMeasurementTaking()
	{
	
	return (iSampleInterval > 0 && 
			iCurrentFileNo >= iSampleInterval && 
			((iCurrentFileNo % iSampleInterval) == 0));
	}

TInt CDirUnit::Run(const TInt aCurrentPriority)
	{
	if (aCurrentPriority != iPriority)
		{
		return KErrNotReady;
		}


	if (iCurrentRunNo <= iRuns)
		{
		
		TUint i;
		for (i = 0; i < iFilesPerRun; ++i, ++iCurrentFileNo)
			{
			// check currentFileNo < totalFileNo
			if (iCurrentFileNo > iTotalFileNo)
				{
				// Error
				User::Panic(_L("<<CZDirUnit::Run>>: file overflow!"), 100);
				}
			// generate file name
			TFileName fileName;
			GenerateFileName(fileName);
			
			// check if is taking measurement
			TBool isTakingMeasurement = CheckMeasurementTaking();

			// file operation
			iFileOpUnit->Run(iDirName, fileName, isTakingMeasurement, iCurrentFileNo, iCurrentFileNo);
			}
		iCurrentRunNo++;
		}

	if (iCurrentRunNo > iRuns)
		{
		return KErrCompletion;
		}
	return KErrNone;
	}



CExecutionUnit::CExecutionUnit()
	{

	}

void CExecutionUnit::ConstructL(CMeasureAndLogUnit* aMLUnitPtr, const TChar aDriveChar)
	{
	iCurPriority = -1;
	iDriveChar = aDriveChar;

	if (aMLUnitPtr != NULL)
		{
		iMLUnitPtr = aMLUnitPtr;
		}
	}

CExecutionUnit* CExecutionUnit::NewLC(CMeasureAndLogUnit* aMLUnitPtr, const TChar aDriveChar)
	{
	CExecutionUnit* self = new(ELeave) CExecutionUnit();
	CleanupStack::PushL(self);
	self->ConstructL(aMLUnitPtr, aDriveChar);
	return self;
	}

CExecutionUnit* CExecutionUnit::NewL(CMeasureAndLogUnit* aMLUnitPtr, const TChar aDriveChar)
	{
	CExecutionUnit* self = CExecutionUnit::NewLC(aMLUnitPtr, aDriveChar);
	CleanupStack::Pop(self);
	return self;
	}

CExecutionUnit::~CExecutionUnit()
	{
	TInt count = iDirUnitArray.Count();
	if (count > 0)
		{
		TInt i;
		for (i = 0; i < count; ++i)
			{
			CDirUnit* tempDirUnit = iDirUnitArray[i];
			delete tempDirUnit;
			}
		}
	iDirUnitArray.Close();
	}


TInt CExecutionUnit::AddDirUnitL(const TDirUnitParam& aParam)
	{
	CDirUnit* dirUnit = CDirUnit::NewL(aParam, iDriveChar);
	if (iMLUnitPtr)
		{
		dirUnit->SetMLUnit(iMLUnitPtr);
		}
	TInt rel = iDirUnitArray.Append(dirUnit);
	return rel;
	}


// To Re-calculate the priority
TInt CExecutionUnit::RecalculateCurrentPrioirty()
	{
	// findout the least number of iPriority in current array
	if (iDirUnitArray.Count() == 0)
		{
		iCurPriority = -1;
		return iCurPriority;
		}

	TBool found = EFalse;

	TInt i;
	
	for (i = 0; i < iDirUnitArray.Count(); ++i)
		{
		if (iDirUnitArray[i]->Priority() == iCurPriority)
			{
			found = ETrue;
			}
		}
	
	if (!found)
		{
		iCurPriority = iDirUnitArray[0]->Priority();
		for (i = 0; i < iDirUnitArray.Count(); ++i)
			{
			if (iDirUnitArray[i]->Priority() < iCurPriority)
				{
				iCurPriority = iDirUnitArray[i]->Priority();
				}
			}
		}

	return iCurPriority;
	}

TInt  CExecutionUnit::Run()
	{
	test.Printf(_L("CZExecutionUnit::Run()\n"));
	
	TInt curPriority = RecalculateCurrentPrioirty();
	
	while (iDirUnitArray.Count() > 0)
		{
		TInt i;
		for (i = 0; i < iDirUnitArray.Count(); ++i)
			{
			TInt rel = iDirUnitArray[i]->Run(curPriority);
			if (rel == KErrCompletion)
				{
				test.Printf(_L("DIR: \"%S\" terminated.\n"), &iDirUnitArray[i]->Name());
				CDirUnit* dirUnit = iDirUnitArray[i];
				iDirUnitArray.Remove(i);
				delete dirUnit;
				--i;
				curPriority = RecalculateCurrentPrioirty();
				}
			else if (rel == KErrNotReady)
				{
				// do nothing
            	}
			}
		}

	test.Printf(_L("\n"));
	test.Printf(_L("CZExecutionUnit::Finished.\n"));
	return KErrNone;
	}

/*-- EOF--*/
