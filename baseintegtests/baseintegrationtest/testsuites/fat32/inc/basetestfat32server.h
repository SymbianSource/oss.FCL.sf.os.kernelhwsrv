// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef BASETESTFAT32SERVER_H
#define BASETESTFAT32SERVER_H

#include <testexecuteserverbase.h>


class CBaseTestFat32Server : public CTestServer
	{
public:
	static CBaseTestFat32Server* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	};

#endif // BASETESTFAT32SERVER_H
