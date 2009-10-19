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
// template\template_variant\camerasc\camerasc.cpp
// Implementation of the template shared chunk camera physical device driver (PDD).
// This file is part of the Template Base port
// 
//

#include "camerasc_plat.h"

_LIT(KCameraScPddName, "CameraSc.TE");
_LIT(KCameraScDfcQueueName, "CameraSc.TE.DfcQ");

/**
Standard export function for PDD factories.  This creates a DPhysicalDevice derived object, in this case,
DTemplateCameraScPddFactory.
*/
DECLARE_STANDARD_PDD()
	{
	return new DTemplateCameraScPddFactory;
	}

/**
Constructor for the shared chunk camera PDD factory class.
*/
DTemplateCameraScPddFactory::DTemplateCameraScPddFactory()
	{
	// We currently support only unit 0
	iUnitsMask = 0x01;

	// Set the version number for this device.  This is used to allow code to specify that it requires a
	// minimum version of the device in order to operate.  If the version requested is less than this then
	// the device is safe to be used
	iVersion = RDevCameraSc::VersionRequired();
	}

/**
Destructor for the shared chunk camera PDD factory class.
*/
DTemplateCameraScPddFactory::~DTemplateCameraScPddFactory()
	{
	}

/**
Second stage constructor for the shared chunk camera PDD factory class.  This must at least set a name for
the driver object.
@return KErrNone if successful, otherwise one of the system wide error codes.
*/
TInt DTemplateCameraScPddFactory::Install()
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPddFactory::Install()"));

	TInt r;

	// Create a DFC queue so that handling of both camera hardware callbacks and requests made to the LDD from
	// user mode can be processed in the same thread, to avoid the use of semaphores
	if ((r = Kern::DynamicDfcQCreate(iDfcQ, 26, KCameraScDfcQueueName)) == KErrNone)
		{
		// All PDD factories must have a unique name
		r = SetName(&KCameraScPddName);
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPddFactory::Install() => Returning %d", r));

	return r;
	}

/**
Returns the PDD's capabilities.  This is not used by the Symbian OS device driver framework
or by the LDD but is here as some LDDs will make use of it.
@param aDes	A descriptor into which to write capability information.
*/
void DTemplateCameraScPddFactory::GetCaps(TDes8& /*aDes*/) const
	{
	}

/**
Called by the kernel's device driver framework to check if this PDD is suitable for use
with a logical channel.  This is called in the context of the client thread which requested
the creation of a logical channel, through a call to RBusLogicalChannel::DoCreate().  The
thread is in a critical section.
@param aUnit	The unit argument supplied by the client to RBusLogicalChannel::DoCreate()
				This is used to determine which sensor to use.
@param aInfo	The info argument supplied by the client to RBusLogicalChannel::DoCreate().
@param aVer		The version number of the logical channel which will use this physical channel.
@return KErrNone if successful, otherwise one of the system wide error codes.
*/
TInt DTemplateCameraScPddFactory::Validate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check that the version requested is less than or equal to the version of this PDD
	if (!Kern::QueryVersionSupported(RDevCameraSc::VersionRequired(), aVer))
		{
		return KErrNotSupported;
		}

	// Check that the unit number specifies the available sensor
	if ((aUnit < 0) || (aUnit > 0))
		{
		return KErrNotSupported;
		}

	return KErrNone;
	}

/**
Called by the kernel's device driver framework to create a physical channel object.  This
is called in the context of the client thread which requested the creation of a logical
channel, through a call to RBusLogicalChannel::DoCreate().  The thread is in a critical section.
@param aChannel	Set by this function to point to the created physical channel object.
@param aUnit	The unit argument supplied by the client to RBusLogicalChannel::DoCreate().
@param aInfo	The info argument supplied by the client to RBusLogicalChannel::DoCreate().
@param aVer		The version number of the logical channel which will use this physical channel.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateCameraScPddFactory::Create(DBase*& aChannel, TInt aUnit, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPddFactory::Create()"));

	// Create an instance of the PDD channel object that will work with the Template sensor
	DTemplateCameraScPdd* pD = new DTemplateCameraScPdd;

	aChannel = pD;
	TInt r = KErrNoMemory;

	if (pD)
		{
		r = pD->DoCreate(this, aUnit);
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPddFactory::Create() => Returning %d", r));

	return r;
	}

/**
Called by SetUnitOpen() to see if a particular unit is open.  When called, the
iUnitInfoMutex fast mutex will be taken, ensuring safe access to iUnitsOpenMask.
@param aUnit	The unit number to be checked for being open.
@return ETrue if the unit specified by aUnit is already open, otherwise EFalse.
*/
TBool DTemplateCameraScPddFactory::IsUnitOpen(TInt aUnit)
	{
	return (iUnitsOpenMask & (1 << aUnit));
	}

/**
Attempt to change the state of the unit open state for a particular unit.
@param aUnit	The unit number to be set to open or closed state.
@param aIsOpen	The required new state for the unit;  either ETrue to set the state
				to open or EFalse to set the state to closed.
@return KErrNone if the state was updated successfully, otherwise KErrInUse if an attempt
		was made to set the unit status to open while it is already open.
*/
TInt DTemplateCameraScPddFactory::SetUnitOpen(TInt aUnit, TBool aIsOpen)
	{
	// Wait until it is safe to access the unit state mask
	NKern::FMWait(&iUnitInfoMutex);

	// Fail a request to open a unit that is already open
	if (aIsOpen && IsUnitOpen(aUnit))
		{
		__KTRACE_CAM(Kern::Printf("+ DTemplateCameraScPddFactory::SetUnitOpen() => Unit %d is already in use", aUnit));

		// Release the unit state mask mutex
		NKern::FMSignal(&iUnitInfoMutex);

		return KErrInUse;
		}

	// Set or clear the unit's open status bit as required
	if (aIsOpen)
		{
		iUnitsOpenMask |= (1 << aUnit);
		}
	else
		{
		iUnitsOpenMask &= ~(1 << aUnit);
		}

	// Release the unit state mask mutex
	NKern::FMSignal(&iUnitInfoMutex);

	return KErrNone;
	}

/**
Constructor for the shared chunk camera PDD class.
*/
DTemplateCameraScPdd::DTemplateCameraScPdd()
	{
	// Set the unit number to -1 to indicate that this channel has never been registered
	// with the PDD factory
	iUnit = -1;

	// The channel has been created but not yet configured */
	iState = EUnconfigured;
	}

/**
Destructor for the shared chunk camera PDD class.  This is called in the context of the client thread
once an 'ECloseMsg' message has been sent to the device driver DFC thread.
*/
DTemplateCameraScPdd::~DTemplateCameraScPdd()
	{
	delete [] iCapsBuffer;
	delete iSensor;

	// Indicate that a physical channel is no longer open on this unit
	if (iUnit >= 0)
		{
		iPhysicalDevice->SetUnitOpen(iUnit, EFalse);
		}
	}

/**
Second stage constructor for the H4 camera PDD.
@param aPhysicalDevice	A pointer to the factory class that is creating this PDD
@param aUnit			The unit argument supplied by the client to RBusLogicalChannel::DoCreate().
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateCameraScPdd::DoCreate(DTemplateCameraScPddFactory* aPhysicalDevice, TInt aUnit)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::DoCreate()"));

	TInt r;

	iPhysicalDevice = aPhysicalDevice;

	// Check that a physical channel hasn't already been opened on this unit
	if ((r = iPhysicalDevice->SetUnitOpen(aUnit, ETrue)) == KErrNone)
		{
		iUnit = aUnit;

		// Create an abstracted sensor interface
		if ((iSensor = new DTemplateSensorIf(*this, DfcQ(aUnit))) != NULL)
			{
			if ((r = iSensor->DoCreate()) == KErrNone)
				{
				// Setup the capabilities of this device for later reference
				if ((r = iSensor->GetCaps(iCaps)) > 0)
					{
					// And save the size as returned from the sensor
					iCapsSize = r;

					// Although iCaps now points to a TCameraCapsV02 structure, it is actually a variable
					// sized structure that was allocated as an array of TUint8 so save it to a TUint8
					// ptr so that it can be deleted properly
					iCapsBuffer = (TUint8*) iCaps;

					// Enable the clocks needed by the camera subsystem and power up the sensor
					r = iSensor->RequestPower();

					// Some sensors power themselves up automatically in their DoCreate() function,
					// so take this into account here
					if (r == KErrAlreadyExists)
						{
						r = KErrNone;
						}
					}
				}
			}
		else
			{
			r = KErrNoMemory;
			}
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::DoCreate() => Returning %d", r));

	return r;
	}

/**
An appropriate DFC queue to use for processing client requests (that is, those that won't be processed
in the context of the client thread), and also for processing image completion requests from the sensor
will have been setup by the PDD factory.  Anything needing to run in this same DFC thread can access the
queue via this function.
@param	aUnit	The unit number for which to get the DFC queue.
@return	The DFC queue to be used.
*/
TDfcQue* DTemplateCameraScPdd::DfcQ(TInt /*aUnit*/)
	{
	return iPhysicalDevice->iDfcQ;
	}

/**
Called by the LDD in order to query the capabilities of the PDD.
@param	aCapsBuf	A reference to a descriptor owned by the LDD, containing a TCameraCapsV02 structure
					for the capabilities.
*/
void DTemplateCameraScPdd::Caps(TDes8& aCapsBuf) const
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::Caps()"));

	// The iCaps structure will already have been created by a call to iSensor->SetCaps() in DoCreate().
	// Simply copy it into the supplied TPckgBuf, taking into account the fact that the TCameraCapsV02
	// buffer is of a variable size *and* may be smaller or larger than the iCaps structure
	TPtrC8 ptr((const TUint8*) iCaps, iCapsSize);
	aCapsBuf.FillZ(aCapsBuf.MaxLength());
	aCapsBuf = ptr.Left(Min(ptr.Length(), aCapsBuf.MaxLength()));

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::Caps()"));
	}

/**
Called by the LDD to setup a new image configuration, including such things as image size, framerate
and pixel format.
@param	aConfigBuf	A reference to a TPckgBuf containing a TCameraConfigV02 configuration structure.
@return KErrNone if successful, otherwise one of the system wide error codes.
*/
TInt DTemplateCameraScPdd::SetConfig(const TDesC8& aConfigBuf)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::SetConfig()"));

	TInt r;

	// It is only legal to call this if image capture is not already underway, so check for this
	// before doing anything
	if (iState <= EConfigured)
		{
		// Read the new configuration from the LDD into a local copy of the configuration structure,
		// taking into account for compatibility that the TPckgBuf may be smaller or larger than the
		// TCameraConfigV02 structure
		TCameraConfigV02 config;
		TPtr8 ptr((TUint8*) &config, sizeof(config));
		Kern::InfoCopy(ptr, aConfigBuf);

		// Save the new configuration for later and let the sensor also know about it
		iConfig = config;
		iSensor->SetConfig(config);

		// Signal success and set the channel to the configured state
		r = KErrNone;
		iState = EConfigured;
		}
	else
		{
		r = KErrInUse;
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::SetConfig() => Returning %d", r));

	return r;
	}

/**
Begins capture into the address pointed to by aLinAddr and aPhysAddr.  Both of these addresses point to
the same buffer;  The address used by the sensor is hardware dependent.
@param	aCaptureMode	Whether to capture in video, viewfinder or single image mode.
@param	aLinAddr		The virtual address of the buffer into which to capture the image.
@param	aPhysAddr		The physical address of the buffer into which to capture the image.
@return	KErrNone if successful, otherwise one of the other system wide error codes.
@pre	SetConfig() must first have been called.
*/
TInt DTemplateCameraScPdd::Start(TDevCamCaptureMode aCaptureMode, TLinAddr aLinAddr, TPhysAddr aPhysAddr)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::Start() => Configuring sensor for %d x %d capture", iConfig.iFrameSize.iWidth, iConfig.iFrameSize.iHeight));

	// Ensure the precondition is met
	__ASSERT_DEBUG((iState == EConfigured), Kern::Fault("camerasc", ENotConfigured));

	// Save the capture mode for use when we call back into the LDD with the captured image
	iCaptureMode = aCaptureMode;

	// And start the sensor running
	TInt r = iSensor->Start(aCaptureMode, aLinAddr, aPhysAddr);

	// If everything was ok, set the channel to the capturing state
	if (r == KErrNone)
		{
		iState = ECapturing;
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::Start() => Returning %d", r));

	return r;
	}

/**
Sets the address of the buffer info which the next image will be captured.  Called by the LDD for successive
images that are requested after the initial call to Start().
@param	aLinAddr		The virtual address of the buffer into which to capture the image.
@param	aPhysAddr		The physical address of the buffer into which to capture the image.
@return	KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateCameraScPdd::CaptureNextImage(TLinAddr aLinAddr, TPhysAddr aPhysAddr)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::CaptureNextImage()"));

	// Pass the call directly to the sensor abstraction
	TInt r = iSensor->CaptureNextImage(aLinAddr, aPhysAddr);

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::CaptureNextImage()=> Returning %d", r));

	return(r);
	}

/**
Stops any image capturing that is currently underway.  It is safe to call this without having called Start().
@return	KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateCameraScPdd::Stop()
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::Stop()"));

	// Pass the call directly to the sensor abstraction
	iSensor->Stop();

	// Det the channel back to the configured state as it is now safe to call Start() again
	iState = EConfigured;

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::Stop()"));

	return KErrNone;
	}

/**
Power down the camera device.  This is called by the LDD when the driver channel is being closed or
when the system is being powered down.  This is always called in the context of the DFC thread.
*/
void DTemplateCameraScPdd::PowerDown()
	{

#ifdef _DEBUG

	// Power off the camera
	TInt r = iSensor->RelinquishPower();

	// Not being able to power down indicates a serious programming error
	__ASSERT_DEBUG((r == KErrNone), Kern::Fault("camerasc", ECannotPowerDown));

#else // ! _DEBUG

	// Power off the camera
	iSensor->RelinquishPower();

#endif // ! _DEBUG

	}

/**
Return the shared chunk creation information to be used by this device.
@param	aChunkCreateInfo	A structure to be filled with the settings required for this device.
*/
void DTemplateCameraScPdd::GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo)
	{
	// Can be opened by any number of user side processes
	aChunkCreateInfo.iType = TChunkCreateInfo::ESharedKernelMultiple;
	// Use both L1 and L2 cache if available.  LDD will take care of pre and post DMA cache handling
	aChunkCreateInfo.iMapAttr = EMapAttrCachedMax;
	// Chunk owns the memory which will be freed when the chunk is destroyed
	aChunkCreateInfo.iOwnsMemory = ETrue;
	// Don't queue the chunk's destruction on an DFC
	aChunkCreateInfo.iDestroyedDfc = NULL;
	}

/**
Returns the size of the variable sized capabilities structure in bytes.  The buffer passed into
DTemplateCameraScPdd::GetCaps() must be at least this large to hold the fixed portion of the TCameraCapsV02
structure, as well as the array of SDevCamPixelFormat structures that follows it.
@return	The size in bytes of the variable sized capabilities structure.
*/
TInt DTemplateCameraScPdd::CapsSize()
	{
	return iCapsSize;
	}

/**
Obtains information regarding the frame sizes and frame rates supported for a given combination of capture mode
and pixel format.
@param	aCaptureMode		The capture mode for which to obtain the information.
@param	aUidPixelFormat		The pixel format for which to obtain the information.
@param	aFrameSizeCapsBuf	A reference to an array of packaged SDevCamFrameSize structures, owned by the LDD, into
							which to place the information.
@@return	KErrNone if successful, else one of the other system wide error codes.
*/
TInt DTemplateCameraScPdd::FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf)
	{
	return iSensor->FrameSizeCaps(aCaptureMode, aUidPixelFormat, aFrameSizeCapsBuf);
	}

/**
Called by the sensor abstraction when an image is available.
@param	aResult	KErrNone if successful, otherwise one of the system wide error codes.
@param	aLinAddr		The virtual address of the buffer into which to capture the image.
@param	aPhysAddr		The physical address of the buffer into which to capture the image.
*/
TInt DTemplateCameraScPdd::NotifyImageCaptureEvent(TInt aResult, TLinAddr& aLinAddr, TPhysAddr& aPhysAddr)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateCameraScPdd::NotifyImageCaptureEvent() => aResult = %d", aResult));

	// Inform the LDD that a new image has been received
	TInt r = iLdd->ImageCaptureCallback(iCaptureMode, aResult, &aLinAddr, &aPhysAddr);

	// If the LDD has returned KErrAbort then something has gone wrong, and if it has returned KErrNotReady
	// then it has no more frames available, so call Stop()
	if (r != KErrNone)
		{
		Stop();
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateCameraScPdd::NotifyImageCaptureEvent() => Returning %d", r));

	return r;
	}


TInt DTemplateCameraScPdd::SetBrightness(TUint aValue)
	{
	return KErrNotSupported;
	}

TInt DTemplateCameraScPdd::SetContrast(TUint aValue)
	{
	return KErrNotSupported;
	}

TInt DTemplateCameraScPdd::SetColorEffect(TUint aValue)
	{
	return KErrNotSupported;
	}
