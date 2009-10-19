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



/**
 @file
*/

#ifndef MSTYPES_H
#define MSTYPES_H

class TMassStorageConfig
    {
public:
    TBuf<8>     iVendorId;
    TBuf<16>    iProductId;
    TBuf<4>     iProductRev;
    };


/**
@publishedPartner
@released

The maximum number of removable drives supported by Mass Storage.
*/
const TUint KUsbMsMaxDrives=8;

/**
@publishedPartner
@released

A buffer to receive the EUsbMsDriveState_DriveStatus property.
For each drive there is a pair of bytes; the drive number
followed by the EUsbMsDriveStates status code.
*/
typedef TBuf8<2*KUsbMsMaxDrives> TUsbMsDrivesStatus;

class CMassStorageDrive;
// typedefs

typedef const RArray<TInt>& TRefDriveMap;




#endif // MSTYPES_H
