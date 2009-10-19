// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file Example Pysical Device Driver
 @publishedPartner
 @released
*/

#include <kernel/kern_priv.h>
#include "driver1.h"
#include "driver1_dev.h"

// Name for PDD, must match LDD name with a '.' and distinguishing name appended
_LIT(KDriver1PddName,"DRIVER1.template");


class DDriver1Device : public DDriver1
	{
public:
	DDriver1Device(DDevice1PddFactory* aFactory);
	~DDriver1Device();
	TInt DoCreate();
	// Inherited from DDriver1. These called by the LDD.
	virtual TInt BufferSize() const;
	virtual TInt Speed() const;
	virtual TInt SetSpeed(TInt aSpeed);
	virtual TInt SendData(const TDesC8& aData);
	virtual void SendDataCancel();
	virtual TInt ReceiveData(TDes8& aBuffer);
	virtual void ReceiveDataCancel();
private:
	static void SendDataTimerCallback(TAny* aPtr);
	void SendDataCallback();
	static void ReceiveDataTimerCallback(TAny* aPtr);
	void ReceiveDataCallback();
private:
	DDevice1PddFactory* iFactory;
	TInt iSpeed;
	NTimer iSendDataTimer;
	NTimer iReceiveDataTimer;
	TBuf8<256> iBuffer;
	TDes8* iReceiveBuffer;
	};



//
// DDevice1PddFactory
//

const TInt KDriver1ThreadPriority = 27;
_LIT(KDriver1Thread,"Driver1Thread");

/**
  Standard export function for PDDs. This creates a DPhysicalDevice derived object,
  in this case, our DDevice1PddFactory
*/
DECLARE_STANDARD_PDD()
	{
	return new DDevice1PddFactory;
	}

DDevice1PddFactory::DDevice1PddFactory()
	{
	// Set version number for this device
	iVersion=RDriver1::VersionRequired();
	}

/**
  Second stage constructor for DPhysicalDevice derived objects.
  This must at least set a name for the driver object.

  @return KErrNone or standard error code.
*/
TInt DDevice1PddFactory::Install()
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KDriver1ThreadPriority, KDriver1Thread);
	if (r == KErrNone)
		{ 
		r = SetName(&KDriver1PddName);
		} 	
	return r;
	}

/**
  Returns the drivers capabilities. This is not used by the Symbian OS device driver framework
  but may be useful for the LDD to use.

  @param aDes Descriptor to write capabilities information into
*/
void DDevice1PddFactory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	DDriver1::TCaps caps;
	caps.iVersion = iVersion;
	// Zero the buffer
	TInt maxLen = aDes.MaxLength();
	aDes.FillZ(maxLen);
	// Copy cpabilities
	TInt size=sizeof(caps);
	if(size>maxLen)
		size=maxLen;
	aDes.Copy((TUint8*)&caps,size);
	}

/**
  Called by the kernel's device driver framework to create a Physical Channel.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aChannel Set to point to the created Physical Channel
  @param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aVer The version number of the Logical Channel which will use this Physical Channel 

  @return KErrNone or standard error code.
*/
TInt DDevice1PddFactory::Create(DBase*& aChannel, TInt aUnit, const TDesC8* aInfo, const TVersion& aVer)
	{
	// Ignore the parameters we aren't interested in...
	(void)aUnit;
	(void)aInfo;
	(void)aVer;

	// Create a new physical channel
	DDriver1Device* device=new DDriver1Device(this);
	aChannel=device;
	if (!device)
		return KErrNoMemory;
	return device->DoCreate();
	}

/**
  Called by the kernel's device driver framework to check if this PDD is suitable for use with a Logical Channel.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aVer The version number of the Logical Channel which will use this Physical Channel 

  @return KErrNone or standard error code.
*/
TInt DDevice1PddFactory::Validate(TInt aUnit, const TDesC8* aInfo, const TVersion& aVer)
	{
	// Check version numbers
	if ((!Kern::QueryVersionSupported(iVersion,aVer)) || (!Kern::QueryVersionSupported(aVer,TVersion(EMinimumLddMajorVersion,EMinimumLddMinorVersion,EMinimumLddBuild))))
		return KErrNotSupported;

	// We don't support units
    if (aUnit != -1)
        return KErrNotSupported;

	// Ignore extra info, (this could be used for validation purposes)
	// Note, aInof is a pointer to a descriptor in user memory, therefore safe methods should
	// be used for reading its contents. E.g. using Kern::KUDesGet()
	(void)aInfo;

	// OK
	return KErrNone;
	}

/**
  Destructor
*/
DDevice1PddFactory::~DDevice1PddFactory()
	{
	if (iDfcQ)
		iDfcQ->Destroy();
	}

//
// DDriver1Device
//

DDriver1Device::DDriver1Device(DDevice1PddFactory* aFactory)
	:	iFactory(aFactory),
		iSpeed(100000),  // 100000us (100ms) per byte
		iSendDataTimer(SendDataTimerCallback,this),
		iReceiveDataTimer(ReceiveDataTimerCallback,this)
	{
	}

DDriver1Device::~DDriver1Device()
	{
	// Driver no longer using hardware resources
	__e32_atomic_add_ord32(&iFactory->iHardwareInUse, TUint32(-1));
	}

TInt DDriver1Device::DoCreate()
	{
	// Claim the hardware resources by incrementing iHardwareInUse.
	// Must do this before any other failure can happen in this function so that
	// the destructor can safely decrement iHardwareInUse.
	//
	// This method of ensuring hardware is only in use by one driver at a time
	// wouldn't be needed if the driver claimed real hardware resources which
	// could only be used once. E.g. binding to an interrupt.
	if (__e32_atomic_add_ord32(&iFactory->iHardwareInUse, 1))
		return KErrInUse;

	// Other setup goes here

	return KErrNone;
	}

TInt DDriver1Device::BufferSize() const
	{
	return iBuffer.MaxSize();
	}

TInt DDriver1Device::Speed() const
	{
	return iSpeed;
	}

TInt DDriver1Device::SetSpeed(TInt aSpeed)
	{
	if(aSpeed<=0)
		return KErrArgument;
	iSpeed = aSpeed;
	return KErrNone;
	}

TInt DDriver1Device::SendData(const TDesC8& aData)
	{
	// Save the last part of the data to 'send', we will pretend to 'receive' this later
	iBuffer=aData.Right(iBuffer.MaxSize());
	// Pretend to send the data by waiting for iSpeed micro-seconds per byte...
	iSendDataTimer.OneShot(aData.Size()*iSpeed/NKern::TickPeriod());

	return KErrNone;
	}

void DDriver1Device::SendDataCancel()
	{
	// Stop the timer we were using to pretend we were processing the send
	iSendDataTimer.Cancel();
	}

void DDriver1Device::SendDataTimerCallback(TAny* aPtr)
	{
	// Just forward callback to non-static callback function
	((DDriver1Device*)aPtr)->SendDataCallback();
	}

void DDriver1Device::SendDataCallback()
	{
	// Tell LDD we've done
	iLdd->SendDataComplete(KErrNone);
	}

TInt DDriver1Device::ReceiveData(TDes8& aBuffer)
	{
	// Save a pointer to the buffer we need to put the 'recevied' data in
	iReceiveBuffer=&aBuffer;
	// Pretend to receive the data by waiting for iSpeed micro-seconds per byte...
	iReceiveDataTimer.OneShot(iBuffer.Size()*iSpeed/NKern::TickPeriod());

	return KErrNone;
	}

void DDriver1Device::ReceiveDataCancel()
	{
	// Stop the timer we were using to pretend we were processing the receive
	iReceiveDataTimer.Cancel();
	}

void DDriver1Device::ReceiveDataTimerCallback(TAny* aPtr)
	{
	// Just forward callback to non-static callback function
	((DDriver1Device*)aPtr)->ReceiveDataCallback();
	}

void DDriver1Device::ReceiveDataCallback()
	{
	// Pretend the data we have received is that saved in iBuffer when we last did a send
	*iReceiveBuffer=iBuffer;
	// Tell LDD we've done
	iLdd->ReceiveDataComplete(KErrNone);
	}
