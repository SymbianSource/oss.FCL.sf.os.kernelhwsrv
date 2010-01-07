// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32base.h>

#include "debug.h"
#include "msdebug.h"
#include "mprotocol.h"
#include "tscsiclientreq.h"


TInt TScsiClientReq::EncodeRequestL(TDes8& aBuffer) const
    {
    __MSFNSLOG
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
    aBuffer.SetLength(length);
    aBuffer.FillZ();

    // OPERATION CODE
    aBuffer[0] = iOperationCode;

    // CONTROL byte is last element of Command
    aBuffer[length - 1] = GetControlByte();
    return length;
    }

