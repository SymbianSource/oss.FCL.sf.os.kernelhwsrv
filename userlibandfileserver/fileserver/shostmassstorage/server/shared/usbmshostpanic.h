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

#ifndef USBMSHOSTPANIC_H
#define USBMSHOSTPANIC_H

_LIT(KUsbMsHostPanicCat, "UsbMsHost");

enum TUsbMsHostPanic
	{
	EMsClientInvalidSessCount,
	EDeviceThreadDoesNotExist,
	EBlockLengthNotSet,
    ESbcInterfaceMissing,
    EBlockDevice,
    EProtocolNotFree,
    EMsFsmEvent
	};

#endif // USBMSHOSTPANIC_H

