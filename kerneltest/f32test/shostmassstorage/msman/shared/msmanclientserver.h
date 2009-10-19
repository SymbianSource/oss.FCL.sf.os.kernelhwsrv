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

#ifndef MSMANCLIENTSERVER_H
#define MSMANCLIENTSERVER_H

/**
   USB HOST SERVER

 */
const TInt KUsbHostSrvMajorVersionNumber = 1;
const TInt KUsbHostSrvMinorVersionNumber = 0;
const TInt KUsbHostSrvBuildVersionNumber = 0;

_LIT(KUsbHostServerName, "usbhostserver");


enum TUsbHostMessages
    {
    EUsbHostStart
    };


/**
   USB OTG SERVER

 */
const TInt KUsbOtgSrvMajorVersionNumber = 1;
const TInt KUsbOtgSrvMinorVersionNumber = 0;
const TInt KUsbOtgSrvBuildVersionNumber = 0;

_LIT(KUsbOtgServerName, "usbotgserver");


enum TUsbOtgMessages
    {
    EUsbOtgDeviceInserted,
    EUsbOtgNotifyChange,
    EUsbOtgNotifyChangeCancel,
	EUsbOtgBusDrop,
    };

#endif // MSMANCLIENTSERVER_H


