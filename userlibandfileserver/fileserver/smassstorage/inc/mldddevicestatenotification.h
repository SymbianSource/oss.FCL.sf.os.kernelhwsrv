/*
* Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/


/**
 @file
 @internalTechnology
*/

#ifndef MLDDDEVICESTATENOTIFICATION_H
#define MLDDDEVICESTATENOTIFICATION_H

#include <e32base.h>

class MLddDeviceStateNotification
    {
public:
    virtual void Activate(TRequestStatus& aStatus, TUint& aValue) = 0;
    virtual void Cancel() = 0;
    virtual ~MLddDeviceStateNotification() {};
    };


#endif

