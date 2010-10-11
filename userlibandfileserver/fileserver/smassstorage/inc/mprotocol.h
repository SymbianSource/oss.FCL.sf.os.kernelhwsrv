// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalTechnology
*/

#ifndef MPROTOCOL_H
#define MPROTOCOL_H

#include <e32base.h>            // C Class Definitions, Cleanup Stack
#include <e32def.h>             // T Type  Definitions
#include <e32std.h>             // ELeave definition
#include "usbmsshared.h"


class MTransportBase;

class MProtocolBase
    {
    public:
    virtual void RegisterTransport(MTransportBase* aTransport) = 0;
    virtual TBool DecodePacket(TPtrC8& aData, TUint aLun) = 0;
    virtual TInt ReadComplete(TInt aError) = 0;
    virtual TInt Cancel() = 0;
    virtual void ReportHighSpeedDevice() {};
    virtual ~MProtocolBase() {};
#ifdef MSDC_MULTITHREADED
    virtual void InitializeBufferPointers(TPtr8& aDes1, TPtr8& aDes2) = 0;
#endif
    };


#endif // __PROTOCOL_H__

