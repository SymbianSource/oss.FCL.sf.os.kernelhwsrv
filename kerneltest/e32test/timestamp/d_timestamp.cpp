// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// d_timestamp.cpp
//

#include <kern_priv.h>
#include <kernel.h>
#include "d_timestamp.h"
#include "d_timestamp_dev.h"

// time stamp test defaults
static const TInt KTimerDurationS = 5;  // time interval for NTimer
static const TInt KNErrPercent = 1;  // percent error acceptable
static const TInt KIterations = 5;  // required number of valid runs (with LPM entry) 
static const TInt KRetries = 4;  // retries are reset on every succesful run

//
// DTimestampTestFactory
//

/**
   Standard export function for LDDs. This creates a DLogicalDevice derived object,
   in this case, our DTimestampTestFactory
*/
DECLARE_STANDARD_LDD()
	{
	return new DTimestampTestFactory;
	}

/**
   Constructor
*/
DTimestampTestFactory::DTimestampTestFactory()
	{
	// Set version number for this device
	iVersion=RTimestampTest::VersionRequired();
    // Indicate that we work with a PDD
	iParseMask=KDeviceAllowPhysicalDevice;
    }

/**
   Second stage constructor for DTimestampTestFactory.
   This must at least set a name for the driver object.

   @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTimestampTestFactory::Install()
	{
	return SetName(&RTimestampTest::Name());
	}

/**
   Destructor
*/
DTimestampTestFactory::~DTimestampTestFactory()
	{
    
    }

/**
   Return the drivers capabilities.
   Called in the response to an RDevice::GetCaps() request.

   @param aDes User-side descriptor to write capabilities information into
*/
void DTimestampTestFactory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	RTimestampTest::TCaps caps;
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
TInt DTimestampTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
	aChannel=new DTimestampTestChannel;
	if(!aChannel)
        {
		return KErrNoMemory;
        }
    
	return KErrNone;
	}


//
// Logical Channel
//

/**
   Constructor
*/
DTimestampTestChannel::DTimestampTestChannel()
    :iTimer(timerExpire,this),iDfc(dfcFn,this,7),iStarted(EFalse)
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
TInt DTimestampTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check version
	if (!Kern::QueryVersionSupported(RTimestampTest::VersionRequired(),aVer))
		return KErrNotSupported;

	// Setup LDD for receiving client messages
    TInt r = Kern::CreateClientRequest(iStartRequest);
    if (r != KErrNone) return r;
    r = Kern::CreateClientDataRequest(iWaitOnTimerRequest);
    if (r != KErrNone) return r;
    r = Kern::DynamicDfcQCreate(iQue,Kern::DfcQue0()->iThread->iPriority,RTimestampTest::Name());
    if (KErrNone!=r) return r;
    iDfc.SetDfcQ(iQue);
    SetDfcQ(iQue);
    iMsgQ.Receive();
    // Done
	return KErrNone;
	}


/**
   Destructor
*/
DTimestampTestChannel::~DTimestampTestChannel()
	{
	// Cancel all processing that we may be doing
	DoCancel(TUint(RTimestampTest::EAllRequests));
    Kern::DestroyClientRequest(iWaitOnTimerRequest);
    Kern::DestroyClientRequest(iStartRequest);
    iQue->Destroy();
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	}

/**
   Called when a user thread requests a handle to this channel.
*/
TInt DTimestampTestChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	// Make sure that only our client can get a handle
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

/**
   override SendMsg method to allow pinning data in the context of the client thread
*/
TInt DTimestampTestChannel::SendMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id = m.iValue;

	// we only support one client
	if (id != (TInt)ECloseMsg && m.Client() != iClient)
		return KErrAccessDenied;
	
	TInt r = KErrNone;
	if (id != (TInt)ECloseMsg && id != KMaxTInt)
		{
		if (id<0)
			{
			TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
			r = SendRequest(aMsg);
			if (r != KErrNone)
				Kern::RequestComplete(pS,r);
			}
		else
			r = SendControl(aMsg);
		}
	else
		r = DLogicalChannel::SendMsg(aMsg);
	
	return r;
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
void DTimestampTestChannel::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;

	// Get message type
	TInt id=m.iValue;

	// Decode the message type and dispatch it to the relevent handler function...

	if (id==(TInt)ECloseMsg)
		{
		// Channel Close
        DoCancel(TUint(RTimestampTest::EAllRequests));
        iMsgQ.CompleteAll(KErrServerTerminated);
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
		DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
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
   Preprocess synchronous 'control' requests
*/
TInt DTimestampTestChannel::SendControl(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;

	switch (id)
		{
        
    case RTimestampTest::EConfig:
        {
        STimestampTestConfig info;
#ifdef __SMP__
        info.iFreq = NKern::TimestampFrequency();
#else
        info.iFreq = NKern::FastCounterFrequency();
#endif
        info.iIterations = KIterations;
        info.iRetries = KRetries;
        info.iTimerDurationS = KTimerDurationS;
        info.iErrorPercent = KNErrPercent;
        // Allow PDD to override defaults
        Pdd().TestConfig(info);
        kumemput(m.Ptr0(),&info,sizeof(STimestampTestConfig));
        return KErrNone;
		}
    
        }
    

	TInt r = DLogicalChannel::SendMsg(aMsg);
	if (r != KErrNone)
		return r;

// 	switch (id)
// 		{
// 		}

	return r;
	}

/**
   Process synchronous 'control' requests
*/
TInt DTimestampTestChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	(void)a2;   
	(void)a1;   
	(void) aFunction;

	// TInt r = KErrNone;
	// switch (aFunction)
	// 	{
    // default:
    //     r = KErrNotSupported;
	// 	}

	return KErrNotSupported;
	}


/**
   Preprocess asynchronous requests.
*/
TInt DTimestampTestChannel::SendRequest(TMessageBase* aMsg)
    {
	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt function = ~m.iValue;
    TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		
	TInt r = KErrNotSupported;

	switch (function)
		{
		case RTimestampTest::EStart:
            if (!iStarted) 
                {
                r = iStartRequest->SetStatus(pS);
                }
            else 
                {
                r = KErrInUse;
                }
            break;
            
		case RTimestampTest::EWaitOnTimer:
            if (iStarted)
                {
                iWaitOnTimerRequest->SetDestPtr(m.Ptr1());
                r = iWaitOnTimerRequest->SetStatus(pS);
                }
            else
                {
                r = KErrNotReady;
                }
            
            break;
        default:
            r = KErrNotSupported;
		}

	if (r == KErrNone)
		r = DLogicalChannel::SendMsg(aMsg);
	return r;
    }


/**
   Process asynchronous requests.
*/
void DTimestampTestChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	(void)a2;   
	(void)a1;   
    (void)aStatus;
    
	TInt r = KErrNone;

	switch(aReqNo)
		{
    case RTimestampTest::EStart:
        iNTicks = (TInt) a1;
        r = iTimer.OneShot(0);
        if (KErrNone!=r) Kern::QueueRequestComplete(iClient,iStartRequest,r);
        break;
    case RTimestampTest::EWaitOnTimer:
        Pdd().StartLPMEntryCheck();   // PDD will start checking if we have entered LPM
        r = iTimer.Again(iNTicks);
        if (KErrNone!=r) Kern::QueueRequestComplete(iClient,iWaitOnTimerRequest,r);
        break;
		}
    
	}



/**
   Process cancelling of asynchronous requests.
*/
void DTimestampTestChannel::DoCancel(TUint aMask)
	{
    (void)aMask;
    iTimer.Cancel(); // no real guarantees on SMP systems
    iDfc.Cancel();
	}


/**
 * process timer expiry
*/
void DTimestampTestChannel::DoTimerExpire()
	{
#ifdef __SMP__
    TUint64 ts = NKern::Timestamp();
#else
    TUint64 ts = NKern::FastCounter();
#endif
    iTimestampDelta = ts-iLastTimestamp;
    iLastTimestamp = ts;
    iDfc.Add();
	}

void DTimestampTestChannel::timerExpire(TAny* aParam)
    {
    DTimestampTestChannel* pD = (DTimestampTestChannel*) aParam;
    pD->DoTimerExpire();
    }



void DTimestampTestChannel::DoDfcFn()
	{
    if (!iStarted)
        {
        iStarted = ETrue;
        Kern::QueueRequestComplete(iClient,iStartRequest,KErrNone);
        }
    else
        {
        iWaitOnTimerRequest->Data().iDelta = iTimestampDelta;
        // PDD will return ETrue here if we have entered LPM
        iWaitOnTimerRequest->Data().iLPMEntered = Pdd().EndLPMEntryCheck(); 
        Kern::QueueRequestComplete(iClient,iWaitOnTimerRequest,KErrNone);
        }
	}

void DTimestampTestChannel::dfcFn(TAny* aParam)
    {
    DTimestampTestChannel* pD = (DTimestampTestChannel*) aParam;
    pD->DoDfcFn();
    }

