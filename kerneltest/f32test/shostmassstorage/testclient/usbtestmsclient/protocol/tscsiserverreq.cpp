// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include <e32def.h>
#include <e32cmn.h>
#include <e32des8.h>
#include <e32std.h>
#include <e32base.h>

#include "msctypes.h"
#include "tscsiserverreq.h"
#include "tscsiservercmds.h"
#include "debug.h"
#include "msdebug.h"

TScsiServerReq* TScsiServerReq::CreateL(TScsiServerReq::TOperationCode aOperationCode,
                                        TDesC8& aDes)
    {
    __MSFNSLOG
    TScsiServerReq* req = NULL;

    __SCSIPRINT1(_L("SCSI CMD 0x%x"), aOperationCode);

    switch (aOperationCode)
        {
        case ETestUnitReady:
            {
            req = new (ELeave) TScsiServerTestUnitReadyReq;
            }
            break;
        case ERequestSense:
            {
            req = new (ELeave) TScsiServerRequestSenseReq;
            }
            break;
        case EInquiry:
            {
            req = new (ELeave) TScsiServerInquiryReq;
            }
            break;
        case EModeSense6:
            {
            req = new (ELeave) TScsiServerModeSense6Req;
            }
            break;
        case EStartStopUnit:
            {
            req = new (ELeave) TScsiServerStartStopUnitReq;
            }
            break;
        case EPreventMediaRemoval:
            {
            req = new (ELeave) TScsiServerPreventMediaRemovalReq;
            }
            break;
        case EReadFormatCapacities:
            {
            req = new (ELeave) TScsiServerReadFormatCapacitiesReq;
            }
            break;
        case EReadCapacity10:
            {
            req = new (ELeave) TScsiServerReadCapacity10Req;
            }
            break;

        case ERead10:
            {
            req = new (ELeave) TScsiServerRead10Req;
            }
            break;
        case EWrite10:
            {
            req = new (ELeave) TScsiServerWrite10Req;
            }
            break;
        default:
            {
            __SCSIPRINT(_L("NOT SUPPORTED!"));
            User::Leave(KErrNotSupported);
            }
        }

    if (req)
        {
        CleanupStack::PushL(req);
        // got a class into which it can parse itself from the protocol
        req->DecodeL(aDes);
        CleanupStack::Pop(req);
        }
    return req;
    }


void TScsiServerReq::DecodeL(const TDesC8& aPtr)
    {
    __MSFNLOG
    // OPERATION CODE
	iOperationCode = static_cast<TOperationCode>(aPtr[0]);

    // CONTROL byte
    TInt length = 0;
    switch (GetGroupCode())
        {
        case 0:
            // 6 byte commands
            length = 6;
            break;
        case 1:
        case 2:
            // 10 byte commands
            length = 10;
            break;
        default:
            User::Leave(KErrNotSupported);
            break;
        }


    // CONTROL byte is last element of Command
    TUint8 control = aPtr[length - 1];

    iLink = control & 0x04 ? ETrue : EFalse;
    iNaca = control & 0x01 ? ETrue : EFalse;
    }
