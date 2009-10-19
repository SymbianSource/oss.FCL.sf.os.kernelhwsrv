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
// CDriveManager and CMassStorageDrive classes for USB Mass Storage.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef DRIVEMANAGER_H
#define DRIVEMANAGER_H

#include "d32locd.h"

// Forward declarations
class CDriveManager;
class RDriveMediaErrorPublisher;
class RDriveStateChangedPublisher;
class TLocalDriveCapsV4;
class CProxyDrive;



class TMediaParams
    {
public:
    const TUint32 KDefaultBlockSize = 0x200;  //default block size for FAT

    void Init(TLocalDriveCapsV4& aCaps);

    TUint32 NumBlocks() const {return iNumBlocks;}
    TUint64 Size() const {return iSize;};
    TUint32 BlockSize() const {return KDefaultBlockSize;}
    TBool IsWriteProtected() const {return iMediaAtt & KMediaAttWriteProtected ? ETrue : EFalse;}
    TBool IsLocked() const {return iMediaAtt & KMediaAttLocked ? ETrue : EFalse;}

private:
    TLocalDriveCapsV4 iCaps;

    TUint32 iNumBlocks;
    TInt64 iSize;
    TUint iMediaAtt;
    };


/**
A private structure that, when Connected, holds references to
the CProxyDrive and the corresponding TBusLocalDrive's Media Changed flag.
*/
class TLocalDriveRef
	{
public:
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


    TLocalDriveRef(CProxyDrive& aProxyDrive,
                   TBool& aMediaChanged,
                   RDriveStateChangedPublisher& aDriveStateChangedPublisher);

    void SetDriveState(TDriveState aState);
    TInt Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia);
    TInt Write(const TInt64& aPos, TDesC8& aBuf, TBool aWholeMedia);
    TBool IsMediaChanged(TBool aReset);
    TInt SetCritical(TBool aCritical);
    TDriveState DriveState() const;
    TInt Caps(TLocalDriveCapsV4& aInfo);

private:
    static TBool IsActive(TDriveState aDriveState);

private:
	CProxyDrive& iProxyDrive;

	TBool& iMediaChanged;
	/**
	The Drive Media state machine
	*/
	TDriveState iDriveState;

    /**
    Reference to publisher for tracking drive state changes.
    */
    RDriveStateChangedPublisher& iDriveStateChangedPublisher;
	};


inline TLocalDriveRef::TLocalDriveRef(CProxyDrive& aProxyDrive,
                                      TBool& aMediaChanged,
                                      RDriveStateChangedPublisher& aDriveStateChangedPublisher)
:   iProxyDrive(aProxyDrive),
    iMediaChanged(aMediaChanged),
    iDriveState(EIdle),
    iDriveStateChangedPublisher(aDriveStateChangedPublisher)
	{
	}


inline TBool TLocalDriveRef::IsActive(TLocalDriveRef::TDriveState aDriveState)
	{
	return aDriveState==TLocalDriveRef::EActive;
	}


inline TLocalDriveRef::TDriveState TLocalDriveRef::DriveState() const
    {
    return iDriveState;
    }


/**
@internalTechnology

Along with CDriveManager, this provides an interface between the generic SCSI
protocol component and the target drive unit.  The CMassStorageDrive class is
instantiated by the drive manager, and contains a pointer to the associated
CProxyDrive that was registered by the Mass Storage File System.
*/
class CMassStorageDrive: public CBase
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

public:
	static CMassStorageDrive* NewL(RCriticalSection& aCritSec,
                                   RDriveStateChangedPublisher& aDriveStateChangedPublisher);
	~CMassStorageDrive();

private:
    void ConstructL();
	CMassStorageDrive(RCriticalSection& aCritSec,
					  RDriveStateChangedPublisher& aDriveStateChangedPublisher);

public:

	TInt Read(const TInt64& aPos, TInt aLength, TDes8& aBuf, TBool aWholeMedia = ETrue);
	TInt Write(const TInt64& aPos, TDesC8& aBuf, TBool aWholeMedia = ETrue);

	TMountState MountState() const;
	TLocalDriveRef::TDriveState DriveState() const;
	TLocalDriveRef::TDriveState CheckDriveState();
	void SetMountDisconnected();
	void SetMountConnecting();
	void SetMountDisconnecting();
	void SetMountConnected();
	void SetMountConnectedL(CProxyDrive& aProxyDrive, TBool& aMediaChanged, RDriveStateChangedPublisher& aDriveStateChangedPublisher);
	TInt SetCritical(TBool aCritical);
	TBool IsMediaChanged(TBool aReset=EFalse);

    const TMediaParams& MediaParams() const {return iMediaParams;}

 private:
	TInt HandleCriticalError();
	TInt ClearCriticalError();
	TInt DoCaps(TLocalDriveCapsV4& aCaps);
	void SetDriveState(TLocalDriveRef::TDriveState aNewState);
	void SetMountState(TMountState aNewState, TBool aCriticalSection = EFalse);

private:
	/**
	A Critical Section, shared by all instances of CMassStorageDrive, used to ensure
	that iMountState and iProxyDrive are changed atomically.
	*/
	RCriticalSection& iCritSec;
	/**
	The Drive Mount state machine
	*/
	TMountState iMountState;

	/**
	When Connected, references to CProxyDrive and TBusLocalDrive's Media Changed flag.
	*/
	TLocalDriveRef* iLocalDrive;
	/**
	Publisher for media errors.
	*/
	RDriveMediaErrorPublisher* iDriveMediaErrorPublisher;
	/**
	Reference to publisher for tracking drive state changes.
	*/
	RDriveStateChangedPublisher& iDriveStateChangedPublisher;

    TMediaParams iMediaParams;
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

	static CDriveManager* NewL(const TLunToDriveMap& aDriveMap);
	~CDriveManager();

	void RegisterDriveL(CProxyDrive& aProxyDrive, TBool& aMediaChanged, TLun aLun);
	void DeregisterDrive(TLun aLun);
	CMassStorageDrive* Drive(TLun aLun) const;

	void Connect();
	void Connect(TLun aLun);

    void Disconnect();
    void Disconnect(TLun aLun);

	TBool IsMediaChanged(TLun aLun, TBool aReset = EFalse);
	TInt SetCritical(TLun aLun, TBool aCritical);

    TLun MaxLun() const;

private:
	// private default constructor to ensure that NewL is used
	CDriveManager(TLun aLun);
	void ConstructL(const TLunToDriveMap& aDriveMap);

private:
	/**
	The array of drives.  The index into the array is a LUN.
	*/
	TMsDriveList iDrives;

	/**
	Publisher for tracking drive state changes.
	*/
	RDriveStateChangedPublisher* iDriveStateChangedPublisher;

	TLun iMaxLun;
	/**
	A resource owned by DriveManager but used by the Drive objects.
	*/
	RCriticalSection iDriveCritSec;
	};

#include "drivemanager.inl"

#endif // DRIVEMANAGER_H
