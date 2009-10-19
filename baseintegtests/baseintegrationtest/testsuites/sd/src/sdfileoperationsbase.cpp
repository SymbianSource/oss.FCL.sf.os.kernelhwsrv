// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Base class for FileOperations and ReadFiles
// 
//

#include "sdfileoperationsbase.h"

/*
Test Step Preamble
 - Initialise attribute iDrive
 - Connect to the File Server
 - Instatiate a CFileMan object
 - Read test step configuration from INI file

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFileOperationsBase::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDriveLetter())
		return TestStepResult();
	if (!InitFileServer())
		return TestStepResult();
	if (!InitFileMan())
		return TestStepResult();
	
	_LIT(KRootEntries, "FileOperationsRootEntries");
	_LIT(KExpandRootFilesNumber, "FileOperationsExpandRootFilesNumber");
	_LIT(KExpandRootFilesSize, "FileOperationsExpandRootFilesSize");
	_LIT(KDeleteRootDirs, "FileOperationsDeleteRootDirs");
	_LIT(KSubDirEntries, "FileOperationsSubDirEntries");
	_LIT(KLargeFileSize, "FileOperationsLargeFileSize");
	_LIT(KVolumeName, "FileOperationsVolumeName");
	_LIT(KExpectedErrorCode, "FileOperationsExpectedErrorCode");
	
	if (!GetIntFromConfig(ConfigSection(), KRootEntries, iRootEntries))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KExpandRootFilesNumber, iExpandRootFilesNumber))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KExpandRootFilesSize, iExpandRootFilesSize))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KDeleteRootDirs, iDeleteRootDirs))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KSubDirEntries, iSubDirEntries))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KLargeFileSize, iLargeFileSize))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetStringFromConfig(ConfigSection(), KVolumeName, iVolumeName))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KExpectedErrorCode, iExpectedErrorCode))
		{
		INFO_PRINTF1(_L("No expected error code found in INI File - KErrNone is expected"));
		iExpectedErrorCode = KErrNone;
		}
	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
/*
Change the volume name of the disk under test

@param None
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::SetVolumeName()
	{
	TInt r;
	INFO_PRINTF2(_L("Set volume label to: %S"), &iVolumeName);
	r = iFs.SetVolumeLabel(iVolumeName);
	return r;
	}
	
/*
Create files and directories under the toot of the drive under test

@param None
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::CreateRootEntries()
	{
	TInt r = iExpectedErrorCode;
	INFO_PRINTF4(_L("Create %d root directory entries (%d directories and %d files)"), iRootEntries, iRootEntries / 2, iRootEntries - (TInt) (iRootEntries / 2));
	for (TInt i = 0; i < iRootEntries / 2; i++)
		{
		TFileName dirname;
		dirname.Format(_L("\\dir%03d\\"), i);
		r = iFs.MkDir(dirname);
		if (r != iExpectedErrorCode)
			{
			ERR_PRINTF3(_L("Error %d when making %S"), r, &dirname);
			return r;
			}
		}
	for (TInt i = iRootEntries / 2; i < iRootEntries; i++)
		{
		RFile file;
		TFileName filename;
		filename.Format(_L("\\file%03d"), i - iRootEntries / 2);
		r = file.Create(iFs, filename, EFileWrite);
		if (r != iExpectedErrorCode)
			{
			ERR_PRINTF3(_L("Error %d RFile::Create %S"), r, &filename);
			return r;
			}
		if (r != KErrNone)
			continue;
		file.Write(_L8("SD"));
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("Error %d RFile::Write %S"), r, &filename);
			return r;
			}
		file.Close();
		}
	return r;
	}
	
/*
Increase the size of some files on the root directory

@param None
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::ExpandRootFiles()
	{
	TInt r = iExpectedErrorCode;
	for (TInt i = 2; i < iExpandRootFilesNumber + 2; i++)
		{
		TFileName filename;
		filename.Format(_L("\\file%03d"), i);
		r = ExpandFile(filename, iExpandRootFilesSize);
		if (r != iExpectedErrorCode)
			break;
		}
	return r;
	}

/*
Delete some directories on the root directory

@param None
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::DeleteRootDirs()
	{
	TInt r = iExpectedErrorCode;
	INFO_PRINTF2(_L("Delete first %d directories"), iDeleteRootDirs);
	for (TInt i = iDeleteRootDirs - 1; i >= 0; i--)
		{
		TFileName dirname;
		dirname.Format(_L("\\dir%03d\\"), i);
		r = iFs.RmDir(dirname);
		if (r != iExpectedErrorCode)
			{
			ERR_PRINTF3(_L("Error %d when deleting %S"), r, &dirname);
			return r;
			}
		}
	return r;
	}

/*
Rename a file

@param aOldFile File to be renamed
@param aNewFile New name
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::RenameFile(const TDesC& aOldFile, const TDesC& aNewFile)
	{
	TInt r;
	INFO_PRINTF3(_L("Rename %S to %S"), &aOldFile, &aNewFile);
	r = iFileMan->Rename(aOldFile, aNewFile, CFileMan::EOverWrite);
	if (r != iExpectedErrorCode)
		{
		ERR_PRINTF2(_L("Error %d"), r);
		}
	return r;
	}

/*
Create files under a directory

@param aDir Path to the directory where the files will be created
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::CreateSubDirEntries(const TDesC& aDir)
	{
	TInt r = iExpectedErrorCode;
	INFO_PRINTF3(_L("Create %d subdir entries in %S"), iSubDirEntries, &aDir);
	for (TInt i = 0; i < iSubDirEntries; i++)
		{
		RFile file;
		TFileName filename;
		filename.Format(_L("%Sfile%04d"), &aDir, i);
		r = file.Create(iFs, filename, EFileWrite);
		if (r != iExpectedErrorCode)
			{
			ERR_PRINTF3(_L("Error %d RFile::Create %S\n"), r, &filename);
			return r;
			}
		if (r != KErrNone)
			break;
		file.Write(_L8("SD"));
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("Error %d RFile::Write %S\n"), r, &filename);
			return r;
			}
		file.Close();
		}
	return r;
	}

/*
Change the volume name of the disk under test

@param None
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::DeleteSubDirEntries(const TDesC& aDir)
	{
	TInt r = iExpectedErrorCode;
	INFO_PRINTF3(_L("Delete %d subdir entries in %S"), iSubDirEntries, &aDir);
	for (TInt i = 0; i < iSubDirEntries; i++)
		{
		TFileName filename;
		filename.Format(_L("%Sfile%04d"), &aDir, i);
		r = iFs.Delete(filename);
		if (r != iExpectedErrorCode)
			{
			ERR_PRINTF3(_L("Error %d RFs::Delete %S\n"), r, &filename);
			return r;
			}
		}
	return r;
	}

/*
Change the size of a file

@param aFile File who's size is to be changed
@param aSize New file size in megabytes
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::ExpandFile(const TDesC& aFile, TInt aSize)
	{
	TInt r;
	INFO_PRINTF3(_L("Expand size of %S to %dMB"), &aFile, aSize);
	RFile file;
	r = file.Open(iFs, aFile, EFileWrite);
	if (r != iExpectedErrorCode)
		{
		ERR_PRINTF2(_L("Error %d RFile::Open\n"), r);
		return r;
		}
	if (r != KErrNone)
		return r;
	r = file.SetSize(1024*1024*aSize);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("Error %d RFile::SetSize\n"), r);
		file.Close();
		return r;
		}
	file.Close();
	return r;
	}

/*
Copy file

@param aOrig File to be copied
@param aDest Destination path
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::CopyFile(const TDesC& aOrig, const TDesC& aDest)
	{
	TInt r;
	INFO_PRINTF3(_L("Copy %S to %S"), &aOrig, &aDest);	
	r = iFileMan->Copy(aOrig, aDest, CFileMan::EOverWrite);
	INFO_PRINTF2(_L("Returned value: %d"), r);
	return r;
	}

/*
Move file

@param aOrig File to be moved
@param aDest Destination path
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDFileOperationsBase::MoveFile(const TDesC& aOrig, const TDesC& aDest)
	{
	TInt r;
	INFO_PRINTF3(_L("Move %S to %S"), &aOrig, &aDest);	
	r = iFileMan->Move(aOrig, aDest, CFileMan::EOverWrite);
	INFO_PRINTF2(_L("Returned value: %d"), r);
	return r;
	}
