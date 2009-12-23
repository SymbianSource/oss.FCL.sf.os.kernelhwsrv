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
// e32/drivers/camerasc/cameraldd.cpp
// 
//

#include <drivers/camerasc.h>
#include <kernel/kern_priv.h>
#include <kernel/cache.h>

//#define __KTRACE_CAM(s) s;
#define __KTRACE_CAM(s)

#define DISCARD_COMPLETED_TO_AVOID_OVERFLOW

static const char KCameraLddPanic[]="CameraSc LDD";

/**
Standard export function for LDDs. This creates a DLogicalDevice derived object,
in this case, DSoundScLddFactory.
*/
DECLARE_STANDARD_LDD()
	{
	return new DCameraScLddFactory;
	}

/**
Constructor for the camera driver factory class.
*/
DCameraScLddFactory::DCameraScLddFactory()
	{
//	iUnitsOpenMask=0;

	__KTRACE_CAM(Kern::Printf(">DCameraScLddFactory::DCameraScLddFactory"));

	// Set version number for this device.
	iVersion=RDevCameraSc::VersionRequired();

	// Indicate that units / PDD are supported.
	iParseMask=KDeviceAllowUnit|KDeviceAllowPhysicalDevice;

	// Leave the units decision to the PDD
	iUnitsMask=0xffffffff;
	}

/**
Second stage constructor for the camera driver factory class.
This must at least set a name for the driver object.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCameraScLddFactory::Install()
	{
	return(SetName(&KDevCameraScName));
	}

/**
Return the 'capabilities' of the camera driver in general.
Called in the response to an RDevice::GetCaps() request.
@param aDes A user-side descriptor to write the capabilities information into.
*/
void DCameraScLddFactory::GetCaps(TDes8 &aDes) const
	{
	// Create a capabilities object
	TCapsDevCameraV01 caps;
	caps.iVersion=iVersion;

	// Write it back to user memory
	Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

/**
Called by the kernel's device driver framework to create a logical channel.
This is called in the context of the client thread which requested the creation of a logical
channel - through a call to RBusLogicalChannel::DoCreate().
The thread is in a critical section.
@param aChannel Set by this function to point to the created logical channel.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCameraScLddFactory::Create(DLogicalChannelBase*& aChannel)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLddFactory::Create"));

	aChannel=new DCameraScLdd;
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
TBool DCameraScLddFactory::IsUnitOpen(TInt aUnit)
	{
	return(iUnitsOpenMask&(1<<aUnit));
	}

/**
Attempt to change the state of the channel open status for a particular channel.
@param aUnit The number of the unit to be updated.
@param aIsOpenSetting	The required new state for the channel open status: either ETrue to set the status to open or
						EFalse to set the status to closed.
@return KErrNone if the status was updated successfully;
		KErrInUse if an attempt has been made to set the channnel status to open while it is already open.
*/
TInt DCameraScLddFactory::SetUnitOpen(TInt aUnit,TBool aIsOpenSetting)
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
Constructor for the camera driver logical channel.
*/
DCameraScLdd::DCameraScLdd()
	:	iRequestQueue(&iMutex),
		iRestartDfc(DCameraScLdd::RestartDfc,this,5),
		iPowerDownDfc(DCameraScLdd::PowerDownDfc,this,3),
		iPowerUpDfc(DCameraScLdd::PowerUpDfc,this,3)
	{
	iState=EOpen;
//	iCaptureMode=ECaptureModeImage;
//	iFrameHeight=0;
//	iFrameWidth=0;
//	iBufManager=NULL;
//	iPowerHandler=NULL;
//	iImageGatherCount=0;

	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DCameraScLdd"));

	iUnit=-1;	// Invalid unit number

	// Get pointer to client thread's DThread object
	iOwningThread=&Kern::CurrentThread();

	// Open a reference on client thread so it's control block can't dissapear until
	// this driver has finished with it. Note, this call to Open() can't fail since
	// it is the thread we are currently running in
	iOwningThread->Open();
	}

/**
Destructor for the camera driver logical channel.
This is called in the context of the client thread once a 'ECloseMsg' message has been
sent to the device driver DFC thread.
*/
DCameraScLdd::~DCameraScLdd()
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::~DCameraScLdd"));

	TInt captureMode;

	// Remove and delete the power handler.
	if (iPowerHandler)
		{
		iPowerHandler->Remove();
		delete iPowerHandler;
		}

	if (iCaptureModeConfig)
		{
		// Delete any buffers and shared chunk we created.
		for (captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
			{
			if (iCaptureModeConfig[captureMode].iBufManager)
				delete iCaptureModeConfig[captureMode].iBufManager;
			}

		// Delete the buffer config. info. structure.
		for (captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
			{
			if (iCaptureModeConfig[captureMode].iBufConfig)
				Kern::Free(iCaptureModeConfig[captureMode].iBufConfig);
			}

			if (iCaptureModeConfig)
				delete[] iCaptureModeConfig;
		}
	// Close our reference on the client thread
	Kern::SafeClose((DObject*&)iOwningThread,NULL);

	// Clear the 'units open mask' in the LDD factory.
	if (iUnit>=0)
		((DCameraScLddFactory*)iDevice)->SetUnitOpen(iUnit,EFalse);
	}

/**
Second stage constructor for the camera driver - called by the kernel's device driver framework.
This is called in the context of the client thread which requested the creation of a logical channel
(e.g. through a call to RBusLogicalChannel::DoCreate()).
The thread is in a critical section.
@param aUnit The unit argument supplied by the client. This is checked by the PDD and not used here.
@param aInfo The info argument supplied by the client. Always NULL in this case.
@param aVer The version argument supplied by the client.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCameraScLdd::DoCreate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoCreate"));

	// Check the client has EMultimediaDD capability.
	if (!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by ECAMERA.LDD (Camera driver)")))
		return(KErrPermissionDenied);

	// Check that the camera driver version specified by the client is compatible.
	if (!Kern::QueryVersionSupported(RDevCameraSc::VersionRequired(),aVer))
		return(KErrNotSupported);

	// Check that a channel hasn't already been opened on this unit.
	TInt r=((DCameraScLddFactory*)iDevice)->SetUnitOpen(aUnit,ETrue); // Try to update 'units open mask' in the LDD factory.
	if (r!=KErrNone)
		return(r);
	iUnit=aUnit;

	// Create the power handler
	iPowerHandler=new DCameraScPowerHandler(this);
	if (!iPowerHandler)
		return(KErrNoMemory);
	iPowerHandler->Add();

	// Create the pending capture request list
	r=iRequestQueue.Create(iOwningThread);
	if (r!=KErrNone)
		return(r);

	// Initialise the PDD
	((DCameraScPdd*)iPdd)->iLdd=this;

	// Setup the default camera config
	iCaptureMode=ECamCaptureModeImage;

	iCaptureModeConfig = new TCaptureModeConfig[ECamCaptureModeMax];
	if(!iCaptureModeConfig)
		return KErrNoMemory;
	TInt capsSize = Pdd()->CapsSize();
	TInt captureMode;
	TAny* capsBuf;
	capsBuf = Kern::Alloc(capsSize);
	if(!capsBuf)
		return KErrNoMemory;

	// Query the driver for its capabilities and set a default pixel format
	// and frame size for each available capture mode.
	TPtr8 capsPtr( (TUint8*)capsBuf, capsSize, capsSize );
	Pdd()->Caps(capsPtr);

	TCameraCapsV02* caps = (TCameraCapsV02*) capsPtr.Ptr();
	SDevCamPixelFormat* pixelFormat = (SDevCamPixelFormat*) (caps + 1);
	SDevCamFrameSize* frameSize;
	TAny* frameSizeCapsBuf=0;
	TPtr8 frameSizeCapsPtr(0,0,0);

	// Set the cache to hold the default dynamic attribute values.
	iBrightnessValue = caps->iDynamicRange[ECamAttributeBrightness].iDefault;
	iContrastValue = caps->iDynamicRange[ECamAttributeContrast].iDefault;
	iColorEffectValue = caps->iDynamicRange[ECamAttributeColorEffect].iDefault;
	
	for (captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
		{
		if ((captureMode==ECamCaptureModeImage) && (caps->iNumImagePixelFormats==0))
			continue;

		if ((captureMode==ECamCaptureModeVideo) && (caps->iNumVideoPixelFormats==0))
			continue;

		if ((captureMode==ECamCaptureModeViewFinder) && (caps->iNumViewFinderPixelFormats==0))
			continue;

		iCaptureModeConfig[captureMode].iCamConfig.iPixelFormat=*pixelFormat;
		frameSizeCapsBuf = Kern::Alloc(pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		new (&frameSizeCapsPtr) TPtr8((TUint8*)frameSizeCapsBuf, pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize), pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize));
		r=Pdd()->FrameSizeCaps((TDevCamCaptureMode)captureMode, pixelFormat->iPixelFormat, frameSizeCapsPtr);
		if(r!=KErrNone)
			{
			Kern::Free(frameSizeCapsBuf);
			return r;
			}
		frameSize=(SDevCamFrameSize*) frameSizeCapsPtr.Ptr();
		iCaptureModeConfig[captureMode].iCamConfig.iFrameSize = *frameSize;
		iCaptureModeConfig[captureMode].iCamConfig.iFrameRate = frameSize->iMinFrameRate;
		Kern::Free(frameSizeCapsBuf);

		iCaptureModeConfig[captureMode].iCamConfig.iFlashMode = ECamFlashNone;
		iCaptureModeConfig[captureMode].iCamConfig.iExposureMode = ECamExposureAuto;
		iCaptureModeConfig[captureMode].iCamConfig.iWhiteBalanceMode = ECamWBAuto;
		iCaptureModeConfig[captureMode].iCamConfig.iZoom = 0;
		iCaptureModeConfig[captureMode].iCamConfig.iPixelWidthInBytes = 0;
		}
	Kern::Free(capsBuf);
	// Setup the default buffer config.
	r=ReAllocBufferConfigInfo(0);	// Zeros the structure
	if (r!=KErrNone)
		return(r);
	for (captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
		{
		iCaptureModeConfig[captureMode].iBufConfig->iNumBuffers=KDefaultNumClientBuffers;
		}

	// Set up the correct DFC queue and enable the reception of client messages.
	TDfcQue* dfcq=((DCameraScPdd*)iPdd)->DfcQ(aUnit);
	SetDfcQ(dfcq);
	iRestartDfc.SetDfcQ(dfcq);
	iPowerDownDfc.SetDfcQ(dfcq);
	iPowerUpDfc.SetDfcQ(dfcq);
	iMsgQ.Receive();

	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::DoCreate"));

	return(KErrNone);
	}

/**
Shutdown the camera device.
Terminate all device activity and power down the hardware.
*/
void DCameraScLdd::Shutdown()
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::Shutdown"));

	iState=EOpen;

	// Power down the hardware
	Pdd()->PowerDown();

	// Cancel any requests that we may be handling
	DoCancel(RDevCameraSc::EAllRequests);

	// Make sure DFCs are not queued.
	iRestartDfc.Cancel();
	iPowerDownDfc.Cancel();
	iPowerUpDfc.Cancel();
	}

/**
Notification to the driver that a handle to it has been requested by a user thread.
The use of a camera driver channel is restricted here to a single thread (that has
EMultimediaDD capability).
@param aThread A pointer to thread which is requesting the handle.
@param aType Whether the requested handle is thread or process relative.
@return	KErrNone, if the request is for a thread relative handle - originating from
		the same the thread that created the channel object;
		KErrAccessDenied, otherwise.
*/
TInt DCameraScLdd::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::RequestUserHandle"));

	// Ensure that each channel can only be used by a single thread.
	if (aType!=EOwnerThread || aThread!=iOwningThread)
		return(KErrAccessDenied);
	return(KErrNone);
	}

/**
Process a request on this logical channel
Called in the context of the client thread.
@param aReqNo	The request number:
				==KMaxTInt: a 'DoCancel' message;
				>=0: a 'DoControl' message with function number equal to value.
				<0: a 'DoRequest' message with function number equal to ~value.
@param a1 The first request argument. For DoRequest(), this is a pointer to the TRequestStatus.
@param a2 The second request argument. For DoRequest(), this is a pointer to the 2 actual TAny* arguments.
@return The result of the request. This is ignored by device driver framework for DoRequest().
*/
TInt DCameraScLdd::Request(TInt aReqNo, TAny* a1, TAny* a2)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::Request(%d)",aReqNo));
	TInt r;
	if (aReqNo<RDevCameraSc::EMsgControlMax && aReqNo>(~RDevCameraSc::EMsgRequestMax))
		{
		// Implement in the context of the kernel thread - prepare and issue a kernel message.
		r=DLogicalChannel::Request(aReqNo,a1,a2);
		}
	else
		{
		// Implement in the context of the client thread.
		// Decode the message type and dispatch it to the relevent handler function.
		if ((TUint)aReqNo<(TUint)KMaxTInt)
			r=DoControl(aReqNo,a1,a2);		// DoControl - process the request.

		else if (aReqNo==KMaxTInt)
			{
			r=DoCancel((TInt)a1);			// DoCancel - cancel the request.
			}

		else
			{
			// DoRequest
			TInt func=~aReqNo;

			// NotifyNewImage() during image capture mode is another case which must be handled in the kernel thread.
			if (iCaptureMode==ECamCaptureModeImage && func==RDevCameraSc::ERequestNotifyNewImage)
				r=DLogicalChannel::Request(aReqNo,a1,a2);
			else
				{
				// Read the arguments from the client thread and process the request.
				TAny* a[2];
				kumemget32(a,a2,sizeof(a));
				TRequestStatus* status=(TRequestStatus*)a1;
				r=DoRequest(func,status,a[0],a[1]);

				// Complete request if there was an error
				if (r!=KErrNone)
					Kern::RequestComplete(iOwningThread,status,r);
				r=KErrNone;
				}
			}
		}
	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::Request - %d",r));
	return(r);
	}

/**
Process a message for this logical channel.
This function is called in the context of the DFC thread.
@param aMsg The message to process.
			The iValue member of this distinguishes the message type:
			iValue==ECloseMsg, channel close message.
			iValue==KMaxTInt, a 'DoCancel' message.
			iValue>=0, a 'DoControl' message with function number equal to iValue.
			iValue<0, a 'DoRequest' message with function number equal to ~iValue.
*/
void DCameraScLdd::HandleMsg(TMessageBase* aMsg)
	{
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::HandleMsg(%d)",id));

	// Decode the message type and dispatch it to the relevent handler function.
	if (id==(TInt)ECloseMsg)
		{
		// Channel close.
		Shutdown();
		m.Complete(KErrNone,EFalse);
		return;
		}
	else if (id<0)	// The only DoRequest handled in the kernel thread is NotifyNewImage(ECamCaptureModeImage).
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt r=DoRequest(~id,pS,m.Ptr1(),m.Ptr2());
		if (r!=KErrNone)
			Kern::RequestComplete(iOwningThread,pS,r);
		m.Complete(KErrNone,ETrue);
		}
	else
		{
		// Must be DoControl (Cancel is handled in the client thread).
		TInt r=DoControl(id,m.Ptr0(),m.Ptr1());
		m.Complete(r,ETrue);
		}
	}

/**
Process a synchronous 'DoControl' request.
This function is called in the context of the DFC thread.
@param aFunction The request number.
@param a1 The first request argument.
@param a2 The second request argument.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCameraScLdd::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoControl(%d)",aFunction));

	TInt r=KErrNotSupported;
	switch (aFunction)
		{
		case RDevCameraSc::EControlCaps:
			{
			r = GetSensorCaps(a1);
			break;
			}
		case RDevCameraSc::EControlSetCaptureMode:
			{
			// Change the capture mode.
			r=SetCaptureMode((TInt)a1);
			break;
			}
		case RDevCameraSc::EControlSetCamConfig:
			{
			// Set the new camera configuration.
			NKern::ThreadEnterCS();
			r=SetCamConfig((TInt)a1, (const TDesC8*)a2);
			NKern::ThreadLeaveCS();
			break;
			}
		case RDevCameraSc::EControlGetCamConfig:
			{
			// Write the config to the client.
			TPtrC8 ptr((const TUint8*)&iCaptureModeConfig[(TInt)a1].iCamConfig,sizeof(iCaptureModeConfig[(TInt)a1].iCamConfig));
			Kern::InfoCopy(*((TDes8*)a2),ptr);
			r=KErrNone;
			break;
			}
		case RDevCameraSc::EControlGetBufferConfig:
			if (iCaptureModeConfig[(TInt)a1].iBufConfig)
				{
				// Write the buffer config to the client.
				TPtrC8 ptr((const TUint8*)&(*iCaptureModeConfig[(TInt)a1].iBufConfig),iCaptureModeConfig[(TInt)a1].iBufConfigSize);
				Kern::InfoCopy(*((TDes8*)a2),ptr);
				r=KErrNone;
				}
			break;
		case RDevCameraSc::EControlSetBufConfigChunkCreate:
			// Need to be in critical section while deleting an exisiting config and creating a new one
			NKern::ThreadEnterCS();
			r=SetBufConfig((TInt)a1,(TInt)a2);
			NKern::ThreadLeaveCS();
			break;
		case RDevCameraSc::EControlSetBufConfigChunkOpen:
			SSetBufConfigChunkOpenInfo info;
			r=Kern::ThreadRawRead(iOwningThread,a2,&info,sizeof(info));
			if (r==KErrNone)
				{
				// Need to be in critical section while deleting an exisiting config and creating a new one
				NKern::ThreadEnterCS();
				r=SetBufConfig((TInt)a1,info.iBufferConfigBuf,info.iChunkHandle);
				NKern::ThreadLeaveCS();
				}
			break;
		case RDevCameraSc::EControlChunkClose:
			r=ChunkClose((TInt)a1);
			break;
		case RDevCameraSc::EControlStart:
			r=Start();
			break;
		case RDevCameraSc::EControlStop:
			if (iState==ECapturing)
				{
				r=Pdd()->Stop();
				DoCancel(1<<RDevCameraSc::ERequestNotifyNewImage);
				if (r==KErrNone)
					iState=EConfigured;
				}
			else
				{
				r=KErrGeneral;
				}
			break;
		case RDevCameraSc::EControlReleaseBuffer:
			r=ReleaseBuffer((TInt)a1);
			break;
		case RDevCameraSc::EControlNotifyNewImageSpecificCancel:
			{
			NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
			iRequestQueue.Cancel((TRequestStatus*)a1);
			NKern::FMSignal(&iMutex); 		// Release the buffer/request list mutex.
			r=KErrNone;
			break;
			}
			
		case RDevCameraSc::EControlBufferIdToOffset:
			{
			// a1 has pointer to buffer for search criteria
			// a2 has pointer to offset for result
			TDevCamBufferModeAndId info;
			TPtr8 inDesc((TUint8*)(&info), sizeof(info));

			r = Kern::ThreadDesRead(iOwningThread,a1,inDesc,0);
			if (r == KErrNone)
				{
				TInt id = info.iId;
				TDevCamCaptureMode captureMode = info.iCaptureMode;

				r = KErrNotFound;
				DBufferManager* mgr = iCaptureModeConfig[captureMode].iBufManager;
				if (mgr)
					{
					if (mgr->iImageBuffer[id].iId == id)
						{
						kumemput32(a2, &mgr->iImageBuffer[id].iChunkOffset, sizeof(TInt));
						r = KErrNone;
						}
					}
				}
			
			break;
			}
		case RDevCameraSc::EControlCapsSize:
			{
			r = Pdd()->CapsSize();
			break;
			}
		case RDevCameraSc::EControlFrameSizeCaps:
			{
			r = GetFrameSizeCaps(a1, a2);
			break;
			}
			
		case RDevCameraSc::EControlSetDynamicAttribute:
			{
			NKern::ThreadEnterCS();
			r = SetDynamicAttribute((TInt)a1, (TUint)a2);
			NKern::ThreadLeaveCS();
			break;
			}
			
		case RDevCameraSc::EControlGetDynamicAttribute:
			{
			TInt attribute = (TInt)(a1);
			TUint value = 0;
			
			r = GetDynamicAttribute(attribute, value);
			if (r == KErrNone)
				{
				kumemput32(a2, &value, sizeof(TUint));
				}
				
			break;
			}
			
		}
	return(r);
	}

/**
Process an asynchronous 'DoRequest' request.
This function is called in the context of the DFC thread.
@param aFunction The request number.
@param aStatus A pointer to the TRequestStatus.
@param a1 The first request argument.
@param a2 The second request argument.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCameraScLdd::DoRequest(TInt aFunction, TRequestStatus* aStatus, TAny* /*a1*/, TAny* /*a2*/)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoRequest(%d)",aFunction));

	TInt r=KErrNotSupported;
	switch (aFunction)
		{
		case RDevCameraSc::ERequestNotifyNewImage:
			r=NotifyNewImage(aStatus);
			break;
		}

	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::DoRequest - %d",r));
	return(r);
	}

/**
Process the cancelling of asynchronous requests.
This function is called in the context of the DFC thread.
@param aMask A mask indicating which requests need to be cancelled.
@return The result of the cancel. Either KErrNone if successful, otherwise one of the other
	system wide error codes.
*/
TInt DCameraScLdd::DoCancel(TUint aMask)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoCancel(%08x)",aMask));

	if (aMask&(1<<RDevCameraSc::ERequestNotifyNewImage))
		{
		NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
		iRequestQueue.CancelAll();
		NKern::FMSignal(&iMutex); 		// Release the buffer/request list mutex.
		}
	return(KErrNone);
	}

/**
@pre The thread must be in a critical section.
*/
TInt DCameraScLdd::ReAllocBufferConfigInfo(TInt aNumBuffers)
	{
	for (TInt captureMode=0; captureMode < ECamCaptureModeMax; captureMode++)
		{
		if (iCaptureModeConfig[captureMode].iBufConfig)
			{
			Kern::Free(iCaptureModeConfig[captureMode].iBufConfig);
			iCaptureModeConfig[captureMode].iBufConfig=NULL;
			}

		iCaptureModeConfig[captureMode].iBufConfigSize=aNumBuffers*(sizeof(SBufSpecList)); // Size of the three integers that hold the offset to the start of each buffer and the buffer id.
		iCaptureModeConfig[captureMode].iBufConfigSize+=sizeof(TSharedChunkBufConfigBase);
		iCaptureModeConfig[captureMode].iBufConfig=(TCameraSharedChunkBufConfig*)Kern::AllocZ(iCaptureModeConfig[captureMode].iBufConfigSize);
		if (!iCaptureModeConfig[captureMode].iBufConfig)
			return(KErrNoMemory);
		}
	return(KErrNone);
	}

/**
Reallocate memory for the new buffer configuration.
@param aNumBuffers The number of buffers.
@pre The thread must be in a critical section.
*/
TInt DCameraScLdd::ReAllocBufferConfigInfo(TInt aCaptureMode, TInt aNumBuffers)
	{
	if (iCaptureModeConfig[aCaptureMode].iBufConfig)
		{
		Kern::Free(iCaptureModeConfig[aCaptureMode].iBufConfig);
		iCaptureModeConfig[aCaptureMode].iBufConfig=NULL;
		}

	iCaptureModeConfig[aCaptureMode].iBufConfigSize=aNumBuffers*(sizeof(SBufSpecList)); // Size of the three integers that hold the offset to the start of each buffer and the buffer id.
	iCaptureModeConfig[aCaptureMode].iBufConfigSize+=sizeof(TSharedChunkBufConfigBase);
	iCaptureModeConfig[aCaptureMode].iBufConfig=(TCameraSharedChunkBufConfig*)Kern::AllocZ(iCaptureModeConfig[aCaptureMode].iBufConfigSize);
	if (!iCaptureModeConfig[aCaptureMode].iBufConfig)
		return(KErrNoMemory);

	return(KErrNone);
	}

/**
@return	A handle to the shared chunk for the owning thread (a value >0), if successful;
		otherwise one of the other system wide error codes, (a value <0).
@param aCamConfigBuf The supplied camera configuration.
@pre The thread must be in a critical section.
*/
TInt DCameraScLdd::SetCamConfig(TInt aCaptureMode, const TDesC8* aCamConfigBuf)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::SetCamConfig()"));

	// Set the configuration of the sensor
	TInt r=DoSetConfig(aCaptureMode, aCamConfigBuf);
	return(r);
	}

/**
Allows changing of the dynamic settings.
Checks locally the validity of the arguments passed so as to increase performance by not
forcing a context switch.

If the setting has been accepted by the sensor the new value is cached by the LDD so further
querying does not involve another context switch.

@param aAttribute An enum identifying the dynamic attribute to change.
@param aValue The attributes value.
@return KErrNone if successful, KErrNotSupported if not supported, KErrArgument if aValue out of range.
		Otherwise, one of the system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DCameraScLdd::SetDynamicAttribute(TInt aAttribute, TUint aValue)
	{
	TUint* attrCachePtr = NULL;
	TInt err = KErrNotSupported;
	
	switch (aAttribute)
		{
		case ECamAttributeBrightness:
			err = Pdd()->SetBrightness(aValue);
			attrCachePtr = &iBrightnessValue;
			break;
			
		case ECamAttributeContrast:
			err = Pdd()->SetContrast(aValue);
			attrCachePtr = &iContrastValue;
			break;
			
		case ECamAttributeColorEffect:
			err = Pdd()->SetColorEffect(aValue);
			attrCachePtr = &iColorEffectValue;
			break;
			
		default:
			return err;
		}
	
	if (err == KErrNone)
		{
		// Cache the set value.
		__ASSERT_DEBUG(attrCachePtr, Kern::Fault(KCameraLddPanic, __LINE__));
		*attrCachePtr = aValue;
		}
		
	return err;
	}


/**
Allows querying of a dynamic setting.
The value is read from the cached LDD values.

@param aAttribute An enum identifying the dynamic attribute to change.
@param aValue A reference to a variable that will receive the attribute value.
@return KErrNone if successful, KErrNotFound if aAttribute is an unsupported
        setting. The parameter aValue is not changed if this function fails.
*/
TInt DCameraScLdd::GetDynamicAttribute(TInt aAttribute, TUint& aValue)
	{
	switch (aAttribute)
		{
		case ECamAttributeBrightness:
			aValue = iBrightnessValue;
			break;
			
		case ECamAttributeContrast:
			aValue = iContrastValue;
			break;
			
		case ECamAttributeColorEffect:
			aValue = iColorEffectValue;
			break;
			
		default:
			return KErrNotFound;
		}
		
	return KErrNone;
	}


/**
Updates the buffer configuration of the camera for the specified capture mode.
@return	A handle to the shared chunk for the owning thread (a value >0), if successful;
		otherwise one of the other system wide error codes, (a value <0).
*/
TInt DCameraScLdd::SetBufConfig(TInt aCaptureMode, TInt aNumBuffers)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::SetBufConfig(CaptureMode=%d,NumBuffers=%d)",aCaptureMode,aNumBuffers));

	// Free any memory and chunk already allocated
	TInt r=ChunkClose(aCaptureMode);
	if (r!=KErrNone)
		return(r);

	// Allocate a new shared chunk and create the specified number of buffers within it.
	TInt buffersize=((iCaptureModeConfig[aCaptureMode].iCamConfig.iFrameSize.iWidth*iCaptureModeConfig[aCaptureMode].iCamConfig.iFrameSize.iHeight) * iCaptureModeConfig[aCaptureMode].iCamConfig.iPixelWidthInBytes);
	__KTRACE_CAM(Kern::Printf(">>DCameraScLdd::SetBufConfig - iFrameSize:%d, iPixelWidthInBytes:%d => bufferSize:%d",(iCaptureModeConfig[aCaptureMode].iCamConfig.iFrameSize.iWidth*iCaptureModeConfig[aCaptureMode].iCamConfig.iFrameSize.iHeight),iCaptureModeConfig[aCaptureMode].iCamConfig.iPixelWidthInBytes,buffersize));
	iCaptureModeConfig[aCaptureMode].iBufManager=new DBufferManager(this);
	if (!iCaptureModeConfig[aCaptureMode].iBufManager)
		return(KErrNoMemory);
	r=iCaptureModeConfig[aCaptureMode].iBufManager->Create(aNumBuffers,buffersize);
	if (r!=KErrNone)
		return(r);

	// Update the LDD's chunk/buffer geometry info.
	r=ReAllocBufferConfigInfo(aCaptureMode, aNumBuffers);
	if (r!=KErrNone)
		return(r);
	iCaptureModeConfig[aCaptureMode].iBufManager->GetBufConfig(*iCaptureModeConfig[aCaptureMode].iBufConfig);

	// Create handle to the shared chunk for the owning thread.
	r=Kern::MakeHandleAndOpen(iOwningThread,iCaptureModeConfig[aCaptureMode].iBufManager->iChunk);
	if (r>0)
		{
		// And save the the chunk and handle for later.  Normally the chunk handle will be closed when the chunk
		// is closed, but if the chunk is re-allocated then it will need to be closed before re-allocation.
		iCaptureModeConfig[aCaptureMode].iChunkHandle=r;
		}

	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::SetBufConfig - %d",r));
	return(r);
	}

/**
Updates the buffer configuration of the camera, which has been supplied by the user, for the specified capture mode.
@param aCaptureMode		The capture mode for which the setting of the buffer configuration is made.
@param aBufferConfigBuf	A buffer that holds the buffer configuration for the camera.
@param aChunkHandle		A handle for the shared chunk supplied by the client.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DCameraScLdd::SetBufConfig(TInt aCaptureMode,const TDesC8* aBufferConfigBuf,TInt aChunkHandle)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::SetConfig(Handle-%d)",aChunkHandle));

	// Read the buffer config structure from the client.
	TInt numBuffers;
	TPtr8 ptr((TUint8*)&numBuffers,sizeof(numBuffers));
	TInt r=Kern::ThreadDesRead(iOwningThread,aBufferConfigBuf,ptr,0);
	if (r!=KErrNone)
		return(r);
	// Calculate the minimum length of the descriptor.
	TInt minDesLen=(numBuffers*sizeof(SBufSpecList))+sizeof(TSharedChunkBufConfigBase);
	r=Kern::ThreadGetDesLength(iOwningThread,aBufferConfigBuf);
	if (r<minDesLen)
		return(KErrArgument);
	r=ReAllocBufferConfigInfo(aCaptureMode, numBuffers);
	if (r!=KErrNone)
		return(r);
	ptr.Set((TUint8*)iCaptureModeConfig[aCaptureMode].iBufConfig,0,iCaptureModeConfig[aCaptureMode].iBufConfigSize);
	r=Kern::ThreadDesRead(iOwningThread,aBufferConfigBuf,ptr,0);
	if (r!=KErrNone)
		return(r);

	// Free any memory and chunk already allocated
	r=ChunkClose(aCaptureMode);
	if (r!=KErrNone)
		return(r);

	// Open the shared chunk supplied and create buffer objects for the committed buffers within it.
	iCaptureModeConfig[aCaptureMode].iBufManager=new DBufferManager(this);
	if (!iCaptureModeConfig[aCaptureMode].iBufManager)
		return(KErrNoMemory);
	r=iCaptureModeConfig[aCaptureMode].iBufManager->Create(*iCaptureModeConfig[aCaptureMode].iBufConfig,aChunkHandle,iOwningThread);
	if (r!=KErrNone)
		return(r);

	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::SetConfig - %d",KErrNone));
	return(r);
	}

/**
Frees the buffer manager associated with a chunk, and closes the chunk itself.  The chunk being closed,
and its associated DBufferManager instance should have been allocated by the device driver.  However,
this is not a requirement.
@param aCaptureMode The capture mode for which to free the buffer manager and chunk.
@return	KErrNone if successful.
		KErrInUse if an attempt has been made to free the memory and chunk while they are in use.
		Otherwise one of the other system-wide error codes.
*/
TInt DCameraScLdd::ChunkClose(TInt aCaptureMode)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::ChunkClose(Capture Mode-%d)",aCaptureMode));

	if(iCaptureMode == aCaptureMode)
        {
        if (iState==ECapturing)
            return(KErrInUse);
        }

	// Delete any existing buffers
	if (iCaptureModeConfig[aCaptureMode].iBufManager)
		{
		delete iCaptureModeConfig[aCaptureMode].iBufManager;
		iCaptureModeConfig[aCaptureMode].iBufManager=NULL;
		}

	// If a handle to the shared chunk was created, close it, using the handle of the thread on which
	// it was created, in case a different thread is now calling us
	if (iCaptureModeConfig[aCaptureMode].iChunkHandle>0)
		{
		Kern::CloseHandle(iOwningThread,iCaptureModeConfig[aCaptureMode].iChunkHandle);
		iCaptureModeConfig[aCaptureMode].iChunkHandle=0;
		}

	return(KErrNone);
	}

/**
Set the current capture mode and submits the camera configuration to the PDD, passing it as a descriptor
to support future changes to the config structure.

@param aCaptureMode	The capture mode that the camera switches to.
@return KErrNone if successful;
		otherwise one of the other system-wide error codes.
*/
TInt DCameraScLdd::SetCaptureMode(TInt aCaptureMode)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::SetCaptureMode(Mode-%d)",aCaptureMode));

	TInt r=KErrNone;
	if(aCaptureMode >= ECamCaptureModeMax || aCaptureMode < 0)
		{
		r=KErrNotFound;
		return(r);
		}

	if (!iCaptureModeConfig[aCaptureMode].iBufManager)
		{
		r=KErrNotReady;
		return(r);
		}

	iCaptureMode=(TDevCamCaptureMode)aCaptureMode;	// The capture mode has already been checked for its validity.

	__KTRACE_CAM(Kern::Printf("DCameraScLdd::SetCaptureMode: iFrameSize:%dx%d)",iCaptureModeConfig[iCaptureMode].iCamConfig.iFrameSize.iWidth, iCaptureModeConfig[iCaptureMode].iCamConfig.iFrameSize.iHeight));

	// Call the PDD to change the hardware configuration according to the new capture mode.
	// Pass it as a descriptor - to support future changes to the config structure.
	TPtr8 ptr((TUint8*)&iCaptureModeConfig[iCaptureMode].iCamConfig,sizeof(iCaptureModeConfig[iCaptureMode].iCamConfig),sizeof(iCaptureModeConfig[iCaptureMode].iCamConfig));
	r=Pdd()->SetConfig(ptr);
	if (r!=KErrNone)
		return(r);
	return KErrNone;
	}


/**
Process a start image capture request from the client - in the capture mode supplied.
If this is a free running mode then the PDD is called straight away to commence capturing frames. In one shot mode the driver postpones the capturing
of frames until a NotifyNewImage() request is received.
@return KErrNone if successful; whether capture mode was actually started or deferred until NotifyNewImage();
		KErrNotReady if SetConfig() has not been previously called;
		otherwise one of the other system-wide error codes.
*/
TInt DCameraScLdd::Start()
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::Start(Current Mode-%d)",iCaptureMode));

	if (iState==ECapturing)
		return(KErrInUse);
	TInt r=KErrNone;

	// Only continue if the mode being started has been configured
	if (iCaptureModeConfig[iCaptureMode].iBufManager)
		iState=EConfigured;

	if (iState==EOpen)
		r=KErrNotReady;
	else if (iState==EConfigured)
		{
		iCaptureModeConfig[iCaptureMode].iBufManager->Reset();
		if (iCaptureMode!=ECamCaptureModeImage)
			r=DoStart();
		if (r==KErrNone)
			iState=ECapturing;
		}
	else
		r=KErrGeneral;
	return(r);
	}

/**
Start the PDD capturing images.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DCameraScLdd::DoStart()
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoStart()"));

	DBufferManager* bufManager=iCaptureModeConfig[iCaptureMode].iBufManager;
	TLinAddr linAddr=(bufManager->iChunkBase)+(bufManager->iCurrentBuffer->iChunkOffset);
	TPhysAddr physAddr=bufManager->iCurrentBuffer->iPhysicalAddress;
	TInt r=Pdd()->Start(iCaptureMode,linAddr,physAddr);

/*
 * 	James Cooper: Uncommenting this code will cause the ASSERT_DEBUG in SetImageCaptured() to fail
 * 	if (r==KErrNone && bufManager->iNextBuffer)
		{
		linAddr=(bufManager->iChunkBase)+(bufManager->iNextBuffer->iChunkOffset);
		physAddr=bufManager->iNextBuffer->iPhysicalAddress;
		r=Pdd()->CaptureNextImage(linAddr,physAddr);
		}
*/	
	return(r);
	}

/**
Process a notify a new image request from the client.
If there is an image already available then the request is completed straight away, otherwise it is added to the capture request queue.
@param aStatus	The request status to be signalled when the request is complete. If the request is successful then this is set
				to the offset within the shared chunk where the record data resides. Alternatively, if an error occurs,
				it will be set to one of the system wide error values.
@return KErrNone if successful - whether the request was completed or simply queued;
		KErrNotReady if Start() hasn't been previousely called;
		KErrInUse: if the client needs to free up buffers before further requests can be accepted;
		KErrGeneral: if the client has more requests queued than there are buffers;
		otherwise one of the other system wide error codes.
*/
TInt DCameraScLdd::NotifyNewImage(TRequestStatus* aStatus)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::NotifyNewImage(%x) - iState(%d)",aStatus,iState));
	DBufferManager* bufManager=iCaptureModeConfig[iCaptureMode].iBufManager;
	TInt r;
	if (iState!=ECapturing || !bufManager)
		return(KErrNotReady);

	NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
	if (iCaptureMode!=ECamCaptureModeImage)
		{
		// We're operating in one of the free running modes, see if an image is already available.
		__KTRACE_CAM(Kern::Printf(">DCameraScLdd::NotifyNewImage - Getting image for client"));
		TImageBuffer* buf=bufManager->GetImageForClient(EFalse);
		if (buf)
			{
			__KTRACE_CAM(Kern::Printf(">DCameraScLdd::NotifyNewImage - There is an image available already"));
			// There is an image available already - complete the request.
			r=buf->iResult;
			NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
			if (r==KErrNone)
				{
				// Only complete if successful here. Errors will be completed on returning from this method.
				__KTRACE_CAM(Kern::Printf(">DCameraScLdd::NotifyNewImage(iId:%d)",buf->iId));
				Kern::RequestComplete(iOwningThread,aStatus,(buf->iId));
				}
			return(r);
			}

		// The buffer 'completed' list is empty. If the 'in-use' list contains all the buffers apart from the one being filled
		// then let the client know they need to free some buffers.
		if (bufManager->iFreeBufferQ.IsEmpty() && !bufManager->iNextBuffer)
			{
			NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
			return(KErrInUse);
			}
		}
	else
		{
		// We're operating in one shot image capture mode. Check if the client needs to free up some buffers
		// before we can accept the request.
		if (bufManager->iCompletedBufferQ.IsEmpty() && bufManager->iFreeBufferQ.IsEmpty() && !bufManager->iNextBuffer)
			{
			NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
			return(KErrInUse);
			}

		// Enough buffers are available so we can start capturing data. First
		// check that there isn't already a capture request in progress.
		if (iRequestQueue.IsEmpty())
			{
			// No previous request in progress so start the PDD.
			NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
			r=DoStart();
			if (r!=KErrNone)
				return(r);
			NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex again.
			}
		}

	// Save the request in the pending queue and return. The request will be completed from the PDD and the DFC thread when
	// an image is available.
	r=iRequestQueue.Add(aStatus);
	NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
	return(r);
	}

/**
Process a release buffer request from the client.
@param aChunkOffset The chunk offset corresponding to the buffer to be freed.
@return KErrNone if successful;
		KErrNotFound if no 'in use' buffer had the specified chunk offset;
		KErrNotReady if the driver hasn't been configured for the current capture mode.
*/
TInt DCameraScLdd::ReleaseBuffer(TInt aBufferId)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::ReleaseBuffer(%d)",aBufferId));
	if(!iCaptureModeConfig[iCaptureMode].iBufManager)
		return KErrNotReady;
	DBufferManager* bufManager=iCaptureModeConfig[iCaptureMode].iBufManager;
	TInt chunkOffset = 0;

	TInt r=KErrNone;
	/*	The driver is left in an ECapturing state after capturing frames. However, it can be left in an
		EConfigured state as a result of Stop() being called. Stop() cancels all pending capture requests and
		leaves the driver in a state in which it can be restarted without needing reconfiguring.	*/
	if (iState!=EOpen && bufManager)
		{
		chunkOffset = bufManager->iImageBuffer[aBufferId].iChunkOffset;
		TImageBuffer* buf=NULL;
		NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
		buf=bufManager->FindInUseImage(chunkOffset);
		NKern::FMSignal(&iMutex); 		// Release the buffer/request list mutex.
		if (buf)
			{
			// The buffer specified by the client has been found in the 'in-use' list.
			bufManager->Purge(buf);
			}
		else
			r=KErrNotFound;

		if (r==KErrNone)
			{
			NKern::FMWait(&iMutex); 		// Acquire the buffer/request list mutex.
			// Release it from the 'in-use list into the 'free' list.
			r=bufManager->ReleaseImage(chunkOffset);
			if (r>0)
				{
				// The buffer needs to be queued straight away - so signal this to the PDD
				TLinAddr linAddr=(bufManager->iChunkBase)+(bufManager->iNextBuffer->iChunkOffset);
				TPhysAddr physAddr=bufManager->iNextBuffer->iPhysicalAddress;
				buf=bufManager->iNextBuffer;
				NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
				r=Pdd()->CaptureNextImage(linAddr,physAddr);
				if (r==KErrNotReady)
					r=KErrNone;
				}
			else
				NKern::FMSignal(&iMutex); 	// Release the buffer/request list mutex.
			}
		}
	else
		r=KErrNotReady;
	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::ReleaseBuffer() - r(%d)",r));
	return(r);
	}

/**
Called from the PDD in the DFC thread each time it finishes capturing an image frame.
This will complete a pending capture request and update buffer lists.
@param aCaptureMode The capture mode of the image captured. @see TDevCamCaptureMode.
@param aResult The result of the image capture request being completed.
@param aLinAddr	If this function returns KErrNone then on return, this holds the linear address of the start of the next buffer
				to use for image capture.
@param aPhysAddr If this function returns KErrNone then on return, this holds the physical address that corresponds to the
				 linear address: aLinAddr.
@return KErrNone if capturing should continue - with holding information on the next buffer to use for image capture.
		KErrNotReady if capturing should continue - but with no further buffer available for image capture just yet.
		KErrAbort if image capturing should now be terminated.
*/
TInt DCameraScLdd::ImageCaptureCallback(TDevCamCaptureMode /*aCaptureMode*/,TInt aResult,TLinAddr* aLinAddr,TPhysAddr* aPhysAddr)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::ImageCaptureCallback"));

	DBufferManager* bufManager=iCaptureModeConfig[iCaptureMode].iBufManager;
	// Update the buffer list and get the next buffer for capture.
	NKern::FMWait(&iMutex); 				// Acquire the buffer/request list mutex.
	TImageBuffer* nextBuffer=bufManager->SetImageCaptured(aResult);	// Puts the captured image's buffer in the completed buffer queue.

	// Check if there is a capture request pending.
	if (!iRequestQueue.IsEmpty())
		{
		// A capture request is pending.
		TBool removeLast=((iCaptureMode==ECamCaptureModeImage) ? (TBool) ETrue : (TBool) EFalse);
		TImageBuffer* buf=bufManager->GetImageForClient(removeLast);	// Retrieved the captured image from the buffer in the completed buffer queue.
		if (buf)
			{
			// Update the request pending list and complete the request.
			TRequestStatus* rs=iRequestQueue.Remove();
			TInt reason=(buf->iResult==KErrNone) ? buf->iId : buf->iResult;
			NKern::FMSignal(&iMutex); 													// Release the buffer/request list mutex.
			buf->SyncMemoryAfterDmaRead();
			Kern::RequestComplete(iOwningThread,rs,reason);								// Complete the request.
			}
		else
			NKern::FMSignal(&iMutex); 													// Release the buffer/request list mutex.
		}
	else
		NKern::FMSignal(&iMutex); 														// Release the buffer/request list mutex.

	// Now work out what instruction to give to the PDD
	TInt r=KErrNone;
	if (iCaptureMode==ECamCaptureModeImage)
		{
		// Image capture mode. If we've just completed a one shot request, see if there is yet another one pending.
		if (!iRequestQueue.IsEmpty())
			{
			// Another request is pending so let the PDD carry on.
			// If an error occured we need to first stop and re-start image capture
			if (aResult!=KErrNone)
				{
				iRestartDfc.Enque();	// Queue a DFC to re-start the PDD later.
				r=KErrAbort;
				}
			}
		else
			{
			r=KErrAbort;	// End of image gather mode so stop the PDD.
			}
		}
	else
		{
		// One of the free running modes. If an error occured we need to first stop and re-start image capture
		if (aResult!=KErrNone)
			{
			iRestartDfc.Enque();	// Queue a DFC to re-start the PDD later.
			r=KErrAbort;
			}
		}

	// If capture should continue, check if there is a further buffer available to use for image capture.
	if (r==KErrNone)
		{
		if (nextBuffer)
			{
			*aLinAddr=(bufManager->iChunkBase)+(nextBuffer->iChunkOffset);
			*aPhysAddr=nextBuffer->iPhysicalAddress;
			}
		else
			r=KErrNotReady;
		}
	return(r);
	}

/**
Stores the camera configuration passed in from the user after checking and validating it.
@param	aCaptureMode	The capture mode for which the setting of the camera configuration is made.
@param	aCamConfigBuf	A buffer that contains the camera configuration.
@return	KErrNone if successful
		KErrInUse if the camera is capturing an image
		KErrArgument if the camera configuration passed in is invalid
		otherwise a system wide error code.
*/
TInt DCameraScLdd::DoSetConfig(TInt aCaptureMode, const TDesC8* aCamConfigBuf)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoSetConfig(CaptureMode=%d)",aCaptureMode));
	
    if(iCaptureMode == aCaptureMode)
        {
        if (iState==ECapturing)
            return(KErrInUse);
        }

	// Read the config structure from the client
	TCameraConfigV02 config;
	TPtr8 ptr((TUint8*)&config,sizeof(config));
	TInt r=Kern::ThreadDesRead(iOwningThread,aCamConfigBuf,ptr,0);
	if (r!=KErrNone)
		return(r);

	// Check that it is compatible with this camera device
	r=ValidateConfig(aCaptureMode, config);
	if (r!=KErrNone)
		{
		if (r == KErrNotFound)
			r = KErrArgument;
		return(r);
		}

	// We're about to replace any previous configuration - so set the
	// status back to un-configured. A new buffer configuration must be calculated as a result of that.
	//iState=EOpen;

	// Save the new configuration.
	iCaptureModeConfig[aCaptureMode].iCamConfig=config;
	iCaptureModeConfig[aCaptureMode].iFrameHeight=iCaptureModeConfig[aCaptureMode].iCamConfig.iFrameSize.iHeight;
	iCaptureModeConfig[aCaptureMode].iFrameWidth=iCaptureModeConfig[aCaptureMode].iCamConfig.iFrameSize.iWidth;

	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::DoSetConfig - %d",KErrNone));
	return(r);
	}

/**
Validates the configuration that is about to be used with the driver.
@param aCaptureMode	The capture mode that the configuration is for.
@param aConfig 		The buffer that contains the camera configuration, as passed in from the user.
@return	KErrNotFound if the configuration is not supported by the camera sensor.
		KErrNotSupported if the driver does not support aCaptureMode
		KErrNone if successful.
*/
TInt DCameraScLdd::ValidateConfig(TInt aCaptureMode, TCameraConfigV02& aConfig)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::ValidateConfig"));

	TInt capsSize = Pdd()->CapsSize();
	NKern::ThreadEnterCS();
	TAny* capsBuf = Kern::Alloc(capsSize);
	if(!capsBuf)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}

	TPtr8 capsPtr( (TUint8*)capsBuf, capsSize, capsSize );
	Pdd()->Caps(capsPtr);
	NKern::ThreadLeaveCS();

	TCameraCapsV02* camCaps = (TCameraCapsV02*) capsPtr.Ptr();

	TInt r;
	if(aCaptureMode==ECamCaptureModeImage && camCaps->iNumImagePixelFormats)
		{
		r=DoValidateConfig(camCaps, aCaptureMode, aConfig);
		}
	else if(aCaptureMode==ECamCaptureModeVideo && camCaps->iNumVideoPixelFormats)
		{
		r=DoValidateConfig(camCaps, aCaptureMode, aConfig);
		}
	else if(aCaptureMode==ECamCaptureModeViewFinder && camCaps->iNumViewFinderPixelFormats)
		{
		r=DoValidateConfig(camCaps, aCaptureMode, aConfig);
		}
	else
		r=KErrNotSupported;

	if(r==KErrNone)
		{
		// Calculate the pixel width (in bytes) for the format specified
		aConfig.iPixelWidthInBytes=aConfig.iPixelFormat.iPixelWidthInBytes;
		}

	NKern::ThreadEnterCS();
	Kern::Free(capsBuf);
	NKern::ThreadLeaveCS();

	__KTRACE_CAM(Kern::Printf("<DCameraScLdd::ValidateConfig - %d",r));
	return(r);
	}

/**
Validates the configuration that is about to be used with the driver by checking it against what the camera sensor supports.
@param aCamCaps		The buffer that contains the capabilities of the camera driver.
@param aCaptureMode	The capture mode that the configuration is for.
@param aConfig 		The buffer that contains the camera configuration, as passed in from the user.
@return	KErrNotFound if the configuration is not supported by the camera sensor
		KErrNone if successful
		or one of the system wide error values.
*/
TInt DCameraScLdd::DoValidateConfig(TCameraCapsV02* aCamCaps, TInt& aCaptureMode, TCameraConfigV02& aConfig)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::DoValidateConfig"));
	TAny* frameSizeCapsBuf;
	TInt frameSizeCapsSize;
	SFrameSizeCapsInfo info;
	SDevCamFrameSize* frameSize;
	TUint i;
	TUint l;
	SDevCamPixelFormat* pixelFormat;
	TUint start;
	TUint end;
	TInt r;
	pixelFormat = (SDevCamPixelFormat*) (aCamCaps + 1);
	if(aCaptureMode==ECamCaptureModeImage)
		{
		start=0;
		end=aCamCaps->iNumImagePixelFormats;
		}
	else if(aCaptureMode==ECamCaptureModeVideo)
		{
		start=aCamCaps->iNumImagePixelFormats;
		end=aCamCaps->iNumImagePixelFormats + aCamCaps->iNumVideoPixelFormats;
		pixelFormat += aCamCaps->iNumImagePixelFormats;
		}
	else if(aCaptureMode==ECamCaptureModeViewFinder)
		{
		start=aCamCaps->iNumImagePixelFormats+aCamCaps->iNumVideoPixelFormats;
		end=aCamCaps->iNumImagePixelFormats + aCamCaps->iNumVideoPixelFormats + aCamCaps->iNumViewFinderPixelFormats;
		pixelFormat += aCamCaps->iNumImagePixelFormats;
		pixelFormat += aCamCaps->iNumVideoPixelFormats;
		}
	else
		return KErrNotSupported;

	for (i=start; i<end; i++)
		{
		if(aConfig.iPixelFormat.iPixelFormat==pixelFormat->iPixelFormat)
			{
			info.iUidPixelFormat = pixelFormat->iPixelFormat;
			info.iCaptureMode = (TDevCamCaptureMode) aCaptureMode;
			frameSizeCapsSize = pixelFormat->iNumFrameSizes*sizeof(SDevCamFrameSize);
			NKern::ThreadEnterCS();
			frameSizeCapsBuf = Kern::Alloc(frameSizeCapsSize);
			NKern::ThreadLeaveCS();
			if (!frameSizeCapsBuf)
				{
				return KErrNoMemory;
				}
			TPtr8 frameSizeCapsPtr( (TUint8*)frameSizeCapsBuf, frameSizeCapsSize, frameSizeCapsSize );
			if ((r = Pdd()->FrameSizeCaps(info.iCaptureMode, info.iUidPixelFormat, frameSizeCapsPtr)) == KErrNone)
				{
				frameSize = (SDevCamFrameSize*) frameSizeCapsPtr.Ptr();
				for(l=0; l<pixelFormat->iNumFrameSizes; l++ )
					{
					if (aConfig.iFrameSize.iWidth == frameSize->iWidth &&
						aConfig.iFrameSize.iHeight == frameSize->iHeight &&
						aConfig.iFrameRate >= frameSize->iMinFrameRate &&
						aConfig.iFrameRate <= frameSize->iMaxFrameRate)
						{
						NKern::ThreadEnterCS();
						Kern::Free(frameSizeCapsBuf);
						NKern::ThreadLeaveCS();
						__KTRACE_CAM(Kern::Printf("<DCameraScLdd::DoValidateConfig"));
						return KErrNone;
						}
					frameSize++;
					}
				NKern::ThreadEnterCS();
				Kern::Free(frameSizeCapsBuf);
				NKern::ThreadLeaveCS();
				return KErrNotFound;
				}
			else
				{
				NKern::ThreadEnterCS();
				Kern::Free(frameSizeCapsBuf);
				NKern::ThreadLeaveCS();
				return r;
				}
			}
		pixelFormat++;
		}
	return KErrNotFound;
	}

/**
The DFC used to re-start the PDD following a data capture error.
@param aChannel A pointer to the camera driver logical channel object.
*/
void DCameraScLdd::RestartDfc(TAny* aChannel)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::RestartDfc"));

	DCameraScLdd& drv=*(DCameraScLdd*)aChannel;

	if (!drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->iCurrentBuffer)
		drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->iCurrentBuffer=drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->NextAvailableForCapture();
	__ASSERT_ALWAYS(drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->iCurrentBuffer,Kern::Fault(KCameraLddPanic,__LINE__));

	if (!drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->iNextBuffer)
		drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->iNextBuffer=drv.iCaptureModeConfig[drv.iCaptureMode].iBufManager->NextAvailableForCapture();

	drv.DoStart();
	}

/**
The DFC used to handle power down requests from the power manager before a transition into system
shutdown/standby.
@param aChannel A pointer to the camera driver logical channel object.
*/
void DCameraScLdd::PowerDownDfc(TAny* aChannel)
	{
	DCameraScLdd& drv=*(DCameraScLdd*)aChannel;
	drv.Shutdown();
	drv.iPowerHandler->PowerDownDone();
	}

/**
The DFC used to handle power up requests from the power manager following a transition out of system standby.
@param aChannel A pointer to the camera driver logical channel object.
*/
void DCameraScLdd::PowerUpDfc(TAny* aChannel)
	{
	DCameraScLdd& drv=*(DCameraScLdd*)aChannel;
	drv.iPowerHandler->PowerUpDone();
	}

void DCameraScLdd::PanicClientThread(TInt aReason)
	{
	Kern::ThreadKill(iOwningThread, EExitPanic, aReason, KDevCameraScName);
	}

/**
Retrieves the capabilities of the camera sensor.
@param aBuffer	A pointer to a descriptor passed in by the user.
*/
TInt DCameraScLdd::GetSensorCaps(TAny* aBuffer)
	{
	// Return the capabilities for this device. Read this from the PDD and
	// then write it to the client
	TInt capsSize = Pdd()->CapsSize();
	TInt bufferSize;
	TInt maxBufferSize;
	Kern::KUDesInfo(*((TDes8*)aBuffer), bufferSize, maxBufferSize);
	if(capsSize>maxBufferSize)
		{
		return KErrArgument;
		}
	NKern::ThreadEnterCS();
	TAny* capsBuf = Kern::Alloc(capsSize);
	if(!capsBuf)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}

	TPtr8 capsPtr( (TUint8*)capsBuf, capsSize, capsSize );
	Pdd()->Caps(capsPtr);
	NKern::ThreadLeaveCS();
	Kern::InfoCopy(*((TDes8*)aBuffer), capsPtr.Ptr(), capsSize);
	NKern::ThreadEnterCS();
	Kern::Free((TAny*)capsBuf);
	NKern::ThreadLeaveCS();
	return KErrNone;
	}

/**
Retrieves the frame sizes supported for a given pixel format.
@param aBuffer	A pointer to descriptor passed in by the user.
@param aFrameSizeCapsInfo A structure that holds information regarding the requested capabilities.
*/
TInt DCameraScLdd::GetFrameSizeCaps(TAny* aBuffer, TAny* aFrameSizeCapsInfo)
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScLdd::GetFrameSizeCaps()"));
	TInt frameSizeCapsMaxSize;
	TInt frameSizeCapsSize;
	Kern::KUDesInfo(*((TDes8*)aBuffer),frameSizeCapsSize,frameSizeCapsMaxSize);
	SFrameSizeCapsInfo info;
	kumemget((TAny*)&info,aFrameSizeCapsInfo,sizeof(info));
	NKern::ThreadEnterCS();
	// Allocate memory on the heap for the frame size structure.
	TAny* frameSizeCapsBuf = Kern::Alloc(frameSizeCapsMaxSize);
	if (!frameSizeCapsBuf)
		{
		NKern::ThreadLeaveCS();
		return KErrNoMemory;
		}
	TPtr8 frameSizeCapsPtr( (TUint8*)frameSizeCapsBuf, frameSizeCapsMaxSize, frameSizeCapsMaxSize );
	// Request the frame sizes from the Pdd.
	TInt r=Pdd()->FrameSizeCaps(info.iCaptureMode, info.iUidPixelFormat, frameSizeCapsPtr);
	NKern::ThreadLeaveCS();
	if (r!=KErrNone)
		{
		NKern::ThreadEnterCS();
		Kern::Free((TAny*)frameSizeCapsBuf);
		NKern::ThreadLeaveCS();
		return r;
		}
	Kern::InfoCopy(*((TDes8*)aBuffer),frameSizeCapsPtr.Ptr(), frameSizeCapsMaxSize);
	NKern::ThreadEnterCS();
	Kern::Free((TAny*)frameSizeCapsBuf);
	NKern::ThreadLeaveCS();
	return KErrNone;
	}


/**
Constructor for the buffer manager.
*/
DBufferManager::DBufferManager(DCameraScLdd* aLdd)
	: iLdd(aLdd)
	{
//	iChunk=NULL;
//	iNumBuffers=0;
//	iImageBuffer=NULL;
	}

/**
Destructor for the buffer manager.
@pre The thread must be in a critical section.
*/
DBufferManager::~DBufferManager()
	{
	if (iChunk)
		Kern::ChunkClose(iChunk);
	delete[] iImageBuffer;
	}

/**
Second stage constructor for the buffer manager. This version creates a shared chunk and a buffer object for each
buffer specified within this. Then it commits memory within the chunk for each of these buffers. This also involves the
creation of a set of buffer lists to manage the buffers.
@param aNumBuffers The number of buffers required in the shared chunk.
@param aBufferSize The size of each buffer required in the shared chunk.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DBufferManager::Create(TInt aNumBuffers,TInt aBufferSize)
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::Create(Bufs-%d,Sz-%d)",aNumBuffers,aBufferSize));

	TInt r=CreateBufferLists(aNumBuffers);
	if (r!=KErrNone)
		return(r);

	// Calculate the size of the chunk required for the buffer configuration specified.
	aBufferSize=Kern::RoundToPageSize(aBufferSize);
	TInt pageSize=Kern::RoundToPageSize(1);
	// Leave space for guard pages around each buffer.  There is a guard page in between each buffer but
	// NO guard page before the first buffer or after the last buffer
	TUint64 chunkSize=TUint64(aBufferSize+pageSize)*aNumBuffers-pageSize;
	if (chunkSize>(TUint64)KMaxTInt)
		return(KErrNoMemory); // Need more than 2GB of memory!

	// Create the shared chunk. The PDD supplies most of the chunk create info - but not the maximum size.
	TChunkCreateInfo info;
	info.iMaxSize=(TInt)chunkSize;
	iLdd->Pdd()->GetChunkCreateInfo(info);		// Call down to the PDD for the rest.

	r = Kern::ChunkCreate(info,iChunk,iChunkBase,iChunkMapAttr);
	if (r!=KErrNone)
		return(r);

	// Commit memory in the chunk for each buffer.
	TInt offset=0;
	TBool isContiguous;
	for (TInt i=0; i<aNumBuffers ; i++)
		{
		r=CommitMemoryForBuffer(offset,aBufferSize,isContiguous);
		if (r!=KErrNone)
			return(r);
		r=iImageBuffer[i].Create(iChunk,offset,aBufferSize,i,isContiguous);
		iImageBuffer[i].iId=i;
		if (r!=KErrNone)
			return(r);
		offset += (aBufferSize+pageSize);
		}

	return(KErrNone);
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
TInt DBufferManager::Create(TCameraSharedChunkBufConfig& aBufConfig,TInt aChunkHandle,DThread* anOwningThread)
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::Create(Handle-%d)",aChunkHandle));

	// Validate the buffer configuration information
	if (!aBufConfig.iFlags&KScFlagBufOffsetListInUse)
		return(KErrArgument);

	TInt numBuffers=aBufConfig.iNumBuffers;
	TInt r=CreateBufferLists(numBuffers);
	if (r!=KErrNone)
		return(r);

	DChunk* chunk;
	chunk=Kern::OpenSharedChunk(anOwningThread,aChunkHandle,ETrue);
	if (!chunk)
		return(KErrBadHandle);
	iChunk=chunk;

	// Read the physical address for the 1st buffer in order to determine the kernel address and the map attributes.
	TInt bufferSizeInBytes=aBufConfig.iBufferSizeInBytes;

	SBufSpecList* bufferSpec=&aBufConfig.iSpec;

	TInt offset=bufferSpec[0].iBufferOffset;

	TPhysAddr physAddr;
	r=Kern::ChunkPhysicalAddress(iChunk,offset,bufferSizeInBytes,iChunkBase,iChunkMapAttr,physAddr,NULL);
	if (r!=KErrNone)
		return(r);

	// Store the supplied buffer info. into each buffer object.

	for (TInt i=0; i<numBuffers; i++)
		{
		offset=bufferSpec[i].iBufferOffset;
		// Assume it isn't contiguous here - Create() will detect and do the right thing if it is contiguous.
		r=iImageBuffer[i].Create(iChunk,offset,bufferSizeInBytes,i,EFalse);
		iImageBuffer[i].iId=i;
		if (r!=KErrNone)
			return(r);
		}
	__KTRACE_CAM(Kern::Printf("<DBufferManager::Create - %d",KErrNone));
	return(KErrNone);
	}

/**
Copies the contents of the Buffer Manager's configuration into aBufConfig
@param aBufConfig The buffer that the data is copied into.
*/
void DBufferManager::GetBufConfig(TCameraSharedChunkBufConfig& aBufConfig)
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::GetBufConfig"));
	TInt numBuffers=iNumBuffers;
	if (numBuffers<=0)
		return;

	SBufSpecList* bufferSpec=&aBufConfig.iSpec;

	while (numBuffers--)
		{
		bufferSpec[numBuffers].iBufferOffset=iImageBuffer[numBuffers].iChunkOffset;
		bufferSpec[numBuffers].iBufferId=iImageBuffer[numBuffers].iId;
		}

	aBufConfig.iNumBuffers=iNumBuffers;
	aBufConfig.iBufferSizeInBytes=iImageBuffer[0].iSize;	// They're all the same size - so read from the 1st one.
	aBufConfig.iFlags|=KScFlagBufOffsetListInUse;
	return;
	}

/**
Allocate an array of buffer objects, - one for each buffer contained within the shared chunk.
@param aNumBuffers The number of buffer objects required.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt DBufferManager::CreateBufferLists(TInt aNumBuffers)
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::CreateBufferLists(Bufs-%d)",aNumBuffers));

	// Construct the array of buffers.
	iNumBuffers=aNumBuffers;
	iImageBuffer=new TImageBuffer[aNumBuffers];
	if (!iImageBuffer)
		return(KErrNoMemory);
	return(KErrNone);
	}

TInt DBufferManager::CommitMemoryForBuffer(TInt aChunkOffset,TInt aSize,TBool& aIsContiguous)
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::CommitMemoryForBuffer(Offset-%x,Sz-%d)",aChunkOffset,aSize));

	// Try for physically contiguous memory first.
	TPhysAddr physicalAddress;
	TInt r=Kern::ChunkCommitContiguous(iChunk,aChunkOffset,aSize,physicalAddress);
	if (r==KErrNone)
		{
		aIsContiguous=ETrue;
		return(r);
		}

	// Commit memory that isn't contiguous instead.
	aIsContiguous=EFalse;
	r=Kern::ChunkCommit(iChunk,aChunkOffset,aSize);
	return(r);
	}

/**
Reset all image buffer lists to reflect the state at the start of the image capture process.
@pre The buffer/request queue mutex must be held.
*/
void DBufferManager::Reset()
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::Reset"));

	TImageBuffer* pBuf;

	// Before reseting buffer lists, purge the cache for all cached buffers currently in use by client.
	pBuf=(TImageBuffer*)iInUseBufferQ.First();
	SDblQueLink* anchor=&iInUseBufferQ.iA;
	while (pBuf!=anchor)
		{
		Purge(pBuf);
		pBuf=(TImageBuffer*)pBuf->iNext;
		}

	// Start by reseting all the lists.
	iFreeBufferQ.iA.iNext=iFreeBufferQ.iA.iPrev=&iFreeBufferQ.iA;
	iCompletedBufferQ.iA.iNext=iCompletedBufferQ.iA.iPrev=&iCompletedBufferQ.iA;
	iInUseBufferQ.iA.iNext=iInUseBufferQ.iA.iPrev=&iInUseBufferQ.iA;

	// Set the pointers to the current and the next record buffers.
	pBuf=iImageBuffer; 		// This is the first buffer
	iCurrentBuffer=pBuf++;
	iNextBuffer = pBuf++;

	// Add all other buffers to the free list.
	TImageBuffer* bufferLimit=iImageBuffer+iNumBuffers;
	while(pBuf<bufferLimit)
		iFreeBufferQ.Add(pBuf++);
	}

/**
Purge the cache for a cached image buffer.
@param aBuffer The buffer to be purged.
*/
void DBufferManager::Purge(TImageBuffer* aBuffer)
	{
	aBuffer->SyncMemoryBeforeDmaRead();
	}

/**
Update buffer lists after an image has been captured.
@param aResult The result of the image capture operation that has just completed.
@return A pointer to the next image buffer for capture - or NULL if none are available.
@pre The buffer/request queue mutex must be held.
*/
TImageBuffer* DBufferManager::SetImageCaptured(TInt aResult)
	{
	// Take a copy of the buffer with the image just captured.
	__ASSERT_DEBUG(iCurrentBuffer,Kern::Fault(KCameraLddPanic,__LINE__));
	TImageBuffer* cur=iCurrentBuffer;

	// Make the queued buffer the current one.
	iCurrentBuffer=iNextBuffer;

	// Now we need to identify the next image buffer to queue.
	iNextBuffer=NextAvailableForCapture();

	// Now add the buffer with the image just captured to the 'completed' list.
	if (cur)
		{
		cur->iResult=aResult;						// Store the result of the capture operation in the image buffer object.
		iCompletedBufferQ.Add(cur);
		}

	__KTRACE_CAM(Kern::Printf("<DBufferManager::SetImageCaptured(buf=%08x)-%d",cur->iChunkOffset,aResult));
	return(iNextBuffer);
	}

/**
Remove from the buffer lists the next buffer that is available to queue for transfer.
@return A pointer to the next image buffer for capture - or NULL if none are available.
@pre The buffer/request queue mutex must be held.
*/
TImageBuffer* DBufferManager::NextAvailableForCapture()
	{
	// We need to identify the next image buffer to queue. Try to get one from the 'free' list.
	TImageBuffer* buffer=(TImageBuffer*)iFreeBufferQ.GetFirst();
#ifdef DISCARD_COMPLETED_TO_AVOID_OVERFLOW
	// If there are none left on the 'free' list then take one from the completed list.
	if (!buffer)
		buffer=(TImageBuffer*)iCompletedBufferQ.GetFirst();
#endif
	return(buffer);
	}

/**
Get the next image from the 'completed' capture list. If there is no error associated with the buffer,
make it 'in use' by the client. Otherwise, return the buffer to the free list.
@param aRemoveLast	If true, the buffer is removed from the tail of the completed capture list, otherwise
					it is removed from the head of this list.
@return A pointer to the next completed image buffer - or NULL if there is no buffer available.
@pre The buffer/request queue mutex must be held.
*/
TImageBuffer* DBufferManager::GetImageForClient(TBool aRemoveLast)
	{
	__KTRACE_CAM(Kern::Printf("<DBufferManager::GetImageForClient"));
	TImageBuffer* buffer=NULL;
	if (!iCompletedBufferQ.IsEmpty())
		{
		buffer = (aRemoveLast) ? (TImageBuffer*)iCompletedBufferQ.Last() : (TImageBuffer*)iCompletedBufferQ.First();
		buffer->Deque();

		if (buffer->iResult==KErrNone)
			iInUseBufferQ.Add(buffer);
		else
			iFreeBufferQ.Add(buffer);
		}
	return(buffer);
	}

/**
Release (move to free list) the 'in use' image specified by the given chunk offset.
@param aChunkOffset The chunk offset corresponding to the buffer to be freed.
@return The freed image buffer, or NULL if no 'in use' buffer had the specified chunk offset.
@return KErrNone if buffer moved to the free list;
		1 if the buffer needs to be queued straight away
		KErrArgument if no 'in use' buffer had the specified chunk offset;
@pre The buffer/request queue mutex must be held.
*/
TInt DBufferManager::ReleaseImage(TInt aChunkOffset)
	{
	__KTRACE_CAM(Kern::Printf(">DBufferManager::ReleaseImage(chunkOffset=%08x)",aChunkOffset));
	TInt r=KErrArgument;

	// Scan 'in use' list for the image buffer
	TImageBuffer* pBuf;
	pBuf=(TImageBuffer*)iInUseBufferQ.First();
	SDblQueLink* anchor=&iInUseBufferQ.iA;
	while (pBuf!=anchor && pBuf->iChunkOffset!=aChunkOffset)
		pBuf=(TImageBuffer*)pBuf->iNext;

	if (pBuf!=anchor)
		{
		// Buffer found in 'in-use' list.
		if (!iNextBuffer)
			{
			// We need to signal the pdd to queue this buffer straight away.
			iNextBuffer=(TImageBuffer*)pBuf->Deque();
			r=1;
			}
		else
			{
			// Move buffer to the free list.
			iFreeBufferQ.Add(pBuf->Deque());
			r=KErrNone;
			}
		}

	__KTRACE_CAM(Kern::Printf("<DBufferManager::ReleaseImage(buf=%08x)",((pBuf!=anchor) ? pBuf->iChunkOffset : -1)));
	return(r);
	}

/**
Find the 'in use' image specified by the given chunk offset
@param aChunkOffset The chunk offset corresponding to the buffer to be freed
@return The image buffer, or NULL if no 'in use' buffer had the specified chunk offset
@pre The buffer/request queue mutex must be held.
*/
TImageBuffer* DBufferManager::FindInUseImage(TInt aChunkOffset)
	{
	// Scan 'in use' list for the image buffer
	TImageBuffer* pBuf;
	pBuf=(TImageBuffer*)iInUseBufferQ.First();
	SDblQueLink* anchor=&iInUseBufferQ.iA;
	while (pBuf!=anchor && pBuf->iChunkOffset!=aChunkOffset)
		pBuf=(TImageBuffer*)pBuf->iNext;

	return((pBuf!=anchor)?pBuf:NULL);
	}

/**
Constructor for the image buffer class.
Clears all member data
*/
TImageBuffer::TImageBuffer()
	{
	memclr(this,sizeof(*this));
	}

/**
Destructor for the image buffer class.
*/
TImageBuffer::~TImageBuffer()
	{
	delete[] iPhysicalPages;
	}

/**
Second stage constructor for the image buffer class - get information on the memory
allocated to this buffer.
@param aChunk	The chunk into which the memory is to be commited
@param aOffset	The offset within aChunk for the start of the comitted memory.
				Must be a multiple of the MMU page size.
@param aSize	The number of bytes of memory commited.
				Must be a multiple of the MMU page size.
@return KErrNone if successful, otherwise one of the other system wide error codes.
@pre The thread must be in a critical section.
*/
TInt TImageBuffer::Create(DChunk* aChunk,TInt aOffset,TInt aSize, TInt aId, TBool aIsContiguous)
	{
	__KTRACE_CAM(Kern::Printf(">TImageBuffer::Create(Off-%x,Sz-%d,Contig-%d)",aOffset,aSize,aIsContiguous));

	// Save info. on the chunk the buffer is in, and the offset and size of the buffer.
	iChunk=aChunk;
	iChunkOffset=aOffset;
	iId=aId;
	iSize=aSize;

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
		// Get the physical addresses of the pages in the buffer.
		TUint32 mapAttr;
		r=Kern::ChunkPhysicalAddress(aChunk,aOffset,aSize,iLinearAddress,mapAttr,iPhysicalAddress,iPhysicalPages);
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
	__KTRACE_CAM(Kern::Printf("<TImageBuffer::Create - %d",r));
	return(r);
	}

/**
Prepares a cacheable buffer for use by the DMA engine, before an image capture.
*/
void TImageBuffer::SyncMemoryBeforeDmaRead()
	{
#ifndef __WINS__
	if (iChunk->iMapAttr&EMapAttrCachedMax)
		{
		Cache::SyncMemoryBeforeDmaRead(iLinearAddress,iSize);
		}
#endif
	}

/**
Prepare a cacheable buffer for use by the CPU, after an image capture using DMA.
*/
void TImageBuffer::SyncMemoryAfterDmaRead()
	{
#ifndef __WINS__
	if (iChunk->iMapAttr&EMapAttrCachedMax)
		{
		Cache::SyncMemoryAfterDmaRead(iLinearAddress,iSize);
		}
#endif
	}

/**
Constructor for the capture request queue.
*/
TCameraScRequestQueue::TCameraScRequestQueue(NFastMutex* aMutexPtr)
	: iMutexPtr(aMutexPtr)
	{
	iOwningThread=NULL;
	memclr(&iRequest[0],sizeof(TCameraScRequest*)*KMaxCamScRequestsPending);
	}

/**
Destructor for the capture request queue.
*/
TCameraScRequestQueue::~TCameraScRequestQueue()
	{
	for (TInt i=0 ; i<KMaxCamScRequestsPending ; i++)
		delete iRequest[i];
	}

/**
Second stage constructor for the capture request queue.
@param anOwningThread A pointer to the owning client thread.
@return KErrNone if successful;
		KErrNoMemory if unable to allocate memory for the capture request queue.
@pre The thread must be in a critical section.
*/
TInt TCameraScRequestQueue::Create(DThread* anOwningThread)
	{
	iOwningThread=anOwningThread;

	// Create the set of available request objects and add them to the unused request queue.
	for (TInt i=0 ; i<KMaxCamScRequestsPending ; i++)
		{
		iRequest[i]=new TCameraScRequest;
		if (!iRequest[i])
			return(KErrNoMemory);
		iUnusedRequestQ.Add(iRequest[i]);
		}

	return(KErrNone);
	}

/**
Store a request status pointer onto the tail of the capture request queue.
@param aStatus The request status pointer to be stored.
@return KErrNone if successful;
		KErrGeneral if the limit on the number of pending capture request (KMaxCamScRequestsPending) would be exceeded.
@pre The buffer/request queue mutex must be held.
*/
TInt TCameraScRequestQueue::Add(TRequestStatus* aStatus)
	{
	TCameraScRequest* req=(TCameraScRequest*)iUnusedRequestQ.GetFirst();
	if (!req)
		return(KErrGeneral);								// Must have exceeded KMaxCamScRequestsPending

	req->iStatus=aStatus;
	iPendRequestQ.Add(req);
	return(KErrNone);
	}

/**
Retrieve the next request status pointer from the head of the capture request queue.
@return The request status pointer removed or NULL if the list is empty.
@pre The buffer/request queue mutex must be held.
*/
TRequestStatus* TCameraScRequestQueue::Remove()
	{
	TRequestStatus* status=NULL;
	TCameraScRequest* req=(TCameraScRequest*)iPendRequestQ.GetFirst();
	if (req)
		{
		status=req->iStatus;
		iUnusedRequestQ.Add(req);
		}
	return(status);
	}

/**
Remove a specifc request status pointer from the the capture request queue, completing it with a 'KErrCancel' completion reason.
@param aStatus The request status pointer to be completed.
@pre The buffer/request queue mutex must be held.
*/
void TCameraScRequestQueue::Cancel(TRequestStatus* aStatus)
	{
	// Find the entry concerned
	TCameraScRequest* req=(TCameraScRequest*)iPendRequestQ.First();
	SDblQueLink* anchor=&iPendRequestQ.iA;
	while (req!=anchor && req->iStatus!=aStatus)
		req=(TCameraScRequest*)req->iNext;
	if (req==anchor)
		return;

	// Remove and cancel it.
	req->Deque();
	iUnusedRequestQ.Add(req);
	NKern::FMSignal(iMutexPtr); 	// Release the request list mutex while we complete the request. This is safe.
	Kern::RequestComplete(iOwningThread,req->iStatus,KErrCancel);
	NKern::FMWait(iMutexPtr); 		// Re-acquire the request list mutex.
	}

/**
Remove each request status pointer from the the capture request queue, completing each with a 'KErrCancel' completion reason.
@pre The buffer/request queue mutex must be held.
*/
void TCameraScRequestQueue::CancelAll()
	{

	TRequestStatus* status;
	while ((status=Remove())!=NULL)
		{
		NKern::FMSignal(iMutexPtr); 	// Release the request list mutex while we complete the request. This is safe.
		Kern::RequestComplete(iOwningThread,status,KErrCancel);
		NKern::FMWait(iMutexPtr); 		// Re-acquire the request list mutex.
		}
	}

/**
Constructor for the camera driver power handler class.
@param aChannel A pointer to the camera driver logical channel which owns this power handler.
*/
DCameraScPowerHandler::DCameraScPowerHandler(DCameraScLdd* aChannel)
:	DPowerHandler(KDevCameraScName),
	iChannel(aChannel)
	{
	}

/**
A request from the power manager for the power down of the camera device.
This is called during a transition of the phone into standby or power off.
@param aState The target power state; can be EPwStandby or EPwOff only.
*/
void DCameraScPowerHandler::PowerDown(TPowerState aPowerState)
	{
	(void)aPowerState;
	__KTRACE_CAM(Kern::Printf(">DCameraScPowerHandler::PowerDown(State-%d)",aPowerState));

	// Power-down involves hardware access so queue a DFC to perform this from the driver thread.
	iChannel->iPowerDownDfc.Enque();
	}

/**
A request from the power manager for the power up of the camera device.
This is called during a transition of the phone out of standby.
*/
void DCameraScPowerHandler::PowerUp()
	{
	__KTRACE_CAM(Kern::Printf(">DCameraScPowerHandler::PowerUp"));

	// Power-up involves hardware access so queue a DFC to perform this from the driver thread.
	iChannel->iPowerUpDfc.Enque();
	}
