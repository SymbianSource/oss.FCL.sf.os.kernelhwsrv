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
// usbmshostpanic.h
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef USBMSCLIENTPANIC_H
#define USBMSCLIENTPANIC_H

_LIT(KUsbMsClientPanicCat, "UsbMsClient");

enum TUsbMsClientPanic
	{
    EErrInUse,
	EMsWrongEndpoint,
	EMsBulkOnlyStillActive,
	EMsControlInterfaceBadState,
	EMsControlInterfaceStillActive
	};

/*
_LIT(KUsbMsSvrPncCat, "CUsbMsServer");

enum TUsbPanicServer
	{
	EMsClientInvalidSessCount,
	EMsControlInterfaceBadState,
	EMsControlInterfaceStillActive,
	EMsBulkOnlyStillActive,
	EMsWrongEndpoint
	};
*/

#endif // USBMSCLIENTPANIC_H
