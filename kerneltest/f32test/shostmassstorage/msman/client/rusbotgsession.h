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

#ifndef RUSBOTGSESSION_H
#define RUSBOTGSESSION_H

static const TUid KUsbOtgServerUid3 = {0x101fe1db};

class RUsbOtgSession : public RSessionBase
	{
public:
    IMPORT_C RUsbOtgSession();
    IMPORT_C RUsbOtgSession(TInt aParam);

    IMPORT_C TInt Connect();

    IMPORT_C TBool DeviceInserted();
    IMPORT_C void NotifyChange(TBool& aChanged, TRequestStatus& aStatus);
    IMPORT_C TInt NotifyChangeCancel();
    IMPORT_C TInt BusDrop();

private:
	TInt StartServer();
	TVersion Version() const;
	};


#endif // RUSBOTGSESSION_H
