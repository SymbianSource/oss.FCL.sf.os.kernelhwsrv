// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Drive publishing classes for USB Mass Storage.
// RDriveMediaErrorPublisher,
// RDriveStateChangedPublisher, 
// CDriveTransferPublisher,
// CDriveWriteTransferPublisher,
// CDriveReadTransferPublisher,
// CUsbTransferPublisher,
// CUsbReadTransferPublisher,
// CUsbReadTransferPublisher.
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __DRIVEPUBLISHER_H__
#define __DRIVEPUBLISHER_H__

#include <e32base.h>			// C Class Definitions, Cleanup Stack 
#include <e32def.h>				// T Type  Definitions
#include <e32property.h>
#include "usbmsshared.h"		// Publish and subscribe property definitions

//#define USB_TRANSFER_PUBLISHER
#ifdef MSDC_MULTITHREADED
// Bytes transferred can be measured at the USB interface or the drive interface.
// Since read/write to the drive is performed by background threads we must publish
// the bytes transferred at the USB interface.
#ifndef USB_TRANSFER_PUBLISHER
#define USB_TRANSFER_PUBLISHER
#endif
#endif

// forward declaration
class CMassStorageDrive;

// typedefs
typedef TFixedArray<CMassStorageDrive*, KUsbMsMaxDrives>& TRefMsDriveList;
typedef const RArray<TInt>& TRefDriveMap;

typedef TFixedArray<TInt64, KUsbMsMaxDrives>& TRefBytesTransferedList;

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes the EUsbMsDriveState_MediaError property.
*/
class RDriveMediaErrorPublisher
{
public:
	RDriveMediaErrorPublisher();
	~RDriveMediaErrorPublisher();

	void PublishError(TBool aError);

private:
	/**
	Publish and subscribe property for EUsbMsDriveState_MediaError property
	*/
	RProperty iMediaErrorProperty;
};

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes the EUsbMsDriveState_DriveStatus property value for a drive state change.
The published drive state is mapped from the drive's mount state and drive state.
*/

class RDriveStateChangedPublisher
{
public:
	RDriveStateChangedPublisher(TRefMsDriveList aDrives, TRefDriveMap aDriveMap);
	~RDriveStateChangedPublisher();
	void DriveStateChanged();

private:
	/**
	Reference to the array of drives. The index into the array is a LUN.
	*/
	TRefMsDriveList iDrives;
	
	/**
	Reference to the drive map to convert LUN to Drive Number.
	*/
	TRefDriveMap iDriveMap;
};

#ifndef USB_TRANSFER_PUBLISHER
//----------------------------------------------------------------------------
// measure transfer of bytes at the drive interface
//----------------------------------------------------------------------------

/**
@internalTechnology

Base class for Read and Write publihsers.
*/
class CDriveTransferPublisher : public CBase
{
protected:
	~CDriveTransferPublisher();

	CDriveTransferPublisher(TUsbMsDriveState_Subkey iSubKey,
							TRefMsDriveList aDrives);
	void ConstructL();

public:
	void StartTimer();
	void StopTimer();
	void DoPublishDataTransferredEvent();

private:
	virtual TUint GetBytesTransferred(TUint aLun) const = 0;

	// No of calls to wait without an data transfer from iTimer
	// before stopping the publish timer.
	enum {ETimerCancelDelay = 5};

	static TInt PublishDataTransferredEvent(TAny* obj);
	TBool PublishDataTransferred();

protected:
	TUsbMsDriveState_Subkey iSubKey;
	/**
	Reference to the array of drives. The index into the array is a LUN.
	*/
	TRefMsDriveList iDrives;

	/**
	Publish and subscribe properties for tracking data transfer volume
	*/
	RProperty iProperty;

private:
	/**
	An active object which triggers periodic updates to subscribers.
	*/
	CPeriodic* iTimer;

	/**
	Set to ETrue when iTimer is running, EFalse otherwise
	*/
	TBool iTimerRunning;

	/** 
	Adds delay between data not being transferred and iTimer being cancelled
	*/
	TInt iTimerCancelCnt;
};

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes EUsbMsDriveState_KBytesWritten property values for tracking data transfer write volume.
*/
class CDriveWriteTransferPublisher: public CDriveTransferPublisher
{
public:
	static CDriveWriteTransferPublisher* NewL(TRefMsDriveList aDrives);

private:
	CDriveWriteTransferPublisher(TRefMsDriveList aDrives);

	TUint GetBytesTransferred(TUint aLun) const;
};


//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes EUsbMsDriveState_KBytesRead property value for tracking data transfer read volume.
*/
class CDriveReadTransferPublisher: public CDriveTransferPublisher
{
public:
	static CDriveReadTransferPublisher* NewL(TRefMsDriveList aDrives);

private:
	CDriveReadTransferPublisher(TRefMsDriveList aDrives);

	TUint GetBytesTransferred(TUint aLun) const;
};

#else
//----------------------------------------------------------------------------
// measure bytes transfered at the USB interface
//----------------------------------------------------------------------------

/**
@internalTechnology

Base class for Read and Write publihsers.
*/
class CUsbTransferPublisher : public CBase
{
protected:
	~CUsbTransferPublisher();

	CUsbTransferPublisher(TUsbMsDriveState_Subkey iSubKey,
						  TRefBytesTransferedList aBytesTransferred);
	void ConstructL();

public:
	void StartTimer();
	void StopTimer();
	void DoPublishDataTransferredEvent();

private:
	TUint GetBytesTransferred(TUint aLun) const;

	// No of calls to wait without an data transfer from iTimer
	// before stopping the publish timer.
	enum {ETimerCancelDelay = 5};

	static TInt PublishDataTransferredEvent(TAny* obj);
	TBool PublishDataTransferred();

protected:
	TUsbMsDriveState_Subkey iSubKey;
	/**
	Reference to the array of drives. The index into the array is a LUN.
	*/
	TRefBytesTransferedList iArray;

	/**
	Publish and subscribe properties for tracking data transfer volume
	*/
	RProperty iProperty;

private:
	/**
	An active object which triggers periodic updates to subscribers.
	*/
	CPeriodic* iTimer;

	/**
	Set to ETrue when iTimer is running, EFalse otherwise
	*/
	TBool iTimerRunning;

	/** 
	Adds delay between data not being transferred and iTimer being cancelled
	*/
	TInt iTimerCancelCnt;
};

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes EUsbMsDriveState_KBytesWritten property values for tracking data transfer write volume.
*/
class CUsbWriteTransferPublisher: public CUsbTransferPublisher
{
public:
	static CUsbWriteTransferPublisher* NewL(TRefBytesTransferedList aBytesTransferred);

private:
	CUsbWriteTransferPublisher(TRefBytesTransferedList aBytesTransferred);
};

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes EUsbMsDriveState_KBytesRead property value for tracking data transfer read volume.
*/
class CUsbReadTransferPublisher: public CUsbTransferPublisher
{
public:
	static CUsbReadTransferPublisher* NewL(TRefBytesTransferedList aBytesTransferred);

private:
	CUsbReadTransferPublisher(TRefBytesTransferedList aBytesTransferred);
};
#endif

#endif //__DRIVEPUBLISHER_H__
