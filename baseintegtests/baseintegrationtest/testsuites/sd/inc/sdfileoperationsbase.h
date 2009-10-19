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
//

#ifndef SDFILEOPERATIONSBASE_H
#define SDFILEOPERATIONSBASE_H

#include "sdbase.h"

/*
Base class for file operation test steps.
*/
class CBaseTestSDFileOperationsBase : public CBaseTestSDBase
	{
public:
	virtual TVerdict doTestStepPreambleL();

protected:
	TInt iRootEntries;
	TInt iExpandRootFilesNumber;
	TInt iExpandRootFilesSize;
	TInt iDeleteRootDirs;
	TInt iSubDirEntries;
	TInt iLargeFileSize;
	TPtrC iVolumeName;
	TInt iExpectedErrorCode;

	TInt SetVolumeName();
	TInt CreateRootEntries();
	TInt ExpandRootFiles();
	TInt DeleteRootDirs();
	TInt RenameFile(const TDesC& aOldFile, const TDesC& aNewFile);
	TInt CreateSubDirEntries(const TDesC& aDir);
	TInt DeleteSubDirEntries(const TDesC& aDir);
	TInt ExpandFile(const TDesC& aFile, TInt aSize);
	TInt CopyFile(const TDesC& aOrig, const TDesC& aDest);
	TInt MoveFile(const TDesC& aOrig, const TDesC& aDest);
	};

#endif // SDFILEOPERATIONSBASE_H
