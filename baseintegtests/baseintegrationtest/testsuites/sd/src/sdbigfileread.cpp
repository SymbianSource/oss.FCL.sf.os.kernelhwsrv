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
// Read a x MB file from the disk (user is expected to take the card out when
// the file is being read)
// 
//

#include "sdbigfileread.h"

static const TInt KBlockSize = 65536;
static TBuf8<KBlockSize> DataBlock;

/*
Class constructor

@param None
@return None
*/
CBaseTestSDBigFileRead::CBaseTestSDBigFileRead()
	{
	SetTestStepName(KTestStepBigFileRead);
	}

/*
Test Step Preamble
 - Initialise attribute iDrive
 - Connect to the File Server

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDBigFileRead::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDriveLetter())
		return TestStepResult();
	if (!InitFileServer())
		return TestStepResult();

	SetTestStepResult(EPass);
	return TestStepResult();
	}

/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDBigFileRead::doTestStepL()
	{
	TInt r;
	TPtrC name;
	TInt size;
	
	_LIT(KTestName, "BigFileName");
	_LIT(KTestSize, "BigFileSize");
	_LIT8(KTestPattern, "0123456789ABCDEF");
	
	if (!GetStringFromConfig(ConfigSection(), KTestName, name))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	if (!GetIntFromConfig(ConfigSection(), KTestSize, size))
		{
		ERR_PRINTF1(_L("INI File Read Error"));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	TInt i;
	for (i = 0; i < KBlockSize; i += KTestPattern().Length())
		{
		DataBlock.Append(KTestPattern);
		}
	
	TFileName filename;
	RFile file;
	filename.Format(_L("%c:\\%S"), 'A' + iDrive, &name);
	r = file.Open(iFs, filename, EFileRead);
	if (r != KErrNone)
		{
		ERR_PRINTF3(_L("Error %d RFile::Open %S"), r, &filename);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	for (i = 0; i < (size << 20); i += KBlockSize)
		{
		r = file.Read(i, DataBlock, KBlockSize);
		if (r != KErrNone)
			{
			INFO_PRINTF2(_L("Error %d RFile::Write"), r);
			break;
			}
		}
	file.Close();
	SetTestStepResult(EPass);
	return TestStepResult();
	}


/*
Test step postamble

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDBigFileRead::doTestStepPostambleL()
	{
	DataBlock = _L8("");
	return TestStepResult();
	}
