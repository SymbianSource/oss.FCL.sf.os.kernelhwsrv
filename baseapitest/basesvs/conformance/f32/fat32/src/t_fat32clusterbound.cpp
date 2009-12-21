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




#include "t_fat32clusterbound.h"

/**
Class Constructor
*/		    
CBaseTestFat32ClusterBound::CBaseTestFat32ClusterBound()
	{
	SetTestStepName(KTestStepClusterBound);
	}
	
/**
Class Destructor
*/	
CBaseTestFat32ClusterBound::~CBaseTestFat32ClusterBound() 
	{
	}


/** 
This test should be performed once the disk has been filled to its max capacity. 
Obtains the cluster bounds action from the configuration file and calls the function 
that is required to be carried out
 
@return EPass if test passes and EFail if test fails
*/ 		    
TVerdict CBaseTestFat32ClusterBound::doTestStepL()
	{
	SetTestStepResult(EFail);
	TInt r;
	TBufC<20> clusterAction;
	TPtrC16 clusteraction = clusterAction;
	_LIT(KClusterAction,"ClusterAction");
	TBool alright = GetStringFromConfig(ConfigSection(), KClusterAction, clusteraction);
	if (alright)		
		{			
		if (clusteraction == _L("WriteToFile"))
			{
			r = TestClusterBoundsWriteFile();
			if (r != KErrNone)
				{
				INFO_PRINTF1(_L("ClusterAction:WriteToFile failed"));
				SetTestStepResult(EFail);
				}
			else
				{
				INFO_PRINTF1(_L("ClusterAction:WriteToFile passed"));
				SetTestStepResult(EPass);
				}
			}
		if (clusteraction == _L("CreateFiles"))
			{
			r = TestClusterBoundsCreateFiles();
			if (r != KErrNone)
				{
				INFO_PRINTF1(_L("ClusterAction:CreateFiles failed"));
				SetTestStepResult(EFail);
				}
			else
				{
				INFO_PRINTF1(_L("ClusterAction:CreateFiles passed"));
				SetTestStepResult(EPass);
				}
			}
		}
	else 
		{
		INFO_PRINTF1(_L("No ClusterAction specified in ini file"));
		SetTestStepResult(EFail);
		}						
	return TestStepResult();
	}

/** 
Attepts to access out of bounds clusters by creating another file and 
attempting to write to it.

@return KErrNone if successfull, otherwise one of the other system-wide 
error codes.
*/
TInt CBaseTestFat32ClusterBound::TestClusterBoundsWriteFile()
{
	TInt r;
	TVolumeInfo iInfo;
	
	TInt freeSpace;
	RFile rFile;
	
	r = iTheFs.Volume(iInfo, CurrentDrive());
	if(r != KErrNone)
		{
		INFO_PRINTF2(_L("Unable to obtain the volume information, error = %d"), r);
		return r;
		}
	freeSpace = iInfo.iFree;
	TBuf8<100> buffer3(freeSpace + 1); 
	r = rFile.Replace(iTheFs, _L("TESTClusterBound.txt"), EFileWrite);
	if(r != KErrNone)
		{
		INFO_PRINTF2(_L("Unable to create the file, error = %d"), r);
		return r;
		}				
	r = rFile.Write(buffer3,freeSpace + 1);
	if (r == KErrDiskFull)
		{
		INFO_PRINTF1(_L("Disk is Full - Attempting to access out of bounds clusters returns the correct error value"));
		rFile.Close();
		return KErrNone;
		}
	else 
		{
		INFO_PRINTF2(_L("Attempting to access out of bounds clusters returns the incorrect error value %d"), r);
		rFile.Close();
		return r;
		}

}

/** 
Attepts to access out of bounds clusters by creating empty files to fill
the root directory and following this, attempting to create another empty file 

@return KErrNone if successfull, otherwise one of the other system-wide 
error codes.
*/
TInt CBaseTestFat32ClusterBound::TestClusterBoundsCreateFiles()
	{
	RFile rFile;
	_LIT(KFileName, "File%d.txt");
	
	TInt sizeOfShortDir = 16 * 4; //Short dir is 64 bytes long
	TInt bytesPerSector = iBPB_BytsPerSec;
	TInt sizeOfCluster = iBPB_SecPerClus * bytesPerSector;
	TInt filesToCreate = sizeOfCluster / sizeOfShortDir;
	filesToCreate = filesToCreate - 1;
	
	TInt r;
	TInt i;
	for (i = 0; i < filesToCreate; i++)
		{
		TBuf<20> fileName;
		TInt fileNumber = i + 1;
		fileName.Format(KFileName, fileNumber);
		r= rFile.Replace(iTheFs,fileName , EFileWrite);
		if (r == KErrDiskFull)
		break;
		rFile.Close(); 
		}
	

	r = rFile.Replace(iTheFs, _L("TFile.txt") , EFileWrite);
	if (r == KErrDiskFull)
		{
		INFO_PRINTF1(_L("Attempting to access out of bounds clusters returns the correct error value"));
		return KErrNone;
		}
	else 
		{
		INFO_PRINTF2(_L("Attempting to access out of bounds clusters returns the incorrect error value %d"), r);
		return r;
		}
	}
