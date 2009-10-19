/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
* Class declaration for Device State.Notifier Base Class
*
*/


/** 
 @file
 @internalTechnology
*/

#ifndef CACTIVEDEVICESTATENOTIFIERBASE_H
#define CACTIVEDEVICESTATENOTIFIERBASE_H
#include <e32base.h>

#include "cbulkonlytransport.h"
#include "protocol.h"
#include "cusbmassstoragecontroller.h"

class CActiveDeviceStateNotifierBase : public CActive
	{
public:
	// Construction
	static CActiveDeviceStateNotifierBase* NewL(CBulkOnlyTransport& aBot,
                                                MLddDeviceStateNotification& aLddDeviceStateNotification);

	// Destruction
	~CActiveDeviceStateNotifierBase();

	void Activate();

protected:
	// Construction
	CActiveDeviceStateNotifierBase(CBulkOnlyTransport& aBot,
                                   MLddDeviceStateNotification& aLddDeviceStateNotification);
	void ConstructL();

	// Cancel request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();

	// Service completed request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();

protected:
	CBulkOnlyTransport& iBot;
    MLddDeviceStateNotification& iLddDeviceStateNotification;
	TUint iDeviceState;
	TUint iOldDeviceState;
	};

#endif


