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




#include <e32def.h> 
#include "t_fat32filldisk.h"

/**
Class Constructor
*/		    
CBaseTestFat32FillDisk::CBaseTestFat32FillDisk()
	{
	SetTestStepName(KTestStepFillDisk);
	}
	
/**
Class Destructor
*/
CBaseTestFat32FillDisk::~CBaseTestFat32FillDisk() 
	{
	}

/**
Thread that sends a signal along the serial port so that it is not assumed 
that the board has hung.
*/
TInt Thread1Func(TAny* /*aPtr*/)
	{
	for(;;)
		{
		RDebug::Printf("Filling the disk...");
		User::After(600000000);
		}
	}
	
/** 
Filling the disk to its maximum capacity by writing 1 file.
If the disk is greater than 2GB in size, 2 files are created.  

@return EPass if test passes and EFail if test fails
*/ 		    
TVerdict CBaseTestFat32FillDisk::doTestStepL()
	{
	SetTestStepResult(EFail);
	TVolumeInfo iInfo;
	RFile rFile;
	TInt filesize = 0;
	RFile rFile2;
	TInt filesize2 = 0;	
	RFile rFile3;
	TInt filesize3 = 0;	
	TInt  r = KErrNone;
	r = iTheFs.Volume(iInfo, CurrentDrive());
	if (r != KErrNone)
	INFO_PRINTF2(_L("volume info for %C:"), (TUint)iDriveToTest);
	_LIT(KFileReplace, "RFile::Replace, epecting KErrNone");
	r = rFile.Replace(iTheFs, _L("\\TEST.txt"), EFileWrite);				
	FAT_TEST_VAL(r==KErrNone, KFileReplace, r);
	
	TBuf<20> threadName1 =_L("Thread1");
	RThread thread1;
	r = thread1.Create(threadName1,Thread1Func,KDefaultStackSize,0x1000,0x1000,NULL);
	if (r != KErrNone)
		{
		INFO_PRINTF2(_L("Could not create thread1 - r=%d"),r);
		}
	thread1.Resume();
	
	TInt64 count = 0;
	TBuf8<4096> buffer(4096); 
	r = KErrNone;
	while ((count < iInfo.iSize) && (r == KErrNone))
		{
		r=rFile.Write(buffer,4096);
		count = count + 4096;
		} ;
	rFile.Size(filesize);
	INFO_PRINTF2(_L("after filling r = %d"),r);
	if (r == KErrTooBig)
		{
		r = rFile2.Replace(iTheFs, _L("\\TEST2.txt"), EFileWrite);				
		FAT_TEST_VAL(r == KErrNone, KFileReplace, r);
		TInt64 count2 = 0;
		TBuf8<4096> buffer2(4096); 
		r = KErrNone;
		while ((count2 < (iInfo.iSize - filesize)) && (r == KErrNone))
			{
			r=rFile2.Write(buffer2,4096);
			count2 = count2 + 4096;
			} ;
		rFile2.Size(filesize2);
		}

	if (r == KErrDiskFull)
		{
		INFO_PRINTF2(_L("Disk full on %c:\n"), (TUint)iDriveToTest);
		SetTestStepResult(EPass);
		}

	if(r != KErrNone && r != KErrDiskFull && r != KErrTooBig)
		{
		INFO_PRINTF4(_L("Write Failed:%d FileSize:%d DiskFreeSize:%d"), r, filesize,I64INT(iInfo.iFree));
		SetTestStepResult(EFail);
		thread1.Kill(r);
		thread1.Close();
		return 	TestStepResult();
		}
	if (r == KErrNone && r != KErrDiskFull)
		{
		r = rFile3.Replace(iTheFs, _L("\\TEST3.txt"), EFileWrite);	
		r = iTheFs.Volume(iInfo, CurrentDrive());
		TInt extra = iInfo.iFree;
		INFO_PRINTF2(_L("extra = %d:\n"), extra);
		rFile3.SetSize(extra);
		rFile3.Size(filesize3);
		}
	
	r = iTheFs.Volume(iInfo, CurrentDrive());
	if (iInfo.iSize - iInfo.iFree == iInfo.iSize)
		{
		SetTestStepResult(EPass);
		}
	rFile.Close();	
	rFile2.Close();
	rFile3.Close();
	TInt64 totalFileSize = filesize + filesize2 + filesize3;	

	r = iTheFs.Volume(iInfo, CurrentDrive());
	INFO_PRINTF2(_L("File size1 = %d"),filesize);
	INFO_PRINTF2(_L("File size2 = %d"),filesize2);
	INFO_PRINTF2(_L("File size3 = %d"),filesize3);
	INFO_PRINTF2(_L("Total File size = %Ld"),totalFileSize);
	INFO_PRINTF4(_L("Free space on %c: %Ld KB (out of %Ld KB)\n"),
				(TUint)iDriveToTest,((iInfo.iFree) / 1024),((iInfo.iSize) / 1024));
	
	thread1.Kill(r);
	thread1.Close();
	return TestStepResult();

	}

