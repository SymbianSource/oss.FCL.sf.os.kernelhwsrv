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
// Check card contents after FileOperations2
// 
//

#include "sdreadfiles2.h"

/*
Class constructor

@param None
@return None
*/
CBaseTestSDReadFiles2::CBaseTestSDReadFiles2()
	{
	SetTestStepName(KTestStepReadFiles2);
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDReadFiles2::doTestStepL()
	{
	if (TestStepResult() != EPass)
		{
		ERR_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		return TestStepResult();
		}

	TInt r;
	TBuf<4> sessionPath;
	sessionPath.Format(_L("%c:\\"), 'A' + iDrive);
	r = iFs.SetSessionPath(sessionPath);
	if (r != KErrNone)
		{
		ERR_PRINTF3(_L("Could not set session path to %c: (r = %d)"), 'A' + iDrive, r);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	// Check Volume Name
	INFO_PRINTF1(_L("Check volume name"));
	TVolumeInfo vi;
	iFs.Volume(vi);
	if (vi.iName != iVolumeName)
		{
		ERR_PRINTF3(_L("Expected '%S', got '%S'"), &iVolumeName, &vi.iName);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	// Check Entries
	INFO_PRINTF1(_L("Check entries"));
	if (CheckEntries() != KErrNone)
		{
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	return TestStepResult();
	}

/*
Check disk contains expected entries

@param None
@return KErrNone if successful, otherwise any other system-wide error code
*/
TInt CBaseTestSDReadFiles2::CheckEntries()
	{
	TInt r;
	CDir* entryList;
	TFileName sessionPath;
	sessionPath.Format(_L("%c:\\"), 'A' + iDrive);
	r = iFs.GetDir(sessionPath, KEntryAttMaskSupported, ESortByName | EDirsFirst, entryList);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("GetDir returned %d"), r);
		return r;
		}
	
	INFO_PRINTF1(_L("Check number of entries in root dir"));
	TInt noentries = entryList->Count();
	if (noentries != iRootEntries - iDeleteRootDirs)
		{
		ERR_PRINTF3(_L("Expected %d entries in root dir, got %d"), iRootEntries - iDeleteRootDirs, noentries);
		return KErrUnknown;
		}

	INFO_PRINTF1(_L("Check directories are all here"));
	TInt i;
	for (i = iDeleteRootDirs; i < iRootEntries / 2; i++)
		{
		TFileName dirname;
		dirname.Format(_L("%Sdir%03d"), &sessionPath, i);
		RDir dir;
		r = dir.Open(iFs, dirname, KEntryAttNormal);
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("RDir::Open on %S returned %d"), &dirname, r);
			return r;
			}
		dir.Close();
		}

	INFO_PRINTF1(_L("Check files are all here"));
	for (i = iRootEntries / 2; i < iRootEntries; i++)
		{
		TFileName filename;
		if (i - iRootEntries / 2 == 0)
			{
			filename.Format(_L("BACK"));
			}
		else if (i - iRootEntries / 2 == 1)
			{
			filename.Format(_L("Large File"));
			}
		else
			{
			filename.Format(_L("%Sfile%03d"), &sessionPath, i - iRootEntries / 2);
			}
		RFile file;
		r = file.Open(iFs, filename, EFileRead);
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("RFile::Open on %S returned %d"), &filename, r);
			return r;
			}
		// Check size of expanded files
		TInt size;
		file.Size(size);
		if (i - iRootEntries / 2 == 1)
			{
			if (size != iLargeFileSize * (1 << 20))
				{
				ERR_PRINTF4(_L("%S: expected size %d got %d"), &filename, iExpandRootFilesSize * (1 << 20), size);
				return KErrUnknown;
				}
			}
		else if ((i - iRootEntries / 2 > 1) && (i - iRootEntries / 2 - 2 < iExpandRootFilesNumber))
			{
			if (size != iExpandRootFilesSize * (1 << 20))
				{
				ERR_PRINTF4(_L("%S: expected size %d got %d"), &filename, iExpandRootFilesSize * (1 << 20), size);
				return KErrUnknown;
				}
			}
		file.Close();
		}
	delete entryList;

	INFO_PRINTF1(_L("Now check the subdir entries"));
	sessionPath.Format(_L("%c:\\dir%03d\\"), 'A' + iDrive, iRootEntries / 2 - 2);
	r = iFs.GetDir(sessionPath, KEntryAttMaskSupported, ESortByName | EDirsFirst, entryList);
	if (r != KErrNone)
		{
		ERR_PRINTF2(_L("GetDir returned %d"), r);
		return r;
		}
	noentries = entryList->Count();
	if (noentries != iSubDirEntries)
		{
		ERR_PRINTF4(_L("Expected %d entries in %S, got %d"), &sessionPath, iSubDirEntries, noentries);
		return KErrUnknown;
		}
	for (i = 0; i < iSubDirEntries; i++)
		{
		TFileName filename;
		filename.Format(_L("%Sfile%04d"), &sessionPath, i);
		RFile file;
		r = file.Open(iFs, filename, EFileRead);
		if (r != KErrNone)
			{
			ERR_PRINTF3(_L("RFile::Open on %S returned %d"), &filename, r);
			return r;
			}
		file.Close();
		}
	delete entryList;
	return KErrNone;
	}
