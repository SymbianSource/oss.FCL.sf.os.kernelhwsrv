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
// CDriveManager and CMassStorageDrive classes for USB Mass Storage.
//
//

/**
 @file
 @internalTechnology
*/

#ifndef SMASSSTORAGE_H
#define SMASSSTORAGE_H

_LIT(KUsbMsSvrPncCat, "CUsbMsServer");

enum TUsbPanicServer
    {
    EMsCBulkOnlyTransportNull = 3300,
    EMsCDeviceStateNotifierNull,
    EMsClientInvalidSessCount,
    EMsControlInterfaceBadState,
    EMsControlInterfaceStillActive,
    EMsBulkOnlyStillActive,
    EMsWrongEndpoint,
    EMsInvalidLun,
    EMsThreadWriteToDrive = 3400,
    EMsThreadIsRecentlyWritten,
    EMsThreadReadDriveThread,
    EMsCMassStorageDriveWrite = 3500,
    EMsCMassStorageDriveSetMountState_iLocalDrive,
    EMsCMassStorageDriveSetMountState_aLocalDrive,
    EMsCMassStorageDriveSetDriveState_State,
    EMsCMassStorageDriveSetDriveState_Mount
    };

#endif
