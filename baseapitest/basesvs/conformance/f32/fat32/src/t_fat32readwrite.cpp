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




#include "t_fat32readwrite.h"

/**
Class Constructor
*/		    
CBaseTestFat32ReadWrite::CBaseTestFat32ReadWrite()
	{
	SetTestStepName(KTestStepReadWrite);
	}
	
/**
Class Destructor
*/
CBaseTestFat32ReadWrite::~CBaseTestFat32ReadWrite() 
	{
	} 

/**
Writes and reads a file on the drive
The disk should be removed while doing read or write operation. 

@return EPass if test passes and EFail if test fails
*/		    
TVerdict CBaseTestFat32ReadWrite::doTestStepL()
	{
	SetTestStepResult(EPass);
	
	TInt r;
	RFile rfile;
	_LIT(KFileCreate, "RFs::Replace, expecting KErrNone");
	_LIT(KTestFilename, "%c:\\TEST.txt");
	TBuf<255> testFilename;
	testFilename.Format(KTestFilename, (TUint)iDriveToTest);
	r =  rfile.Replace(iTheFs,testFilename, EFileWrite);
	FAT_TEST(r == KErrNone, KFileCreate);
	
	_LIT(KData, "Testing file operation");	
	TBuf8<25> buffer;
	buffer.Copy(KData);
	TUint index = 0;
	TInt pos = 0;
	
	while(index++<1000)
		{
		r = rfile.Write(buffer);
		if(r == KErrNotReady)
			{
		  	INFO_PRINTF2(_L("Write Failed:%d"), r);
			break;
			}					
		}
	if(r == KErrNone)
		{
		rfile.Seek(ESeekStart, pos);
		index = 0;
		while(index++ < 1000)
			{
			rfile.Read(buffer, 25);	
			if(r==KErrNotReady)
				{
		  		INFO_PRINTF2(_L("Read Failed:%d"), r);
				break;
				}				
			}	    	
		}		
	rfile.Close();	
	return 	TestStepResult();
	}

