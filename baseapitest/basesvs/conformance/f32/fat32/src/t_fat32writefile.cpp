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



#include "t_fat32writefile.h"

static RRawDisk TheDisk;
static RFile TheFile;

/**
Class Constructor
*/		    
CBaseTestFat32WriteFile::CBaseTestFat32WriteFile() 
	{
	SetTestStepName(KTestStepWriteFile);
	}
	
/**
Class Destructor
*/
CBaseTestFat32WriteFile::~CBaseTestFat32WriteFile() 
	{
	}


/** 
The function performs the following actions
1. If there is a file specified in the ini file then 
	-> create a file with that filename 
	-> Else create a file name ReadTestFile.txt
2. Get the file action required from the ini file and carry out that action

@return EPass if test passes and EFail if test fails
*/ 
TVerdict CBaseTestFat32WriteFile::doTestStepL()
	{
	TInt r = 0;
	TInt errcode;
	SetTestStepResult(EFail);
	_LIT(KAction,"Action");
	TBuf<10> actionAfterCreate;
	TPtrC16 action = actionAfterCreate;
	_LIT(KName,"SetFileName");
	_LIT(KPath,"%c:\\");
	TBuf<10> setFileName;
	TPtrC16 name = setFileName;
	TBuf<255> fileName;
	_LIT(KCreatedFileName,"%c:\\ReadTestFile.txt");

	TBool alright = GetStringFromConfig(ConfigSection(), KName, name);
	if (alright)
		{
		fileName.Format(KPath, (TUint)iDriveToTest);
		fileName.Append(name);
		}
	else
		{
		fileName.Format(KCreatedFileName, (TUint)iDriveToTest);
		}
	TInt res = TheFile.Replace(iTheFs,fileName,EFileWrite);
	TheFile.Close();
	TBool alright2 = GetStringFromConfig(ConfigSection(), KAction, action);
	if (alright2)
		{
		errcode = SetAttribs(fileName);
		if (action == _L("Write"))
			{
			errcode = WriteFile(fileName);
			}
		if (action == _L("DirList"))
			{
			errcode = DirList(fileName);
			}
		if (action == _L("CheckAtt"))
			{
			errcode = SetAttribs(fileName);
			}
		if (action == _L("CheckCode"))
			{
			errcode = CheckErrCode(res);
			}
		if (action == _L("MakeDirectory"))
			{
			errcode = MakeDirectory(_L("TestDirectory\\"));
			}
		if (action == _L("MakeTwoDirectory"))
			{
			errcode = MakeDirectory(_L("TestDirectory\\"));
			if (errcode == KErrNone)
				{
				errcode = MakeDirectory(_L("TestDirectory\\TestDirectory2\\"));
				}
			}
		if (action == _L("SetLabel"))
			{
			errcode = SetLabel(_L("TestLabel"));
			}
		if (action == _L("SetTwoLabels"))
			{
			errcode = SetLabel(_L("TestLabel"));
			if (errcode == KErrNone)
				{
				errcode = SetLabel(_L("TestSecondLabel"));
				}
			}
		if (action == _L("SetFileSize"))
			{
			errcode = SetFileSize(fileName);
			}
		if (action == _L("DeleteFile"))
			{
			errcode = DeleteFile(fileName);
			}
		r = CheckErrCode(errcode);
		if(r != KErrNone)
			{
			_LIT(KErrorWriteFile, "Error writing the file, Error Code = %d");
			INFO_PRINTF2(KErrorWriteFile, r);
			SetTestStepResult(EFail);
			}
		else
			{
			SetTestStepResult(EPass);
			_LIT(KReadPass, "Read Passed");
			INFO_PRINTF1(KReadPass);
			}
		}
	else
		{
		r = CheckErrCode(res);
		if(r != KErrNone)
			{
			_LIT(KErrorWriteFile, "Error writing the file, Error Code = %d");
			INFO_PRINTF2(KErrorWriteFile, r);
			SetTestStepResult(EFail);
			}
		else
			{
			_LIT(KReadPass, "Read Passed");
			INFO_PRINTF1(KReadPass);
			SetTestStepResult(EPass);
			}					
		}
	return TestStepResult(); 
	}

/** 
Open the file

@param aFile The name of the file to write 

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteFile::WriteFile(const TDesC16& aFile)
	{
	TBuf8<255> temp;
	temp.Copy(aFile);
	TInt r = 0; 
	r = TheFile.Open(iTheFs,aFile,EFileWrite);
	if(r != KErrNone)
		{
		_LIT(KErrorOpenFile, "Unable to open the file, Error Code = %d");
		INFO_PRINTF2(KErrorOpenFile, r);
		return r; 
		}
	TheFile.Close();
	return r; 
	}

/** 
Get the attribute to set from the ini file and set the attributes of the file

@param aFile The name of the file whos attributes are to be set 

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteFile::SetAttribs(const TDesC16& aFile)
	{		
	TBuf8<255> temp;
	temp.Copy(aFile);
	TInt r = 0; 
	TUint setMask = KEntryAttNormal;
	_LIT(KAtt,"Attributes");
	TBuf<10> fileAttributes;
	TPtrC16 attributes = fileAttributes;

	TBool alright3 = GetStringFromConfig(ConfigSection(), KAtt, attributes);
	if (alright3)
		{
		if (attributes == _L("Normal"))
			{
			setMask = KEntryAttNormal;
			}	
		else if (attributes == _L("ReadOnly"))
			{
			setMask = KEntryAttReadOnly;
			}
		else if (attributes == _L("Hidden"))
			{
			setMask = KEntryAttHidden;
			}
		else if (attributes == _L("System"))
			{
			
			setMask = KEntryAttSystem;
			}
		else if (attributes == _L("Dir"))
			{
			setMask = KEntryAttDir;
			}
			
		r = TheFile.Open(iTheFs,aFile,EFileWrite);
		if (r != KErrNone)
			{
			_LIT(KOpenFail, "Cannot open the file, error = %d");
			INFO_PRINTF2(KOpenFail, r);
			return r;
			}
		r = TheFile.SetAtt(setMask, 0);
		if (r != KErrNone)
			{
			_LIT(KSetAttFail, "Cannot set the attributes, error = %d");
			INFO_PRINTF2(KSetAttFail, r);
			return r;
			}
			
		TheFile.Close();
		
		r = CheckAtt(aFile, setMask);	
		if (r != KErrNone)
			{
			_LIT(KAttFail, "Cannot check the attributes, error = %d");
			INFO_PRINTF2(KAttFail, r);
			return r;
			}
		}
	else
		{
		_LIT(KNoAttribs, "No attributes specified in the ini file");
		INFO_PRINTF1(KNoAttribs);
		return KErrNone;	
		}
	return r; 
	}

/** 
Check the attributes of the file and compare them to the attributes expected

@param aFile The name of the file whos attributes are to be checked 

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteFile::CheckAtt(const TDesC16& aFile, TUint setMask)
	{
	TInt r = 0;
	TUint fileAttributes;
	r = iTheFs.Att(aFile, fileAttributes);
	if (r != KErrNone)
		{
		_LIT(KGetAttFail, "Failed to get the file attributes");
		INFO_PRINTF1(KGetAttFail);
		return KErrGeneral;
		}
	else
		{
		if (fileAttributes&setMask)
			{
			_LIT(KAttChanged, "Attributes changed correctly");
			INFO_PRINTF1(KAttChanged);
			return r;
			}
		else
			{
			_LIT(KAttNotChanged, "Attributes have not been changed");
			INFO_PRINTF1(KAttNotChanged);
			return KErrGeneral;
			}
		}
	}

/** 
Searching the directory to see if it contains the file who's attributes 
were set as hidden. 

@param aFile The name of the file to search for 

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteFile::DirList(const TDesC16& aFile)
	{

	TInt i; 	
	CDir* dirPtr;	
	TBuf<10> dir1;
	dir1 = iSessionPath;
	dir1.Append(_L("*.*"));
	TBuf<20> buf;
	buf.Append(aFile);
	buf.Delete(0,3);
	TInt match = 0;
	iTheFs.GetDir(dir1,KEntryAttMatchExclude|(KEntryAttHidden|0x24),ESortNone,dirPtr);
	TInt num = dirPtr->Count();
	TEntry entry;
	for (i=0; i<num; i++)
		{
		entry=(*dirPtr)[i];
		if (entry.iName == _L("ReadTestFile.txt"))
			{
			match = match + 1;
			}
		}
	if (match > 0)
		{	
		_LIT(KFileFound, "File found");
		INFO_PRINTF1(KFileFound);
		return KErrNone;
		}
	else
		{
		_LIT(KFileNotFound, "File not found");
		INFO_PRINTF1(KFileNotFound);
		return KErrNotFound;
		}
	}

/** 
Compare the error code that is expected to the one returned from the action

@param aReturnCode The return value that is to be compared 

@return KErrNone if successfull
*/
TInt CBaseTestFat32WriteFile::CheckErrCode(TInt aReturnCode)
	{
	_LIT(KCheckCode,"CheckCode");
	TInt checkcode;
	TBool alright4 = GetIntFromConfig(ConfigSection(), KCheckCode, checkcode);
	if (alright4)
		{
		if (aReturnCode == checkcode)
			{
			_LIT(KCorrectCode, "Correct error code = %d");
			INFO_PRINTF2(KCorrectCode, aReturnCode);
			return KErrNone;
			}
		else
			{	
			_LIT(KBadCode, "Incorrect error code = %d");
			INFO_PRINTF2(KBadCode, aReturnCode);
			return KErrGeneral;
			}
		}
	else 
		{
		_LIT(KNoIniCode, "CheckCode not specified in ini file");
		INFO_PRINTF1(KNoIniCode);
		return KErrGeneral;
		}		
	}

/** 
Creating a directory

@param aDir The name of the directory to create 

@return KErrNone if successfull
*/	
TInt CBaseTestFat32WriteFile::MakeDirectory(const TDesC16& aDir)
	{
	TInt r = 0;
	TBuf<255> dirName;
	dirName.Append(iSessionPath);
	dirName.Append(aDir);
	TUint setMask = KEntryAttDir;
	r = iTheFs.MkDir(dirName);
	if (r != KErrNone)
		{
		_LIT(KMkDirFail, "Failed make the directory");
		INFO_PRINTF1(KMkDirFail);
		return r;
		}	
	r = CheckAtt(dirName, setMask);	
	if (r != KErrNone)
		{
		_LIT(KChkAttFail, "Failed to check the attributes");
		INFO_PRINTF1(KChkAttFail);
		return r;
		}
	return r; 
	}

/** 
Setting a volume label

@param aLabel The name of the lable to set

@return KErrNone if successfull
*/		
TInt CBaseTestFat32WriteFile::SetLabel(const TDesC16& aLabel)
	{
	TInt r = 0;
	r = iTheFs.SetVolumeLabel(aLabel, CurrentDrive());
	if (r != KErrNone)
		{
		_LIT(KSetLabelFail, "Failed to set volume label");
		INFO_PRINTF1(KSetLabelFail);
		return r;
		}
	return r; 
	}

/** 
Setting the size of the file

@param aFile The name of the file whos size to set

@return KErrNone if successfull
*/			
TInt CBaseTestFat32WriteFile::SetFileSize(const TDesC16& aFile)
	{
	TInt r = 0;
	r = TheFile.Open(iTheFs,aFile,EFileWrite);
	if (r != KErrNone)
		{
		_LIT(KOpenFail, "Failed to open the file");
		INFO_PRINTF1(KOpenFail);
		return r;
		}
	TInt fileSize = (iBPB_SecPerClus/2) * 3 * 1024; // File accomodates 3 clusters
	r = TheFile.SetSize(fileSize);
	if (r != KErrNone)
		{
		_LIT(KSetSizeFail, "Failed set the size of the file");
		INFO_PRINTF1(KSetSizeFail);
		return r;
		}
	TheFile.Close();
	r = iTheFs.FinaliseDrive(CurrentDrive(), RFs::EFinal_RW);
	return r; 
	}

/** 
Delete the file

@param aFile The name of the file to delete

@return KErrNone if successfull
*/				
TInt CBaseTestFat32WriteFile::DeleteFile(const TDesC16& aFile)
	{
	TInt r = 0;
	r = iTheFs.Delete(aFile);
	if (r != KErrNone)
		{
		_LIT(KDeleteFail, "Failed delete the file");
		INFO_PRINTF1(KDeleteFail);
		return r;
		}
	return r; 
	}
	
