/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/




#include "t_fat32readraw.h"

static RRawDisk TheDisk;

/**
Class Constructor
*/		    
CBaseTestFat32ReadRaw::CBaseTestFat32ReadRaw() 
	{
	SetTestStepName(KTestStepReadRaw);
	}
	
/**
Class Destructor
*/
CBaseTestFat32ReadRaw::~CBaseTestFat32ReadRaw() 
	{
	}


/** 
This function performs the following actions:
1. Gets the position to read on the raw disk from the ini file
	-> if a cluster is specified then get the position from GetCluster(). 
2. Get the number of bytes to read starting from the obtained position 
3. Read the disk byte by byte and place into an array. 
	-> If a mask is specified then compare the mask with the read value. 
	-> If the correct results are specified in the ini file then compare 
	   the array contain the read values with the array containing the 
	   correct results.

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32ReadRaw::doTestStepL()
	{
	SetTestStepResult(EFail);

	TInt ascivalue[20];
	TInt correctresults[20];
	TInt r; 
	
	_LIT(KPosition,"ReadPosition");
	TInt position; 
	TBool alright = GetHexFromConfig(ConfigSection(), KPosition, position);
	if(alright)
		{
		if (iMedia == 1 && position >= 16384)
			{
			INFO_PRINTF2(_L("Position = %d"), position);
			INFO_PRINTF2(_L("iBPB_ResvdSecCnt = %d"), iBPB_ResvdSecCnt);
			position = (iBPB_ResvdSecCnt * 512) + (position - 0x4000);
			INFO_PRINTF2(_L("New Position = %d"), position);
			}
		_LIT(KCluster,"ClusterNumber");
		TInt64 clusterPosition;
		TInt cluster; 
		TBool alright2 = GetIntFromConfig(ConfigSection(), KCluster, cluster);
		if (alright2)
			{
			r = GetCluster(cluster, clusterPosition);
			if (r != KErrNone)
				{
				SetTestStepResult(EFail);
				INFO_PRINTF1(_L("Could not get disk position"));
				return TestStepResult();
				}
			else
				{
				position = clusterPosition;
				} 
			}
			
		_LIT(KFATEntryReadPosition,"FATEntryReadPosition");
        _LIT(KFATEntry0,"FATEntry0");
		_LIT(KFATEntry1,"FATEntry1");
		_LIT(KFATEntry3,"FATEntry3");
		_LIT(KFATEntry4,"FATEntry4");
        TPtrC fat_entry_str;
        if(GetStringFromConfig(ConfigSection(), KFATEntryReadPosition, fat_entry_str))
        {
            if(!fat_entry_str.Compare(KFATEntry0))
            {
                position = iBPB_ResvdSecCnt * iBPB_BytsPerSec;
            }
            else if(!fat_entry_str.Compare(KFATEntry1))
            {
                position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4;
            }
            else if(!fat_entry_str.Compare(KFATEntry3))
            {
                position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*3;
            }
            else if(!fat_entry_str.Compare(KFATEntry4))
            {
                position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*4;
            }           
        }
		_LIT(KReadPosition, "The position on the disk being read is %d");
		INFO_PRINTF2(KReadPosition,position);

		_LIT(KNumOfBytes,"NumberOfBytes");
		TInt numbyte;
		TBool alright3 = GetHexFromConfig(ConfigSection(), KNumOfBytes, numbyte);
		if (alright3)
			{
			r = ReadRaw(position, numbyte, ascivalue);
			if(r != KErrNone)
				{
				_LIT(KErrorRead, "Cannot read the raw disk at position %d and length %d - error = %d");
				INFO_PRINTF4(KErrorRead,position, numbyte, r);
				SetTestStepResult(EFail);
				return TestStepResult();
				}
			_LIT(KMask,"Mask");
			TInt mask;
			TBool alright4 = GetHexFromConfig(ConfigSection(), KMask, mask);
			if (alright4)
				{
				r = CheckMask(mask,numbyte, ascivalue, position);
				if (r == KErrNone)
				{
				_LIT(KValueCorrect, "Result Correct");
					INFO_PRINTF1(KValueCorrect);
					SetTestStepResult(EPass);
					_LIT(KReadPass, "Read Passed");
					INFO_PRINTF1(KReadPass);
					return TestStepResult();
					}
				else
					{
					_LIT(KValueWrong, "Result Incorrect, value is %X but should be %X");
					INFO_PRINTF3(KValueWrong, ascivalue[0], mask);
					SetTestStepResult(EFail);
					_LIT(KReadFail, "Read Failed");							
					INFO_PRINTF1(KReadFail);
					return TestStepResult();
					}
				}
			else
				{
				r = GetCorrectResult(numbyte,correctresults);
				TInt i;
				for (i = 0; i < numbyte; i++)
					{
					if (ascivalue[i] != correctresults[i])
						{
						if ((ascivalue[i] == 0xFF) && (correctresults[i] == 0x00))
							{
							_LIT(KValueCorrect, "Result Correct");
							INFO_PRINTF1(KValueCorrect);
							SetTestStepResult(EPass);
							_LIT(KReadPass, "Read Passed");
							INFO_PRINTF1(KReadPass);
							return TestStepResult();
							}
						else
							{
							_LIT(KValueWrong, "Result Incorrect, value is %X but should be %X");
							INFO_PRINTF3(KValueWrong, ascivalue[i], correctresults[i]);
							SetTestStepResult(EFail);
							_LIT(KReadFail, "Read Failed");
							INFO_PRINTF1(KReadFail);
							return TestStepResult();
							}
						}
					else
						{
						_LIT(KValueCorrect, "Result Correct");
						INFO_PRINTF1(KValueCorrect);
						SetTestStepResult(EPass);
						_LIT(KReadPass, "Read Passed");
						INFO_PRINTF1(KReadPass);
						return TestStepResult();
						}
					}
				}
			}
			else
			{
			_LIT(KErrorRead, "Cannot read the ini file Number of Bytes ");
			INFO_PRINTF3(KErrorRead,position, numbyte);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		}
	else
		{
		_LIT(KErrorRead, "Cannot read the ini file Read Position ");
		INFO_PRINTF1(KErrorRead);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}
	return TestStepResult();
	}


/** 
Read from the raw disk byte by byte and place the results into an array

@param aPos The position from which to start reading the raw disk
@param aNumberOfBytes The number of bytes to read from the raw disk
@param aAsciiValue Array containing the values that are read from the raw disk

@return KErrNone if successfull
*/
TInt CBaseTestFat32ReadRaw::ReadRaw(TInt64 aPos,TInt aNumberOfBytes,TInt *aAsciiValue)
	{
	TInt r;
	r = TheDisk.Open(iTheFs, CurrentDrive());
		if (r != KErrNone)
		{
		_LIT(KErrorRead, "Cannot open the raw disk - r=%d");
		INFO_PRINTF2(KErrorRead, r);
		return r;
		}
	
	TUint8 data[20];
	TPtr8 buf(&data[0], 20);
	r = TheDisk.Read(aPos, buf);
	if (r != KErrNone)
		{
		_LIT(KErrorRead, "Cannot read the raw disk - r=%d");
		INFO_PRINTF2(KErrorRead, r);
		return r;
		}
	TInt i;	
	for (i = 0; i < aNumberOfBytes; i++)
		{
		aAsciiValue[i] = data[i];
		}
	TheDisk.Close();	
	return r; 
	}

/** 
Get the correct result value byte by byte and place into an array

@param aNumberOfBytes The number of bytes to read from the raw disk
@param aCorrectResultArray Array containing the values that are expected

@return KErrNone if successful
*/

TInt CBaseTestFat32ReadRaw::GetCorrectResult(TInt aNumOfBytes,TInt* aCorrectResultArray)
	{
	TInt result;
	TInt i;
	_LIT(KCorrectResult,"CorrectResult%d");
	for (i=1; i<aNumOfBytes+1; i++)
		{
		TBuf<20> a;
		a.Format(KCorrectResult, i);
		TBool alright3 = GetHexFromConfig(ConfigSection(), a, result);
		if (alright3)
			{
			aCorrectResultArray[i-1] = result;
			}
		else
			{
			INFO_PRINTF1(_L("Could not get correct result from ini file"));
			return KErrGeneral;
			}
		}
	return KErrNone;
	}

/** 
Get the position by calulating from the entry and position in the entry 
specified in the ini file 


@param aClusterNumber The cluster number on the raw disk
@param aPosition The position from which to start reading the raw disk

@return KErrNone if successful
*/	
TInt CBaseTestFat32ReadRaw::GetCluster(TInt aClusterNumber,TInt64 &aPosition)
	{
	TInt entry;
	_LIT(KEntry,"Entry");
	TInt entryposition;
	_LIT(KPositionInEntry,"PositionInEntry");
	TBool alright = GetIntFromConfig(ConfigSection(), KEntry, entry);
	if (alright)
		{
		TBool alright2 = GetIntFromConfig(ConfigSection(), KPositionInEntry, entryposition);
		if (alright2)
			{
			// Calculating the byte number on the disk when given the 
			// cluster number, the entry number and the position in the entry
			TInt sizeOfEntry = 96;
			aPosition = (iBPB_ResvdSecCnt + (iBPB_FATSz32 * 2) + (iBPB_SecPerClus * (aClusterNumber - 2))) * 512;
			aPosition = aPosition + ((entry - 1) * sizeOfEntry) + entryposition;
			return KErrNone;
			}
		else
			{
			INFO_PRINTF1(_L("Could not read position in entry from ini file"));
			return KErrGeneral;
			}
		}
	else 
		{
		INFO_PRINTF1(_L("Could not read entry number from ini file"));
		return KErrGeneral;
		}
	}

/** 
Check if the mask value is correct with the mask obtained from the ini file

@param aMask Value of the mask
@param aNumberOfBytes The number of bytes to read from the raw disk
@param aAsciiValue Array containing the values that are read from the raw disk
@param aPos The position on the raw disk

@reutrn KErrNone is successfull
*/
TInt CBaseTestFat32ReadRaw::CheckMask(TInt aMask, TInt aNumOfBytes, TInt *aAsciiValue, TInt64 aPos)
	{
	TUint32 readValue;
	TInt maskreturn;
	_LIT(KMaskReturn,"MaskReturn");
	TInt i;
	
	readValue = 0;
	for (i=aNumOfBytes-1; i>=0; i--)
		{

		readValue = readValue << 8;
		readValue = readValue + aAsciiValue[i];
		}
	// Special case for the extension flag field. Checks whether mirroring is
	// enabled or disabled and whether is contains the correct value. Position
	// on the disk for this fiels is 0x29
	if ((aPos == 0x29) && (aMask == 0))
		{
		if(readValue != 0)
			{
			aMask = 0x80;
			if ((readValue&aMask) == (TUint32)aMask)
				{
				aMask = 0x0D;
				if ((aMask&readValue) == 0)
					{
					INFO_PRINTF1(_L("Mirroring is DISABLED and the active FAT is correct"));
					return KErrNone;
					}
				else 
					{
					INFO_PRINTF1(_L("Mirroring is DISABLED and the active FAT is incorrect"));
					return KErrGeneral;
					}
				}
			}
		else 
			{
			INFO_PRINTF1(_L("Mirroring is ENABLED and the field is 0"));
			return KErrNone;
			}
		}
	else
		{
		TBool alright = GetIntFromConfig(ConfigSection(), KMaskReturn, maskreturn);
		if (alright)
			{
			if (maskreturn == 0)
				{
				if ((readValue&aMask) == 0)
					{
					return KErrNone;
					}
				else 
					{
					return KErrGeneral;
					}
				}
			else
				{
				if ((readValue&aMask) == (TUint32)aMask)
					{
					return KErrNone;
					}
				else 
					{
					return KErrGeneral;
					}
				}
			}
		}
	return KErrNone;
	}
