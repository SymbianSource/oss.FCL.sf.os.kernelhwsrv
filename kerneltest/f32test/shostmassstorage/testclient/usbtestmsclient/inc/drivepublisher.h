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
// Drive publishing classes for USB Mass Storage.
// RDriveMediaErrorPublisher,
// RDriveStateChangedPublisher,
// CUsbTransferPublisher,
// CUsbReadTransferPublisher,
// CUsbReadTransferPublisher.
// 
//



/**
 @file
 @internalTechnology
*/

#ifndef DRIVEPUBLISHER_H
#define DRIVEPUBLISHER_H


// forward declaration
class CMassStorageDrive;

typedef TFixedArray<CMassStorageDrive*, KUsbMsMaxDrives>& TRefMsDriveList;

typedef TFixedArray<TInt64, KUsbMsMaxDrives> TBytesTransferedList;

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

	void PublishErrorL(TBool aError);

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
	RDriveStateChangedPublisher(const TMsDriveList& aDrives, const TLunToDriveMap& aDriveMap);
	~RDriveStateChangedPublisher();
	void DriveStateChanged();

private:
	/**
	Reference to the array of drives. The index into the array is a LUN.
	*/
	const TMsDriveList& iDrives;

	/**
	Reference to the drive map to convert LUN to Drive Number.
	*/
	const TLunToDriveMap& iDriveMap;
};


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
						  const TBytesTransferedList& aBytesTransferred);
	void ConstructL();

public:
	void StartTimer();
	void StopTimer();
	void DoPublishDataTransferredEvent();

private:
	TUint GetBytesTransferred(TLun aLun) const;

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
	const TBytesTransferedList& iArray;

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
	static CUsbWriteTransferPublisher* NewL(const TBytesTransferedList& aBytesTransferred);

private:
	CUsbWriteTransferPublisher(const TBytesTransferedList& aBytesTransferred);
};

//----------------------------------------------------------------------------
/**
@internalTechnology

Publishes EUsbMsDriveState_KBytesRead property value for tracking data transfer read volume.
*/
class CUsbReadTransferPublisher: public CUsbTransferPublisher
{
public:
	static CUsbReadTransferPublisher* NewL(const TBytesTransferedList& aBytesTransferred);

private:
	CUsbReadTransferPublisher(const TBytesTransferedList& aBytesTransferred);
};

#endif //__DRIVEPUBLISHER_H__
