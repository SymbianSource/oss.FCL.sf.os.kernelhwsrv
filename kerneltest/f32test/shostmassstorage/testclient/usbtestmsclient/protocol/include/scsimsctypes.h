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
// SCSI Protocol layer for USB Mass Storage
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef SCSIMSCTYPES_H
#define SCSIMSCTYPES_H


class TPeripheralInfo
    {
public:
    TBool iRemovable;

    TUint8 iPeripheralQualifier;
    TUint8 iPeripheralDeviceType;
    TUint8 iResponseDataFormat;
    TUint8 iVersion;
    TMassStorageConfig iIdentification;
    };

#include "scsimsctypes.inl"

#endif // SCSIMSCTYPES_H
