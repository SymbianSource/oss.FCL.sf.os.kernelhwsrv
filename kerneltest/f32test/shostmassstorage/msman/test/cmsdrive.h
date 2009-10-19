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
 @internalTechnology
*/

#ifndef CMSDRIVE_H
#define CMSDRIVE_H

class CMsDrive: public CBase
	{
public:
	static CMsDrive* NewL();
	~CMsDrive();
protected:
    void ConstructL();
	CMsDrive();

public:
    void GetInitialDriveMapL();
    TBool GetUsbDeviceL();

    void SetSessionPathL();
    const TFileName& GetSessionPath() const;
    TInt DriveNumber() const;
    TBool DrivePresent() const;

private:
    TInt iDriveNumber;
    TFileName iSessionPath;

    TDriveList iOriginalDrivelist;

    // Drive list after new MS device has been mounted
    TDriveList iMsDrivelist;
	};


inline TInt CMsDrive::DriveNumber() const
    {
    return iDriveNumber;
    }

#endif // CMSDRIVE_H
