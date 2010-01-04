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



#ifndef T_FAT32WRITEFILE_H
#define T_FAT32WRITEFILE_H

#include <test/testexecutestepbase.h>
#include <test/testexecuteserverbase.h>
#include "t_fat32base.h"

/**
Fat32 WriteFile Class. Inherits from the base class.
Contains functions needed to perform various different write file operations
*/
class CBaseTestFat32WriteFile : public CBaseTestFat32Base
	{
	public:
		CBaseTestFat32WriteFile(); 
		~CBaseTestFat32WriteFile();
		virtual TVerdict doTestStepL();	
		TInt WriteFile(const TDesC16& aFile);	
		TInt SetAttribs(const TDesC16& aFile);
		TInt DirList(const TDesC16& aFile);	
		TInt CheckErrCode(TInt aReturnCode);
		TInt CheckAtt(const TDesC16& aFile, TUint setMask);
		TInt MakeDirectory(const TDesC16& aDir);
		TInt SetLabel(const TDesC16& aLabel);
		TInt SetFileSize(const TDesC16& aFile);	
		TInt DeleteFile(const TDesC16& aFile);	
	
	
};

_LIT(KTestStepWriteFile, "WriteFile");

#endif // T_FAT32WRITEFILE_H
