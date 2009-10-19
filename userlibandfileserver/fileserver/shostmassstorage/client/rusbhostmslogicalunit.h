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

/**
 @file
 @internalTechnology
*/

#ifndef RUSBHOSTMSLOGICALUNIT_H
#define RUSBHOSTMSLOGICALUNIT_H


/**
Provides a subsession to the USB Mass Storage Host server. This class is used to
create a channel to interface to a single logical unit supported by the attached
device.
*/
class RUsbHostMsLogicalUnit : public RSubSessionBase
	{
public:
	/* Constructor */
	IMPORT_C RUsbHostMsLogicalUnit();
	IMPORT_C TInt Initialise(const RMessage2& aMsg, TInt aDevHandleIndex, TUint32 aLun);
	IMPORT_C TInt Read(TInt64 aPos, TInt aLength, TDes8& aTrg);
	IMPORT_C TInt Write(TInt64 aPos, TInt aLength, const TDesC8& aTrg);
	IMPORT_C TInt Erase(TInt64 aPos, TInt aLength);
	IMPORT_C TInt Caps(TCapsInfo& aCapsInfo);
	IMPORT_C void NotifyChange(TDes8& aChanged, TRequestStatus& aStatus);
	IMPORT_C void SuspendLun();
    IMPORT_C TInt UnInitialise();
    IMPORT_C TInt ForceRemount(TUint aFlags);
	IMPORT_C void NotifyChangeCancel();

private:
	TVersion Version() const;
	RUsbHostMsDevice dev;
	};

#endif // RUSBHOSTMSLOGICALUNIT_H
