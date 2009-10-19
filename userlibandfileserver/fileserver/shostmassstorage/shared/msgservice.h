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
// Shared message service definitions 
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __HOSTUSBMSMSGSERVICE_H__
#define __HOSTUSBMSMSGSERVICE_H__

/**
Package structure for a Read or Write request
*/
struct TReadWrite
    {
    /** position */
	TInt64 iPos;
    /** Length */
	TInt iLen;
    };


const TInt KUsbHostMsSrvMajorVersionNumber = 1;
const TInt KUsbHostMsSrvMinorVersionNumber = 0;
const TInt KUsbHostMsSrvBuildVersionNumber = 0;

_LIT(KUsbHostMsServerName, "usbhostmssrv");

/** Types of requests Host USB mass storage file extention and MM can make */
enum TUsbHostMsReqType
	{
	EUsbHostMsRegisterInterface,	// Adds the mass storage device to the server
	EUsbHostMsInitialiseInterface,	// Initialises the interface with the transport and setsup async call for GetMaxLun
	EUsbHostMsUnRegisterInterface,	// Removes the mass storage device from the server
	EUsbHostMsFinalCleanup,	// Called after unregistering the interface to cleanup any final bits in the session
	EUsbHostMsGetNumLun,	// Gets the maximum lun for found in the MS device
	EUsbHostMsRegisterLun,	// Opens a subsession and registers the LunId
	EUsbHostMsUnRegisterLun,// Closes the subsession
	EUsbHostMsShutdown,		// Shuts down the server
	EUsbHostMsNotifyChange,	// Registers with MSC to get any media change notifications
	EUsbHostMsSuspendLun,	// Requests MSC to suspend the logical unit and registers with MSC to invoke when resumed
	EUsbHostMsCapacity,		// Get the logical unit or device capacity
	EUsbHostMsRead,			// Do a read on the logical unit or device
	EUsbHostMsWrite,		// Do a write to the logical unit or device
    EUsbHostMsErase,        // Erase an area on the logical unit or device
    EUsbHostMsForceRemount, // Force a remount of the drive
	EUsbHostMsCancelChangeNotifier, // Cancel the media change notifier
	EUsbHostMsLast		    // This should always be at the end of the enum
	};

#endif //__HOSTUSBMSMSGSERVICE_H__
