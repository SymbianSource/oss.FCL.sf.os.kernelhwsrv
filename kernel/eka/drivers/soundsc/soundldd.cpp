// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\soundsc\soundldd.cpp
// LDD for the shared chunk sound driver.
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

#include <drivers/soundsc.h>
#include <kernel/kern_priv.h>
#include <kernel/cache.h>

//#define USE_PLAY_EOF_TIMER

// Define TEST_WITH_PAGING_CACHE_FLUSHES to flush the paging cache when testing read/writes to user thread in a data-paging system
//#define TEST_WITH_PAGING_CACHE_FLUSHES

static const char KSoundLddPanic[]="Sound LDD";

LOCAL_C TInt HighestCapabilitySupported(TUint32 aCapsBitField)
	{
	TInt n;
	for (n=31 ; n>=0 ; n--)
		{
		if (aCapsBitField&(1<<n))
			break;
		}
	return(n);
	}

/**
Standard export function for LDDs. This creates a DLogicalDevice derived object,
in this case, DSoundScLddFactory.
*/
DECLARE_STANDARD_LDD()
	{
	return new DSoundScLddFactory;
	}

/**
Constructor for the sound driver factory class.
*/
DSoundScLddFactory::DSoundScLddFactory()
	{
//	iUnitsOpenMask=0;

	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLddFactory::DSoundScLddFactory"));

	// Set version number for this device.
	iVersion=RSoundSc::VersionRequired();
	
	// Indicate that units / PDD are supported.
	iParseMask=KDeviceAllowUnit|KDeviceAllowPhysicalDevice;
	
	// Leave the units decision to the PDD
	iUnitsMask=0xffffffff;
	}
	
/**
Second stage constructor for the sound driver factory class.
This must at least set a name for the driver object.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DSoundScLddFactory::Install()
	{
	return(SetName(&KDevSoundScName));
	}

/**
Return the 'capabilities' of the sound driver in general.
Called in the response to an RDevice::GetCaps() request.
@param aDes A user-side descriptor to write the capabilities information into.
*/
void DSoundScLddFactory::GetCaps(TDes8& aDes) const
	{
	// Create a capabilities object
	TCapsSoundScV01 caps;
	caps.iVersion=iVersion;
	
	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

/**
Called by the kernel's device driver framework to create a logical channel.
This is called in the context of the client thread which requested the creation of a logical channel.
The thread is in a critical section.
@param aChannel Set by this function to point to the created logical channel.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DSoundScLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLddFactory::Create"));
	aChannel=new DSoundScLdd;
	if (!aChannel)
		return(KErrNoMemory);

	return(KErrNone);
	}
	
/**
Check whether a channel has is currently open on the specified unit.
@param aUnit The number of the unit to be checked.
@return ETrue if a channel is open on the specified channel, EFalse otherwise.
@pre The unit info. mutex must be held.
*/
TBool DSoundScLddFactory::IsUnitOpen(TInt aUnit)
	{
	return(iUnitsOpenMask&(1<<aUnit));
	}

/**
Attempt to change the state of the channel open status for a particular channel.
@param aUnit The number of the unit to be updated.
@param aIsOpenSetting The required new state for the channel open status: either ETrue to set the status to open or 
	EFalse to set the status to closed.
@return KErrNone if the status was updated successfully, KErrInUse if an attempt has been made to set the channnel status
	to open while it is already open.
*/		
TInt DSoundScLddFactory::SetUnitOpen(TInt aUnit,TBool aIsOpenSetting)
	{
	NKern::FMWait(&iUnitInfoMutex); // Acquire the unit info. mutex.
	
	// Fail a request to open an channel that is already open
	if (aIsOpenSetting && IsUnitOpen(aUnit))
		{
		NKern::FMSignal(&iUnitInfoMutex); // Release the unit info. mutex.
		return(KErrInUse);
		}
	
	// Update the open status as requested
	if (aIsOpenSetting)
		iUnitsOpenMask|=(1<<aUnit);
	else
		iUnitsOpenMask&=~(1<<aUnit);
	
	NKern::FMSignal(&iUnitInfoMutex); // Release the unit info. mutex.	
	return(KErrNone);
	}	

/**
Constructor for the sound driver logical channel.
*/
DSoundScLdd::DSoundScLdd()
	: iPowerDownDfc(DSoundScLdd::PowerDownDfc,this,3),
	  iPowerUpDfc(DSoundScLdd::PowerUpDfc,this,3),
	  iEofTimer(DSoundScLdd::PlayEofTimerExpired,this),
	  iPlayEofDfc(DSoundScLdd::PlayEofTimerDfc,this,3)
	{
//	iDirection=ESoundDirRecord;
//	iState=EOpen;
// 	iBufConfig=NULL;
//	iPowerHandler=NULL;
//	iSoundConfigFlags=0;
//	iBytesTransferred=0;
//	iBufManager=NULL;
//	iTestSettings=0;
//	iPlayEofTimerActive=EFalse;
//	iThreadOpenCount=0;

	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::DSoundScLdd"));

	iUnit=-1;	// Invalid unit number
	
	// Many drivers would open the client thread's DThread object here. However, since this driver allows a channel to be shared by multiple client 
	// threads - we have to open and close the relevent DThread object for each request. 
	}

/**
Destructor for the sound driver logical channel.
*/
DSoundScLdd::~DSoundScLdd()
	{

	if (iNotifyChangeOfHwClientRequest)
		Kern::DestroyClientRequest(iNotifyChangeOfHwClientRequest);

	// Free the TClientRequest structures associated with requests
	if (iClientRequests)
		{
		for (TInt index=0; index<RSoundSc::ERequestRecordData+1; ++index)
			if (iClientRequests[index])
				Kern::DestroyClientRequest(iClientRequests[index]);

		delete[] iClientRequests;
		}

	// Check if we need to delete the shared chunk / audio buffers.
	if (iBufManager)
		delete iBufManager;
	
	// Delete any memory allocated to hold the current buffer configuration.
	if (iBufConfig)
		delete iBufConfig;
	
	// Remove and delete the power handler.
	if (iPowerHandler)
		{
		iPowerHandler->Remove(); 
		delete iPowerHandler;
		}
		
	// Delete the request queue
	if (iReqQueue)
		delete iReqQueue;		
	
	__ASSERT_DEBUG(iThreadOpenCount==0,Kern::Fault(KSoundLddPanic,__LINE__));	
	
	// Clear the 'units open mask' in the LDD factory.
	if (iUnit>=0)
		((DSoundScLddFactory*)iDevice)->SetUnitOpen(iUnit,EFalse);
	}
	
/**
Second stage constructor for the sound driver - called by the kernel's device driver framework.
This is called in the context of the client thread which requested the creation of a logical channel.
The thread is in a critical section.
@param aUnit The unit argument supplied by the client.
@param aInfo The info argument supplied by the client. Always NULL in this case.
@param aVer The version argument supplied by the client.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DSoundScLdd::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::DoCreate"));
	
	// Check the client has ECapabilityMultimediaDD capability.
	if (!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by ESOUNDSC.LDD (Sound driver)")))
		return(KErrPermissionDenied);

	// Check that the sound driver version specified by the client is compatible.
	if (!Kern::QueryVersionSupported(RSoundSc::VersionRequired(),aVer))
		return(KErrNotSupported);
	
	// Check that a channel hasn't already been opened on this unit.
	TInt r=((DSoundScLddFactory*)iDevice)->SetUnitOpen(aUnit,ETrue); // Try to update 'units open mask' in the LDD factory.
	if (r!=KErrNone)
		return(r);
	iUnit=aUnit;

	// Create a TClientRequest for each request that can be completed by the DFC thread.  These TClientRequest
	// instances are separate to those embedded in the TSoundScRequest structures and are used for requests that
	// have no associated TSoundScRequest structure or which are completing prematurely before they can be
	// associated with a TSoundScRequest structure
	if ((iClientRequests=new TClientRequest*[RSoundSc::ERequestRecordData+1])==NULL)
		return KErrNoMemory;

	for (TInt index=0; index<RSoundSc::ERequestRecordData+1; ++index)
		if ((r=Kern::CreateClientRequest(iClientRequests[index]))!=KErrNone)
			return r;

	if ((r=Kern::CreateClientDataRequest(iNotifyChangeOfHwClientRequest))!=KErrNone)
		return r;

	// Initialise the PDD
	Pdd()->iLdd=this;
	
	// Read back the capabilities of this device from the PDD and determine the data transfer direction for this unit.
	TPckg<TSoundFormatsSupportedV02> capsBuf(iCaps);
	Pdd()->Caps(capsBuf);
	iDirection=iCaps.iDirection;

	// Check the client has UserEnvironment capability if recording.
	if(iDirection==ESoundDirRecord)
		{
		if (!Kern::CurrentThreadHasCapability(ECapabilityUserEnvironment,__PLATSEC_DIAGNOSTIC_STRING("Checked by ESOUNDSC.LDD (Sound driver)")))
			return(KErrPermissionDenied);
		}
	
	// Create the appropriate request queue
	if (iDirection==ESoundDirPlayback)
		iReqQueue=new TSoundScPlayRequestQueue(this);
	else
		iReqQueue=new TSoundScRequestQueue(this);
	if (!iReqQueue)
		return(KErrNoMemory);
	r=iReqQueue->Create();
	if (r!=KErrNone)
		return(r);
	
	// Setup the default audio configuration acording to these capabilities.
	iSoundConfig.iChannels=HighestCapabilitySupported(iCaps.iChannels)+1;
	__ASSERT_ALWAYS(iSoundConfig.iChannels>0,Kern::Fault(KSoundLddPanic,__LINE__));
	iSoundConfig.iRate=(TSoundRate)HighestCapabilitySupported(iCaps.iRates);
	__ASSERT_ALWAYS(iSoundConfig.iRate>=0,Kern::Fault(KSoundLddPanic,__LINE__));
	iSoundConfig.iEncoding=(TSoundEncoding)HighestCapabilitySupported(iCaps.iEncodings);
	__ASSERT_ALWAYS(iSoundConfig.iEncoding>=0,Kern::Fault(KSoundLddPanic,__LINE__));
	iSoundConfig.iDataFormat=(TSoundDataFormat)HighestCapabilitySupported(iCaps.iDataFormats);
	__ASSERT_ALWAYS(iSoundConfig.iDataFormat>=0,Kern::Fault(KSoundLddPanic,__LINE__));
	__ASSERT_ALWAYS(ValidateConfig(iSoundConfig)==KErrNone,Kern::Fault(KSoundLddPanic,__LINE__));
	iSoundConfigFlags=0;
	
	// Setup the default setting for the record level / play volume.
	iVolume=KSoundMaxVolume;
		
	// Set up the correct DFC queue
	TDfcQue* dfcq=((DSoundScPdd*)iPdd)->DfcQ(aUnit);
	SetDfcQ(dfcq);
	iPowerDownDfc.SetDfcQ(dfcq);
	iPowerUpDfc.SetDfcQ(dfcq);
	iMsgQ.Receive();
	
	// Create the power handler
	iPowerHandler=new DSoundScPowerHandler(this);
	if (!iPowerHandler)
		return(KErrNoMemory);
	iPowerHandler->Add();
	
	// Power up the hardware.
	r=Pdd()->PowerUp();
	
	return(r);
	}
	
/**
Shutdown the audio device.
Terminate all device activity and power down the hardware.
*/
void DSoundScLdd::Shutdown()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::Shutdown"));

	Pdd()->StopTransfer();
	
	// Power down the hardware
	Pdd()->PowerDown();

	// Cancel any requests that we may be handling	
	DoCancel(RSoundSc::EAllRequests);
	
	iState=EOpen;

	// Make sure DFCs and timers are not queued.
	iPowerDownDfc.Cancel();
	iPowerUpDfc.Cancel();
	CancelPlayEofTimer();
	}
	
/**
Process a request on this logical channel
Called in the context of the client thread.
@param aReqNo The request number:
  	          ==KMaxTInt: a 'DoCancel' message;
	          >=0: a 'DoControl' message with function number equal to value.
	          <0: a 'DoRequest' message with function number equal to ~value.
@param a1 The first request argument. For DoRequest(), this is a pointer to the TRequestStatus.
@param a2 The second request argument. For DoRequest(), this is a pointer to the 2 actual TAny* arguments.
@return The result of the request. This is ignored by device driver framework for DoRequest().
*/ 
TInt DSoundScLdd::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
//	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::Request(%d)",aReqNo));
	TInt r;
	
	// Check for DoControl or DoRequest functions which are configured to execute in kernel thread context. This
	// also applies to DoCancel functions and ERequestRecordData requests where recording mode is not yet enabled.
	if ((aReqNo<RSoundSc::EMsgControlMax && aReqNo>(~RSoundSc::EMsgRequestMax)) ||
	    aReqNo==KMaxTInt ||
	    ((~aReqNo)==RSoundSc::ERequestRecordData && (iState==EOpen || iState==EConfigured)) 
	   )
		{
		// Implement in the context of the kernel thread - prepare and issue a kernel message.
		r=DLogicalChannel::Request(aReqNo,a1,a2);		
		}	
	else
		{
		// Implement in the context of the client thread.	
		// Decode the message type and dispatch it to the relevent handler function.
		if ((TUint)aReqNo<(TUint)KMaxTInt)
			r=DoControl(aReqNo,a1,a2,&Kern::CurrentThread());	// DoControl - process the request.
		
		else
			{
			// DoRequest - read the arguments from the client thread and process the request.
			TAny* a[2];
			kumemget32(a,a2,sizeof(a)); 
			TRequestStatus* status=(TRequestStatus*)a1;
			NKern::ThreadEnterCS(); 				// Need to be in critical section while manipulating the request/buffer list (for record).
			r=DoRequest(~aReqNo,status,a[0],a[1],&Kern::CurrentThread());
		
			// Complete request if there was an error
			if (r!=KErrNone)
				CompleteRequest(&Kern::CurrentThread(),status,r);
			r=KErrNone;
			NKern::ThreadLeaveCS();
			}
		}
//	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::Request - %d",r));
	return(r);
	}

/**
Send a message to the DFC thread for processing by HandleMsg().

This function is called in the context of the client thread.

Overridden to ensure client data is copied kernel-side to avoid page-faults.

@param aMsg  The message to process.

@return KErrNone if the message was send successfully, otherwise one of the other system-wide error
        codes.
*/
TInt DSoundScLdd::SendMsg(TMessageBase* aMsg)
	{
	// Executes in context of client thread

	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id = m.iValue;

	TInt r(KErrNone);
	if (id == ~RSoundSc::EMsgRequestPlayData)
		{
		r = PrePlay(aMsg);
		if (r!=KErrNone)
			{
			// This is an asynchronous request so need to return error through the TRequestStatus
			TRequestStatus* status = (TRequestStatus*)(m.Ptr0());
			Kern::RequestComplete(status,r);
			return(r);
			}
		r = DLogicalChannel::SendMsg(aMsg);
		if (r!=KErrNone)
			{
			iReqQueue->Free((TSoundScPlayRequest*)m.iArg[1]);	// Return the unused request object	
			}
		return(r);
		}
	else if (id == RSoundSc::EMsgControlSetBufChunkCreate || id == RSoundSc::EMsgControlSetBufChunkOpen)
		{
		r = PreSetBufferChunkCreateOrOpen(aMsg);
		if (r!=KErrNone)
			{
			return(r);
			}
		}
	else if (id == RSoundSc::EMsgControlSetAudioFormat)
		{
		r = PreSetSoundConfig(aMsg);
		if (r!=KErrNone)
			{
			return(r);
			}
		}

	r = DLogicalChannel::SendMsg(aMsg);
		

	return(r);
	}

/**
PreProcess a play request on this logical channel
Called in the context of the client thread.

@param aMsg  The message to process.

@return KErrNone if the parameters are validated and request structure populated. Otherwise a system-wide error.
*/ 
TInt DSoundScLdd::PrePlay(TMessageBase* aMsg)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::PrePlay"));

	// Executes in context of client thread

	TThreadMessage* m=(TThreadMessage*)aMsg;

	// Copy play information to kernel side before checking
	SRequestPlayDataInfo info;
	kumemget(&info,m->iArg[1],sizeof(info));

	__KTRACE_OPT(KSOUND1, Kern::Printf("DSoundScLdd::PrePlay - off %x len %x flg %x ",info.iBufferOffset,info.iLength,info.iFlags));

	// validate parameters in the play structure

	// Check that the offset argument is aligned correctly for the PDD.
	TUint32 alignmask=(1<<iCaps.iRequestAlignment)-1; // iRequestAlignment holds log to base 2 of alignment required
	if ((info.iBufferOffset & alignmask) != 0)
		return(KErrArgument);
	
	// Check that the length argument is compatible with the minimum request size required for the PDD.
	if (iCaps.iRequestMinSize && info.iLength%iCaps.iRequestMinSize)
		return(KErrArgument);
	
	// Check that the specified offset and length are valid in the chunk. If so, get a pointer to the corresponding 
	// audio buffer object.
	TAudioBuffer* buf;
	if (iBufManager)
		{
		TInt r=iBufManager->ValidateRegion(info.iBufferOffset,info.iLength,buf);
		if (r!=KErrNone)
			return(r);
		}
	else
		{
		return(KErrNotReady);
		}	

	// Acquire a free request object and add it to the queue of pending requests.
	TSoundScPlayRequest* req=(TSoundScPlayRequest*)iReqQueue->NextFree();
	if (!req)
		return(KErrGeneral);										// Must have exceeded KMaxSndScRequestsPending.
	req->iTf.Init((TUint)buf,info.iBufferOffset,info.iLength,buf); 	// Use pointer to audio buffer as unique ID
	req->iFlags=info.iFlags;

	// replace the argument with a pointer to the kernel-side structure
	m->iArg[1]=req;
	
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::PrePlay"));

	return(KErrNone);
	}

/**
PreProcess a SetBufferChunkCreate and SetBufferChunkOpen on this logical channel
Called in the context of the client thread.
This is synchronous so only need one copy of the data on the kernel-side.

@param aMsg  The message to process.

@return KErrNone if the parameters are validated and request structure populated. Otherwise a system-wide error.
*/ 
TInt DSoundScLdd::PreSetBufferChunkCreateOrOpen(TMessageBase* aMsg)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::PreSetBufferChunkCreateOrOpen"));
	TInt r(KErrNone);

	TThreadMessage* m=(TThreadMessage*)aMsg;

	TInt length, maxLength;
	const TDesC8* userDesc = (const TDesC8*)m->Ptr0();
	const TUint8* configData = Kern::KUDesInfo(*userDesc,length,maxLength);

	//__KTRACE_OPT(KSOUND1, Kern::Printf("DSoundScLdd::PreSetBufferChunkCreateOrOpen - len %x maxlen %x",length,maxLength));

	// check the descriptor length is >= the base class size
	TInt minDesLen=sizeof(TSharedChunkBufConfigBase);
	if (length<minDesLen)
		return(KErrArgument);

	// Temporary copy of client-side buffer config structure  
	TSharedChunkBufConfigBase chunkBufConfig;

	kumemget(&chunkBufConfig, configData, minDesLen);

	//__KTRACE_OPT(KSOUND1, Kern::Printf("DSoundScLdd::PreSetBufferChunkCreateOrOpen - num %x size %x flg %x ",chunkBufConfig.iNumBuffers,chunkBufConfig.iBufferSizeInBytes,chunkBufConfig.iFlags));

	// check the buffer argument
	if (chunkBufConfig.iNumBuffers<=0)
		return(KErrArgument);

	// Validate the rest of the configuration supplied.
	if (chunkBufConfig.iBufferSizeInBytes<=0)
		return(KErrArgument);

	if (iDirection==ESoundDirRecord)
		{
		// If this is a record channel then the size of each buffer must comply with the PDD contraints.
		if (iCaps.iRequestMinSize && chunkBufConfig.iBufferSizeInBytes%iCaps.iRequestMinSize)
			return(KErrArgument);
		}	

	//Allocate space for the buffer list 
	NKern::ThreadEnterCS();
	r=ReAllocBufferConfigInfo(chunkBufConfig.iNumBuffers);
	NKern::ThreadLeaveCS();
	if (r!=KErrNone)
		return(r);

	//__KTRACE_OPT(KSOUND1, Kern::Printf("DSoundScLdd::PreSetBufferChunkCreateOrOpen - cfg %x size %x",iBufConfig,iBufConfigSize));

	// copy all data into the buffer list 
	kumemget(iBufConfig, configData, iBufConfigSize);

	return(r);
	}

/**
PreProcess a SetSoundConfig on this logical channel
Called in the context of the client thread.
This is synchronous so only need one copy of the data on the kernel-side.

@param aMsg  The message to process.

@return KErrNone if the parameters are validated and request structure populated. Otherwise a system-wide error.
*/ 
TInt DSoundScLdd::PreSetSoundConfig(TMessageBase* aMsg)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::PreSetSoundConfig"));

	TThreadMessage* m=(TThreadMessage*)aMsg;

	TPtr8 localPtr((TUint8*)&iTempSoundConfig, sizeof(TCurrentSoundFormatV02));

	Kern::KUDesGet(localPtr,*(const TDesC8*)m->Ptr0());

	//__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::PreSetSoundConfig chan %x rate %x enc %x form %x",
	//	iTempSoundConfig.iChannels,iTempSoundConfig.iRate,iTempSoundConfig.iEncoding,iTempSoundConfig.iDataFormat));

	// Check that it is compatible with this sound device.
	TInt r=ValidateConfig(iTempSoundConfig);
	
	return(r);
	}

/**
Processes a message for this logical channel.
This function is called in the context of a DFC thread.
@param aMsg The message to process.
	        The iValue member of this distinguishes the message type:
	          iValue==ECloseMsg: channel close message.
	          iValue==KMaxTInt: a 'DoCancel' message
	          iValue>=0: a 'DoControl' message with function number equal to iValue
	          iValue<0: a 'DoRequest' message with function number equal to ~iValue
*/
void DSoundScLdd::HandleMsg(TMessageBase* aMsg)
	{
#ifdef _DEBUG
#ifdef TEST_WITH_PAGING_CACHE_FLUSHES
	Kern::SetRealtimeState(ERealtimeStateOn);  
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
#endif
#endif

	TThreadMessage& m=*(TThreadMessage*)aMsg;
    TInt id=m.iValue;
//	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::HandleMsg(%d)",id));
    
	if (id==(TInt)ECloseMsg)
		{
		// Channel close.
		Shutdown();
		m.Complete(KErrNone,EFalse);
		return;
		}
    else if (id==KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}
    else if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2(),m.Client());
		if (r!=KErrNone)
			{
			iClientRequests[~id]->SetStatus(pS);
			CompleteRequest(m.Client(),NULL,r,iClientRequests[~id]);
			}
		m.Complete(KErrNone,ETrue);
		}
    else
		{
		// DoControl
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1(),m.Client());
		m.Complete(r,ETrue);
		}
	}

/**
Process a synchronous 'DoControl' request.
@param aFunction The request number.
@param a1 The first request argument.
@param a2 The second request argument.
@param aThread The client thread which issued the request.
@return The result of the request.
*/
TInt DSoundScLdd::DoControl(TInt aFunction,TAny* a1,TAny* a2,DThread* aThread)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::DoControl(%d)",aFunction));
	
	TInt r=KErrNotSupported;
	switch (aFunction)
		{
		case RSoundSc::EControlGetCaps:
			{
			// Return the capabilities for this device. Read this from the PDD and 
			// then write it to the client. 
			TSoundFormatsSupportedV02Buf caps;
			Pdd()->Caps(caps);
			Kern::InfoCopy(*((TDes8*)a1),caps);
			r=KErrNone;
			break;	
			}
		case RSoundSc::EControlGetAudioFormat:
			{
			// Write the current audio configuration back to the client.
			TPtrC8 ptr((const TUint8*)&iSoundConfig,sizeof(iSoundConfig));
			Kern::InfoCopy(*((TDes8*)a1),ptr);
			r=KErrNone;
			break;	
			}
		case RSoundSc::EMsgControlSetAudioFormat:
			{
			if (iState==EOpen || iState==EConfigured || iPlayEofTimerActive)
				{
				// If the play EOF timer is active then it is OK to change the audio configuration - but we
				// need to bring the PDD out of transfer mode first.
				if (iPlayEofTimerActive)
					{
					CancelPlayEofTimer();
					Pdd()->StopTransfer();
					}
				
				r=SetSoundConfig(); 
				if (r==KErrNone && (iSoundConfigFlags&KSndScVolumeIsSetup) && iBufConfig)
					iState=EConfigured;
				}		
			else
				r=KErrInUse;
			break;
			}
		case RSoundSc::EControlGetBufConfig:
			if (iBufConfig)
				{
				// Write the buffer config to the client.
				TPtrC8 ptr((const TUint8*)iBufConfig,iBufConfigSize);
				Kern::InfoCopy(*((TDes8*)a1),ptr);
				r=KErrNone;	
				}	
			break;
		case RSoundSc::EMsgControlSetBufChunkCreate:
			{
			if (iState==EOpen || iState==EConfigured || iPlayEofTimerActive)
				{
				// Need to be in critical section while deleting an exisiting config and creating a new one
				NKern::ThreadEnterCS();
				r=SetBufferConfig(aThread);
				NKern::ThreadLeaveCS();
				if (r==KErrNone && (iSoundConfigFlags&KSndScSoundConfigIsSetup) && (iSoundConfigFlags&KSndScVolumeIsSetup))
					iState=EConfigured; 		
				}
			else
				r=KErrInUse;
			break;
			}
		case RSoundSc::EMsgControlSetBufChunkOpen:
			{
			if (iState==EOpen || iState==EConfigured || iPlayEofTimerActive)
				{
				// Need to be in critical section while deleting an exisiting config and creating a new one
				NKern::ThreadEnterCS();
				r=SetBufferConfig((TInt)a2,aThread);
				NKern::ThreadLeaveCS();
				if (r==KErrNone && (iSoundConfigFlags&KSndScSoundConfigIsSetup) && (iSoundConfigFlags&KSndScVolumeIsSetup))
					iState=EConfigured; 		
				}
			else
				r=KErrInUse;
			break;
			}
		case RSoundSc::EControlGetVolume:
			r=iVolume;
			break;
		case RSoundSc::EMsgControlSetVolume:
			{
			r=SetVolume((TInt)a1);
			if (r==KErrNone && iState==EOpen && (iSoundConfigFlags&KSndScSoundConfigIsSetup) && iBufConfig)
				iState=EConfigured;
			break;	
			}
		case RSoundSc::EMsgControlCancelSpecific:
			{
			if (iDirection==ESoundDirPlayback)
				{
				// Don't try to cancel a play transfer that has already started - let it complete in its own time.
				TSoundScPlayRequest* req=(TSoundScPlayRequest*)iReqQueue->Find((TRequestStatus*)a1);
				if (req && req->iTf.iTfState==TSndScTransfer::ETfNotStarted)
					{
					iReqQueue->Remove(req);
					CompleteRequest(req->iOwningThread,NULL,KErrCancel,req->iClientRequest);
					iReqQueue->Free(req);
					}
				}
			else
				{
				// Need to aquire the buffer/request list mutex when removing record requests - RecordData() runs in
				// client thread context and this may access the queue. Record requests a treated differently to play
				// requests and you don't have to worry about record requests already being in progress.
				NKern::FMWait(&iMutex);
				TSoundScRequest* req=iReqQueue->Find((TRequestStatus*)a1);
				if (req)
					{
					iReqQueue->Remove(req);
					DThread* thread=req->iOwningThread;				// Take a copy before we free it.
					TClientRequest* clreq=req->iClientRequest;		// Take a copy before we free it.
					NKern::FMSignal(&iMutex);
					iReqQueue->Free(req);
					CompleteRequest(thread,NULL,KErrCancel,clreq);
					}
				else
					NKern::FMSignal(&iMutex);	
				}
			r=KErrNone;	
			break;	
			}
		case RSoundSc::EControlBytesTransferred:
			r=iBytesTransferred;
			break;
		case RSoundSc::EControlResetBytesTransferred:
			iBytesTransferred=0;
			r=KErrNone;	
			break;
		case RSoundSc::EMsgControlPause:
			if (iState==EActive)
				{
				// Have to update the status early here because a record PDD may call us back with RecordCallback() in
				// handling PauseTransfer() - to complete a partially filled buffer.  
				iState=EPaused;
				iCompletesWhilePausedCount=0;		
				r=Pdd()->PauseTransfer();
				if (r!=KErrNone)
					iState=EActive;
				else if (iDirection==ESoundDirRecord)
					{
					// For record, complete any pending record requests that are still outstanding following PauseTransfer().
					iReqQueue->CompleteAll(KErrCancel);	
					}
				}
			else
				r=KErrNotReady;	
			break;
		case RSoundSc::EMsgControlResume:
			if (iState==EPaused)
				{
				r=Pdd()->ResumeTransfer();
				if (r==KErrNone && iDirection==ESoundDirRecord)
					r=StartNextRecordTransfers();
				if (r==KErrNone)
					iState=EActive;	// Successfully resumed transfer - update the status.
				}
			else
				r=KErrNotReady;	
			break;
		case RSoundSc::EControlReleaseBuffer:
			if (iDirection==ESoundDirRecord)
				r=ReleaseBuffer((TInt)a1);
			break; 
		case RSoundSc::EMsgControlCustomConfig:
			r=CustomConfig((TInt)a1,a2);
			break;
		case RSoundSc::EControlTimePlayed:
			if (iDirection==ESoundDirPlayback)
				{
				TInt64 time=0;
				r=Pdd()->TimeTransferred(time,iState);
				TPtrC8 timePtr((TUint8*)&time,sizeof(TInt64));
				Kern::ThreadDesWrite(aThread,a1,timePtr,0,KTruncateToMaxLength,NULL);
				}
			else
				r=KErrNotSupported;
			break;
		case RSoundSc::EControlTimeRecorded:
			if (iDirection==ESoundDirRecord)
				{
				TInt64 time=0;
				r=Pdd()->TimeTransferred(time,iState);
				TPtrC8 timePtr((TUint8*)&time,sizeof(TInt64));
				Kern::ThreadDesWrite(aThread,a1,timePtr,0,KTruncateToMaxLength,NULL);
				}
			else
				r=KErrNotSupported;
			break;
		}
		
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::DoControl - %d",r));
	return(r);
	}

/**
Process an asynchronous 'DoRequest' request.
@param aFunction The request number.
@param aStatus A pointer to the TRequestStatus.
@param a1 The first request argument.
@param a2 The second request argument.
@param aThread The client thread which issued the request.
@return The result of the request.
*/
TInt DSoundScLdd::DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* a1, TAny* /*a2*/,DThread* aThread)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::DoRequest(%d)",aFunction));
	
	// Open a reference on the client thread while the request is pending so it's control block can't disappear until this driver has finished with it.
	TInt r=aThread->Open();
	__ASSERT_ALWAYS(r==KErrNone,Kern::Fault(KSoundLddPanic,__LINE__));
#ifdef _DEBUG
	__e32_atomic_add_ord32(&iThreadOpenCount, 1);
#endif		

	r=KErrNotSupported;
	switch (aFunction)
		{
		case RSoundSc::EMsgRequestPlayData:
			{
			if (iDirection==ESoundDirPlayback)
				{
				if (iState==EOpen)
					{
					// Not yet fully configured - maybe we can use the default settings.
					r=KErrNone;
					if (!iBufConfig)
						r=KErrNotReady;	// Can't guess a default buffer configuration.
					else
						{
						if (!(iSoundConfigFlags&KSndScSoundConfigIsSetup))
							r=DoSetSoundConfig(iSoundConfig);	// Apply default sound configuration.
						if (r==KErrNone && !(iSoundConfigFlags&KSndScVolumeIsSetup))
							r=SetVolume(iVolume);				// Apply default volume level
						}
					if (r!=KErrNone)
						break;
					else
						iState=EConfigured;		
					}
					
				if (iState==EConfigured || iState==EActive || iState==EPaused)
					{
					r=PlayData(aStatus, (TSoundScPlayRequest*)a1,aThread);
					}
				else
					r=KErrNotReady;
				}
			break;
			}
		case RSoundSc::ERequestRecordData:
			if (iDirection==ESoundDirRecord)
				{
				// Check if the device has been configured yet
				if (iState==EOpen)
					{
					// Not yet fully configured - maybe we can use the default settings.
					r=KErrNone;
					if (!iBufConfig)
						r=KErrNotReady;	// Can't guess a default buffer configuration.
					else
						{
						if (!(iSoundConfigFlags&KSndScSoundConfigIsSetup))
							r=DoSetSoundConfig(iSoundConfig);	// Apply default sound configuration.
						if (r==KErrNone && !(iSoundConfigFlags&KSndScVolumeIsSetup))
							r=SetVolume(iVolume);				// Apply default volume level
						}
					if (r!=KErrNone)
						break;
					else
						iState=EConfigured;		
					}
				// Check if we need to start recording
				if (iState==EConfigured)
					{
					r=StartRecord();
					if (r!=KErrNone)
						break;
					else
						iState=EActive;
					}	
				
				// State must be either active or paused so process the record request as appropriate for these states.
				r=RecordData(aStatus,(TInt*)a1,aThread);
				}
			break;
		case RSoundSc::ERequestNotifyChangeOfHwConfig:
			{
			// Check if this device can detect changes in its hardware configuration.
			if (iCaps.iHwConfigNotificationSupport)
				{
				r=KErrNone;
				if (!iNotifyChangeOfHwClientRequest->IsReady())
					{
					iChangeOfHwConfigThread=aThread;
					iNotifyChangeOfHwClientRequest->SetDestPtr((TBool*)a1);
					r = iNotifyChangeOfHwClientRequest->SetStatus(aStatus);
					}
				else
					r=KErrInUse;
				}
			else
				r=KErrNotSupported;	
			break;
			}
		}
		
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::DoRequest - %d",r));
	return(r);
	}

/**
Process the cancelling of asynchronous requests.
@param aMask A mask indicating which requests need to be cancelled.
@return The result of the cancel.
*/
TInt DSoundScLdd::DoCancel(TUint aMask)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::DoCancel(%08x)",aMask));

	if (aMask&(1<<RSoundSc::EMsgRequestPlayData))
		{
		Pdd()->StopTransfer();
		iReqQueue->CompleteAll(KErrCancel);					// Cancel any outstanding play requests
		if ((iState==EActive)||(iState==EPaused))
			iState=EConfigured;
		}
	if (aMask&(1<<RSoundSc::ERequestRecordData))
		{
		Pdd()->StopTransfer();
		iReqQueue->CompleteAll(KErrCancel,&iMutex);		// Cancel any outstanding record requests
		if ((iState==EActive)||(iState==EPaused))
			iState=EConfigured;
		}
	if (aMask&(1<<RSoundSc::ERequestNotifyChangeOfHwConfig))
		{
		// Complete any pending hardware change notifier with KErrCancel.
		if (iNotifyChangeOfHwClientRequest->IsReady())
			CompleteRequest(iChangeOfHwConfigThread,NULL,KErrCancel,iNotifyChangeOfHwClientRequest); 
		}		
	return(KErrNone);
	}

/**
Set the current buffer configuration - creating a shared chunk.
@param aBufferConfigBuf A packaged TSharedChunkBufConfigBase derived object holding the buffer configuration settings of
	the shared chunk required.
@param aThread The client thread which has requested to own the chunk.
@return A handle to the shared chunk for the owning thread (a value >0), if successful;
        otherwise one of the other system wide error codes, (a value <0).
@pre The thread must be in a critical section. 
*/	
TInt DSoundScLdd::SetBufferConfig(DThread* aThread)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd:SetBufferConfig"));
	
	TInt r(KErrNone);

	// Delete any existing buffers and the shared chunk.
	if (iBufManager)
		{
		delete iBufManager;
		iBufManager=NULL;
		} 
						
	// If a handle to the shared chunk was created, close it, using the handle of the thread on which
	// it was created, in case a different thread is now calling us
	if (iChunkHandle>0)
		{
		Kern::CloseHandle(iChunkHandleThread,iChunkHandle);
		iChunkHandle=0;
		}

	// Create the shared chunk, then create buffer objects for the committed buffers within it. This is
	// done by creating a buffer manager - create the apppropraiate version according to the audio direction.
	if (iDirection==ESoundDirPlayback)
		iBufManager=new DBufferManager(this);
	else
		iBufManager=new DRecordBufferManager(this);
	if (!iBufManager)
		return(KErrNoMemory);
	r=iBufManager->Create(iBufConfig);
	if (r!=KErrNone)
		{
		delete iBufManager;
		iBufManager=NULL;
		return(r);
		} 
	
	// Create handle to the shared chunk for the owning thread.
	r=Kern::MakeHandleAndOpen(aThread,iBufManager->iChunk);

	// And save the the chunk and thread handles for later.  Normally the chunk handle will be closed when the chunk
	// is closed, but if the chunk is re-allocated then it will need to be closed before re-allocation.
	iChunkHandle=r;
	iChunkHandleThread=aThread;

	return(r);
	}	

/**
Set the current buffer configuration - using an existing shared chunk.
@param aBufferConfigBuf A packaged TSharedChunkBufConfigBase derived object holding the buffer configuration settings of
	the shared chunk supplied.
@param aChunkHandle A handle for the shared chunk supplied by the client.
@param aThread The thread in which the given handle is valid.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section. 
*/	
TInt DSoundScLdd::SetBufferConfig(TInt aChunkHandle,DThread* aThread)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd:SetBufferConfig(Handle-%d)",aChunkHandle));

	TInt r(KErrNone);

	// Delete any existing buffers and the shared chunk.
	if (iBufManager)
		{
		delete iBufManager;
		iBufManager=NULL;
		} 
	
	// Open the shared chunk supplied and create buffer objects for the committed buffers within it. This is
	// done by creating a buffer manager - create the apppropraiate version according to the audio direction.
	if (iDirection==ESoundDirPlayback)
		iBufManager=new DBufferManager(this);
	else
		iBufManager=new DRecordBufferManager(this);
	if (!iBufManager)
		return(KErrNoMemory);
	r=iBufManager->Create(*iBufConfig,aChunkHandle,aThread);
	if (r!=KErrNone)
		{
		delete iBufManager;
		iBufManager=NULL;
		} 
	return(r);
	}	

/**
Set the current audio format configuration.
@param aSoundConfigBuf A packaged sound configuration object holding the new audio configuration settings to be used.
@param aThread The client thread which contains the sound configuration object.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DSoundScLdd::SetSoundConfig()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd:SetSoundConfig"));
	
	TInt r=DoSetSoundConfig(iTempSoundConfig);

	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::SetSoundConfig - %d",KErrNone));
	return(r);
	}

/**
Apply a new audio format configuration.
@param aSoundConfig A reference to a sound configuration object holding the new audio configuration settings to be applied.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/	
TInt DSoundScLdd::DoSetSoundConfig(const TCurrentSoundFormatV02& aSoundConfig)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd:DoSetSoundConfig"));
		
	// We're about to replace any previous configuration - so set the
	// status back to un-configured in case we don't succeed with the new one.
	iSoundConfigFlags&=~KSndScSoundConfigIsSetup;
	
	// Call the PDD to change the hardware configuration according to the new specification.
	// Pass it as a descriptor - to support future changes to the config structure.
	TPtrC8 ptr((TUint8*)&aSoundConfig,sizeof(aSoundConfig));
	TInt r=Pdd()->SetConfig(ptr);
	if (r!=KErrNone)
		return(r);
	
	// Setting up the new play configuration has succeeded so save the new configuration.
	iSoundConfig=aSoundConfig;
	iSoundConfigFlags|=KSndScSoundConfigIsSetup;
	
	// For some devices, the maximum transfer length supported will vary according to the configuration.
	if (iBufManager)
		iBufManager->iMaxTransferLen=Pdd()->MaxTransferLen();
	
	return(r);
	}	
	
/**
Set the current play volume or record level.
@param aVolume The play volume / record level to be set - a value in the range 0 to 255. The value 255 equates to
	the maximum volume and each value below this equates to a 0.5dB step below it.
@return KErrNone if successful, otherwise one of the other system wide error codes.	
*/
TInt DSoundScLdd::SetVolume(TInt aVolume)
	{
	TInt r;
	// Check if the volume specified is in range.
	if (aVolume>=0 && aVolume<=255)
		{
		// Check if we need to change it.
		if (!(iSoundConfigFlags&KSndScVolumeIsSetup) || aVolume!=iVolume)
			{
			// We're about to replace any previous volume setting - so set the
			// status back to un-set in case we don't succeed with the new setting.
			iSoundConfigFlags&=~KSndScVolumeIsSetup;
			
			r=Pdd()->SetVolume(aVolume);
			if (r==KErrNone)
				{
				iVolume=aVolume;
				iSoundConfigFlags|=KSndScVolumeIsSetup;
				}
			}
		else
			r=KErrNone;	
		}
	else
		r=KErrArgument;	
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::SetVolume(%d) - %d",aVolume,r));
	return(r);
	}
	
/**
Handle a play request from the client.
@param aStatus The request status to be signalled when the play request is complete.
@param aChunkOffset Offset from the beginning of the play chunk for the start of data to be played.
@param aLength The number of bytes of data to be played.
@param aFlags The play request flags which were supplied by the client for this request.
@param aThread The client thread which issued the request and which supplied the request status.
@return KErrNone if successful;
        KErrArgument if the offset or length arguments are not fully contained within a buffer or don't meet the
        	alignment contraints of the PDD;
        KErrNoMemory if a memory error was ecountered in the handling of this request.
        otherwise one of the other system-wide error codes.
*/
TInt DSoundScLdd::PlayData(TRequestStatus* aStatus,TSoundScPlayRequest* aRequest,DThread* aThread)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd:PlayData(off:%x len:%d)",aRequest->iTf.GetStartOffset(),aRequest->iTf.GetNotStartedLen()));
	
	// Purge the region of the play chunk concerned.
	iBufManager->FlushData(aRequest->iTf.GetStartOffset(),aRequest->iTf.GetNotStartedLen(),DBufferManager::EFlushBeforeDmaWrite);
	
	
	TInt r(KErrNone);

	// finalise the request data here
	r = aRequest->iClientRequest->SetStatus(aStatus);
	if (r!=KErrNone)
		return(r);

	aRequest->iOwningThread = aThread;


	// Check whether we have started the codec yet.
	CancelPlayEofTimer();
	if (iState==EConfigured)
		{
		r=Pdd()->StartTransfer();
	
		// Test settings - only possible in debug mode. Test handling of an error returned from the PDD for StartTransfer().
#ifdef _DEBUG	
		if (iTestSettings & KSoundScTest_StartTransferError)
			{
			iTestSettings&=(~KSoundScTest_StartTransferError);
			r=KErrTimedOut;
			// Any time that StartTransfer() is called on the PDD it must have a matching StopTransfer() before
			// it is called again
			Pdd()->StopTransfer();
			}
#endif
		}
	
	if (r==KErrNone)
		{
		// No further error is possible at this stage so add the request to the queue.
		iReqQueue->Add(aRequest);
		
		if (iState!=EPaused)
			{
			iState=EActive;	
			StartNextPlayTransfers(); // Queue as many transfer requests on the PDD as it can accept.
			}
		}
	else
		iReqQueue->Free(aRequest);	// Return the unused request object	
	
	return(r);
	}
	
/**
@publishedPartner
@prototype

Called from the PDD each time it has completed a data transfer from a play buffer.  This function must be called
in the context of the DFC thread used for processing requests.
The function performed here is to check whether the entire transfer for the current request is now complete. Also to
queue further requests on the PDD which should now have the capability to accept more transfers. If the current
request is complete then we signal completion to the client.
@param aTransferID A value provided by the LDD when it initiated the transfer allowing the transfer fragment to be 
	uniquely identified.
@param aTransferResult The result of the transfer being completed: KErrNone if successful, otherwise one of the other
	system wide error codes.
@param aBytesPlayed The number of bytes played from the play buffer.	
*/
void DSoundScLdd::PlayCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesPlayed)
	{
#ifdef _DEBUG
#ifdef TEST_WITH_PAGING_CACHE_FLUSHES
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
#endif
#endif
	// Test settings - only possible in debug mode.
#ifdef _DEBUG	
	if (iTestSettings & KSoundScTest_TransferDataError)
		{
		iTestSettings&=(~KSoundScTest_TransferDataError);
		aTransferResult=KErrTimedOut;
		}
#endif
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::PlayCallback(ID:%xH,Len:%d) - %d",aTransferID,aBytesPlayed,aTransferResult));		
	
	// The PDD has completed transfering a fragment. Find the associated request from its ID
	TBool isNextToComplete;
	TSoundScPlayRequest* req=((TSoundScPlayRequestQueue*)iReqQueue)->Find(aTransferID,isNextToComplete);
	
	// Check if this is a fragment from an earlier request which failed - which we should ignore. This is the case if the request cannot be found 
	// (because it was already completed back to client) or if the request status is already set as 'done'. 
	if (req && req->iTf.iTfState!=TSndScTransfer::ETfDone)
		{
		__ASSERT_DEBUG(req->iTf.iTfState!=TSndScTransfer::ETfNotStarted,Kern::Fault(KSoundLddPanic,__LINE__));
		
		// Update the count of bytes played.
		iBytesTransferred+=aBytesPlayed;
		
		if (aTransferResult!=KErrNone)
			{
			// Transfer failed - immediately mark the request as being complete.
			req->SetFail(aTransferResult);
			}
		else
			req->UpdateProgress(aBytesPlayed);	// Transfer successful so update the progress of the request.
									
		// If we have just played an entire request and the PDD has not signalled it ahead of any earlier unfinished ones then complete it back to client.
		if (req->iTf.iTfState==TSndScTransfer::ETfDone && isNextToComplete)
			CompleteAllDonePlayRequests(req);
		}
	
	// PDD should now have the capacity to accept another transfer so queue as many transfers
	// on it as it can accept.
	StartNextPlayTransfers();
		
	
	return;
	}
	
/**
This function checks whether there are any outstanding play requests. While there are, it breaks these down into
transfers sizes which are compatible with the PDD and then repeatedly attempts to queue these data transfers on the
PDD until it indicates that it can accept no more for the moment.
@post Data transfer may be stopped in the PDD and the operating state of the channel moved back to EConfigured.
*/
void DSoundScLdd::StartNextPlayTransfers()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::StartNextPlayTransfers"));
	
	// Queue as many transfers on the PDD as it can accept.
	TSoundScPlayRequest* req;
	TInt r=KErrNone;
	while (r==KErrNone && (req=((TSoundScPlayRequestQueue*)iReqQueue)->NextRequestForTransfer())!=NULL)
		{
		TInt pos=req->iTf.GetStartOffset();
		TPhysAddr physAddr;
		TInt len=req->iTf.iAudioBuffer->GetFragmentLength(pos,req->iTf.GetNotStartedLen(),physAddr);
		if (len>0)
			{
			r=Pdd()->TransferData(req->iTf.iId,(iBufManager->iChunkBase+pos),physAddr,len);
			__KTRACE_OPT(KSOUND1, Kern::Printf("<PDD:TransferData(off:%x len:%d) - %d",pos,len,r));
			if (r==KErrNone)
				req->iTf.SetStarted(len);	// Successfully queued a transfer - update the request status.
			else if (r!=KErrNotReady)
				{
				// Transfer error from PDD, fail the request straight away. (Might not be the one at the head of queue).	
				CompletePlayRequest(req,r);
				}	
			}
		else
			{
			// This can only be a zero length play request - just complete it straight away
			CompletePlayRequest(req,KErrNone);	
			}
		} 
	return;	
	}

/**
Complete a client play request back to the client and remove it from the request queue.
@param aReq A pointer to the play request object to be completed.
@post Data transfer may be stopped in the PDD and the operating state of the channel moved back to EConfigured.
*/	
void DSoundScLdd::DoCompletePlayRequest(TSoundScPlayRequest* aReq)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::DoCompletePlayRequest(%x) - %d",aReq,aReq->iCompletionReason));
	
	iReqQueue->Remove(aReq);
	
	// If the request queue is now empty then turn off the codec
	if (iReqQueue->IsEmpty())
		{
#ifdef USE_PLAY_EOF_TIMER
		StartPlayEofTimer();
#else
		Pdd()->StopTransfer();
		iState=EConfigured;
#endif						
		// This is an underflow situation.
		if (aReq->iCompletionReason==KErrNone && aReq->iFlags!=KSndFlagLastSample)
			aReq->iCompletionReason=KErrUnderflow;
		}
		
	CompleteRequest(aReq->iOwningThread,NULL,aReq->iCompletionReason,aReq->iClientRequest);
	iReqQueue->Free(aReq);
	return;	
	}

/**
Complete one or more play requests. This function completes the play request specified. It also completes any other play 
requests which immediately follow the one specified in the play request queue and for which transfer has been completed by the PDD.
@param aReq A pointer to the play request object to be completed.
@post Data transfer may be stopped in the PDD and the operating state of the channel moved back to EConfigured.
*/		
void DSoundScLdd::CompleteAllDonePlayRequests(TSoundScPlayRequest* aReq)
	{
	TSoundScPlayRequest* nextReq=aReq;
	TSoundScPlayRequest* req;
	do 
		{
		req=nextReq;
		nextReq=(TSoundScPlayRequest*)req->iNext;
		DoCompletePlayRequest(req);
		}
	while (!iReqQueue->IsAnchor(nextReq) && nextReq->iTf.iTfState==TSndScTransfer::ETfDone);
	return;	
	}	
		
/**
Start the audio device recording data.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DSoundScLdd::StartRecord()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::StartRecord"));
		
	// Reset all the audio buffer lists 
	NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
	((DRecordBufferManager*)iBufManager)->Reset();
	NKern::FMSignal(&iMutex); 		// Release the buffer/request list mutex.
	
	// Reset the transfer status for the current and pending record buffers.
	TAudioBuffer* buf=((DRecordBufferManager*)iBufManager)->GetCurrentRecordBuffer();
	iCurrentRecBufTf.Init((TUint)buf,buf->iChunkOffset,buf->iSize,buf);		// Use pointer to record buffer as unique ID
	buf=((DRecordBufferManager*)iBufManager)->GetNextRecordBuffer();
	iNextRecBufTf.Init((TUint)buf,buf->iChunkOffset,buf->iSize,buf);		// Use pointer to record buffer as unique ID
	
	// Call the PDD to prepare the hardware for recording.
	TInt r=Pdd()->StartTransfer();
	
	// Test settings - only possible in debug mode. Test handling of an error returned from the PDD for StartTransfer().
#ifdef _DEBUG	
	if (iTestSettings & KSoundScTest_StartTransferError)
		{
		iTestSettings&=(~KSoundScTest_StartTransferError);
		r=KErrTimedOut;
		}
#endif		
	
	// Initiate data transfer into the first record buffer(s).	
	if (r==KErrNone)
		r=StartNextRecordTransfers();
	return(r);	
	}
	
/**
Handle a record request from the client once data transfer has been intiated.
@param aStatus The request status to be signalled when the record request is complete. If the request is successful
   then this is set to the offset within the shared chunk where the record data resides. Alternatively, if an error 
   occurs, it will be set to one of the system wide error values.
@param aLengthPtr A pointer to a TInt object in client memory. On completion, the number of bytes successfully
   recorded are written to this object.
@param aThread The client thread which issued the request and which supplied the request status.    	
@return KErrNone if successful;
		KErrInUse: if the client needs to free up record buffers before further record requests can be accepted;
		KErrCancel: if the driver is in paused mode and there are no complete or partially full buffers to return.
        otherwise one of the other system-wide error codes.
*/
TInt DSoundScLdd::RecordData(TRequestStatus* aStatus,TInt* aLengthPtr,DThread* aThread)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd:RecordData"));
	
	TInt r=KErrNone;

	NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
	 
	// Check if we have had an overflow since the last record request was completed.
	if (((DRecordBufferManager*)iBufManager)->iBufOverflow)
		{
		((DRecordBufferManager*)iBufManager)->iBufOverflow=EFalse;
		NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.	
		return(KErrOverflow);	
		}
	
	// See if there is a buffer already available.
	TAudioBuffer* buf=((DRecordBufferManager*)iBufManager)->GetBufferForClient();
	if (buf)
		{
		// There is an buffer available already - complete the request returning the offset of the buffer to the client.
		NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
		
		r=buf->iResult;

		if (r==KErrNone)
			{
			kumemput(aLengthPtr,&buf->iBytesAdded,sizeof(TInt));
			// Only complete if successful here. Errors will be completed on returning from this method.
			CompleteRequest(aThread,aStatus,(buf->iChunkOffset));
			}
		return(r);	
		}
	
	// If we are paused and there was no un-read data to return to the client then return KErrCancel to prompt them to resume.
	if (iState==EPaused)
		{
		NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
		return(KErrCancel);
		}			
		
	// The buffer 'completed' list is empty. If the buffer 'free' list is empty too then the client needs
	// to free some buffers up - return an error.
	if (((DRecordBufferManager*)iBufManager)->iFreeBufferQ.IsEmpty())
		{
		NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.	
		return(KErrInUse);	
		}	
	
	// Acquire a new request object and add it to the queue of pending requests. The request will be completed
	// from the PDD and the DFC thread when a buffer is available.
	NKern::FMSignal(&iMutex);
	TSoundScRequest* req=iReqQueue->NextFree();
	NKern::FMWait(&iMutex);
	if (req)
		{
		r=req->iClientRequest->SetStatus(aStatus);
		req->iOwningThread=aThread;
		((TClientDataRequest<TInt>*)req->iClientRequest)->SetDestPtr((TInt*)aLengthPtr);
		// Add the request to the queue
		iReqQueue->Add(req);		
		}
	else
		r=KErrGeneral;				// Must have exceeded KMaxSndScRequestsPending.
	NKern::FMSignal(&iMutex); 		// Release the buffer/request list mutex.
	
	return(r);
	}

/**
Release a buffer which was being used by client.
@param aChunkOffset The chunk offset corresponding to the buffer to be freed.
@return KErrNone if successful;
		KErrNotFound if no 'in use' buffer had the specified chunk offset.
		KErrNotReady if the channel is not configured (either for audio or its buffer config).
*/
TInt DSoundScLdd::ReleaseBuffer(TInt aChunkOffset)	
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::ReleaseBuffer(%x)",aChunkOffset));

	TInt r=KErrNotReady;
	if (iState!=EOpen && iBufManager)
		{
		TAudioBuffer* buf=NULL;
		NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
		buf=((DRecordBufferManager*)iBufManager)->ReleaseBuffer(aChunkOffset);
		NKern::FMSignal(&iMutex); 		// Release the buffer/request list mutex.	
		if (buf)
			{
			buf->Flush(DBufferManager::EFlushBeforeDmaRead);
			r=KErrNone;
			}
		else
			r=KErrNotFound;
		}
	return(r);
	}
	
/**
Handle a custom configuration request.
@param aFunction A number identifying the request.
@param aParam A 32-bit value passed to the driver. Its meaning depends on the request.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/	
TInt DSoundScLdd::CustomConfig(TInt aFunction,TAny* aParam)
	{
	
	TInt r;
	if (aFunction>KSndCustomConfigMaxReserved)
		r=Pdd()->CustomConfig(aFunction,aParam);
	else
		{
		r=KErrNotSupported;
#ifdef _DEBUG		
		switch (aFunction)
			{
			case KSndCustom_ForceHwConfigNotifSupported:
				iCaps.iHwConfigNotificationSupport=ETrue;
				r=KErrNone;
				break;
			case KSndCustom_CompleteChangeOfHwConfig:
				NotifyChangeOfHwConfigCallback((TBool)aParam);
				r=KErrNone;
				break;
			case KSndCustom_ForceStartTransferError:
				iTestSettings|=KSoundScTest_StartTransferError;
				r=KErrNone;
				break;
			case KSndCustom_ForceTransferDataError:
				iTestSettings|=KSoundScTest_TransferDataError;
				r=KErrNone;
				break;
			case KSndCustom_ForceTransferTimeout:
				iTestSettings|=KSoundScTest_TransferTimeout;
				r=KErrNone;
				break;
			}
#endif	
		}
	return(r);	
	}
	
/**
@publishedPartner
@prototype
 
Called from the PDD each time it has completed a data transfer into the current record buffer. 
The function performed here is to check whether the transfer into the current buffer is now complete. Also to queue
further requests on the PDD which should now have the capability to accept more transfers. If transfer into the
current buffer is now complete then we need to update the buffer lists and possibly complete a request back the client.
While recording hasn't been paused and no error has occured then this completed buffer ought to be full. However, when
recording has just been paused, the PDD can also call this function to complete a partially filled record buffer. In fact
in some circumstances, pausing may result in the PDD calling this function where it turns out that no data has been
recorded into this buffer. In this case we don't want to signal a null transfer back to the client.
@param aTransferID A value provided by the LDD when it initiated the transfer allowing the transfer fragment to be 
	uniquely identified.
@param aTransferResult The result of the transfer being completed: KErrNone if successful, otherwise one of the other
	system wide error codes.
@param aBytesRecorded The number of bytes recorded into the record buffer.	
*/
void DSoundScLdd::RecordCallback(TUint aTransferID,TInt aTransferResult,TInt aBytesRecorded)
	{
#ifdef _DEBUG
#ifdef TEST_WITH_PAGING_CACHE_FLUSHES
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
#endif
#endif

#ifdef _DEBUG	
	// Test settings - only possible in debug mode.
	if (iTestSettings & KSoundScTest_TransferDataError)
		{
		iTestSettings&=(~KSoundScTest_TransferDataError);
		aTransferResult=KErrTimedOut;
		}
#endif
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::RecordCallback(ID:%xH,Len:%d) - %d (iCurrentRecBufTf.iTfState %d)",aTransferID,aBytesRecorded,aTransferResult, iCurrentRecBufTf.iTfState));
	
	// If the transfer fragment is not for the current record buffer and were not paused then ignore it. Either the PDD
	// has got very confused or more likely its a trailing fragment from an earlier buffer we have already failed. If 
	// we're paused, the PDD doesn't need to bother with a transfer ID, we assume its for the current buffer.
	if (iCurrentRecBufTf.iTfState != TSndScTransfer::ETfDone &&
		(aTransferID==iCurrentRecBufTf.iId || (aTransferID == 0 && iState==EPaused)))
		{
		// Update the count of bytes recorded.
		iBytesTransferred+=aBytesRecorded;
		
		// Update the transfer status of the current buffer.
		if (aTransferResult!=KErrNone)
			{
			// Transfer failed. Mark the buffer as being complete.
			iCurrentRecBufTf.iTfState=TSndScTransfer::ETfDone;	
			}
		else	
			iCurrentRecBufTf.SetCompleted(aBytesRecorded); // Transfer successful so update the progress.
		
		// Check if this is the PDD completing a fragment due to record being paused. In this situation we only allow the
		// PDD to complete one fragment.	
		TAudioBuffer* buf;
		if (iState==EPaused && ++iCompletesWhilePausedCount<2)
			{
			// Complete (i.e. abort) the transfer to the current buffer.
			iCurrentRecBufTf.iTfState=TSndScTransfer::ETfDone; 
			
			// Reset the transfer status for the pending record buffer. This will be switched to the current buffer later
			// in this function - ready for when record is resumed.
			buf=((DRecordBufferManager*)iBufManager)->GetNextRecordBuffer();
			iNextRecBufTf.Init((TUint)buf,buf->iChunkOffset,buf->iSize,buf);		// Use pointer to record buffer as unique ID
			}	
		
		// Check if we have just completed the transfer into the current buffer.	
		if (iCurrentRecBufTf.iTfState==TSndScTransfer::ETfDone)
			HandleCurrentRecordBufferDone(aTransferResult);
		}
			
	// If we're not paused then the PDD should now have the capacity to accept another transfer so queue as many
	// transfers on it as it can accept.
	if (iState==EActive)
		{
#ifdef _DEBUG	
		// Test settings - only possible in debug mode. Test LDD being slow servicing transfer completes from PDD. Disabled.
/*		if (iTestSettings & KSoundScTest_TransferTimeout)
			{
			iTestSettings&=(~KSoundScTest_TransferTimeout);
			Kern::NanoWait(500000000); // Pause for 0.5 second
			} */
#endif
		TInt r=StartNextRecordTransfers();
		if (r!=KErrNone)
			{
			// Problem starting the next transfer. That's fairly serious so complete all pending record requests and 
			// stop recording.
			Pdd()->StopTransfer();
			iReqQueue->CompleteAll(r,&iMutex);
			iState=EConfigured;
			}
		}
	return;
	}
	
/** Perform the necessary processing required when transfer into the current buffer is complete. This involves updating
the buffer lists and possibly complete a request back the client.
@param aTransferResult The result of the transfer being completed: KErrNone if successful, otherwise one of the other
	system wide error codes.
*/
void DSoundScLdd::HandleCurrentRecordBufferDone(TInt aTransferResult)
	{
	TAudioBuffer* buf;
	
	// Flush the buffer before acquiring the mutex.
	buf=((DRecordBufferManager*)iBufManager)->GetCurrentRecordBuffer();
	buf->Flush(DBufferManager::EFlushAfterDmaRead);
	
	NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
	
	// Update the buffer list (by either adding the current buffer to the completed list or the free list).
	TInt bytesRecorded=iCurrentRecBufTf.GetLengthTransferred();
	((DRecordBufferManager*)iBufManager)->SetBufferFilled(bytesRecorded,aTransferResult);
    
    // The pending buffer now becomes the current one and we need to get a new pending one.
    iCurrentRecBufTf=iNextRecBufTf;
    buf=((DRecordBufferManager*)iBufManager)->GetNextRecordBuffer();
    iNextRecBufTf.Init((TUint)buf,buf->iChunkOffset,buf->iSize,buf);	// Use pointer to record buffer as unique ID
    
    // Check if there is a client record request pending.
    if (!iReqQueue->IsEmpty())
    	{
    	// A record request is pending. Check if we have had an overflow since the last record request was completed.
    	if (((DRecordBufferManager*)iBufManager)->iBufOverflow)
    		{
    		TSoundScRequest* req=iReqQueue->Remove();
    		DThread* thread=req->iOwningThread;					// Take a copy before we free it.
			TClientRequest* clreq = req->iClientRequest;		// Take a copy before we free it.
    		((DRecordBufferManager*)iBufManager)->iBufOverflow=EFalse;
			NKern::FMSignal(&iMutex); 							// Release the buffer/request list mutex.
			iReqQueue->Free(req);
    		CompleteRequest(thread,NULL,KErrOverflow,clreq);			// Complete the request.
    		}
    	else
    		{
	    	// Check there really is a buffer available. (There's no guarentee the one just completed hasn't
	    	// immediately been queued again: if the client has too many 'in-use' or the one completed was a NULL
	    	// transfer due to pausing).
			TAudioBuffer* buf=((DRecordBufferManager*)iBufManager)->GetBufferForClient();
			if (buf)
				{
				// There still a buffer available so complete the request.
				TSoundScRequest* req=iReqQueue->Remove();
				DThread* thread=req->iOwningThread;							// Take a copy before we free it.
				TClientRequest* clreq = req->iClientRequest;				// Take a copy before we free it.
				NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
				iReqQueue->Free(req);
				if (buf->iResult==KErrNone)
					{
					((TClientDataRequest<TInt>*)clreq)->Data() = buf->iBytesAdded;
					CompleteRequest(thread,NULL,buf->iChunkOffset,clreq);					// Complete the request.	
					}
				else	
					CompleteRequest(thread,NULL,buf->iResult,clreq);				// Complete the request.
				}
			else
				NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.	
    		}
    	}
    else
    	NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.	
	}
	
/**
This function starts the next record data transfer. It starts with the current record buffer - checking whether all of
this has now been transferred or queued for transfer. If not it breaks this down into transfers sizes which are
compatible with the PDD and then repeatedly attempts to queue these on the PDD until the PDF indicates that it can
accept no more transfers for the moment. If the record buffer is fully started in this way and the PDD still has the
capacity to accept more transfers then it moves on to start the pending record buffer.
@return Normally KErrNone unless the PDD incurs an error while attempting to start a new transfer.
*/
TInt DSoundScLdd::StartNextRecordTransfers()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::StartNextRecordTransfers"));
	
	// First start with the current record buffer - keep queuing transfers either until this buffer is
	// fully started or until the PDD can accept no more transfers.
	TInt r=KErrNone;
	while (r==KErrNone && iCurrentRecBufTf.iTfState<TSndScTransfer::ETfFullyStarted)
		{
		TInt pos=iCurrentRecBufTf.GetStartOffset();
		TPhysAddr physAddr;
		TInt len=iCurrentRecBufTf.iAudioBuffer->GetFragmentLength(pos,iCurrentRecBufTf.GetNotStartedLen(),physAddr);
		
		r=Pdd()->TransferData(iCurrentRecBufTf.iId,(iBufManager->iChunkBase+pos),physAddr,len);
		__KTRACE_OPT(KSOUND1, Kern::Printf("<PDD:TransferData(off:%x len:%d) A - %d",pos,len,r));
		if (r==KErrNone)
			iCurrentRecBufTf.SetStarted(len);	// Successfully queued a transfer - update the status.
		}
	
	// Either the current record transfer is now fully started, or the PDD can accept no more transfers
	// If the PDD can still accept more transfers then move on to the next record buffer - again, keep queuing
	// transfers either until this buffer is fully started or until the PDD can accept no more.
	while (r==KErrNone && iNextRecBufTf.iTfState<TSndScTransfer::ETfFullyStarted)
		{
		TInt pos=iNextRecBufTf.GetStartOffset();
		TPhysAddr physAddr;
		TInt len=iNextRecBufTf.iAudioBuffer->GetFragmentLength(pos,iNextRecBufTf.GetNotStartedLen(),physAddr);
		
		r=Pdd()->TransferData(iNextRecBufTf.iId,(iBufManager->iChunkBase+pos),physAddr,len);
		__KTRACE_OPT(KSOUND1, Kern::Printf("<PDD:TransferData(off:%x len:%d) B - %d",pos,len,r));
		if (r==KErrNone)
			iNextRecBufTf.SetStarted(len);	// Successfully queued a transfer - update the status.
		}
	if (r==KErrNotReady)
		r=KErrNone;		// KErrNotReady means the PDD the cannot accept any more requests - this isn't an error.	
	return(r);	
	}	
	
/**
@publishedPartner
@prototype

Called from the PDD each time it detects a change in the hardware configuration of the device.
@param aHeadsetPresent This is set by the PDD to ETrue if a microphone or headset socket is now present or EFalse if 
such a device is not present.
*/
void DSoundScLdd::NotifyChangeOfHwConfigCallback(TBool aHeadsetPresent)
	{
#ifdef _DEBUG
#ifdef TEST_WITH_PAGING_CACHE_FLUSHES 
	Kern::HalFunction(EHalGroupVM,EVMHalFlushCache,0,0);
#endif
#endif

	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::NotifyChangeOfHwConfigCallback(Pres:%d)",aHeadsetPresent));
	
	__ASSERT_DEBUG(iCaps.iHwConfigNotificationSupport,Kern::Fault(KSoundLddPanic,__LINE__));		
	
	if (iNotifyChangeOfHwClientRequest->IsReady())
		{
		iNotifyChangeOfHwClientRequest->Data() = aHeadsetPresent;
		CompleteRequest(iChangeOfHwConfigThread,NULL,KErrNone,iNotifyChangeOfHwClientRequest);	// Complete the request.
		}
	}
	
/**
This function validates that a new sound format configuration is both sensible and supported by this device.
@param aConfig A reference to the new sound format configuration object.
*/	
TInt DSoundScLdd::ValidateConfig(const TCurrentSoundFormatV02& aConfig)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScLdd::ValidateConfig"));
	
	// Check that the audio channel configuration requested is sensible and supported by this device.
	if (aConfig.iChannels<0) 
		return(KErrNotSupported);
	TInt chans=(aConfig.iChannels-1);
	if (!(iCaps.iChannels & (1<<chans)))
		return(KErrNotSupported);
	
	// Check that the sample rate requested is sensible and supported by this device.
	if (aConfig.iRate<0 || !(iCaps.iRates & (1<<aConfig.iRate)))
		return(KErrNotSupported);
	
	// Check that the encoding format requested is sensible and supported by this device.
	if (aConfig.iEncoding<0 || !(iCaps.iEncodings & (1<<aConfig.iEncoding)))
		return(KErrNotSupported);
	
	// Check that the data format requested is sensible and supported by this device.
	if (aConfig.iDataFormat<0 || !(iCaps.iDataFormats & (1<<aConfig.iDataFormat)))
		return(KErrNotSupported);
		
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DSoundScLdd::ValidateConfig - %d",KErrNone));
	return(KErrNone);
	}
	
/**
Increase or decrease the memory area allocated to hold the current buffer configuration in the play/record chunk.
@param aNumBuffers The number of buffers within the new buffer configuration. This determines the size of the memory
	area required.
@pre The thread must be in a critical section. 
*/
TInt DSoundScLdd::ReAllocBufferConfigInfo(TInt aNumBuffers)
	{
	if (iBufConfig)
		{
		delete iBufConfig;
		iBufConfig=NULL;
		}	 
	
	iBufConfigSize=aNumBuffers*sizeof(TInt);	
	iBufConfigSize+=sizeof(TSharedChunkBufConfigBase);
	iBufConfig=(TSoundSharedChunkBufConfig*)Kern::AllocZ(iBufConfigSize);
	if (!iBufConfig)
		return(KErrNoMemory);	
	
	return(KErrNone);
	}

/**
Start the EOF play timer. A 2 second timer is queued each time the transfer of playback data ceases by the LLD and if it
is allowed to expire, then the PDD is called to release any resources in use for playback transfer. 
*/
void DSoundScLdd::StartPlayEofTimer()
	{
	iEofTimer.Cancel();
	iPlayEofDfc.Cancel();
	iEofTimer.OneShot(NKern::TimerTicks(2000)); // Queue the 2 second EOF timer to stop transfer on the PDD.
	iPlayEofTimerActive=ETrue;
	}

/**
Cancel the EOF play timer. 
*/	
void DSoundScLdd::CancelPlayEofTimer()
	{
	iEofTimer.Cancel();
	iPlayEofDfc.Cancel();
	iPlayEofTimerActive=EFalse;
	}
	
/**
@publishedPartner
@prototype

Returns the buffer configuration of the play/record chunk.
@return A pointer to the current buffer configuration of the play/record chunk.
*/	
TSoundSharedChunkBufConfig* DSoundScLdd::BufConfig()
	{
	return(iBufConfig);
	}

/**
@publishedPartner
@prototype

Returns the address of the start of the play/record chunk.
@return The linear address of the start of the play/record chunk.
*/	
TLinAddr DSoundScLdd::ChunkBase()
	{
	return(iBufManager->iChunkBase);
	}			
		
/**
The ISR to handle the EOF play timer.
@param aChannel A pointer to the sound driver logical channel object.
*/	
void DSoundScLdd::PlayEofTimerExpired(TAny* aChannel)
	{
	DSoundScLdd& drv=*(DSoundScLdd*)aChannel;
	
	drv.iPlayEofDfc.Add();
	}
	
/**
The DFC used to handle the EOF play timer. 
@param aChannel A pointer to the sound driver logical channel object.
*/	
void DSoundScLdd::PlayEofTimerDfc(TAny* aChannel)
	{	
	DSoundScLdd& drv=*(DSoundScLdd*)aChannel;
	
	drv.Pdd()->StopTransfer();
	drv.iState=EConfigured;
	drv.iPlayEofTimerActive=EFalse;
	}				
	
/**
The DFC used to handle power down requests from the power manager before a transition into system
shutdown/standby.
@param aChannel A pointer to the sound driver logical channel object.
*/
void DSoundScLdd::PowerDownDfc(TAny* aChannel)
	{
	DSoundScLdd& drv=*(DSoundScLdd*)aChannel;
	drv.Shutdown();
	drv.iPowerHandler->PowerDownDone();
	}
	
/**
The DFC used to handle power up requests from the power manager following a transition out of system standby.
@param aChannel A pointer to the sound driver logical channel object.
*/	
void DSoundScLdd::PowerUpDfc(TAny* aChannel)
	{
	DSoundScLdd& drv=*(DSoundScLdd*)aChannel;
	
	// Restore the channel to a default state.
	drv.DoCancel(RSoundSc::EAllRequests);
	drv.Pdd()->PowerUp();
	drv.DoSetSoundConfig(drv.iSoundConfig);
	drv.SetVolume(drv.iVolume);			
	drv.iState=(!drv.iBufConfig)?EOpen:EConfigured;
	
	drv.iPowerHandler->PowerUpDone();
	}	

/** 
Complete an asynchronous request back to the client.
@param aThread The client thread which issued the request.
@param aStatus The TRequestStatus instance that will receive the request status code or NULL if aClientRequest used. 
@param aReason The request status code.  
@param aClientRequest The TClientRequest instance that will receive the request status code or NULL if aStatus used. 
@pre The thread must be in a critical section. 
*/

void DSoundScLdd::CompleteRequest(DThread* aThread, TRequestStatus* aStatus, TInt aReason, TClientRequest* aClientRequest)
	{
	if (aClientRequest)
		{
		if (aClientRequest->IsReady())
			{
			Kern::QueueRequestComplete(aThread,aClientRequest,aReason);
			}
		else
			{
			// should always be ready
			__ASSERT_DEBUG(EFalse,Kern::Fault(KSoundLddPanic,__LINE__));
			}
		}
	else if (aStatus)
		{
		Kern::RequestComplete(aStatus,aReason);		// Complete the request back to the client.
		}
	else
		{
		// never get here - either aStatus or aClientRequest must be valid
		__ASSERT_DEBUG(EFalse,Kern::Fault(KSoundLddPanic,__LINE__));
		}
	
	aThread->AsyncClose();	// Asynchronously close our reference on the client thread - don't want to be blocked if this is final reference. 
	
#ifdef _DEBUG	
	__e32_atomic_add_ord32(&iThreadOpenCount, TUint32(-1));
#endif		
	}

/**
Constructor for the play request object.
*/
TSoundScPlayRequest::TSoundScPlayRequest()
	: TSoundScRequest()
	{
	iFlags=0; 
	iCompletionReason=KErrGeneral;
	}

/*
Second phase construction of the requests
*/
TInt TSoundScPlayRequest::Construct()
	{
	return Kern::CreateClientRequest(iClientRequest);
	}

/*
Second phase construction of the requests
*/
TInt TSoundScRequest::Construct()
	{
	TClientDataRequest<TInt>* tempClientDataRequest=0;
	TInt r = Kern::CreateClientDataRequest(tempClientDataRequest);
	iClientRequest = tempClientDataRequest;
	return r;
	}

/**
Destructor of play requests
*/
TSoundScRequest::~TSoundScRequest()
	{
	Kern::DestroyClientRequest(iClientRequest);
	}

/**
Constructor for the request object queue.
*/
TSoundScRequestQueue::TSoundScRequestQueue(DSoundScLdd* aLdd)
	{
	iLdd=aLdd;
	memclr(&iRequest[0],sizeof(TSoundScRequest*)*KMaxSndScRequestsPending);
	}
	
/**
Destructor for the request object queue.
*/
TSoundScRequestQueue::~TSoundScRequestQueue()
	{
	for (TInt i=0 ; i<KMaxSndScRequestsPending ; i++)
		{
		delete iRequest[i];
		}
	}
	
/**
Second stage constructor for the basic request object queue.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt TSoundScRequestQueue::Create()	
	{
	// Create the set of available request objects and add them to the unused request queue. 	
	for (TInt i=0 ; i<KMaxSndScRequestsPending ; i++)
		{
		iRequest[i]=new TSoundScRequest;		// Normal request object
		if (!iRequest[i])
			return(KErrNoMemory);
		TInt retConstruct = iRequest[i]->Construct();
		if ( retConstruct != KErrNone)
			{
			return(retConstruct);
			}
		iUnusedRequestQ.Add(iRequest[i]);
		}
		
	return(KErrNone);
	}
		
/**
Second stage constructor for the play request object queue.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt TSoundScPlayRequestQueue::Create()	
	{
	// Create the set of available play request objects and add them to the unused request queue. 	
	for (TInt i=0 ; i<KMaxSndScRequestsPending ; i++)
		{
		iRequest[i]=new TSoundScPlayRequest();
		if (!iRequest[i])
			return(KErrNoMemory);
		TInt retConstruct = iRequest[i]->Construct();
		if ( retConstruct != KErrNone)
			{
			return(retConstruct);
			}
		iUnusedRequestQ.Add(iRequest[i]);
		}
		
	return(KErrNone);
	}	

/**
Get an unused request object.
@return A pointer to a free request object or NULL if there are none available.
*/ 	
TSoundScRequest* TSoundScRequestQueue::NextFree()
	{
	NKern::FMWait(&iUnusedRequestQLock);
	TSoundScRequest* req = (TSoundScRequest*)iUnusedRequestQ.GetFirst();
	NKern::FMSignal(&iUnusedRequestQLock);
	return req;
	}
		
/**
Add a request object to the tail of the pending request queue. 
@param aReq A pointer to the request object to be added to the queue.
*/ 
void TSoundScRequestQueue::Add(TSoundScRequest* aReq)	
	{
	iPendRequestQ.Add(aReq);
	} 	

/**
If the pending request queue is not empty, remove the request object from the head of this queue.
@return A pointer to request object removed or NULL if the list was empty.
*/ 
TSoundScRequest* TSoundScRequestQueue::Remove()
	{
	return((TSoundScRequest*)iPendRequestQ.GetFirst());
	}
						
/**
Remove a request object from anywhere in the pending request queue. 
@param aReq A pointer to the request object to be removed from the queue.
@return A pointer to request object removed or NULL if it wasn't found.
*/	
TSoundScRequest* TSoundScRequestQueue::Remove(TSoundScRequest* aReq)	
	{
	TSoundScRequest* retReq;
	
	// Scan through the pending queue looking for a request object which matches.
	retReq=(TSoundScRequest*)iPendRequestQ.First();
	while (!IsAnchor(retReq) && retReq!=aReq)
		retReq=(TSoundScRequest*)retReq->iNext;
	
	// If we got a match then remove the request object from the queue and return it.
	if (!IsAnchor(retReq))
		retReq->Deque();
	else
		retReq=NULL;
	return(retReq);
	}

/**
Free up a request object - making it available for further requests. 
@param aReq A pointer to the request object being freed up.
*/ 
void TSoundScRequestQueue::Free(TSoundScRequest* aReq)	
	{
	NKern::FMWait(&iUnusedRequestQLock);
	iUnusedRequestQ.Add(aReq);
	NKern::FMSignal(&iUnusedRequestQLock);
	}
	 
/**
Find a request object (specified by its associated request status pointer) within in the pending request queue. 
@param aStatus The request status pointer of the request object to be found in the queue.
@return A pointer to the request object if it was found or NULL if it wasn't found.
*/	
TSoundScRequest* TSoundScRequestQueue::Find(TRequestStatus* aStatus)	
	{
	TSoundScRequest* retReq;
	
	// Scan through the queue looking for a request object containing a TRequestStatus* which matches.
	retReq=(TSoundScRequest*)iPendRequestQ.First();
	while (!IsAnchor(retReq) && retReq->iClientRequest->StatusPtr()!=aStatus)
		retReq=(TSoundScRequest*)retReq->iNext;
		
	return((IsAnchor(retReq))?NULL:retReq);
	}				
	
/**
Remove each request object from the pending request queue, completing each request removed with a specified completion
reason.
@param aCompletionReason The error value to be returned when completing any requests in the queue.
@param aMutex A pointer to a mutex to be aquired when removing requests from the queue. May be NULL.
*/		
void TSoundScRequestQueue::CompleteAll(TInt aCompletionReason,NFastMutex* aMutex)	
	{
	if (aMutex)
		NKern::FMWait(aMutex); 							// Acquire the mutex.
	
	TSoundScRequest* req;
	while ((req=Remove())!=NULL)
		{
		if (aMutex)
			NKern::FMSignal(aMutex); 					// Release the mutex while we complete the request.
		iLdd->CompleteRequest(req->iOwningThread,NULL,aCompletionReason,req->iClientRequest);	
		Free(req);	
		if (aMutex)
			NKern::FMWait(aMutex); 						// Re-acquire the mutex.

		}
		
	if (aMutex)	
		NKern::FMSignal(aMutex); 						// Release mutex.	
	}

/**
Constructor for the play request object queue.
*/					
TSoundScPlayRequestQueue::TSoundScPlayRequestQueue(DSoundScLdd* aLdd)
	: TSoundScRequestQueue(aLdd)
{
}

/**
Return the play request object from the request queue which is next to be transferrred. If this
play request is being handled using multiple data transfers then the transfer of earlier parts of
this request may already be in progress. 
@return Either a pointer to the next play request object for transfer, or NULL if no more are pending. 	
*/
TSoundScPlayRequest* TSoundScPlayRequestQueue::NextRequestForTransfer()
	{
	TSoundScPlayRequest* retReq;
	
	retReq=(TSoundScPlayRequest*)iPendRequestQ.First();
	while (!IsAnchor(retReq) && retReq->iTf.iTfState>TSndScTransfer::ETfPartlyStarted)
		retReq=(TSoundScPlayRequest*)retReq->iNext;
		
	return((IsAnchor(retReq))?NULL:retReq);	
	}	

/**
Search the play request queue for a particular play request object specified by its transfer ID.
@param aTransferID The transfer ID of the particular play request object to be found.
@param aIsNextToComplete If the search is successful then this indicates whether the request 
object found is the next in the queue to be completed to the client. ETrue if next to be 
completed, EFalse otherwise.
@return Either a pointer to the specified request object, or NULL if it was not found.
*/	
TSoundScPlayRequest* TSoundScPlayRequestQueue::Find(TUint aTransferID,TBool& aIsNextToComplete)
	{
	TSoundScPlayRequest* retReq;
	TSoundScPlayRequest* nextToCompleteReq=NULL;
	
	retReq=(TSoundScPlayRequest*)iPendRequestQ.First();
	
	// Walk all the way through the list either until we find the specified object or until we get to the end
	for ( ; !IsAnchor(retReq) ; retReq=(TSoundScPlayRequest*)retReq->iNext )
		{
		// The first request we find which isn't complete must be the next to complete
		if (!nextToCompleteReq && retReq->iTf.iTfState!=TSndScTransfer::ETfDone)
			nextToCompleteReq=retReq;
		if (retReq->iTf.iId==aTransferID)
			break;
		}
	
	if (IsAnchor(retReq))
		return(NULL);		// Object not found
	else
		{
		aIsNextToComplete=(retReq==nextToCompleteReq);
		return(retReq);		// Object found
		}
	}					
/**
Constructor for the audio data transfer class. 
*/	
TSndScTransfer::TSndScTransfer()
	{
	iId=0;
	iTfState=ETfNotStarted;
	iAudioBuffer=NULL;
	iLengthTransferred=0;
	iTransfersInProgress=0;
	}
	
/**
Initialisation function for the audio data transfer class.
@param aId A value to uniquely identify this particular transfer.
@param aChunkOffset The start postition of the transfer - an offset within the shared chunk.
@param aLength The total length of the transfer in bytes.
@param anAudioBuffer The audio buffer associated with the transfer.
*/		
void TSndScTransfer::Init(TUint aId,TInt aChunkOffset,TInt aLength,TAudioBuffer* anAudioBuffer)
	{
	iId=aId;
	iTfState=ETfNotStarted;
	iAudioBuffer=anAudioBuffer;
	iStartedOffset=aChunkOffset;
	iEndOffset=aChunkOffset+aLength;
	iLengthTransferred=0;
	iTransfersInProgress=0;
	}
	
/**
Update the progress of the audio data transfer with the amount of data now queued for transfer on the audio device.
@param aLength The amount of data (in bytes) that has just been queued on the audio device.
*/
void TSndScTransfer::SetStarted(TInt aLength)
	{	
	iTransfersInProgress++;
	iStartedOffset+=aLength;
	TInt notqueued=(iEndOffset - iStartedOffset);
	__ASSERT_ALWAYS(notqueued>=0,Kern::Fault(KSoundLddPanic,__LINE__));
	iTfState=(notqueued) ? ETfPartlyStarted : ETfFullyStarted;
	}
	
/**
Update the progress of the audio data transfer with the amount of data now successfully transfered by the audio device.
@param aLength The amount of data (in bytes) that has just been transferred by the audio device.
@return ETrue if the transfer is now fully complete, otherwise EFalse.
*/
TBool TSndScTransfer::SetCompleted(TInt aLength)
	{		
	iLengthTransferred+=aLength;
	iTransfersInProgress--;
	__ASSERT_ALWAYS(iTransfersInProgress>=0,Kern::Fault(KSoundLddPanic,__LINE__));
	
	if (GetNotStartedLen()==0 && iTransfersInProgress==0)
		{
		iTfState=ETfDone;	// Transfer is now fully completed
		return(ETrue);
		}
	else
		return(EFalse);	
	}
		
/**
Constructor for the sound driver power handler class.
@param aChannel A pointer to the sound driver logical channel which owns this power handler.
*/
DSoundScPowerHandler::DSoundScPowerHandler(DSoundScLdd* aChannel)
:	DPowerHandler(KDevSoundScName),
	iChannel(aChannel)
	{	
	}

/**
A request from the power manager for the power down of the audio device.
This is called during a transition of the phone into standby or power off.
@param aState The target power state; can be EPwStandby or EPwOff only.
*/
void DSoundScPowerHandler::PowerDown(TPowerState aPowerState)
	{
	(void)aPowerState;
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScPowerHandler::PowerDown(State-%d)",aPowerState));
	
	// Power-down involves hardware access so queue a DFC to perform this from the driver thread.
	iChannel->iPowerDownDfc.Enque();
	}

/**
A request from the power manager for the power up of the audio device.
This is called during a transition of the phone out of standby.
*/
void DSoundScPowerHandler::PowerUp()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DSoundScPowerHandler::PowerUp"));
	
	// Power-up involves hardware access so queue a DFC to perform this from the driver thread.
	iChannel->iPowerUpDfc.Enque();
	}	

/**
Constructor for the buffer manager.
*/
DBufferManager::DBufferManager(DSoundScLdd* aLdd)
	: iLdd(aLdd)
	{
//	iChunk=NULL;
//	iNumBuffers=0;
//	iAudioBuffers=NULL;	
//	iMaxTransferLen=0;	
	}

/**
Destructor for the buffer manager.
@pre The thread must be in a critical section.
*/
DBufferManager::~DBufferManager()
	{
	if (iChunk)
		Kern::ChunkClose(iChunk);
	delete[] iAudioBuffers;
	}
		
/**
Second stage constructor for the buffer manager. This version creates a shared chunk and a buffer object for each
buffer specified within this. Then it commits memory within the chunk for each of these buffers. This also involves the
creation of a set of buffer lists to manage the buffers.
@param aBufConfig The shared chunk buffer configuration object specifying the geometry of the buffer configuration
required.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DBufferManager::Create(TSoundSharedChunkBufConfig* aBufConfig)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::Create(Bufs-%d,Sz-%d)",aBufConfig->iNumBuffers,aBufConfig->iBufferSizeInBytes));

	// Create the required number of buffer objects, and the buffer lists to manage these.
	TInt r=CreateBufferLists(aBufConfig->iNumBuffers);
	if (r!=KErrNone)
		return(r);
		
	TInt chunkSz;
	TInt bufferSz=aBufConfig->iBufferSizeInBytes;
	TInt* bufferOffsetList=&aBufConfig->iBufferOffsetListStart;	
	
	// Calculate the required size for the chunk and the buffer offsets. 
	if (aBufConfig->iFlags & KScFlagUseGuardPages)
		{
		// Commit each buffer separately with an uncommitted guard pages around each buffer.
		TInt guardPageSize=Kern::RoundToPageSize(1);
		bufferSz=Kern::RoundToPageSize(aBufConfig->iBufferSizeInBytes); // Commit size to be a multiple of the MMU page size.
		chunkSz=guardPageSize;											// Leave an un-committed guard page at the start.
		for (TInt i=0 ; i<aBufConfig->iNumBuffers ; i++)
			{
			bufferOffsetList[i]=chunkSz;
			chunkSz += (bufferSz + guardPageSize);						// Leave an un-committed guard page after each buffer.
			}
		}
	else
		{
		// Commit all the buffers contiguously into a single region (ie with no guard pages between each buffer).
		chunkSz=0;
		for (TInt i=0 ; i<aBufConfig->iNumBuffers ; i++)
			{
			bufferOffsetList[i]=chunkSz;
			chunkSz += bufferSz;
			}
		chunkSz=Kern::RoundToPageSize(chunkSz);							// Commit size to be a multiple of the MMU page size.
		}	
	aBufConfig->iFlags|=KScFlagBufOffsetListInUse;
	__KTRACE_OPT(KSOUND1, Kern::Printf("Chunk size is %d bytes",chunkSz));	
    
    // Create the shared chunk. The PDD supplies most of the chunk create info - but not the maximum size.
	TChunkCreateInfo info;
	info.iMaxSize=chunkSz;
	iLdd->Pdd()->GetChunkCreateInfo(info);		// Call down to the PDD for the rest.
	
	r = Kern::ChunkCreate(info,iChunk,iChunkBase,iChunkMapAttr);
	__KTRACE_OPT(KSOUND1, Kern::Printf("Create chunk - %d",r));	
	if (r!=KErrNone)
     	return(r);
	
	if (aBufConfig->iFlags & KScFlagUseGuardPages)
		{
		// Map each of the buffers into the chunk separately - try to allocate physically contiguous RAM pages. Create a buffer object for each buffer.
 		TBool isContiguous;
 		for (TInt i=0 ; i<aBufConfig->iNumBuffers ; i++)
			{
			r=CommitMemoryForBuffer(bufferOffsetList[i],bufferSz,isContiguous);
			if (r!=KErrNone)
				return(r);
			r=iAudioBuffers[i].Create(iChunk,bufferOffsetList[i],(aBufConfig->iBufferSizeInBytes),isContiguous,this);
			if (r!=KErrNone)
				return(r);	
			}		
		}
	else
		{
		// Map memory for the all buffers into the chunk - try to allocate physically contiguous RAM pages. 
 		TBool isContiguous;
		r=CommitMemoryForBuffer(0,chunkSz,isContiguous);
		if (r!=KErrNone)
			return(r);
		
		// Create a buffer object for each buffer.
 		for (TInt i=0 ; i<aBufConfig->iNumBuffers ; i++)
			{
			r=iAudioBuffers[i].Create(iChunk,bufferOffsetList[i],(aBufConfig->iBufferSizeInBytes),isContiguous,this);
			if (r!=KErrNone)
				return(r);	
			}	
		}

	// Read back and store the maximum transfer length supported by this device from the PDD.
	iMaxTransferLen=iLdd->Pdd()->MaxTransferLen();
	
    __KTRACE_OPT(KSOUND1, Kern::Printf("<DBufferManager::Create - %d",r));
	return(r);
	}
	
/**
Second stage constructor for the buffer manager. This version opens an existing shared chunk using a client supplied
handle. It then creates a buffer object for each buffer that exists within the chunk as well as creating a set of buffer
lists to manage the buffers.
@param aBufConfig The shared chunk buffer configuration object - specifying the geometry of the buffer configuration
within the shared chunk supplied.
@param aChunkHandle A handle for the shared chunk supplied by the client.
@param anOwningThread The thread in which the given handle is valid.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DBufferManager::Create(TSoundSharedChunkBufConfig& aBufConfig,TInt aChunkHandle,DThread* anOwningThread)
	{	
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::Create(Handle-%d)",aChunkHandle));

	// Validate the buffer configuration information.
	if (!aBufConfig.iFlags&KScFlagBufOffsetListInUse)
		return(KErrArgument);
	TInt numBuffers=aBufConfig.iNumBuffers;
	TInt bufferSizeInBytes=aBufConfig.iBufferSizeInBytes;
	TInt* bufferOffsetList=&aBufConfig.iBufferOffsetListStart;
	TInt r=ValidateBufferOffsets(bufferOffsetList,numBuffers,bufferSizeInBytes);
	if (r<0)
		return(r);
	
	// Create the required number of buffer objects, and the buffer lists to manage these.
	r=CreateBufferLists(numBuffers);
	if (r!=KErrNone)
		return(r);
	
	// Open the shared chunk.
	DChunk* chunk;
	chunk=Kern::OpenSharedChunk(anOwningThread,aChunkHandle,ETrue);
	if (!chunk)
		return(KErrBadHandle);
	iChunk=chunk;
	
	// Read the physical address for the 1st buffer in order to determine the kernel address and the map attributes.
	TInt offset=bufferOffsetList[0];
	TPhysAddr physAddr;
	TLinAddr kernelAddress;
	r=Kern::ChunkPhysicalAddress(iChunk,offset,bufferSizeInBytes,kernelAddress,iChunkMapAttr,physAddr,NULL);
	if (r!=KErrNone)
		return(r);
	iChunkBase=(kernelAddress-offset);
	
	// For each buffer, validate that the buffer specified contains committed memory and store the buffer info. into each buffer object.
	while (numBuffers)
		{
		numBuffers--;
		offset=bufferOffsetList[numBuffers];
		// Assume it isn't contiguous here - Create() will detect and do the right thing if it is contiguous.
		r=iAudioBuffers[numBuffers].Create(iChunk,offset,bufferSizeInBytes,EFalse,this);	
		if (r!=KErrNone)
			return(r);
		}
		
	// Read back and store the maximum transfer length supported by this device from the PDD.
	iMaxTransferLen=iLdd->Pdd()->MaxTransferLen();	
		
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DBufferManager::Create - %d",KErrNone));
	return(KErrNone);
	}

/**
Allocate an array of buffer objects, - one for each buffer contained within the shared chunk.
@param aNumBuffers The number of buffer objects required.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DBufferManager::CreateBufferLists(TInt aNumBuffers)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::CreateBufferLists(Bufs-%d)",aNumBuffers));

	// Construct the array of buffers.
	iNumBuffers=aNumBuffers;
	iAudioBuffers=new TAudioBuffer[aNumBuffers];
	if (!iAudioBuffers)
		return(KErrNoMemory);
	
	return(KErrNone);
	}
	
/**
Validate a shared chunk buffer offset list.
@param aBufferOffsetList The buffer offset list to be validated.
@param aNumBuffers The number of offsets that the list contains.
@param aBufferSizeInBytes The size in bytes of each buffer.
@return If the buffer list is found to be valid, the calculated minimum size of the corresponding chunk is returned
	(i.e. a value>=0). Otherwise, KErrArgument is returned.
*/
TInt DBufferManager::ValidateBufferOffsets(TInt* aBufferOffsetList,TInt aNumBuffers,TInt aBufferSizeInBytes)	
	{
	TUint32 alignmask=(1<<iLdd->iCaps.iRequestAlignment)-1; // iRequestAlignment holds log to base 2 of alignment required
			
	// Verify each of the buffer offsets supplied
	TInt offset=0;
	for (TInt i=0 ; i<aNumBuffers ; i++)
		{
		// If this is a record channel then the offset must comply with the PDD alignment constraints.
		if (iLdd->iDirection==ESoundDirRecord && ((TUint32)aBufferOffsetList[i] & alignmask) != 0)	
			return(KErrArgument);
			
		// Check the offset doesn't overlap the previous buffer - offset holds the offset to next byte after
		// the previous buffer.
		if (aBufferOffsetList[i]<offset)
			return(KErrArgument);
		
		offset=(aBufferOffsetList[i]+aBufferSizeInBytes);
		}
	return(offset);	
	}
	
/**
Verify that a specified region of the shared chunk (specified by its offset and length) is valid within the 
chunk and corresponds to a region of committed memory.
@param aChunkOffset Offset of the region from the beginning of the chunk.
@param aLength The length in bytes of the region.
@param anAudioBuffer A reference to a pointer to an audio buffer object. On return this will either contain a pointer
to the audio buffer object which corresonds to the specified region, or NULL if the region is invalid.
@return KErrNone if the region is valid, otherwise KErrArgument.
*/	
TInt DBufferManager::ValidateRegion(TUint aChunkOffset,TUint aLength,TAudioBuffer*& anAudioBuffer)
	{
	
	TUint bufStart;
	TUint bufEnd;
	TUint regEnd=(aChunkOffset+aLength);
	for (TInt i=0 ; i<iNumBuffers ; i++)
		{
		bufStart=iAudioBuffers[i].iChunkOffset;
		bufEnd=iAudioBuffers[i].iChunkOffset+iAudioBuffers[i].iSize;
		if (aChunkOffset<bufStart || aChunkOffset>=bufEnd)
			continue;
		if (regEnd<=bufEnd)
			{
			anAudioBuffer=&iAudioBuffers[i];
			return(KErrNone);
			}
		}
	return(KErrArgument);
	}
			
/**
Commit memory for a single buffer within the shared chunk.
@param aChunkOffset The offset (in bytes) from start of chunk, which indicates the start of the memory region to be
	committed. Must be a multiple of the MMU page size. 
@param aSize The number of bytes to commit. Must be a multiple of the MMU page size.
@param aIsContiguous On return, this is set to ETrue if the function succeeded in committing physically contiguous memory;
	EFalse if the committed memory is not contiguous.	
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/	
TInt DBufferManager::CommitMemoryForBuffer(TInt aChunkOffset,TInt aSize,TBool& aIsContiguous)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::CommitMemoryForBuffer(Offset-%x,Sz-%d)",aChunkOffset,aSize));
	
	// Try for physically contiguous memory first.
	TInt r;	
	TPhysAddr physicalAddress;
	r=Kern::ChunkCommitContiguous(iChunk,aChunkOffset,aSize,physicalAddress);
	if (r==KErrNone)
		{
		aIsContiguous=ETrue;
		return(r);
		}
			
	// Try to commit memory that isn't contiguous instead.
	aIsContiguous=EFalse;
	r=Kern::ChunkCommit(iChunk,aChunkOffset,aSize);
	return(r);
	}
	
/**
Purge a region of the audio chunk. That is, if this region contains cacheable memory, flush it.
@param aChunkOffset The offset within the chunk for the start of the data to be flushed.
@param aLength The length in bytes of the region to be flushed.
@param aFlushOp The type of flush operation required - @see TFlushOp.
*/
void DBufferManager::FlushData(TInt aChunkOffset,TInt aLength,TFlushOp aFlushOp)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::FlushData(%d)",aFlushOp));
	TLinAddr dataAddr=(iChunkBase+aChunkOffset);
	switch (aFlushOp)
		{
		case EFlushBeforeDmaWrite:
			Cache::SyncMemoryBeforeDmaWrite(dataAddr,aLength,iChunkMapAttr);
			break;
		case EFlushBeforeDmaRead:
			Cache::SyncMemoryBeforeDmaRead(dataAddr,aLength,iChunkMapAttr);
			break;
		case EFlushAfterDmaRead:
			Cache::SyncMemoryAfterDmaRead(dataAddr,aLength);
			break;
		default:
			break;
		}
	}
		
/**
Constructor for the record buffer manager.
*/
DRecordBufferManager::DRecordBufferManager(DSoundScLdd* aLdd)
	: DBufferManager(aLdd)
	{
	}
			
/**
Reset all the audio buffer lists to reflect the state at the start of the record capture process.
@pre The buffer/request queue mutex must be held.
*/
void DRecordBufferManager::Reset()
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::Reset"));
	TAudioBuffer* pBuf;
	
	// Before reseting buffer lists, purge the cache for all cached buffers currently in use by client.
	pBuf=(TAudioBuffer*)iInUseBufferQ.First();
	SDblQueLink* anchor=&iInUseBufferQ.iA;
	while (pBuf!=anchor)
		{
		pBuf->Flush(DBufferManager::EFlushBeforeDmaRead);
		pBuf=(TAudioBuffer*)pBuf->iNext;
		}
		
	// Start by reseting all the lists.
	iFreeBufferQ.iA.iNext=iFreeBufferQ.iA.iPrev=&iFreeBufferQ.iA;	
	iCompletedBufferQ.iA.iNext=iCompletedBufferQ.iA.iPrev=&iCompletedBufferQ.iA;
	iInUseBufferQ.iA.iNext=iInUseBufferQ.iA.iPrev=&iInUseBufferQ.iA;	
		
	// Set the pointers to the current and the next record buffers.
	pBuf=iAudioBuffers; 		// This is the first buffer
	iCurrentBuffer=pBuf++;
	iNextBuffer = pBuf++;
	
	// Add all other buffers to the free list.
	TAudioBuffer* bufferLimit=iAudioBuffers+iNumBuffers; 
	while(pBuf<bufferLimit)
		iFreeBufferQ.Add(pBuf++);
	
	iBufOverflow=EFalse;	
	}
	
/**
Update buffer lists after a record buffer has been filled.
@param aBytesAdded The number of bytes added to the buffer to get it into the 'filled' state. Of course, this is
	normally equal to the size of the buffer. The exception is when recording has been paused: in which
	case the number of bytes added to 'fill' the buffer may be less than the buffer size.
@param aTransferResult The result of the transfer.	
@return A pointer to the next buffer for recording.
@pre The buffer/request queue mutex must be held.
*/
TAudioBuffer* DRecordBufferManager::SetBufferFilled(TInt aBytesAdded,TInt aTransferResult)
	{
	// If record has been paused then its possible (depending on the PDD implementation) that although the current
	// buffer is marked as being filled, no data has been added. If this is the case then there is no point in informing 
	// the client about it. Instead we need to return it to the free list. Otherwise the more normal course of action is
	// to add the current buffer to the completed list ready for the client. If there is any amount of data in the record
	// buffer, this needs to be passed to the client (and if we're not paused then each buffer should actually be full).
	// If an error occured then we always add the buffer to the completed list.
	TAudioBuffer* buffer=iCurrentBuffer;
	if (aBytesAdded || aTransferResult)
		{
		buffer->iBytesAdded=aBytesAdded;
		buffer->iResult=aTransferResult;
		iCompletedBufferQ.Add(buffer);
		}
	else
		iFreeBufferQ.Add(buffer);

	// Make the pending buffer the current one.
	iCurrentBuffer=iNextBuffer;

	// Obtain the next pending buffer. If there are none left on the free list then we have to take one back
	// from the completed list.
	iNextBuffer=(TAudioBuffer*)iFreeBufferQ.GetFirst();
	if (!iNextBuffer)
		{
		iNextBuffer=(TAudioBuffer*)iCompletedBufferQ.GetFirst();
		iBufOverflow=ETrue;	// This is the buffer overflow situation.
		}
	__ASSERT_DEBUG(iNextBuffer,Kern::Fault(KSoundLddPanic,__LINE__));	
	iNextBuffer->iBytesAdded=0;
	
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DBufferManager::SetBufferFilled(buf=%08x len=%d)",buffer->iChunkOffset,buffer->iBytesAdded));
	return(iNextBuffer);
	}

/**
Get the next record buffer from the completed buffer list. If there is no error associated with the buffer, 
make it 'in use' by the client. Otherwise, return the buffer to the free list.
@return A pointer to the next completed buffer or NULL if there isn't one available.
@pre The buffer/request queue mutex must be held.
*/
TAudioBuffer* DRecordBufferManager::GetBufferForClient()
	{	
	TAudioBuffer* buffer=(TAudioBuffer*)iCompletedBufferQ.GetFirst();
	if (buffer)
		{
		if (buffer->iResult==KErrNone)
			iInUseBufferQ.Add(buffer);
		else
			iFreeBufferQ.Add(buffer);
		}
	
	__KTRACE_OPT(KSOUND1, Kern::Printf("<DBufferManager::BufferForClient(buf=%08x)",(buffer ? buffer->iChunkOffset : -1)));
	return(buffer);
	}

/**
Release (move to free list) the 'in use' record buffer specified by the given chunk offset.
@param aChunkOffset The chunk offset corresponding to the buffer to be freed.
@return The freed buffer, or NULL if no 'in use' buffer had the specified chunk offset.
@pre The buffer/request queue mutex must be held.
*/
TAudioBuffer* DRecordBufferManager::ReleaseBuffer(TInt aChunkOffset)
	{
	// Scan 'in use' list for the audio buffer
	TAudioBuffer* pBuf;
	pBuf=(TAudioBuffer*)iInUseBufferQ.First();
	SDblQueLink* anchor=&iInUseBufferQ.iA;
	while (pBuf!=anchor && pBuf->iChunkOffset!=aChunkOffset)
		pBuf=(TAudioBuffer*)pBuf->iNext;
	
	// Move buffer to the free list (if found)
	if (pBuf!=anchor)
		iFreeBufferQ.Add(pBuf->Deque());
	else
		pBuf=NULL;
	
	__KTRACE_OPT(KSOUND1, Kern::Printf(">DBufferManager::BufferRelease(buf=%08x)",(pBuf ? pBuf->iChunkOffset : -1)));
	return(pBuf);
	}

/**
Constructor for the audio buffer class.
Clears all member data
*/
TAudioBuffer::TAudioBuffer()
	{
	memclr(this,sizeof(*this));
	}

/**
Destructor for the audio buffer class.
*/
TAudioBuffer::~TAudioBuffer()
	{
	delete[] iPhysicalPages;
	}
	
/**
Second stage constructor for the audio buffer class - validate and acquire information on the memory
allocated to this buffer.
@param aChunk  The chunk in which this buffer belongs.
@param aChunkOffset The offset within aChunk to the start of the audio buffer.
@param aSize The size (in bytes) of the buffer.
@param aIsContiguous A boolean indicating whether the buffer contains physically contiguous memory. Set to ETrue if it
	does physically contiguous memory, EFalse otherwise.		
@param aBufManager A pointer to the buffer manager which owns this object.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt TAudioBuffer::Create(DChunk* aChunk,TInt aChunkOffset,TInt aSize,TBool aIsContiguous,DBufferManager* aBufManager)
	{
	__KTRACE_OPT(KSOUND1, Kern::Printf(">TAudioBuffer::Create(Off-%x,Sz-%d,Contig-%d)",aChunkOffset,aSize,aIsContiguous));

	// Save info. on the offset and size of the buffer. Also the buffer manager.
	iChunkOffset=aChunkOffset;
	iSize=aSize;
	iBufManager=aBufManager;

	TInt r=KErrNone;
	iPhysicalPages=NULL;
	if (!aIsContiguous)
		{
		// Allocate an array for a list of the physical pages.
		iPhysicalPages = new TPhysAddr[aSize/Kern::RoundToPageSize(1)+2];
		if (!iPhysicalPages)
			r=KErrNoMemory;	
		}
		
	if (r==KErrNone)
		{
		// Check that the region of the chunk specified for the buffer contains committed memory. If so, get the physical addresses of the
		// pages in this buffer.
		TUint32 kernAddr;
		TUint32 mapAttr;
		r=Kern::ChunkPhysicalAddress(aChunk,aChunkOffset,aSize,kernAddr,mapAttr,iPhysicalAddress,iPhysicalPages);
		// r = 0 or 1 on success. (1 meaning the physical pages are not contiguous).
		if (r==1)
			{
			// The physical pages are not contiguous.
			iPhysicalAddress=KPhysAddrInvalid;	// Mark the physical address as invalid.
			r=(aIsContiguous) ? KErrGeneral : KErrNone;
			}
		if (r==0)
			{
			delete[] iPhysicalPages;	// We shouldn't retain this info. if the physical pages are contiguous.
			iPhysicalPages=NULL;
			}
			
		}
	__KTRACE_OPT(KSOUND1, Kern::Printf("<TAudioBuffer::Create - %d",r));
	return(r);
	}

/**
Calculate the length for the next part of a data transfer request so that that the length returned specifies a physically
contiguous region and is also valid for the PDD. If necessary, return a truncated length that meets these criteria. Also,
return the physical address of the start of the region specified.
@param aChunkOffset The offset within the chunk for the start of the data transfer being fragmented.
@param aLengthRemaining The remaining length of the data transfer request.
@param aPhysAddr On return, this contains the physical address corresonding to aChunkOffset.
@return The length calculated.
*/	
TInt TAudioBuffer::GetFragmentLength(TInt aChunkOffset,TInt aLengthRemaining,TPhysAddr& aPhysAddr)		
	{
	TInt len;
	TInt bufOffset=(aChunkOffset - iChunkOffset); 	// Convert from chunk offset to buffer offset.
	
	if (iPhysicalAddress==KPhysAddrInvalid)
		{
		// Buffer is not physically contiguous. Truncate length to the next page boundary. Then calculate physical addr.
		// (This function doesn't look for pages which are contiguous within the physical page list - but it could).
		TInt pageSize=Kern::RoundToPageSize(1);
		TInt pageOffset=bufOffset%pageSize;
		len=pageSize - pageOffset;
		aPhysAddr=iPhysicalPages[bufOffset/pageSize] + pageOffset;
		}
	else
		{
		// Buffer is physically contiguous so no need to truncate the length. Then calculate physical address.  
		len=aLengthRemaining;
		aPhysAddr=iPhysicalAddress + bufOffset;
		}
		
	// Ensure length does not exceed the max. supported by the PDD.
	len=Min(iBufManager->iMaxTransferLen,len);		
	return(len);
	}
	
/**
Purge the entire audio buffer. That is, if it contains cacheable memory, flush it.
@param aFlushOp The type of flush operation required - @see DBufferManager::TFlushOp.
*/
void TAudioBuffer::Flush(DBufferManager::TFlushOp aFlushOp)
	{
	iBufManager->FlushData(iChunkOffset,iSize,aFlushOp);
	}

