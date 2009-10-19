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

#ifndef SDBIGFILEREAD_H
#define SDBIGFILEREAD_H

#include "sdbase.h"

/*
SD Test Step. Read the contents of a large file on the drive under test.
*/
class CBaseTestSDBigFileRead : public CBaseTestSDBase
	{
public:
	CBaseTestSDBigFileRead();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();
	virtual TVerdict doTestStepPostambleL();
	};

_LIT(KTestStepBigFileRead, "BigFileRead");

#endif // SDBIGFILEREAD_H
