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

#ifndef SDREADFILES1_H
#define SDREADFILES1_H

#include "sdfileoperationsbase.h"

/*
SD Test Step. Check that FileOperations1 were performed succesfully.
*/
class CBaseTestSDReadFiles1 : public CBaseTestSDFileOperationsBase
	{
public:
	CBaseTestSDReadFiles1();
	virtual TVerdict doTestStepL();
private:
	TInt CheckEntries();
	};

_LIT(KTestStepReadFiles1, "ReadFiles1");

#endif // SDREADFILES1_H
