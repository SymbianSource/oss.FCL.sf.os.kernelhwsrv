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
// template\template_variant\camerasc\camerasc_plat.h
// Implementation of the Template shared chunk camera physical device driver (PDD).
// This file is part of the Template Base port
// 
//

#ifndef __CAMERASC_PLAT_H__
#define __CAMERASC_PLAT_H__

#include <drivers/camerasc.h>
#include <pixelformats.h>

// Comment out the first #define, and uncomment the second #define in order to have debug
// output for the shared chunk camera driver
#define __KTRACE_CAM(s)
//#define __KTRACE_CAM(s) s

/** Total number of image capture requests that can be handled by the sensor at one time */
const TInt KTotalCameraRequests = 2;

/** NaviEngine specific panics that can be thrown by the shared chunk camera driver */
enum TTemplateCameraScPddPanic
	{
	/** Start() has been called before SetConfig() */
	ENotConfigured,
	/** Unable to power down the camera hardware */
	ECannotPowerDown,
	/** Buffer passed to DSensorIf::FrameSizeCaps() by LDD is too small */
	ECapsBufferTooSmall
	};

/**
The physical device (factory class) for the NaviEngine shared chunk camera driver.

This class is used by the device driver framework to instantiate one or more shared chunk camera driver
PDDs.  An instance of one PDD is allowed for each physical sensor on the device.
*/
class DTemplateCameraScPddFactory : public DPhysicalDevice
	{
public:

	DTemplateCameraScPddFactory();
	~DTemplateCameraScPddFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	TBool IsUnitOpen(TInt aUnit);
	TInt SetUnitOpen(TInt aUnit, TBool aIsOpen);

private:

	/** The DFC queue to be used by both the LDD and the PDD to serialise access to the PDD. */
	TDynamicDfcQue*	iDfcQ;
	/** Mask to keep track of which units have a channel open on them. */
	TUint			iUnitsOpenMask;
	/** A mutex to protect access to the unit information mask. */
	NFastMutex		iUnitInfoMutex;

	friend class DTemplateCameraScPdd;
	};

/**
Defines the interface for notification of an image being captured.

Used by concrete instances of the DSensorIf abstract base class in order to notify an observer class
(typically an DCameraScPdd derived class) that an image has been captured for processing.
*/
class MSensorObserver
	{
public:

	virtual TInt NotifyImageCaptureEvent(TInt aResult, TLinAddr& aLinAddr, TPhysAddr& aPhysAddr) = 0;
	};

/**
Defines an abstract base class for implementing concrete classes for camera sensors.

This class provides an abstract interface to the sensor;  one class is derived from this and implemented
for each sensor available to the camera driver.
*/
class DSensorIf : public DBase
	{
public:

	/**
	Second phase constructor for the sensor interface.  Acquires any resources required for communication with the sensor.
	When this returns, the sensor is ready for use.
	*/
	virtual TInt DoCreate() = 0;

	/**
	Obtains information regarding the frame sizes and frame rates supported for a given combination of capture mode and pixel format.
	@param	aCaptureMode		The capture mode for which to obtain the information.
	@param	aUidPixelFormat		The pixel format for which to obtain the information.
	@param	aFrameSizeCapsBuf	A referenced to an array of packaged SDevCamFrameSize structures into which to place the information.
	@return	KErrNone if successful, else one of the other system wide error codes.
	*/
	virtual TInt FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf) = 0;

	/**
	Obtains the capabilities of the sensor.  This either interrogates the sensor to find out its capabilities, or hard codes
	them into aCameraCaps, or a combination of the two.
	@param	aCameraCaps		A reference to a ptr to the structure into which to place the capabilities of the sensor.
							This structure is of a variable size and contains the fixed part, followed by an array of
							SDevCamPixelFormat structures.
	@return	The size of the variable length structure pointed to by aCameraCaps if successful, else one of the other
			system wide error codes.
	*/
	virtual TInt GetCaps(TCameraCapsV02*& aCameraCaps) = 0;

	/**
	Powers up the sensor.
	*/
	virtual TInt RequestPower() = 0;

	/**
	Powers down the sensor.
	*/
	virtual TInt RelinquishPower() = 0;

	/**
	Configures the sensor for capture in the configuration previously set by SetConfig(), and begins capture into the
	address pointed to by aLinAddr and aPhysAddr.  Both of these addresses point to the same buffer;  The address used
	by the sensor is hardware dependent.
	@param	aCaptureMode	Whether to capture in video, viewfinder or single image mode.
	@param	aLinAddr		The virtual address of the buffer into which to capture the image.
	@param	aPhysAddr		The physical address of the buffer into which to capture the image.
	@return	KErrNone if successful, otherwise one of the other system wide error codes.
	@pre	SetConfig() must first have been called.
	*/
	virtual TInt Start(TDevCamCaptureMode aCaptureMode, TLinAddr aLinAddr, TPhysAddr aPhysAddr) = 0;

	/**
	Sets the address of the buffer into which the next image will be captured.  If is common for this to be called by Start() as
	well as by the class that owns the sensor interface.
	@param	aLinAddr		The virtual address of the buffer into which to capture the image.
	@param	aPhysAddr		The physical address of the buffer into which to capture the image.
	@return	KErrNone if successful, otherwise one of the other system wide error codes.
	@pre	Start() must first have been called.
	*/
	virtual TInt CaptureNextImage(TLinAddr aLinAddr, TPhysAddr aPhysAddr) = 0;

	/**
	Stops any image capturing that is currently underway.  It is safe to call this without having called Start().
	@return	KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt Stop() = 0;

	/**
	Saves a configuration specifying such details as dimensions and pixel format in which the sensor should
	capture images.  The shared implementation of this contains generic code, but this can be overridden by
	derived classes if desired.
	@param	aConfig	A TCameraConfigV02 structure containing the settings to be used.
	@return	KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	virtual TInt SetConfig(const TCameraConfigV02& aConfig);

protected:

	/** Pointer to the observer to call when a frame of data is available */
	MSensorObserver*	iObserver;
	/** ETrue if capture is under way, else EFalse*/
	TBool				iEnabled;
	/** Width of the frames to be captured in pixels */
	TInt				iWidth;
	/** Height of the frames to be captured in pixels */
	TInt				iHeight;
	/** Number of bytes from the start of one line to the start of the next */
	TInt				iLineOffset;
	/** The number of requests setup ready for transfer */
	TInt				iPendingRequests;
	/** The next request to be setup ready for transfer */
	TUint				iNextRequest;
	/** The configuration in which to capture images */
	TCameraConfigV02	iConfig;
	};

/**
This class provides an abstract interface to the Template sensor.
*/
class DTemplateSensorIf : public DSensorIf
	{
public:

	DTemplateSensorIf(MSensorObserver& aObserver, TDfcQue* aDFCQueue);
	TInt DoCreate();
	~DTemplateSensorIf();
	TInt BufferDoneCallback(TInt aResult);
	void FillBuffer(TLinAddr aBuffer);
	TInt FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf);
	TInt GetCaps(TCameraCapsV02*& aCameraCaps);
	TInt RequestPower();
	TInt RelinquishPower();
	TInt Start(TDevCamCaptureMode aCaptureMode, TLinAddr aLinAddr, TPhysAddr aPhysAddr);
	TInt Stop();
	TInt CaptureNextImage(TLinAddr aLinAddr, TPhysAddr aPhysAddr);

	// Static callbacks for various sensor related asynchronous functions
	static TInt HostPowerCallback(TAny* aPtr, TAny* aPoweredUp);
	static TInt SensorClkReqCallback(TAny* aPtr);

private:

	/** X position at which to display the logo */
	TInt		iX;
	/** Y position at which to display the logo */
	TInt		iY;
	/** Current X direction and speed at which the logo is moving */
	TInt		iXDirection;
	/** Current Y direction and speed at which the logo is moving */
	TInt		iYDirection;
	/** Number of nanokernel ticks that represent the time to capture one frame */
	TInt		iImageTimerTicks;
	/** Timers used for emulating images being captured */
	NTimer		iImageTimers[KTotalCameraRequests];
	/** DFC queue used for completing image capture requests */
	TDfcQue*	iDFCQueue;
	/** DFCs used for image capture timer callbacks happeing in our DFC thread */
	TDfc		*iImageTimerDFCs[KTotalCameraRequests];
    
    /* Used for cheesy animation effect */
    TUint8 iCounter;
    TBool iFlipSwitch;
	};

/**
The physical device driver for the NaviEngine shared chunk camera driver.

This is the concrete implementation of the abstract DCameraScPdd base class.  One instance of this
class will be created by the factory class per physical sensor on the device.  Only one instance
per sensor can be instantiated at any given time.  Access to the sensor itself is achieved via the
appropriate DSensorIf derived class.
*/
class DTemplateCameraScPdd : public DCameraScPdd, public MSensorObserver
	{
private:

	/** States in which the channel can be */
	enum TState
		{
		/** Channel created but not yet configured */
		EUnconfigured,
		/** Channel configured but idle and not capturing images */
		EConfigured,
		/** Channel capturing images */
		ECapturing
		};

public:

	DTemplateCameraScPdd();
	TInt DoCreate(DTemplateCameraScPddFactory* aPhysicalDevice, TInt aUnit);
	~DTemplateCameraScPdd();
	TDfcQue* DfcQ(TInt aUnit);
	void Caps(TDes8& aCapsBuf) const;
	void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo);
	TInt SetConfig(const TDesC8& aConfigBuf);
	TInt Start(TDevCamCaptureMode aCaptureMode,TLinAddr aLinAddr, TPhysAddr aPhysAddr);
	TInt CaptureNextImage(TLinAddr aLinAddr, TPhysAddr aPhysAddr);
	TInt Stop();
	void PowerDown();
	TInt CapsSize();
	TInt FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf);

	/**
	Sets the sensor brightness to the desired setting.

	@param aValue A verified brightness setting.
	@return KErrNone if successful, KErrNotSupported if not supported.
	*/
	TInt SetBrightness(TUint aBrightness);

	/**
	Sets the sensor contrast to the desired setting.

	@param aValue A verified contrast setting.
	@return KErrNone if successful, KErrNotSupported if not supported.
	*/
	TInt SetContrast(TUint aContrast);

	/**
	Sets the sensor color effect to the desired setting.

	@param aValue A verified color effect setting.
	@return KErrNone if successful, KErrNotSupported if not supported.
	*/
	TInt SetColorEffect(TUint aColorEffect);

private:

	TInt NotifyImageCaptureEvent(TInt aResult, TLinAddr& aLinAddr, TPhysAddr& aPhysAddr);

private:

	/** The unit number of this channel.  The unit number determines the sensor used. */
	TInt						iUnit;
	/** A pointer to the PDD factory that created this device. */
	DTemplateCameraScPddFactory* iPhysicalDevice;
	/** Ptr to a buffer large enough to hold the variable sized capabilities structure. */
	TUint8*						iCapsBuffer;
	/** The size of the variable sized capabilities structure. */
	TUint						iCapsSize;
	/** The capabilities of this device. */
	TCameraCapsV02*				iCaps;
	/** The current configuration of this device. */
	TCameraConfigV02			iConfig;
	/** The current capture mode of the camera. */
	TDevCamCaptureMode			iCaptureMode;
	/** Abstracted interface to the sensor */
	DSensorIf*					iSensor;
	/** Current state of the channel (configured, capturing etc) */
	TState						iState;
	};

/**
XXX - This structure holds information pertaining to the logo to be rendered in
"photos" returned by the template camera driver.  This structure is temporary and
should be removed when changing this template into a "real" camera driver.
*/
struct SLogo
	{
	TUint	iWidth;
	TUint	iHeight;
	TUint8	iPixelData[80 * 61 * 3 + 1];
    TUint8  iPixelData2[80 * 61 * 3 + 1];
	};

#endif /* __CAMERASC_PLAT_H__ */
