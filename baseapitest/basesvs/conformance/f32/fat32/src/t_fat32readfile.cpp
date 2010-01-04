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



#include "t_fat32readfile.h"


static RFile TheFile;

/**
Class Constructor
*/		    
CBaseTestFat32ReadFile::CBaseTestFat32ReadFile() 
	{
	SetTestStepName(KTestStepReadFile);
	}
	
/**
Class Destructor
*/
CBaseTestFat32ReadFile::~CBaseTestFat32ReadFile() 
	{
	}

/** 
The function performs the following actions:
1. Get the file name and path from the ini file. 
	-> If no file name is specified he create a file 
	-> If no path is specified, use the session path. 
2. Get the action that is required from the ini file
3. Call the function that carries out that particular action

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32ReadFile::doTestStepL()
	{
	SetTestStepResult(EFail);
	TInt r = KErrNone;
	_LIT(KFileName,"FileName");
	_LIT(KCreatedFileName,"ReadTestFile.txt");
	TBufC<255> fileName;
	TPtrC16 filename = fileName;
	_LIT(KPath,"Path");
	TBufC<255> pathName;
	TPtrC16 path = pathName;
	_LIT(KReadAction,"ReadAction");
	TBufC<255> readAction;
	TPtrC16 readaction = readAction;
	TBuf<255> fullPath;
	TBool alright = GetStringFromConfig(ConfigSection(), KFileName, filename);
	if(alright)
		{
		TBool alright2 = GetStringFromConfig(ConfigSection(), KPath, path);
		if(alright2)
			{
			fullPath.Append(path);
			}
		else
			{
			fullPath.Append(iSessionPath);
			}
		fullPath.Append(filename);
	
		TBool alright3 = GetStringFromConfig(ConfigSection(), KReadAction, readaction);
		if(alright3)
			{
			if (readaction == _L("OpenFile"))
				{
				r = OpenFile(fullPath);
				}
			if (readaction == _L("ReadFile"))
				{
				r = ReadFile(fullPath);
				}
			if (readaction == _L("GetModTime"))
				{
				INFO_PRINTF1(_L("Calling the function GetModTime"));
				r = GetModTime(fullPath);
				INFO_PRINTF2(_L("Finished the function GetModTime - r=%d"),r);
				}
			if (readaction == _L("GetModDate"))
				{
				INFO_PRINTF1(_L("Calling the function GetModDate"));
				r = GetModDate(fullPath);
				INFO_PRINTF2(_L("Finished the function GetModDate - r=%d"),r);
				}
		
			if(r != KErrNone)
				{
				_LIT(KErrorReadFile, "Error with read action");
				INFO_PRINTF1(KErrorReadFile);
				SetTestStepResult(EFail);
				return TestStepResult();
				}
			else
				{
				SetTestStepResult(EPass);
				_LIT(KReadPass, "Read Action Passed");
				INFO_PRINTF1(KReadPass);
				return TestStepResult();
				}
			}
		}
	else
		{
		TInt r = TheFile.Replace(iTheFs,KCreatedFileName,EFileRead|EFileWrite);
		if (r != KErrNone)
			{
			_LIT(KErrorCreateNewFile, "Cannot creat new file - Error code = %d");
			INFO_PRINTF2(KErrorCreateNewFile,r);
			SetTestStepResult(EFail);
			return TestStepResult();	
			}
		else
			{
			TInt r = ReadFile(KCreatedFileName);
			if(r != KErrNone)
				{
				_LIT(KErrorReadFile, "Error reading the file, Error Code = %d");
				INFO_PRINTF2(KErrorReadFile, r);
				SetTestStepResult(EFail);
				return TestStepResult();
				}
			else
				{
				SetTestStepResult(EPass);
				_LIT(KReadPass, "Read Passed");
				INFO_PRINTF1(KReadPass);
				return TestStepResult();
				}
			}
		}
	return TestStepResult();
	}


/** 
Read the file

@param aFile The name of the file to read 

@return KErrNone if successfull
*/
TInt CBaseTestFat32ReadFile::ReadFile(const TDesC16& aFile)
	{
	TBuf8<255> temp;
	temp.Copy(aFile);
	TInt r = KErrNone; 
//	_LIT(KReadCheckCode,"ReadCheckCode");
//	TInt readcheckcode;
	r = TheFile.Open(iTheFs, aFile, EFileRead);
	r = TheFile.Read(temp);
	return r;
	}

/** 
Open a file and check whether the error code returned is equal to that
stated in the ini file

@param aFile The name of the file to read 

@return KErrNone if successfull
*/	
TInt CBaseTestFat32ReadFile::OpenFile(const TDesC16& aFile)
	{
	TBuf8<255> temp;
	temp.Copy(aFile);
	_LIT(KReadCheckCode,"ReadCheckCode");
	TInt readcheckcode;
	TInt res = TheFile.Open(iTheFs, aFile, EFileRead);
	TBool alright = GetIntFromConfig(ConfigSection(), KReadCheckCode, readcheckcode);
	if(alright)
		{
		if (res == readcheckcode)
			{
			_LIT(KReadPass, "Check code for open file is correct res = %d");
			INFO_PRINTF2(KReadPass, res);
			return KErrNone;
			}
		else
			{
			_LIT(KReadFail, "Check code for open file is incorrect correct res = %d, correct return = %d");
			INFO_PRINTF3(KReadFail, res, readcheckcode);
			return KErrNone;
			}
			
		}
	else 
		{
		_LIT(KNoIni, "Unable to get ReadCheckCode from ini file");
		INFO_PRINTF1(KNoIni);
		return -1;
		}

	}

/** 
Check what error the RFs::Modified fuction should return from the ini file 
and call RFs::Modified()

@param aFile The name of the file to read 

@return KErrNone if successfull
*/	
TInt CBaseTestFat32ReadFile::GetModDate(const TDesC16& aFile)
	{
	TInt r = KErrNone;
	TTime modifiedTime;
	TBufC<9> checkDate;
	TPtrC16 actualDate = checkDate;
	_LIT(KCheckDate,"Date");
	TBuf <255> date;
	TBool alright = GetStringFromConfig(ConfigSection(), KCheckDate, actualDate);
	if(alright)
		{
		INFO_PRINTF1(_L("Calling the function RFs::Modified within GetModTime"));
		r = iTheFs.Modified(aFile, modifiedTime);
		modifiedTime.FormatL(date, _L("%D%M%Y%1 %2 %3"));
		INFO_PRINTF2(_L("Returned from RFs::Modified within the function GetModTime - r=%d"), r);
		TPtrC16 readdate = date;
		if (readdate == actualDate)
			{
			INFO_PRINTF1(_L("RFs::Modified returns the correct value "));
			return KErrNone;
			}
		else 
			{
			INFO_PRINTF2(_L("RFs::Modified returns the incorrect value %S"), &date);
			return -1;
			}
		}
	return r;
	}

/** 
Check what error the RFs::Modified fuction should return from the ini file 
and call RFs::Modified()

@param aFile The name of the file to read 

@return KErrNone if successfull
*/
TInt CBaseTestFat32ReadFile::GetModTime(const TDesC16& aFile)
	{
	TInt r = KErrNone;
	TTime modifiedTime;
	TBufC<9> checkTime;
	TPtrC16 actualTime = checkTime;
	_LIT(KCheckTime,"Time");
	TBuf <255> time;
	TBool alright = GetStringFromConfig(ConfigSection(), KCheckTime, actualTime);
	if(alright)
		{
		INFO_PRINTF1(_L("Calling the function RFs::Modified within GetModTime"));
		r = iTheFs.Modified(aFile, modifiedTime);
		modifiedTime.FormatL(time, _L("%H%T%S"));
		INFO_PRINTF2(_L("Returned from RFs::Modified within the function GetModTime - r=%d"), r);
		TPtrC16 readtime = time;
		if (readtime == actualTime)
			{
			INFO_PRINTF1(_L("RFs::Modified returns the correct value "));
			return KErrNone;
			}
		else 
			{
			INFO_PRINTF2(_L("RFs::Modified returns the incorrect value %S"), &time);
			return -1;
			}
		}
		return r;
	}
