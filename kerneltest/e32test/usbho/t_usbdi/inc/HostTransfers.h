#ifndef __HOST_TRANSFERS_H
#define __HOST_TRANSFERS_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file HostTransfers.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <e32ver.h>
#include <d32usbdescriptors.h>
#include <d32usbtransfers.h>
#include <d32usbdi.h>
#include "testdebug.h"
#include "ControlTransferRequests.h"

namespace NUnitTesting_USBDI
	{

/**
This class describes an interface to a class which wants to observe transfers
*/
class MTransferObserver
	{
public: 
	/**
	Called when a transfer with the supplied transfer identity has completed
	@param aTransferId the identity of the transfer
	@param aCompletionCode the error completion code for the asynchronous transfer
	*/
	virtual void TransferCompleteL(TInt aTransferId,TInt aCompletionCode) = 0;
	};


/**
This class describes a base class for a transfer
*/
class CBaseTransfer : public CActive
	{
public:
	/**
	Destructor
	*/
	virtual ~CBaseTransfer()
		{
		}
	
	/**
	Retrieve the identity of the transfer
	@return the transfer identity
	*/
	TInt Identity() const
		{
		return iTransferIdentity;
		}
	
protected:
	/**
	Constructor 
	*/
	CBaseTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,MTransferObserver& aObserver,TInt aTransferIdentity)
	:	CActive(EPriorityStandard),
		iPipe(aPipe),
		iInterface(aInterface),
		iObserver(aObserver),
		iTransferIdentity(aTransferIdentity)
		{
		CActiveScheduler::Add(this);
		}
		
	/**
	*/
	void SelfComplete()
		{
		iStatus = KRequestPending;
		TRequestStatus* s = &iStatus;
		User::RequestComplete(s,KErrNone);
		SetActive();
		}
	
protected:
	/**
	*/
	virtual TInt RunError(TInt aError)
		{
		RDebug::Printf("<Error %d> a transfer RunL left",aError);
		return KErrNone;
		}
	
	/**
	*/
	void RunL()
		{
		TInt completionCode(iStatus.Int());
		RDebug::Printf("Transfer err=%d",completionCode);
		
		// Notify of transfer completion (successful or otherwise)
		iObserver.TransferCompleteL(iTransferIdentity,completionCode);
		}
	
	/**
	*/
	void DoCancel()
		{
		// Will cancel all transfers on this pipe
		
		Pipe().CancelAllTransfers();
		}
	
protected:
	
	/**
	Get the pipe object for the transfer
	@return a opened pipe for transfers
	*/
	RUsbPipe& Pipe()
		{
		return iPipe;
		}
	
	/**
	Get the interface for the transfer
	@return the opened interface 
	*/
	RUsbInterface& Interface()
		{
		return iInterface;
		}

	/**
	Access the observer of the transfers
	@return the transfer observer
	*/
	MTransferObserver& Observer()
		{
		return iObserver;
		}
	
private:
	/**
	The usb pipe that will be used for the transfer and where
	the buffer pool is located.
	*/
	RUsbPipe& iPipe;

	/**
	The interface that will be used for the transfer
	*/
	RUsbInterface& iInterface;
	
	/**
	The observer for the transfers
	*/
	MTransferObserver& iObserver;

	/**
	The identity of a transfer (not the type)
	*/
	TInt iTransferIdentity;
	};
	
/**
This class represents a interrupt transfer to the device
*/
class CInterruptTransfer : public CBaseTransfer
	{
public:
	/**
	C++ constructor, builds a interrupt transfer object with a transfer buffer of maximum fixed size.
	The caller must ensure that this instance can handle the transfer specified
	@param aPipe the pipe to be used for the transfer and buffer pool
	@param aTransferSize the required maximum size of the buffer to handle all transfers
	@param aObserver the observer of the transfer
	@param aTransferId a unique identity of the transfer
	*/
	CInterruptTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TInt aMaxTransferSize,MTransferObserver& aObserver,TInt aTransferId);

	/**
	Destructor
	*/
	virtual ~CInterruptTransfer();
	
	/**
	Poll for interrupt data queued by the device and transfer to host 
	@param aSize the size of the data from requested from the client
	@return KErrNone if successful or system-wide error code
	*/
	TInt TransferInL(TInt aSize);
	
	/**
	Register the transfer descriptor
	@return KErrNone if successful or system-wide error code
	*/
	TInt RegisterTransferDescriptor();
	
	/**
	On successful completion of an interrupt 'in' transfer, obtain the polled data
	@return the interrupt data from the device
	*/
	TPtrC8 DataPolled();

private:
	/**
	The transfer descriptor for interrupt transfers
	*/
	RUsbIntrTransferDescriptor iTransferDescriptor;
	};

/**
This class represents a isochronous transfer to a device
*/
class CIsochTransfer : public CBaseTransfer
	{
public:
	/**
	C++ constructor, builds a isochronous transfer object with a transfer buffer of maximum fixed size.
	The caller must ensure that this instance can handle the transfer specified
	@param aPipe the pipe to be used for the transfer and buffer pool
	@param aMaxPacketSize the maximum packet size to send
	@param aMaxNumPackets the maximum number of packets to be transfered on this pipe
	@param aObserver the observer of the transfer
	@param aTransferId a unique identity of the transfer
	*/
	CIsochTransfer(RUsbPipe& aPipe,RUsbInterface& aInterface,TUint16 aMaxPacketSize,
					TInt aMaxNumPackets,MTransferObserver& aObserver,TInt aTransferId);

	/**
	Destructor
	*/
	virtual ~CIsochTransfer();
	
	TInt RegisterTransferDescriptor();

	/**
	Transfer the data to the device
	@param aIsochData the 8bit Isochronous data to transfer
	@return KErrNone if successful, or system wide error code
	*/	
	TInt TransferOut();
	
	/**
	Prepare the transfer before its sending
	@param aIsochData the 8bit Isochronous data to transfer
	@return KErrNone if successful, or system wide error code
	*/	
	TInt PrepareTransfer(const TDesC8& aIsochData);
	
	/**
	Start the IN transfer
	@param aPacketsExpected nb of expected packets
	@return KErrNone if successful, or system wide error code
	*/
	TInt TransferInL(TInt aPacketsExpected);
		
	/**
	Store the polled data into internal buffer
	@param[int] aPacketsToBeRead nb of packets to be read
	@param[out] aDataPolled data being transfered
	@return ETrue if successful, EFalse otherwise
	*/
	TBool DataPolled(TUint aPacketsToBeRead, RBuf8& aDataPolled);
				
private:
	/**
	The transfer descriptor for isochronous transfers
	*/
	RUsbIsocTransferDescriptor iTransferDescriptor;
	
	/**
	The maximum packet size for the respective isochronous endpoint
	*/
	TUint16 iMaxPacketSize;	
	
	};


/**
This class represents a bulk transfer to a device
*/
class CBulkTransfer : public CBaseTransfer
	{
public:
	/**
	C++ constructor
	*/
	CBulkTransfer(RUsbPipe& aPipe,RUsbInterface& aUsbInterface,TInt aMaxTransferSize,
		MTransferObserver& aObserver,TInt aTransferId);

	/**
	*/
	virtual ~CBulkTransfer();
	
	/**
	*/
	void TransferOut(const TDesC8& aBulkData, TBool aUseZLPIfRequired = ETrue);
		
	/**
	*/
	void TransferOut(const TDesC8& aBulkDataPattern, TUint aNumBytes, TBool aUseZLPIfRequired = ETrue);
				
	/**
	*/
	void TransferOut(const TDesC8& aBulkDataPattern, TUint aStartPoint, TUint aNumBytes, TBool aUseZLPIfRequired);
	
	/**
	*/
	void TransferIn(TInt aExpectedDataSize);
	
	/**
	*/
	RUsbPipe&  Pipe(){return CBaseTransfer::Pipe();};
	
	RUsbBulkTransferDescriptor& TransferDescriptor(){return iTransferDescriptor;};

	/**
	*/
	TPtrC8 DataPolled();
	
private:
	/**
	The transfer descriptor for bulk transfers
	*/
	RUsbBulkTransferDescriptor iTransferDescriptor;
	};

	
	}

#endif


