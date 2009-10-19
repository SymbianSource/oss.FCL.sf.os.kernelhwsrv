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
// CDriveManager and CMassStorageDrive classes for USB Mass Storage.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __DRIVEMANAGER_H__
#define __DRIVEMANAGER_H__

#include <e32base.h>			// C Class Definitions, Cleanup Stack 
#include <e32def.h>				// T Type  Definitions
#include <f32fsys.h>
#include <e32property.h>
#include "usbmsshared.h"		// KUsbMsMaxDrives
#include "drivepublisher.h"

// Forward declarations
class CDriveManager;


/**
@internalTechnology

Along with CDriveManager, this provides an interface between the generic SCSI
protocol component and the target drive unit.  The CMassStorageDrive class is 
instantiated by the drive manager, and contains a pointer to the associated 
CProxyDrive that was registered by the Mass Storage File System.
*/
class CMassStorageDrive : public CBase
	{
public:
	/**
	The Drive Mount State Machine.
	*/
	enum TMountState
		{
		/**
		Unmounted
		*/
		EDisconnected,
		/**
		Not mounted, but SCSI started
		*/
		EConnecting,
		/**
		Mounted
		*/
		EConnected,
		/**
		Not unmounted, but SCSI stopped
		*/
		EDisconnecting
		};

	/**
	The Drive Media State Machine.
	*/
	enum TDriveState
		{
		/**
		The media is present and ready for access.
		*/
		EIdle,
		/**
		The media is currently being accessed by Mass Storage. 
		*/
		EActive,
		/**
		The media is present but is password-protected.
		*/
		ELocked,
		/**
		The media is not physically present.
		*/
		EMediaNotPresent,
		/**
		No drive.
		*/
		EErrDisMounted
		};

	CMassStorageDrive(RCriticalSection& aCritSec,
					  RDriveStateChangedPublisher& aDriveStateChangedPublisher);

	~CMassStorageDrive();

	TInt Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia = ETrue);
	TInt Write(const TInt64& aPos, TDesC8& aBuf, TBool aWholeMedia = ETrue);
	TInt Caps(TLocalDriveCapsV4& aCaps);
	inline TMountState MountState() const;
	TDriveState DriveState() const;
	TDriveState CheckDriveState();
#ifndef USB_TRANSFER_PUBLISHER
	inline TUint KBytesRead() const;
	inline TUint KBytesWritten() const;
#endif
	inline TInt SetMountDisconnected();
	inline TInt SetMountConnecting();
	inline TInt SetMountDisconnecting();
	inline TInt SetMountConnected();
	TInt SetMountConnected(CProxyDrive& aProxyDrive, TBool& aMediaChanged);
	TInt SetCritical(TBool aCritical);
	TBool IsMediaChanged(TBool aReset=EFalse);
	TBool IsWholeMediaAccess();

#ifndef USBMSDRIVE_TEST
 private:
#else
 public:
#endif
	// Forward declaration
	struct CLocalDriveRef;

	TInt HandleCriticalError();
	TInt ClearCriticalError();
	TInt DoCaps(TLocalDriveCapsV4& aCaps);
	void SetDriveState(TDriveState aNewState);
	TInt SetMountState(TMountState aNewState, CLocalDriveRef* aLocalDrive=NULL);
	CProxyDrive& SafeProxyDrive() const;
	
	/**
	A Critical Section, shared by all instances of CMassStorageDrive, used to ensure 
	that iMountState and iProxyDrive are changed atomically.
	*/
	RCriticalSection& iCritSec;
	/**
	The Drive Mount state machine
	*/
	TMountState iMountState;
	
#ifndef USB_TRANSFER_PUBLISHER
	/**
	Cumulative bytes read
	*/
	TInt64 iBytesRead;
	/**
	Cumulative bytes written
	*/
	TInt64 iBytesWritten;
#endif

	/**
	When Connected, references to CProxyDrive and TBusLocalDrive's Media Changed flag.
	*/
	CLocalDriveRef* iLocalDrive;
	/**
	Publisher for media errors.
	*/
	RDriveMediaErrorPublisher iDriveMediaErrorPublisher;
	/**
	Reference to publisher for tracking drive state changes.
	*/
	RDriveStateChangedPublisher& iDriveStateChangedPublisher;
	/**
	Indicates whether whole media access is permitted. 
	*/
	TBool iWholeMediaAccess;
	};


/**
@internalTechnology

Along with CMassStorageDrive, this provides an interface between the generic SCSI
protocol component and the target drive unit.  This package is responsible for 
maintaining the list of registered drives.  The owner of the controller registers 
each drive it wishes to make available to USB Mass Storage along with an 
associated Logical Drive Unit identifier.  The SCSI protocol contains a reference 
to the drive manager in order to route the incoming request to a drive.  
*/
class CDriveManager : public CBase
	{
public:
	/**
	The Logical Drive Unit Identifiers (LUN) must be in the range 0..7 due to the
	fact that the status for all drives is encoded into one 32-bit word.
	*/
	enum { KAllLuns = 0xff };

	static CDriveManager* NewL(TRefDriveMap aDriveMap);
	~CDriveManager();

	TInt RegisterDrive(CProxyDrive& aProxyDrive, TBool& aMediaChanged, TUint aLun);
	TInt DeregisterDrive(TUint aLun);
	CMassStorageDrive* Drive(TUint aLun, TInt& aError) const;
	TInt Connect(TUint aLun);
	TInt Disconnect(TUint aLun);
	TBool IsMediaChanged(TUint aLun, TBool aReset=EFalse);
	TInt SetCritical(TUint aLun, TBool aCritical);

private:
	// private default constructor to ensure that NewL is used
	CDriveManager(const RArray<TInt>& aDriveMap);
	void ConstructL();

public:
	/**
	The array of drives.  The index into the array is a LUN.
	*/
	TFixedArray<CMassStorageDrive*,KUsbMsMaxDrives> iDrives;

private:
	/**
	For converting LUN to Drive Number.
	*/
	const RArray<TInt>& iDriveMap;
	/**
	A resource owned by DriveManager but used by the Drive objects.
	*/
	RCriticalSection iDriveCritSec;

	/**
	Publisher for tracking drive state changes.
	*/
	RDriveStateChangedPublisher* iDriveStateChangedPublisher;
	};

#include "drivemanager.inl"

#endif //__DRIVEMANAGER_H__
