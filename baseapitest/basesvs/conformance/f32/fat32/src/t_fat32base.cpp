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



#include "t_fat32base.h"
#include "sl_bpb1.h"
#include "basetedefs.h"
#include <f32file.h>

static RRawDisk TheDisk;
static  TFatBootSector TheBootSector;

TDiskType gDiskType = EFatUnknown;
TInt gBytesPerCluster;
TInt gEntriesPerCluster;
TInt gDataStartBytes;
TInt gRootDirSectors;
TInt gTotalSectors;
TInt gRootDirStart;
TInt gRootSector;
TInt gRootCluster;
TInt gFatStartBytes;
TInt gFatTestEntries;
TInt gFatSizeSectors;
TInt gFirstDataSector;
TInt gFirstDataCluster;
TInt gMaxDataCluster;
TInt gClusterCount;
TInt gEndOfChain;      

/**
Class Constructor
*/		    
CBaseTestFat32Base::CBaseTestFat32Base() 
	{
	SetTestStepName(KTestStepBase);
	}
	
/**
Class Destructor
*/		    
CBaseTestFat32Base::~CBaseTestFat32Base() 
	{
	}


/** 
Base step to all test steps which carries out the followng actions:
1. Gets the drive letter
2. Checks that it is not a RAM drive
3. Obtains the size of the disk
4. Ensures that the volume is either FAT or FAT32
5. Sets the session path
6. Ensures that the number of sectors per cluster is correct
7. Calculates the cluster count of the volume. 

@return EPass if test passes and EFail if test fails
*/ 

TVerdict CBaseTestFat32Base::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	RDebug::Printf("Starting next test step...");
	CheckDebug(); 
	ParseCommandArguments();
	iTheFs.Connect();
	TInt drvNum = CurrentDrive();
	if (!IsFileSystemFAT32() && !IsFileSystemFAT(iTheFs,drvNum))
		{
		_LIT(KNotFat, "Test requires FAT filesystem");
		INFO_PRINTF1(KNotFat);
		return TestStepResult();
		}
	
	
	// check this is not the internal ram drive and get the size of the disk
	TVolumeInfo volumeInfo;
	iTheFs.Volume(volumeInfo, CurrentDrive());
	TBool isRamDrive = volumeInfo.iDrive.iMediaAtt&KMediaAttVariableSize;
	iDiskSize = volumeInfo.iSize;

	// Obtain the Size of the disk from the media driver
	TBusLocalDrive mmcDrive;
	TBool changeFlag;
	changeFlag=EFalse;
	TInt r = mmcDrive.Connect(1,changeFlag);
	TTime startTime;
	startTime.HomeTime();
	TLocalDriveCapsV2 information;
	TPckg<TLocalDriveCapsV2> pckg(information);
	r = mmcDrive.Caps(pckg);
	iDriveSize = information.iSize;


	// Set the Session Path	
	_LIT(Ksp, "%c:\\");
	TBuf<4> sessionPath;
	sessionPath.Format(Ksp, (TUint)iDriveToTest);
	r = iTheFs.SetSessionPath(sessionPath);
	r = iTheFs.SessionPath(iSessionPath);
	 
	

	CalculateClusCount();
	
	// Check the Sectors per cluster and Cluster count	
	r = CheckSecPerClus();
	if (r != KErrNone)
		{
		INFO_PRINTF1(_L("WARNING: Sector Per Cluster check was incorrect - Card should have special format requirements"));
		iMedia = 1;
		}
	else
		{
		INFO_PRINTF1(_L("Sector Per Cluster check is correct"));
		iMedia = 0;
		}
	
	if (isRamDrive)
		{
		SetTestStepResult(EAbort);
		_LIT(KRamNotValid, "Tests not valid on internal ram drive ");
		INFO_PRINTF1(KRamNotValid);
		return TestStepResult();
		}
	else
	SetTestStepResult(EPass);
	return TestStepResult();
	}

/**
Get the drive letter from the ini file
*/
void CBaseTestFat32Base::ParseCommandArguments(void)
	{
	TBufC<2> driveLetter;
	TPtrC16 letter = driveLetter;
	_LIT(KLetter,"DriveLetter");
	_LIT(KCommon,"Common");
	TBool alright = GetStringFromConfig(KCommon, KLetter, letter);
	if (alright)		
		{			
		iDriveToTest = letter[0];
		iDriveToTest.UpperCase();
		}
	
	}

/** 
Sets the debug register value for debug builds if specified in the ini file
*/	
void CBaseTestFat32Base::CheckDebug()
	{
	TInt debug;
	_LIT(KDebug,"SetDebug");
	_LIT(KCommon,"Common");
	TBool alright = GetIntFromConfig(KCommon, KDebug, debug);
	if (alright)		
		{
		if (debug == 1)
			{
			#ifdef _DEBUG			
			iTheFs.SetDebugRegister(KFSYS);
			#else
			_LIT(KNotDebug,"This is not a Debug build");
			INFO_PRINTF1(KNotDebug);
			#endif
			}
		else
			{
			_LIT(KSpecifyDebug,"Debugging not specified in ini file");
			INFO_PRINTF1(KSpecifyDebug);
			}
		}
	
	}
/** 
Converts the drive letter to the drive number

@return driveNum The number of the drive
*/	
TInt CBaseTestFat32Base::CurrentDrive()
	{
	TInt driveNum;
	iTheFs.CharToDrive(iDriveToTest,driveNum);
	return(driveNum);
	}
	
	
/** 
Check to see whether the volume is FAT32

@return TRUE if is Fat32 else FALSE
*/	
TBool CBaseTestFat32Base:: IsFileSystemFAT32()
	{
//	_LIT(KFat32Name,"Fat32");
	if(TheBootSector.RootDirEntries() == 0)
		{
		gEndOfChain = 0x0FFFFFFF;
		_LIT(KIsFat32, "Is Fat32 filesystem");
		INFO_PRINTF1(KIsFat32);
		return TRUE;
		}
	else
		{
		_LIT(KIsNotFat32, "Is Not Fat32 filesystem");
		INFO_PRINTF1(KIsNotFat32);
		return FALSE;
		}
	}

/** 
Check to see whether the volume is FAT

@return TRUE if is Fat else FALSE
*/	
TBool CBaseTestFat32Base::IsFileSystemFAT(RFs &aFsSession,TInt aDrive)
	{
	_LIT(KFatName,"Fat");
	TFileName f;
	TInt r = aFsSession.FileSystemName(f,aDrive);
	if (r != KErrNone)
		{
		_LIT(KNoFatName, "Unable to get file system name");
		INFO_PRINTF1(KNoFatName);
		return FALSE;
		}
	return (f.CompareF(KFatName) == 0);
	}


/** 
Check that the sectors per cluster is correct for the disk size used

@return KErrNone if Sectors Per Cluster is correct
*/	
TInt CBaseTestFat32Base::CheckSecPerClus()
	{
	TInt64 diskSizeSec = iDiskSize / 512;
	if (diskSizeSec <= 16777216)
		{
		if (iBPB_SecPerClus == 8)
		return KErrNone;
		else return -1;
		}
	else if (diskSizeSec <= 33554432)
		{
		if (iBPB_SecPerClus == 16)
		return KErrNone;
		else return -1;
		}
	else if (diskSizeSec <= 67108864)
		{
		if (iBPB_SecPerClus == 32)
		return KErrNone;
		else return -1;
		}
	else if (diskSizeSec <= 0xFFFFFFFF)
		{
		if (iBPB_SecPerClus == 64)
		return KErrNone;
		else return -1;
		}
	return KErrNone;
	}


/**
Calculate the cluster count of the volume

@return KErrNone Successful
*/	
TInt CBaseTestFat32Base::CalculateClusCount()
	{
	TInt r;
	r = TheDisk.Open(iTheFs, CurrentDrive());
	if (r != KErrNone)
		{
		_LIT(KErrorRead, "Cannot open the raw disk - r=%d");
		INFO_PRINTF2(KErrorRead, r);
		return r;
		}

	r = ReadField(4, 32,  &iBPB_TotSec32); 
	if (r != KErrNone)
		{
		_LIT(KErrorRead, "Cannot get the field value BPB_TotSec32 - r=%d");
		INFO_PRINTF2(KErrorRead, r);
		return r;
		}
	r = ReadField(2, 14,  &iBPB_ResvdSecCnt);
	if (r != KErrNone)
		{
		_LIT(KErrorReadField, "Cannot get the field value BPB_ResvdSecCnt - r=%d");
		INFO_PRINTF2(KErrorReadField, r);
		return r;
		}
	r = ReadField(1, 16,  &iBPB_NumFATs);
	if (r != KErrNone)
		{
		_LIT(KErrorReadField, "Cannot get the field value BPB_NumFATs - r=%d");
		INFO_PRINTF2(KErrorReadField, r);
		return r;
		}
	r = ReadField(4, 36,  &iBPB_FATSz32);
	if (r != KErrNone)
		{
		_LIT(KErrorReadField, "Cannot get the field value BPB_FATSz32 - r=%d");
		INFO_PRINTF2(KErrorReadField, r);
		return r;
		}
	r = ReadField(1, 13,  &iBPB_SecPerClus);
	if (r != KErrNone)
		{
		_LIT(KErrorReadField, "Cannot get the field value BPB_SecPerClus - r=%d");
		INFO_PRINTF2(KErrorReadField, r);
		return r;
		}
	r = ReadField(2, 11,  &iBPB_BytsPerSec);    
    if (r != KErrNone)
        {
		_LIT(KErrorReadField, "Cannot get the field value iBPB_BytsPerSec - r=%d");
		INFO_PRINTF2(KErrorReadField, r);
		return r;         
        }
	iClusterCount = (iBPB_TotSec32-(iBPB_ResvdSecCnt+(iBPB_NumFATs*iBPB_FATSz32)))/(iBPB_SecPerClus);
	_LIT(KClusterCountValue, "The cluster count of the volume is %d");
	INFO_PRINTF2(KClusterCountValue, iClusterCount);	
	TheDisk.Close();
	return KErrNone;	
	}

/** 
Convert the field to the correct endien format

@param aLen The length of the field being read
@param aBuffer The original endien format of the field
@param aField the correct endien format of the field

@return KErrNone Successful
*/	
TInt CBaseTestFat32Base::Convert(TInt aLen, TUint8 *aBuffer, TUint32 *aField)
	{
	TInt i = aLen-1;
	*aField = 0;
	while(i >= 0)
		{
		*aField = *aField << 8;
		*aField = *aField + aBuffer[i];
		i--;
		}
	return KErrNone;
	}

/** 
Read the entire field

@param aLen The length of the field being read
@param aOffSt The position at which to start reading
@param aName The entire field to read

@return KErrNone Successful
*/		
TInt CBaseTestFat32Base::ReadField(TInt aLen, TInt aOffSet, TUint32 *aName)
	{
	TInt r;
	TUint8  aField[4];
	TPtr8 bufPtr(&aField[0], aLen);
	r=TheDisk.Read(aOffSet, bufPtr);
	if (r != KErrNone)
		{
		_LIT(KErrorRead, "Cannot read the raw disk - r=%d");
		INFO_PRINTF2(KErrorRead, r);
		return r;
		}
	r = Convert(aLen, aField,aName); 
	return r;
	}


/** 
Get the number of bytes per sector

@param aSecPerCluster The number of sectors per cluster

@return The number of bytes per cluster
*/		
TInt64 getBytesPerCluster(TUint32 aSecPerClus)
	{
	TInt64 res = aSecPerClus * 512;
	return res;
	}

/** 
Convert the cluster number to byte offset on disk

@param aCluster The cluster number 

@return The offset in bytes
*/
TInt64 CBaseTestFat32Base::ClusterToByte(TInt aCluster)
	{
	TInt64 FirstDataSector = (iBPB_ResvdSecCnt + (iBPB_NumFATs * iBPB_FATSz32));
	TInt64 pos;
	pos = ((aCluster - 2) * iBPB_SecPerClus + FirstDataSector) * 512 ;
	return pos;
	}

/** 
Return number of bytes into the FAT

@param aFatIndex The Fat index 

@return The Fat position in bytes
*/
TInt CBaseTestFat32Base::PosInBytes(TInt aFatIndex)
	{
	TInt fatPosInBytes = -1;
	if(IsFileSystemFAT32())
		{
		gDiskType = EFat32;
		}	
	switch (gDiskType)
		{
		case EFat32:
			fatPosInBytes=aFatIndex<<2;
			break;
		case EFat16:
			fatPosInBytes=aFatIndex<<1;
			break;
		case EFat12:
			fatPosInBytes=(aFatIndex*3>>1);
			break;
		default:
			break;
		}
	return(fatPosInBytes);
	}
	
	
