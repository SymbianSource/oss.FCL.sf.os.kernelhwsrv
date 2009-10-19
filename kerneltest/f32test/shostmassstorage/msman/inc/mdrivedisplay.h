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


#ifndef MDRIVEDISPLAY_H
#define MDRIVEDISPLAY_H

class MDriveDisplay
    {
public:
    virtual void DriveListL() const = 0;
    virtual void DevicesNumber(TInt aDevicesNumber) const = 0;
    virtual void DriveMapL(const TDriveMap& aDriveMap) const = 0;
    virtual void DeviceMapL(TInt aRow, TInt deviceIndex, const TDeviceMap& aDeviceMap) const = 0;
    virtual void DeviceMapClear(TInt deviceIndex) const = 0;
    };


#endif
