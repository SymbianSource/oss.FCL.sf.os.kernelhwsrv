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



#include "t_fat32writeraw.h"

static RRawDisk TheDisk;

/**
Class Constructor
*/		    
CBaseTestFat32WriteRaw::CBaseTestFat32WriteRaw() 
	{
	SetTestStepName(KTestStepWriteRaw);
	}
	
/**
Class Destructor
*/
CBaseTestFat32WriteRaw::~CBaseTestFat32WriteRaw() 
	{
	}

/** 
This function performs the following actions:
1. Get the position at which to write on the raw disk from the ini file
	-> if a cluster is specified, get the position from the GetCluster function 
2. Get the number of bytes to write from the ini file 
3. Obtain the values to be written to the disk from the ini file. 
4. Write the values to the disk byte by byte.

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32WriteRaw::doTestStepL()
	{
	SetTestStepResult(EFail);
	_LIT(KPosition,"WritePosition");
	_LIT(KNumOfBytes,"NumberOfBytes");
	_LIT(KCluster,"ClusterNumber");
	TInt writevalues[12];
	TInt position; 
	TInt64 clusterPosition;
	TInt cluster; 
	TInt numbyte;
	TInt r;
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
		TBool alright2 = GetHexFromConfig(ConfigSection(), KNumOfBytes, numbyte);
		if (alright2)
			{
			TBool alright2 = GetIntFromConfig(ConfigSection(), KCluster, cluster);
			if (alright2)
				{
				r = GetCluster(cluster, clusterPosition);
				position = clusterPosition;
				}
			else
				{
				position = position;
				}
            _LIT(KFATEntryWritePosition,"FATEntryWritePosition");
            _LIT(KFATEntry0,"FATEntry0");
            _LIT(KFATEntry1,"FATEntry1");
            _LIT(KFATEntry2,"FATEntry2");
            _LIT(KFATEntry3,"FATEntry3");
            _LIT(KFATEntry4,"FATEntry4");
            _LIT(KFATEntry6,"FATEntry6");
            _LIT(KFATEntry18,"FATEntry18");
            _LIT(KFATEntry52,"FATEntry52");
            _LIT(KFATEntry78,"FATEntry78");

            
            TPtrC fat_entry_str;
            if(GetStringFromConfig(ConfigSection(), KFATEntryWritePosition, fat_entry_str))
            {
                if(!fat_entry_str.Compare(KFATEntry0))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec;
                }
                else if(!fat_entry_str.Compare(KFATEntry1))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4;
                }
                else if(!fat_entry_str.Compare(KFATEntry2))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*2;
                }
                else if(!fat_entry_str.Compare(KFATEntry3))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*3;
                }
                else if(!fat_entry_str.Compare(KFATEntry4))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*4;
                }           
                else if(!fat_entry_str.Compare(KFATEntry6))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*6;
                }
                else if(!fat_entry_str.Compare(KFATEntry18))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*18;
                }
                else if(!fat_entry_str.Compare(KFATEntry52))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*52;
                }
                else if(!fat_entry_str.Compare(KFATEntry78))
                {
                    position = iBPB_ResvdSecCnt * iBPB_BytsPerSec + 4*78;
                }   

            }
			_LIT(KWritePosition, "The position on the disk being written to is %d");
			INFO_PRINTF2(KWritePosition,position);
			r = GetWriteValue(numbyte,writevalues);
			TInt i;
			r = TheDisk.Open(iTheFs, CurrentDrive());
			if (r!=KErrNone)
				{
				_LIT(KErrorOpen, "Cannot open the raw disk - r=%d");
				INFO_PRINTF2(KErrorOpen, r);
				}
			for (i=0;i<numbyte;i++)
				{
				r = WriteRaw(position+i, writevalues[i]);
				if(r != KErrNone)
					{
					_LIT(KErrorRead, "Cannot write to the raw disk at position %d with value %d");
					INFO_PRINTF3(KErrorRead,position, writevalues[i]);
					SetTestStepResult(EFail);
					return TestStepResult();
					}
				}
			TheDisk.Close();
			}
		else
			{
			_LIT(KNoIniNumByte, "Cannot read the number of bytes from the ini file");
			INFO_PRINTF1(KNoIniNumByte);
			SetTestStepResult(EFail);
			return TestStepResult();		
			}
		}
	else
		{
		_LIT(KNoIniPos, "Cannot read the position from the ini file");
		INFO_PRINTF1(KNoIniPos);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}

	
	SetTestStepResult(EPass);
	_LIT(KWritePass, "Write Passed");
	INFO_PRINTF1(KWritePass);
	return TestStepResult();
	}

/** 
Writing a value to the raw disk

@param aPos The position from which to start writing to the raw disk
@param aValue The value to write to the raw disk

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteRaw::WriteRaw(TInt64 aPos,TInt aValue)
	{
//	int val = 0;
	TInt r;
	TUint8 data[1];
	TPtr8 buffer((TUint8*)&data[0],1);
	r = TheDisk.Read(aPos,buffer);
	data[0] = aValue;
	r = TheDisk.Write(aPos,buffer);
	if (r != KErrNone)
		{
		_LIT(KErrorWrite, "Cannot write to the raw disk - r=%d");
		INFO_PRINTF2(KErrorWrite,r);
		}		
	return r; 
	}
	
/** 
Get the values that are to be written to the disk byte by byte 
and place into an array

@param aNumberOfBytes the nummber of bytes to write to the raw disk
@param aValueArray Array containing the values to write to the raw disk

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteRaw::GetWriteValue(TInt aNumOfBytes,TInt* aValueArray)
	{
	TInt value;
	TInt i;
	_LIT(KWriteValues,"WriteValue%d");
	for (i = 1; i < aNumOfBytes + 1; i++)
		{
		TBuf<20> writeValue;
		writeValue.Format(KWriteValues, i);
		TBool alright3 = GetHexFromConfig(ConfigSection(), writeValue, value);
		if (alright3)
			{
			aValueArray[i-1] = value;
			}
		else
			{
			INFO_PRINTF1(_L("Cannot read WriteValue from ini file"));
			return KErrGeneral;
			}
		}
	return KErrNone;
	}

/** 
Get the position by calulating from the entry and position in the entry 
specified in the ini file

@param aClusterNumber The cluster on the disk to write to
@param aPosition the position within an entry

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteRaw::GetCluster(TInt aClusterNumber,TInt64 &aPosition)
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
				aPosition = (iBPB_ResvdSecCnt + (iBPB_FATSz32*2) + (iBPB_SecPerClus * (aClusterNumber - 2))) * 512;
				aPosition = aPosition + ((entry - 1)*96)+ entryposition;//(32 * (entry - 1));
				}
			}
	return KErrNone;
	}

