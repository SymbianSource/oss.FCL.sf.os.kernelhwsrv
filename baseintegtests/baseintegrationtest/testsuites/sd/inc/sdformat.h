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

#ifndef SDFORMAT_H
#define SDFORMAT_H

#include "sdbase.h"

/*
SD Test Step. Format drive.
*/
class CBaseTestSDFormat : public CBaseTestSDBase
	{
public:
	CBaseTestSDFormat();
	virtual TVerdict doTestStepPreambleL();

	virtual TVerdict doTestStepL();

private:
	};

_LIT(KTestStepFormat, "Format");

#endif // SDFORMAT_H
