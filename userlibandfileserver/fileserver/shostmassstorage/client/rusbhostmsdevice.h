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
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef RUSBHOSTMSDEVICE_H
#define RUSBHOSTMSDEVICE_H

#include <shared.h>


struct TMassStorageUnitInfo
    {
    TUint32 iLunID;
    TUint32 iSpare1;
    TUint32 iSpare2;
    };

/**
Creates a session over which a USB device mass storage interface can be added/removed
*/
class RUsbHostMsDevice : public RSessionBase
	{
public:
	IMPORT_C RUsbHostMsDevice();
	IMPORT_C void Add(const THostMassStorageConfig& aConfig, TRequestStatus& aStatus);
    IMPORT_C void Remove();
	IMPORT_C TInt GetNumLun(TUint32& aNumLuns);
	IMPORT_C TInt MountLun(TUint32 aLunId, TInt aDriveNum);
	IMPORT_C TInt DismountLun(TInt aDriveNum);
private:
	TInt StartServer();
	TVersion Version() const;
	};

#endif // RUSBHOSTMSDEVICE_H
