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

#ifndef SDBIGFILEWRITE_H
#define SDBIGFILEWRITE_H

#include "sdbase.h"

/*
SD Test Step. Write a large file on the drive under test.
*/
class CBaseTestSDBigFileWrite : public CBaseTestSDBase
	{
public:
	CBaseTestSDBigFileWrite();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();
	virtual TVerdict doTestStepPostambleL();
	};

_LIT(KTestStepBigFileWrite, "BigFileWrite");

#endif // SDBIGFILEWRITE_H
