// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Symbian Foundation License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
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

#ifndef TMSDRIVE_H
#define TMSDRIVE_H

class TMsDrive
	{
public:

    void GetUsbDeviceL();
    void SetSessionPathL();
    const TFileName& GetSessionPath() const;
    TInt DriveNumber() const;
    TBool DrivePresent() const;

private:
    TInt iDriveNumber;
    TFileName iSessionPath;
	};


inline TInt TMsDrive::DriveNumber() const
    {
    return iDriveNumber;
    }

#endif // TMSDRIVE_H
