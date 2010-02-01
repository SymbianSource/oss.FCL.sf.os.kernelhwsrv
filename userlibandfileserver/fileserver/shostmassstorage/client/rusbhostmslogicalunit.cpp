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

#include <e32std.h>

#include "msgservice.h"
#include "rusbhostmsdevice.h"
#include "rusbhostmslogicalunit.h"
#include "debug.h"

TVersion RUsbHostMsLogicalUnit::Version() const
	{
	return(TVersion(KUsbHostMsSrvMajorVersionNumber,
                    KUsbHostMsSrvMinorVersionNumber,
                    KUsbHostMsSrvBuildVersionNumber));
	}

EXPORT_C RUsbHostMsLogicalUnit::RUsbHostMsLogicalUnit()
	{
	// Intentionally left blank
	}

/**
Send a command to initialise the Mass Storage device.

@param aMsg
@param aDevHandleIndex
@param aLun

@return TInt
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::Initialise(const RMessage2& aMsg,
                                                TInt aDevHandleIndex,
												TUint32 aLun)
	{
	__FNLOG("RUsbHostMsLogicalUnit::Initialise");
	TInt r = dev.Open(aMsg, aDevHandleIndex);
	if (r != KErrNone)
		{
		__PRINT1(_L("Session handle can not be opened %d"),r);
		return r;
		}

	r = CreateSubSession(dev, EUsbHostMsRegisterLun, TIpcArgs(aLun));
	if (r != KErrNone)
		{
		__PRINT1(_L("SubSession creation failed %d"),r);
		return r;
		}
	return r;
	}


/**
Send a command to read the Mass Storage device.

@param aPos Position to start reading from
@param aLength Number of Bytes
@param aTrg Buffer to copy data to

@return TInt KErrNone, if the send operation is successful;
	KErrServerTerminated, if the server no longer present; KErrServerBusy, if there are no message slots available;
	KErrNoMemory, if there is insufficient memory available.
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::Read(TInt64 aPos, TInt aLength, TDes8& aTrg)
	{
	__FNLOG("RUsbHostMsLogicalUnit::Read");

	TReadWrite data;
	data.iPos = aPos;
	data.iLen = aLength;

	__PRINT2(_L("pos = 0x%lx, len = x%x"), data.iPos, data.iLen);

	TPckg<TReadWrite> pckg(data);
	/* We handle the message asynchronously in the thread modelled MSC */
	TRequestStatus	status;
	SendReceive(EUsbHostMsRead, TIpcArgs(&pckg, &aTrg), status);
	User::WaitForRequest(status);
    __PRINT2(_L("pos = 0x%lx, len = x%x"), data.iPos, data.iLen);
	return status.Int();
	}


/**
Send a command to write to the Mass Storage device.

@param aPos Position to start reading from
@param aLength Number of Bytes
@param aTrg Buffer to copy data from

@return TInt KErrNone, if the send operation is successful;
	KErrServerTerminated, if the server no longer present; KErrServerBusy, if there are no message slots available;
	KErrNoMemory, if there is insufficient memory available.
available.
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::Write(TInt64 aPos, TInt aLength, const TDesC8& aTrg)
	{
	__FNLOG("RUsbHostMsLogicalUnit::Write");

	TReadWrite data;
	data.iPos = aPos;
	data.iLen = aLength;

	__PRINT2(_L("pos = 0x%lx, len = x%x"), data.iPos, data.iLen);

	TPckg<TReadWrite> pckg(data);
	/* We handle the message asynchronously in the thread modelled MSC */
	TRequestStatus	status;
	SendReceive(EUsbHostMsWrite, TIpcArgs(&aTrg, &pckg), status);
	User::WaitForRequest(status);
	return status.Int();
	}


/**
Send a command to erase an area of the Mass Storage device.

@param aPos Position to start reading from
@param aLength Number of Bytes

@return TInt KErrNone, if the send operation is successful;
	KErrServerTerminated, if the server no longer present; KErrServerBusy, if there are no message slots available;
	KErrNoMemory, if there is insufficient memory available.
available.
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::Erase(TInt64 aPos, TInt aLength)
	{
	__FNLOG("RUsbHostMsLogicalUnit::Erase");

	TReadWrite data;
	data.iPos = aPos;
	data.iLen = aLength;

	__PRINT2(_L("pos = 0x%lx, len = x%x"), data.iPos, data.iLen);

	TPckg<TReadWrite> pckg(data);
	/* We handle the message asynchronously in the thread modelled MSC */
	TRequestStatus	status;
	SendReceive(EUsbHostMsErase, TIpcArgs(&pckg), status);
	User::WaitForRequest(status);
	return status.Int();
	}


/**
Send a command to get the nedia's capacity info.

@param aCapsInfo [OUT] A buffer to copy the capacity info to.

@return TInt KErrNone, if the send operation is successful;
KErrServerTerminated, if the server no longer present; KErrServerBusy, if there
are no message slots available; KErrNoMemory, if there is insufficient memory
available.
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::Caps(TCapsInfo& aCapsInfo)
	{
	__FNLOG("RUsbHostMsLogicalUnit::Caps");

    TPckg<TCapsInfo> data(aCapsInfo);

	/* We handle the message asynchronously in the thread modelled MSC */
	TRequestStatus	status;
	SendReceive(EUsbHostMsCapacity, TIpcArgs(&data),status);
	User::WaitForRequest(status);
	return status.Int();
	}

/**
Request notification of media change to the file server

@param aChanged The descriptor pointing to iChanged flag in TDrive to be updated
when error occurs during read or write.
@param aStatus The request status This is set to KErrNone on completion, or KErrCancel when the logical unit is closed;
	KErrServerTerminated, if the server no longer present; KErrServerBusy, if there are no message slots available;
	KErrNoMemory, if there is insufficient memory available.
@return None
*/
EXPORT_C void RUsbHostMsLogicalUnit::NotifyChange(TDes8& aChanged, TRequestStatus &aStatus)
	{
	__FNLOG("RUsbHostMsLogicalUnit::NotifyChange");

	SendReceive(EUsbHostMsNotifyChange, TIpcArgs(&aChanged), aStatus);
	}

/**
Request to suspend the logical unit associated with this drive
*/
EXPORT_C void RUsbHostMsLogicalUnit::SuspendLun()
	{
	__FNLOG("RUsbHostMsLogicalUnit::SuspendLun");

	SendReceive(EUsbHostMsSuspendLun, TIpcArgs(NULL));
	}

/**
Close the sub-session.

@return TInt KErrNone
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::UnInitialise()
	{
	__FNLOG("RUsbHostMsLogicalUnit::UnInitialise");

	CloseSubSession(EUsbHostMsUnRegisterLun);
	dev.Close();
	return KErrNone;
	}


/**
Request that the drive is remounted.

@param aFlags Flags to be passed to the drive

@return EXPORT_C TInt KErrNone, if the send operation is successful;
KErrServerTerminated, if the server no longer present; KErrServerBusy, if there
are no message slots available; KErrNoMemory, if there is insufficient memory
available.
*/
EXPORT_C TInt RUsbHostMsLogicalUnit::ForceRemount(TUint aFlags)
	{
	__FNLOG("RUsbHostMsLogicalUnit::ForceRemount");

	__PRINT1(_L("flags = %d"), aFlags);

	TRequestStatus	status;
	SendReceive(EUsbHostMsForceRemount, TIpcArgs(aFlags), status);
	User::WaitForRequest(status);
	return status.Int();
	}

EXPORT_C void RUsbHostMsLogicalUnit::NotifyChangeCancel()
	{
	__FNLOG("RUsbHostMsLogicalUnit::NotifyChangeCancel");
	SendReceive(EUsbHostMsCancelChangeNotifier, TIpcArgs(NULL));
	}
