// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementations of inline functions declared in drivemanager.h.
// 
//

/**
 @file
 @internalTechnology
 
 @return Current drive mount state
*/
inline CMassStorageDrive::TMountState CMassStorageDrive::MountState() const
	{
	return iMountState;
	}

/**
Set the mount state
*/
inline TInt CMassStorageDrive::SetMountDisconnected()
	{
	return SetMountState(EDisconnected);
	}

/**
Set the mount state
*/
inline TInt CMassStorageDrive::SetMountConnecting()
	{
	return SetMountState(EConnecting);
	}

/**
Set the mount state
*/
inline TInt CMassStorageDrive::SetMountDisconnecting()
	{
	return SetMountState(EDisconnecting);
	}

/**
Set the mount state
*/
inline TInt CMassStorageDrive::SetMountConnected()
	{
	iCritSec.Wait();
	return SetMountState(EConnected);
	}

/**
 Is Whole Media Access Permitted for Mass Storage Drive
 */
inline TBool CMassStorageDrive::IsWholeMediaAccess()
	{
	return iWholeMediaAccess;
	}

#ifndef USB_TRANSFER_PUBLISHER
/**
@return Cumulative bytes read since the host connected to the drive, 
        in multiples of KBytesPerKilobyte rounded to nearest integer value.
        The KBytes refers to multiples of 1000, not 1024.
*/
inline TUint CMassStorageDrive::KBytesRead() const
	{
	return I64LOW(iBytesRead / (TUint64)1000);
	}

/**
@return Cumulative bytes written since the host connected to the drive, 
        in multiples of KBytesPerKilobyte rounded to nearest integer value.
        The KBytes refer to multiples of 1000, not 1024.
*/
inline TUint CMassStorageDrive::KBytesWritten() const
	{
	return I64LOW(iBytesWritten / (TUint64)1000);
	}

#else
/**
Transfer function for the property

@return Cumulative bytes read since the host connected to the drive, 
        in multiples of KBytesPerKilobyte rounded to nearest integer value.
        The KBytes refer to multiples of 1000, not 1024.
*/
inline TUint CUsbTransferPublisher::GetBytesTransferred(TUint aLun) const
{
	return I64LOW(iArray[aLun] / (TUint64)1000);
}

#endif // USB_TRANSFER_PUBLISHER
