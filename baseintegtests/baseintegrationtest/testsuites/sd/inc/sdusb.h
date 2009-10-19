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

#ifndef SDUSB_H
#define SDUSB_H

#include "sdbase.h"

/*
SD Test Step. Enables USB Mass Storage mode.
*/
class CBaseTestSDUsb : public CBaseTestSDBase
	{
public:
	CBaseTestSDUsb();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();

private:
	};

_LIT(KTestStepUsb, "USB");

#endif // SDUSB_H
