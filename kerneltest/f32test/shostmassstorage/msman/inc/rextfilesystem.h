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
// RUsbMsMountManager class.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef REXTFILESYSTEM_H
#define REXTFILESYSTEM_H


typedef TUint32 TToken;
typedef TInt TLun;

class RExtFileSystem
    {
public:
    RExtFileSystem();
    ~RExtFileSystem();

    void OpenL();
    void CloseL();

    TDriveNumber GetDriveL();

    void MountL(RUsbHostMsDevice& aDevice,
                TDriveNumber aDriveNumber,
                TToken aToken,
                TLun aLun);

    void DismountL(RUsbHostMsDevice& aDevice, TDriveNumber aDriveNumber);
    };




#endif // REXTFILESYSTEM_H
