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

#ifndef SDFILEOPERATIONS1_H
#define SDFILEOPERATIONS1_H

#include "sdfileoperationsbase.h"

/*
SD Test Step. Perform file operations on the drive under test
*/
class CBaseTestSDFileOperations1 : public CBaseTestSDFileOperationsBase
	{
public:
	CBaseTestSDFileOperations1();
	virtual TVerdict doTestStepL();
	};

_LIT(KTestStepFileOperations1, "FileOperations1");

#endif // SDFILEOPERATIONS1_H
