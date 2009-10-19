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
 @file Example Logical Device Driver
 @publishedPartner
 @released
*/

#include <kernel/kern_priv.h>
#include "driver1.h"
#include "driver1_dev.h"

_LIT(KDriver1PanicCategory,"Driver1");


//
// DDriver1Factory
//

/**
  Standard export function for LDDs. This creates a DLogicalDevice derived object,
  in this case, our DDriver1Factory
*/
DECLARE_STANDARD_LDD()
	{
	return new DDriver1Factory;
	}

/**
  Constructor
*/
DDriver1Factory::DDriver1Factory()
	{
	// Set version number for this device
	iVersion=RDriver1::VersionRequired();
	// Indicate that we work with a PDD
	iParseMask=KDeviceAllowPhysicalDevice;
	}


/**
  Second stage constructor for DDriver1Factory.
  This must at least set a name for the driver object.

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DDriver1Factory::Install()
	{
	return SetName(&RDriver1::Name());
	}


/**
     Destructor
   */
DDriver1Factory::~DDriver1Factory()
   	{
   	}


/**
  Return the drivers capabilities.
  Called in the response to an RDevice::GetCaps() request.

  @param aDes User-side descriptor to write capabilities information into
*/
void DDriver1Factory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	RDriver1::TCaps caps;
	caps.iVersion = iVersion;
	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

/**
  Called by the kernel's device driver framework to create a Logical Channel.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aChannel Set to point to the created Logical Channel

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DDriver1Factory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DDriver1Channel;
	if(!aChannel)
		return KErrNoMemory;

	return KErrNone;
	}

//
// Logical Channel
//

/**
  Constructor
*/
DDriver1Channel::DDriver1Channel()
	:	iSendDataDfc(SendDataDfc, this, 1),        // DFC is priority '1'
		iReceiveDataDfc(ReceiveDataDfc, this, 1)   // DFC is priority '1'
	{
	// Get pointer to client threads DThread object
	iClient=&Kern::CurrentThread();

	// Open a reference on client thread so it's control block can't dissapear until
	// this driver has finished with it.
	// Note, this call to Open can't fail since its the thread we are currently running in
	iClient->Open();
	}

/**
  Second stage constructor called by the kernel's device driver framework.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aUnit The unit argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aInfo The info argument supplied by the client to RBusLogicalChannel::DoCreate
  @param aVer The version argument supplied by the client to RBusLogicalChannel::DoCreate

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DDriver1Channel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check Platform Security capabilities of client thread (if required).
	//
	// Here we handle the simple case where:
	// 1. The device driver can only have one client thread
	// 2. The security policy is the binary all-or-nothing policy.
	//    E.g. "If you have the right capability you can do anything with the driver
	//    and if you don't have the capability you can't do anything"
	// 
	// If only some functionality of the driver is restricted, then the security check should
	// go elsewhere. E.g. in DoRequest/DoControl. In that case Kern::CurrentThreadHasCapability
	// shouldn't be used because the 'current thread' isn't the client.
	//
	// In this example we do a check here for ECapability_None (which always passes)...
	if(!Kern::CurrentThreadHasCapability(ECapability_None,__PLATSEC_DIAGNOSTIC_STRING("Checked by DRIVER1")))
		return KErrPermissionDenied;

	// Check version
	if (!Kern::QueryVersionSupported(RDriver1::VersionRequired(),aVer))
		return KErrNotSupported;

	// Setup LDD for receiving client messages
	SetDfcQ(((DDevice1PddFactory*)iPhysicalDevice)->iDfcQ);
	iMsgQ.Receive();

	// Associate DFCs with the same queue we set above to receive client messages on
	iSendDataDfc.SetDfcQ(iDfcQ);
	iReceiveDataDfc.SetDfcQ(iDfcQ);

	// Give PDD a pointer to this channel
	Pdd()->iLdd=this;

	// Done
	return KErrNone;
	}

/**
  Destructor
*/
DDriver1Channel::~DDriver1Channel()
	{
	// Cancel all processing that we may be doing
	DoCancel(RDriver1::EAllRequests);
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/**
  Called when a user thread requests a handle to this channel.
*/
TInt DDriver1Channel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	// Make sure that only our client can get a handle
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

/**
  Process a message for this logical channel.
  This function is called in the context of a DFC thread.

  @param aMessage The message to process.
	              The iValue member of this distinguishes the message type:
	              iValue==ECloseMsg, channel close message
	              iValue==KMaxTInt, a 'DoCancel' message
	              iValue>=0, a 'DoControl' message with function number equal to iValue
	              iValue<0, a 'DoRequest' message with function number equal to ~iValue
*/
void DDriver1Channel::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;

	// Get message type
	TInt id=m.iValue;

	// Decode the message type and dispatch it to the relevent handler function...

	if (id==(TInt)ECloseMsg)
		{
		// Channel Close
		DoCancel(RDriver1::EAllRequests);
		m.Complete(KErrNone, EFalse);
		return;
		}

	if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}

	if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		if (r!=KErrNone)
			Kern::RequestComplete(iClient,pS,r);
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

/**
  Process synchronous 'control' requests
*/
TInt DDriver1Channel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	(void)a2;   // a2 not used in this example

	TInt r;

	switch (aFunction)
		{
		case RDriver1::EGetConfig:
			r = GetConfig((TDes8*)a1);
			break;

		case RDriver1::ESetConfig:
			r = SetConfig((const TDesC8*)a1);
			break;

		default:
			r = KErrNotSupported;
			break;
		}

	return r;
	}

/**
  Process asynchronous requests.
*/
TInt DDriver1Channel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	(void)a2;   // a2 not used in this example

	TInt r;

	switch(aReqNo)
		{
		case RDriver1::ESendData:
			r=SendData(aStatus,(const TDesC8*)a1);
			break;

		case RDriver1::EReceiveData:
			// Example Platform Security capability check which tests the
			// client for ECapability_None (which always passes)...
			if(iClient->HasCapability(ECapability_None,__PLATSEC_DIAGNOSTIC_STRING("Checked by DRIVER1")))
				r=ReceiveData(aStatus,(TDes8*)a1);
			else
				r=KErrPermissionDenied;
			break;

		default:
			r = KErrNotSupported;
			break;
		}

	return r;
	}

/**
  Process cancelling of asynchronous requests.
*/
void DDriver1Channel::DoCancel(TUint aMask)
	{
	if(aMask&(1<<RDriver1::ESendData))
		SendDataCancel();
	if(aMask&(1<<RDriver1::EReceiveData))
		ReceiveDataCancel();
	}

//
// Methods for processing configuration control messages
//

/**
  Process a GetConfig control message. This writes the current driver configuration to a
  RDriver1::TConfigBuf supplied by the client.
*/
TInt DDriver1Channel::GetConfig(TDes8* aConfigBuf)
	{
	// Create a structure giving the current configuration
	RDriver1::TConfig config;
	CurrentConfig(config);

	// Write the config to the client
	TPtrC8 ptr((const TUint8*)&config,sizeof(config));
	return Kern::ThreadDesWrite(iClient,aConfigBuf,ptr,0,KTruncateToMaxLength,NULL);
	}

/**
  Process a SetConfig control message. This sets the driver configuration using a
  RDriver1::TConfigBuf supplied by the client.
*/
TInt DDriver1Channel::SetConfig(const TDesC8* aConfigBuf)
	{
	// Don't allow configuration changes whilst we're busy
	if(iSendDataStatus || iReceiveDataStatus)
		return KErrInUse;

	// Create a config structure.
	RDriver1::TConfig config;
	CurrentConfig(config);

	// Note: We have filled config with the current settings, this is to allow
	// backwards compatibility when a client gives us an old (and shorter) version
	// of the config structure.

	// Read the config structure from client
	TPtr8 ptr((TUint8*)&config,sizeof(config));
	TInt r=Kern::ThreadDesRead(iClient,aConfigBuf,ptr,0);
	if(r!=KErrNone)
		return r;

	// Use config data to setup the driver. Checking that parameters which aren't settable
	// either contain the correct values or are zero (meaning 'default')
	if(config.iPddBufferSize && config.iPddBufferSize!=Pdd()->BufferSize())
		return KErrArgument;

	if(config.iMaxSendDataSize && config.iMaxSendDataSize!=iSendDataBuffer.MaxSize())
		return KErrArgument;

	if(config.iMaxReceiveDataSize && config.iMaxReceiveDataSize!=iReceiveDataBuffer.MaxSize())
		return KErrArgument;

	r=Pdd()->SetSpeed(config.iSpeed);
	if(r!=KErrNone)
		return r;

	return r;
	}

/**
  Fill a TConfig with the drivers current configuration.
*/
void DDriver1Channel::CurrentConfig(RDriver1::TConfig& aConfig)
	{
	aConfig.iSpeed = Pdd()->Speed();
	aConfig.iPddBufferSize = Pdd()->BufferSize();
	aConfig.iMaxSendDataSize = iSendDataBuffer.MaxSize();
	aConfig.iMaxReceiveDataSize = iReceiveDataBuffer.MaxSize();
	}

//
// Methods for processing 'SendData'
//

/**
  Start processing a SendData request.
*/
TInt DDriver1Channel::SendData(TRequestStatus* aStatus,const TDesC8* aData)
	{
	// Check that a 'SendData' isn't already in progress
	if(iSendDataStatus)
		{
		Kern::ThreadKill(iClient,EExitPanic,ERequestAlreadyPending,KDriver1PanicCategory);
		return KErrInUse;
		}

	// Read data from client into our buffer
	TInt r=Kern::ThreadDesRead(iClient,aData,iSendDataBuffer,0);
	if(r!=KErrNone)
		return r;

	// Give data to PDD so that it can do the work
	r=Pdd()->SendData(iSendDataBuffer);
	if(r!=KErrNone)
		return r;

	// Save the client request status and return
	iSendDataStatus = aStatus;
	return KErrNone;
	}

/**
  Cancel a SendData request.
*/
void DDriver1Channel::SendDataCancel()
	{
	if(iSendDataStatus)
		{
		// Tell PDD to stop processing the request
		Pdd()->SendDataCancel();
		// Cancel DFC
		iSendDataDfc.Cancel();
		// Complete clients request
		Kern::RequestComplete(iClient,iSendDataStatus,KErrCancel);
		}
	}

/**
  Called by PDD from ISR to indicate that a SendData operation has completed.
*/
void DDriver1Channel::SendDataComplete(TInt aResult)
	{
	// Save result code
	iSendDataResult = aResult;
	// Queue DFC
	iSendDataDfc.Add();
	}

/**
  DFC callback which gets triggered after the PDD has signalled that SendData completed.
  This just casts aPtr and calls DoSendDataComplete().
*/
void DDriver1Channel::SendDataDfc(TAny* aPtr)
	{
	((DDriver1Channel*)aPtr)->DoSendDataComplete();
	}

/**
  Called from a DFC after the PDD has signalled that SendData completed.
*/
void DDriver1Channel::DoSendDataComplete()
	{
	TInt result = iSendDataResult;
	// Complete clients request
	Kern::RequestComplete(iClient,iSendDataStatus,result);
	}

//
// Methods for processing 'ReceiveData'
//

/**
  Start processing a ReceiveData request.
*/
TInt DDriver1Channel::ReceiveData(TRequestStatus* aStatus,TDes8* aPtr)
	{
	// Check that a 'ReceiveData' isn't already in progress
	if(iReceiveDataStatus)
		{
		Kern::ThreadKill(iClient,EExitPanic,ERequestAlreadyPending,KDriver1PanicCategory);
		return KErrInUse;
		}

	// Ask PDD for data
	TInt r=Pdd()->ReceiveData(iReceiveDataBuffer);
	if(r!=KErrNone)
		return r;

	// Save the client request status and descriptor before returning
	iReceiveDataStatus = aStatus;
	iReceiveDataDescriptor = aPtr;
	return KErrNone;
	}

/**
  Cancel a ReceiveData request.
*/
void DDriver1Channel::ReceiveDataCancel()
	{
	if(iReceiveDataStatus)
		{
		// Tell PDD to stop processing the request
		Pdd()->ReceiveDataCancel();
		// Cancel DFC
		iReceiveDataDfc.Cancel();
		// Finished with client descriptor, so NULL it to help detect coding errors
		iReceiveDataDescriptor = NULL;
		// Complete clients request
		Kern::RequestComplete(iClient,iReceiveDataStatus,KErrCancel);
		}
	}

/**
  Called by PDD from ISR to indicate that a ReceiveData operation has completed.
*/
void DDriver1Channel::ReceiveDataComplete(TInt aResult)
	{
	// Save result code
	iReceiveDataResult = aResult;
	// Queue DFC
	iReceiveDataDfc.Add();
	}

/**
  DFC Callback which gets triggered after the PDD has signalled that ReceiveData completed.
  This just casts aPtr and calls DoReceiveDataComplete().
*/
void DDriver1Channel::ReceiveDataDfc(TAny* aPtr)
	{
	((DDriver1Channel*)aPtr)->DoReceiveDataComplete();
	}

/**
  Called from a DFC after the PDD has signalled that ReceiveData completed.
*/
void DDriver1Channel::DoReceiveDataComplete()
	{
	// Write data to client from our buffer
	TInt result=Kern::ThreadDesWrite(iClient,iReceiveDataDescriptor,iReceiveDataBuffer,0);

	// Finished with client descriptor, so NULL it to help detect coding errors
	iReceiveDataDescriptor = NULL;

	// Use result code from PDD if it was an error
	if(iReceiveDataResult!=KErrNone)
		result = iReceiveDataResult;

	// Complete clients request
	Kern::RequestComplete(iClient,iReceiveDataStatus,result);
	}

