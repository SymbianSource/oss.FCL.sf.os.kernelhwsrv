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


#include "UtilsSetupStep.h"

/*@{*/
// Literals Used
_LIT(KT_FileSize,		"fileSize");
_LIT(KT_NumOfFiles,		"numOfFiles");
_LIT(KT_DirTreeDepth,	"dirTreeDepth");
_LIT(KT_BaseFileName,	"fileBaseName");
_LIT(KT_SubDirName,		"subDirName");
_LIT(KT_FileData,		"fileData");
_LIT(KT_DotSeparator,	".");
_LIT(KT_DirSpearator,	"\\");
/*@}*/

// Function : CT_SetupStep
// Description :CT_SetupStep:: class constructor
CT_SetupStep::CT_SetupStep()
:	CT_UtilsStep()
,	iFileSize(0)
,	iDirTreeDepth(0)
,	iNumOfFiles(0)
	{
	}

// Function : ~CT_SetupStep
// Description :CT_SetupStep class destructor
CT_SetupStep:: ~CT_SetupStep()
	{
	}

//Function:doTestStepL
//Description :
//@return TVerdict pass / fail
TVerdict CT_SetupStep::doTestStepL()
	{
	INFO_PRINTF1(_L("Starting CT_SetupStepdoTestStepL"));
	//create directory structure and contents
    CreateDirTreeL();
	INFO_PRINTF1(_L("CT_SetupStep Completed doTestStepL"));
	return TestStepResult();
	}

//Function:doTestStepPreambleL
//Description :
//@return TVerdict pass / fail
TVerdict CT_SetupStep::doTestStepPreambleL()
	{
	INFO_PRINTF1(_L("Test Step :Perf SetupStep Preamble setup"));
	INFO_PRINTF1(_L("Initiating: :Perf Utils step Preamble  setup"));
	//call Utils base class doTestStepPreambleL
	TVerdict 	result= CT_UtilsStep::doTestStepPreambleL();
	CT_UtilsStep::DoFormatDriveL();	//format drive if requested

	if (!GetIntFromConfig(ConfigSection(),KT_FileSize,iFileSize))
		{
		ERR_PRINTF2( _L("Invalid fileSize given, file size set to: (%d)"), iFileSize );
		SetTestStepResult(EFail);
		}
	if (!GetIntFromConfig(ConfigSection(),KT_DirTreeDepth,iDirTreeDepth))
		{
		ERR_PRINTF2( _L("Invalid directory tree depth given, it is set to:...(%d)"),iDirTreeDepth);
		SetTestStepResult(EFail);
		}
	if (!GetIntFromConfig(ConfigSection(),KT_NumOfFiles,iNumOfFiles))
		{
		ERR_PRINTF2( _L("Invalid number of files to create, it is set to: (%d) "), iNumOfFiles );
		SetTestStepResult(EFail);
		}
	if(!GetStringFromConfig(ConfigSection(),KT_BaseFileName,iFileBaseName))
		{
		ERR_PRINTF1( _L("Unable to read file name to create, Will not be able to create file structure:!! ") );
		SetTestStepResult(EFail);
		}
	if (!GetStringFromConfig(ConfigSection(),KT_SubDirName,iDirSubName))
		{
		ERR_PRINTF1( _L("Unable to read in sub directory name to use within test, Will not be able to create a directory structure! ") );
		SetTestStepResult(EFail);
		}
	if (!GetStringFromConfig(ConfigSection(),KT_FileData,iFileData))
		{
		ERR_PRINTF1( _L("Unable to read file Data to use within test, Will not be able to create files with any data!") );
		SetTestStepResult(EFail);
		}

	return 	result;
	}




// Function CreateDirTreeL
// Description
// @return :void
void CT_SetupStep::CreateDirTreeL()
	{
    RDir dir;
    TFileName 	pathDepth=iDirBaseName;
    TInt err=dir.Open(iFsSession, pathDepth, KEntryAttNormal);

    if (err!=KErrNone)
		{
 		RemoveDirTreeL();
		}
	else
		{
	    dir.Close();//close dir
		}

	//	Create directory tree
	for (TInt i=0;i<iDirTreeDepth ;i++)//offset due to parent dir
		{
		pathDepth+=iDirSubName;
		pathDepth+=KT_DirSpearator;
		}
	User::LeaveIfError(iFsSession.MkDirAll(pathDepth));

	//	Create files in each directory
	pathDepth=iDirBaseName;
	for (TInt j=0;j<iDirTreeDepth ;j++)//offset due to parent dir
		{
		pathDepth+=iDirSubName;
		pathDepth+=KT_DirSpearator;
		CreateFileL(pathDepth);
		}
	}


// Function CreateFileL
// Description
// @return :void
// @param:TDesC& aPath
void CT_SetupStep::	CreateFileL(TDesC& aPath)
	{
	RFile 		file;
	//	File names are of the type filename1.ext, filename2.ext, etc, so create first file with "1" attached to it
 	TFileName 	extNum=_L("");//placeholder
	extNum.AppendNum(1);
	TFileName 	firstFilename=aPath;
	firstFilename.Append(iFileBaseName);
	firstFilename.Insert((firstFilename.Find(KT_DotSeparator)), extNum);
	TInt 	opened=file.Open(iFsSession, firstFilename, EFileStream);
	if ( opened!=KErrNone )
		{
		opened=file.Create(iFsSession, firstFilename, EFileStream);
		}

	if ( opened==KErrNone )
		{
		CleanupClosePushL(file);//make handle leave safe
		CreateFileDataL(file);
		CleanupStack::PopAndDestroy(&file);

		//	Copy first file to subsequent files
		CFileMan*	fileMan=CFileMan::NewL(iFsSession);
		CleanupStack::PushL(fileMan);
		for (TInt i=1;i<iNumOfFiles; i++)
			{
			extNum.Zero();
			extNum.AppendNum(i+1);
			TFileName 	newFilename=aPath;
			newFilename.Append(iFileBaseName);
			newFilename.Insert((newFilename.Find(KT_DotSeparator)), extNum);
			fileMan->Copy(firstFilename, newFilename);
			}
		CleanupStack::PopAndDestroy(fileMan);
		}
	else
		{
		ERR_PRINTF2( _L("File  not created failed with error (%d)"), opened);
		SetTestStepResult(EFail);
		}
	}


// Function : CreateFileDataL
// Description
// @return :void
// @param:RFile& aFile
void    CT_SetupStep::CreateFileDataL(RFile& aFile)
	{
	HBufC8* tempbuf=HBufC8::NewLC(iFileData.Length());
  	TPtr8 	data(tempbuf->Des());
  	data.Copy(iFileData);

  	TInt 	filelength=iFileSize/iFileData.Length();
  	for (TInt i=0;i<filelength;i++)
		{
		TInt 	err= aFile.Write(data);//Write(const TDesC8 &aDes);
		if(err!=KErrNone)
			{
			ERR_PRINTF2( _L("File data not successful failed with error: (%d)"),err);
			SetTestStepResult(EFail);
			}
		}

	CleanupStack::PopAndDestroy(1,tempbuf);
	}
