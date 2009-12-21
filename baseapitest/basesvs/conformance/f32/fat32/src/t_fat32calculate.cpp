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



#include "t_fat32calculate.h"
#include "sl_bpb1.h"

static RRawDisk TheDisk;

/**
Class Constructor
*/		    
CBaseTestFat32Calculate::CBaseTestFat32Calculate() 
	{
	SetTestStepName(KTestStepCalculate);
	}
	
/**
Class Destructor
*/
CBaseTestFat32Calculate::~CBaseTestFat32Calculate() 
	{
	}


/** 
Get the calculation to be performed from the ini file and carry out that
particular calculation
1. 	If SetGreater is specified in the ini file then the function SetToGreater 
	is called the set the cluster count to a value greater than it actually is
2. 	If ComputeFatSize is specified in the ini file then the function 
	ComputeFatSize is called to computer the size of each FAT
3. 	If ClusterCount is specified in the ini file, then the function 
	ClusterCount is called to return the clutser count of the volume

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32Calculate::doTestStepL()
	{
	SetTestStepResult(EFail);
	_LIT(KCalculation,"Calculation");
	TBufC<6> calculationType;
	TPtrC16 calculation = calculationType;	
	TBool alright = GetStringFromConfig(ConfigSection(), KCalculation, calculation);
	if(alright)
		{
		if (calculation == _L("SetGreater"))
			{
			TInt r = SetToGreater(iClusterCount, 1000);
			if (r != KErrNone)
				{
				_LIT(KFailClusterCount, "Failed to Set to a greater cluster count - r=%d");
				INFO_PRINTF2(KFailClusterCount, r);
				return TestStepResult();
				}	
			}
		if (calculation == _L("ComputeFatSize"))
			{
			TInt r = ComputeFatSize();
			if (r != KErrNone)
				{
				_LIT(KFailComputeFatSz, "Failed to compute FatSz - r=%d");
				INFO_PRINTF2(KFailComputeFatSz, r);
				return TestStepResult();
				}	
			}
		if (calculation == _L("ClusterCount"))
			{
			TInt r = CheckClusterCount(iClusterCount);
			if (r != KErrNone)
				{
				_LIT(KFailComputeClusCnt, "Failed to compute ClusterCount - r=%d");
				INFO_PRINTF2(KFailComputeClusCnt, r);
				return TestStepResult();
				}	
			}
		if (calculation == _L("CheckFSInfo"))
			{
			TInt r = CheckFSInfo(0x3E8);
			if (r != KErrNone)
				{
				_LIT(KFailCheckFSInfo, "Failed FSInfo check - r=%d");
				INFO_PRINTF2(KFailCheckFSInfo, r);
				return TestStepResult();
				}	
			}
		}
	else
		{
		_LIT(KIniFileError, "Failed to read calculation type from ini file");
		INFO_PRINTF1(KIniFileError);
		return TestStepResult();
		}
	
	SetTestStepResult(EPass);
	_LIT(KWritePass, "Calculation Pass");
	INFO_PRINTF1(KWritePass);
	return TestStepResult();
	}

/** 
Set the cluster count to a value greater than it should be
The bad cluster count is caluclated as 10000 added to the actual
cluster count value. The bad cluster count is then written to the disk 
in place of the actual cluster count. 

@param aClusterCount The cluster count of the volume
@param aPos The position at which to start writing to the raw disk

@return KErrNone if successful
*/	
TInt CBaseTestFat32Calculate::SetToGreater(TUint32 aClusterCount, TInt aPos)
	{
	TUint32 badClusterCount;
	badClusterCount = aClusterCount+10000;
	TUint8 num[4];
	TInt i;
	TInt r;

	r = TheDisk.Open(iTheFs, CurrentDrive());
	for (i=0; i<4; i++)
		{

		num[i] = 0;
		num[i] = num[i] + badClusterCount;
		badClusterCount = badClusterCount >> 8;
		}

	TUint8 data[1];
	TPtr8 buffer((TUint8*)&data[0],1);
	
	for (i=0; i<4; i++)
		{
		aPos = aPos + 1;
		r=TheDisk.Read(aPos,buffer);
		data[0]=num[i];
		r=TheDisk.Write(aPos,buffer);
		}
	TheDisk.Close();
	return r;
	}

/** 
Calculate the size of each FAT and verify that it is equal the 
value in BPB_FATSz32. The 'BPB_..' values are read directly from the disk
and the calculation is taken from the Microsoft FAT32 speification document

@return KErrNone if successful
*/		
TInt CBaseTestFat32Calculate::ComputeFatSize()
	{		
	TInt TmpVal1 = (iDriveSize/512) - iBPB_ResvdSecCnt;
	TInt TmpVal2 = (256 * iBPB_SecPerClus) + iBPB_NumFATs;
	TmpVal2 = TmpVal2 / 2;
	TInt FatSz32 = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
	INFO_PRINTF2(_L("FatSz32 = %d"), FatSz32);
	if (iBPB_FATSz32 == (TUint32)FatSz32) 
		{
		_LIT(KFatSzPass, "BPB_FATSz32 = %d is equal to calculated value = %d");
		INFO_PRINTF3(KFatSzPass,iBPB_FATSz32, FatSz32 );
		return KErrNone;
		}
	else
		{
		_LIT(KFatSzFail, "BPB_FATSz32 = %d is not equal to the calculated value = %d");
		INFO_PRINTF3(KFatSzFail, iBPB_FATSz32,FatSz32);
		return -1;
		}
	}

/** 
Verify that the cluster count is greater than 65525. 
If the volume is FAT32 then it should have a cluster count greater than
65525. Otherwise the volume should be FAT16. 

@param aClusterCount The cluster count of the volume

@return KErrNone if successful
*/			
TInt CBaseTestFat32Calculate::CheckClusterCount(TUint32 aClusterCount)
	{
	if (aClusterCount > 65525)
		{
		_LIT(KClusCntPass, "Cluster count is greater than 65525");
		INFO_PRINTF1(KClusCntPass);
		return KErrNone;
		}
	else
		{
		_LIT(KClusCntFail, "Cluster count is not greater than 65525");
		INFO_PRINTF1(KClusCntFail);
		return -1;
		}
	}
/** 
Read the FSInfo sector at the FSI_Free_Count field. 
Following this check that the last known cluster count is less than 
the actual cluster count of the volume. 

Note: This function should be called after files have been written 
to the disk to ensure that clusters have been occupied

@param aPos The position at which to start reading from the raw disk

@return KErrNone if successful
*/	
TInt CBaseTestFat32Calculate::CheckFSInfo(TInt aPos)
	{
	TUint32 fsInfo;

	TUint32 data;
	TPtr8 buffer((TUint8*)&data,4);
	
	TInt r = TheDisk.Open(iTheFs, CurrentDrive());
	r = TheDisk.Read(aPos,buffer);
	
	fsInfo = data; 
	
	TheDisk.Close();
	
	if (fsInfo < iClusterCount)
		{
		_LIT(KFSInfoPass, "FSInfo Correct, iClusterCount = %08x fsInfo = %08x fsInfo = %08x");
		INFO_PRINTF4(KFSInfoPass, iClusterCount, data, fsInfo);
		return KErrNone;
		}
	else
		{
		_LIT(KFSInfoFail, "FSInfo incorrect, iClusterCount = %08x fsInfo = %08x");
		INFO_PRINTF3(KFSInfoFail, iClusterCount, fsInfo);
		return -1;
		}
	}
