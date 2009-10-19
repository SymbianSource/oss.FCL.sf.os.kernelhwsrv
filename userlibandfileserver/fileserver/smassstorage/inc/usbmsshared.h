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
// Shared client/server definitions
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalAll
 @released
*/

#ifndef __USBMSSHARED_H__
#define __USBMSSHARED_H__

#include <e32std.h>
#include <f32file.h>


const TInt KUsbMsSrvMajorVersionNumber = 1;
const TInt KUsbMsSrvMinorVersionNumber = 0;
const TInt KUsbMsSrvBuildVersionNumber = 0;

const TInt KUsbMsResourceVersion = 0;

_LIT(KUsbMsServerName, "usbmsserver");

class TMassStorageConfig
    {
public:
    TBuf<8>     iVendorId;
    TBuf<16>    iProductId;
    TBuf<4>     iProductRev;
    };

/** Types of requests USB mass storage class controller can make */
enum TUsbMsReqType
	{
	EUsbMsStart,
	EUsbMsStop,
	EUsbMsShutdown,
	};

/**
@publishedPartner
@released

The USB Class Controller identifier.
*/
const TUid KUsbMsClassControllerUID={0x10204BBC};

/**
@publishedPartner
@released

The Publish & Subscribe Category for all USB Mass Storage events.
*/
const TUid KUsbMsDriveState_Category={KFileServerUidValue};

/**
@publishedPartner
@released

The Publish & Subscribe event subkey enumeration.
*/
enum TUsbMsDriveState_Subkey 
	{
	EUsbMsDriveState_DriveStatus, 
	EUsbMsDriveState_KBytesRead, 
	EUsbMsDriveState_KBytesWritten,
	EUsbMsDriveState_MediaError
	};

/**
@publishedPartner
@released

Possible values for each of EUsbMsDriveState_DriveStatus status codes.
*/
enum EUsbMsDriveStates
	{
	/** File system not available for Mass Storage */
	EUsbMsDriveState_Disconnected	=0x0,
	/** Host has required connection */
	EUsbMsDriveState_Connecting		=0x1,
	/** File system available to Mass Storage */
	EUsbMsDriveState_Connected		=0x2,
	/** Disconnecting from Mass Storage */
	EUsbMsDriveState_Disconnecting	=0x3,
	/** Critical write - do not remove card */
	EUsbMsDriveState_Active			=0x4,
	/** Connected, but locked with a password */
	EUsbMsDriveState_Locked			=0x5,
	/** Connected, but card not present */
	EUsbMsDriveState_MediaNotPresent=0x6,
	/** Card removed while active */
	EUsbMsDriveState_Removed		=0x7,
	/** General error */
	EUsbMsDriveState_Error			=0x8
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

/**
@publishedPartner
@released

A collection of integers, contained in a TBuf8 for compatibility
with RProperty.  Used for the EUsbMsDriveState_KBytesRead/Written
events.
*/
class TUsbMsBytesTransferred : public TBuf8<KUsbMsMaxDrives*sizeof(TInt)>
	{
	public:
	inline TUsbMsBytesTransferred();
	inline TInt& operator[](TUint aLun);
	};
/**
@publishedPartner
@released */
TUsbMsBytesTransferred::TUsbMsBytesTransferred()
	{
	SetLength(MaxLength());
	Fill(0);
	}

/**
@publishedPartner
@released

Return one of the integers contained in this buffer.
@return Bytes transferred count for the specified drive.
@param aLun Drive index
*/
TInt& TUsbMsBytesTransferred::operator[](TUint aLun)
	{
	__ASSERT_DEBUG(aLun < static_cast<TUint>(MaxLength()), User::Invariant());
	return *(reinterpret_cast<TInt*>(const_cast<TUint8*>(Ptr())) + aLun);
	}

#endif //__USBMSSHARED_H__
