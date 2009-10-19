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

#ifndef SDDISPLAYINFO_H
#define SDDISPLAYINFO_H

#include "sdbase.h"

/*
SD Test Step. Display the contents of the card registers.
*/
class CBaseTestSDDisplayInfo : public CBaseTestSDBase
	{
public:
	CBaseTestSDDisplayInfo();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();

private:
	TUint32 Slice128(TUint8* aArrayPtr, TInt aStart, TInt aEnd);
	};

_LIT(KTestStepDisplayInfo, "DisplayInfo");

#endif // SDDISPLAYINFO_H
