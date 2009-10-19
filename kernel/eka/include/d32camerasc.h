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
// e32\include\d32camerasc.h
// User side class definition for the shared chunk camera driver.
// 
//

/**
 @file
 @internalAll
 @prototype
*/

#ifndef __D32CAMERASC_H__
#define __D32CAMERASC_H__

#include <e32cmn.h>
#include <e32ver.h>
#include <pixelformats.h>

_LIT(KDevCameraScName,"CameraSc");

/**
Camera capability constants - bitmasks of possible flash modes. @see TCameraCapsV02.
*/
/** Flash will automatically fire when required. */
const TUint KCamFlashAuto = 0x0001;
/** Flash will always fire. */
const TUint KCamFlashForced = 0x0002;
/** Reduced flash for general lighting */
const TUint KCamFlashFillIn = 0x0004;
/** Red-eye reduction mode. */
const TUint KCamFlashRedEyeReduce = 0x0008;
/** Flash at the moment when shutter opens. */
const TUint KCamFlashSlowFrontSync = 0x0010;
/** Flash at the moment when shutter closes. */
const TUint KCamFlashSlowRearSync = 0x0020;
/** User configurable setting */
const TUint KCamFlashManual = 0x0040;

/**
Camera capability constants - bitmasks of possible exposure modes. @see TCameraCapsV02.
*/
/** Night-time setting for long exposures. */
const TUint KCamExposureNight = 0x0001;
/** Backlight setting for bright backgrounds. */
const TUint KCamExposureBacklight = 0x0002;
/** Centered mode for ignoring surroundings. */
const TUint KCamExposureCenter = 0x0004;
/** Sport setting for very short exposures. */
const TUint KCamExposureSport = 0x0008;
/** Generalised setting for very long exposures. */
const TUint KCamExposureVeryLong = 0x0010;
/** Snow setting for daylight exposure. */
const TUint KCamExposureSnow = 0x0020;
/** Beach setting for daylight exposure with reflective glare. */
const TUint KCamExposureBeach = 0x0040;
/** Programmed exposure setting. */
const TUint KCamExposureProgram = 0x0080;
/** Aperture setting is given priority. */
const TUint KCamExposureAperturePriority = 0x0100;
/** Shutter speed setting is given priority. */
const TUint KCamExposureShutterPriority = 0x0200;
/** User selectable exposure value setting. */
const TUint KCamExposureManual = 0x0400;
/** Exposure night setting with colour removed to get rid of colour noise. */
const TUint KCamExposureSuperNight = 0x0800;
/** Exposure for infra-red sensor on the camera */
const TUint KCamExposureInfra = 0x1000;

/**
Camera capability constants - bitmasks of possible white balance modes. @see TCameraCapsV02.
*/
/** Normal daylight. */
const TUint KCamWBDaylight = 0x0001;
/** Overcast daylight. */
const TUint KCamWBCloudy = 0x0002;
/** Tungsten filament lighting. */
const TUint KCamWBTungsten = 0x0004;
/** Fluorescent tube lighting */
const TUint KCamWBFluorescent = 0x0008;
/** Flash lighting. */
const TUint KCamWBFlash = 0x0010;
/** High contrast daylight primarily snowy */
const TUint KCamWBSnow = 0x0020;
/** High contrast daylight primarily near the sea */
const TUint KCamWBBeach = 0x0040;
/** User configurable mode */
const TUint KCamWBManual = 0x0080;

/**
Camera capability constants - bitmasks of other miscellaneous camera capabilities supported. @see TCameraCapsV02.
*/
/** The camera has zoom capability. */
const TUint KCamMiscZoom = 0x0001;
/** The camera supports contrast adjustment. */
const TUint KCamMiscContrast = 0x0002;
/** The camera supports brightness adjustment. */
const TUint KCamMiscBrightness = 0x0004;
/** The camera supports color effect adjustment. */
const TUint KCamMiscColorEffect = 0x0008;


/**
Enumeration of capture modes in which to run the sensor.
*/
enum TDevCamCaptureMode
	{
	/** Used to specify that still image mode is to be used. */
	ECamCaptureModeImage,
	/** Used to specify that streaming video mode is to be used. */
	ECamCaptureModeVideo,
	/** Used to specify that streaming viewfinder mode is to be used. */
	ECamCaptureModeViewFinder,
	/** The last value here, helps keep track of the number of capture modes. */
	ECamCaptureModeMax
	};

/**
Enumeration of camera flash modes. @see TCameraConfigV02.
*/
enum TDevCamFlashMode
	{
	/** No flash, always supported. */
	ECamFlashNone=0x0000,
	/** Flash will automatically fire when required. */
	ECamFlashAuto=0x0001,
	/** Flash will always fire. */
	ECamFlashForced=0x0002,
	/** Reduced flash for general lighting */
	ECamFlashFillIn=0x0004,
	/** Red-eye reduction mode. */
	ECamFlashRedEyeReduce=0x0008,
	/** Flash at the moment when shutter opens. */
	ECamFlashSlowFrontSync=0x0010,
	/** Flash at the moment when shutter closes. */
	ECamFlashSlowRearSync=0x0020,
	/** User configurable setting */
	ECamFlashManual=0x0040
	};

/**
Enumeration of camera exposure modes. @see TCameraConfigV02.
*/
enum TDevCamExposureMode
	{
	/** Set exposure automatically. Default, always supported. */
	ECamExposureAuto=0x0000,
	/** Night-time setting for long exposures. */
	ECamExposureNight=0x0001,
	/** Backlight setting for bright backgrounds. */
	ECamExposureBacklight=0x0002,
	/** Centered mode for ignoring surroundings. */
	ECamExposureCenter=0x0004,
	/** Sport setting for very short exposures. */
	ECamExposureSport=0x0008,
	/** Generalised setting for very long exposures. */
	ECamExposureVeryLong=0x0010,
	/** Snow setting for daylight exposure. */
	ECamExposureSnow=0x0020,
	/** Beach setting for daylight exposure with reflective glare. */
	ECamExposureBeach=0x0040,
	/** Programmed exposure setting. */
	ECamExposureProgram=0x0080,
	/** Aperture setting is given priority. */
	ECamExposureAperturePriority=0x0100,
	/** Shutter speed setting is given priority. */
	ECamExposureShutterPriority=0x0200,
	/** User selectable exposure value setting. */
	ECamExposureManual=0x0400,
	/** Exposure night setting with colour removed to get rid of colour noise. */
	ECamExposureSuperNight=0x0800,
	/** Exposure for infra-red sensor on the camera */
	ECamExposureInfra=0x1000
	};

/**
Enumeration of camera white balance modes. @see TCameraConfigV02.
*/
enum TDevCamWhiteBalanceMode
	{
	/** Set white balance automatically. Default, always supported. */
	ECamWBAuto=0x0000,
	/** Normal daylight. */
	ECamWBDaylight=0x0001,
	/** Overcast daylight. */
	ECamWBCloudy=0x0002,
	/** Tungsten filament lighting. */
	ECamWBTungsten=0x0004,
	/** Fluorescent tube lighting */
	ECamWBFluorescent=0x0008,
	/** Flash lighting. */
	ECamWBFlash=0x0010,
	/** High contrast daylight primarily snowy */
	ECamWBSnow=0x0020,
	/** High contrast daylight primarily near the sea */
	ECamWBBeach=0x0040,
	/** User configurable mode */
	ECamWBManual=0x0080
	};

/**
Enumeration of possible directions in which the camera may point. @see TCameraCapsV02.
*/
enum TDevCamOrientation
	{
	/** Outward pointing camera for taking pictures. Camera is directed away from the user. */
	ECamOrientationOutwards,
	/** Inward pointing camera for conferencing. Camera is directed towards the user. */
	ECamOrientationInwards,
	/** Mobile camera capable of multiple orientations. Camera orientation may be changed by the user. */
	ECamOrientationMobile,
	/** Camera orientation is not known. */
	ECamOrientationUnknown
	};

/**
Each item in the iPixelFormatSupported array is represented by an instance of this structure.
*/
struct SDevCamPixelFormat
	{
	/** The UID of the pixel format supported */
	TUidPixelFormat iPixelFormat;
	/** The number of frame sizes represented by the pixel format. */
	TUint iNumFrameSizes;
	/** The pixel width in number of bytes */
	TUint iPixelWidthInBytes;
	};

/**
Each frame size supported is represented by an instance of this structure.
*/
struct SDevCamFrameSize
	{
	/** Width of the frame in pixels. */
	TUint iWidth;
	/** Height of the frame in pixels. */
	TUint iHeight;
	/** Minimum frame rate supported by this frame size. */
	TUint iMinFrameRate;
	/** Maximum frame rate supported by this frame size. */
	TUint iMaxFrameRate;
	};

/**
Lets us associate buffers to their mode when working out the buffer offset in a chunk.
**/
class TDevCamBufferModeAndId
	{
public:
	TDevCamCaptureMode iCaptureMode;
	TInt iId;
	};
typedef TPckgBuf<TDevCamBufferModeAndId> TDevCamBufferModeAndIdBuf;

/**
The general driver capabilites class - returned by the LDD factory in response to RDevice::GetCaps().
*/
class TCapsDevCameraV01
	{
public:
	TVersion iVersion;
	};

/**
Defines a list of settings that are changable often (dynamically) within a single use of the device.
*/
enum TDevCamDynamicAttributes
	{
	ECamAttributeBrightness,
	ECamAttributeContrast,
	ECamAttributeColorEffect,
	ECamAttributeMax
	};

/**
Holds the range and interval (rate of change) values for a dynamic capability.
An array of these would be indexed by TDevCamDynamicAttributes
*/
struct TDynamicRange
	{
	TUint iMin;
	TUint iMax;
	};

/**
The main camera capabilities class. This is used to get the capabilities of a specific camera
device once a channel to it has been opened.
*/
class TCameraCapsV02
	{
public :
	/** The flash modes supported - a bit field. */
	TUint iFlashModes;
	/** The exposure modes supported - a bit field. */
	TUint iExposureModes;
	/** The white balance modes supported - a bit field. */
	TUint iWhiteBalanceModes;
	/** The orientation of the camera device. */
	TDevCamOrientation iOrientation;
	/** The minimum value that may be set for the zoom factor. Must be negative or zero. Negative values
	represent macro functionality. @see TCameraCapsV02::iCapsMisc. @see TCameraConfigV02::iZoom. */
	TUint iMinZoom;
	/** The maximum value that may be set for the zoom factor. Must be positive or zero.
	@see TCameraCapsV02::iCapsMisc. @see TCameraConfigV02::iZoom. */
	TUint iMaxZoom;
	/** Whether other miscellaneous capabilities are supported - a bitfield. These
	capabilities include whether the device supports simultaneous capture modes, zoom capabilities, contrast
	adjustment, brightness, and color effect adjustment. */
	TUint iCapsMisc;
	/** Number of pixel formats supported in still image capture mode.
	Will be set to 0 if image capture is not supported. */
	TUint iNumImagePixelFormats;
	/** Number of pixel formats supported in video capture mode.
	Will be set to 0 if image capture is not supported. */
	TUint iNumVideoPixelFormats;
	/** Number of pixel formats supported in view finder capture mode.
	Will be set to 0 if image capture is not supported. */
	TUint iNumViewFinderPixelFormats;
	
	/** An array specifying the range in values for settings as defined by TDevCamDynamicAttributes.
		Indices for settings are in the order defined in TDevCamDynamicAttributes.
		If the setting is not supported then the entry is still present for performance reasons,
		i.e. indexing over searching.
		@see TDevCamDynamicAttributes
		@see TDynamicRange
	*/
	TDynamicRange iDynamicRange[ECamAttributeMax];
	
	/** A variable length array specifying the pixel formats supported by the sensor.
		The size of the TCameraCapsV02 structure is determined by each sensor's capabilities
		thus the array of supported pixel formats is of variable length. It is stored in memory
		exactly after TCameraCapsV02 whenever memory is allocated for it and the array cannot be
		accessed by a private member.
	SDevCamPixelFormat iPixelFormatsSupported[];
	*/
	};

typedef TPckgBuf<TCameraCapsV02> TCameraCapsV02Buf;

/**
The camera configuration class. This is used to get and set the current
configuration of the camera. @see SDevCamFrameSize and @see SDevCamPixelFormat.
*/
class TCameraConfigV02
	{
public:
	/** The size of the image to get from the sensor. */
	SDevCamFrameSize iFrameSize;
	/** The pixel format (RGB, YUV, RGB Bayer etc). */
	SDevCamPixelFormat iPixelFormat;
	/** The frame rate (in frame/s). */
	TUint iFrameRate;
	/** The flash mode setting. */
	TDevCamFlashMode iFlashMode;
	/** The exposure mode setting. */
	TDevCamExposureMode iExposureMode;
	/** The white balance mode setting. */
	TDevCamWhiteBalanceMode iWhiteBalanceMode;
	/** The zoom factor. Can be zero, positive or negative, Negative values represent macro functionality.*/
	TInt iZoom;
	/** Specifies the number of bytes used to store one pixel's worth of data. */
	TInt iPixelWidthInBytes;
	};
typedef TPckgBuf<TCameraConfigV02> TCameraConfigV02Buf;

/** A structure used to assemble arguments for the function RDevCameraSc::SetBufConfigChunkOpen() and to pass
these to the driver. */
struct SSetBufConfigChunkOpenInfo
	{
	const TDesC8* iBufferConfigBuf;
	TInt iChunkHandle;
	};

/** A structure used to assemble arguments for the function RDevCameraSc::FrameSizeCaps() and to pass
these to the driver. */
struct SFrameSizeCapsInfo
	{
	TUidPixelFormat iUidPixelFormat;
	TDevCamCaptureMode iCaptureMode;
	};

/**
The camera device driver API supporting shared chunks. This is the principle interface to communicate with
an attached camera.
*/
class RDevCameraSc : public RBusLogicalChannel
	{
public:
	enum TRequest
	/**
	 Asynchronous request types
	*/
		{
		EMsgRequestMax=3,				// All requests less than this value are handled in the driver DFC thread.
		ERequestNotifyNewImage,
		ENumRequests,
		EAllRequests = (1<<ENumRequests)-1
		};

	enum TControl
	/**
	 Synchronous request types
	*/
		{
		EControlSetBufConfigChunkCreate,
		EControlSetBufConfigChunkOpen,
		EControlStart,
		EControlStop,
		EControlSetCamConfig,
		EControlSetCaptureMode,
		EControlChunkClose,
		EMsgControlMax=100,				// All requests less than this value are handled in the driver DFC thread.
		EControlCaps,
		EControlGetCamConfig,
		EControlGetBufferConfig,
		EControlReleaseBuffer,
		EControlNotifyNewImageSpecificCancel,
		EControlBufferIdToOffset,
		EControlCapsSize,
		EControlFrameSizeCaps,
		EControlSetDynamicAttribute
		};

public:
	/**
	Get the version number of camera driver interface.
	@return The camera driver interface version number.
	*/
	inline static TVersion VersionRequired();

#ifndef __KERNEL_MODE__
	
	/**
	 Constructor.
	 Initialises private members.
	 */
	inline RDevCameraSc();
	
	/**
	Open a channel on a specified camera device. This driver only allows one channel to be opened on each device.
	@param aUnit The unit number of the camera device.
	@return KErrNone, if successful;
			KErrInUse, if a channel is already opened on the unit concerned;
			otherwise one of the other system-wide error codes.
	*/
	inline TInt Open(TInt aUnit);

	/**
	Close the Channel and clean up.
	*/
	inline void Close();

	/**
	Get the capabilities of the camera device.
	@param aCapsBuf A packaged TCameraCapsV02 object which on return will be filled with the capabilities of the device.
	@return KErrNone, if successful;
			KErrArgument, if aCapsBuf is too small.
	@see TCameraCapsV02.
	*/
	inline TInt Caps(TDes8& aCapsBuf);

	/**
	Get the capabilities of the camera device.
	@return TPtrC8 pointing to a locally held TCameraCapsV02 structure owned by the driver.
	@see TCameraCapsV02.
	*/
	inline TPtrC8 Caps();

	/**
	Set the camera configuration settings.
	@param aCaptureMode	The capture mode that camera will be configured for.
	@param aConfigBuf	A packaged TCameraConfigV02 object which contains the configuration settings.
	@see TCameraConfigV02.
	*/
	inline TInt SetCamConfig(TDevCamCaptureMode aCaptureMode, const TDesC8& aConfigBuf);

	/**
	Trigger the buffer allocation and configuration setting - creating a shared chunk.
	From the frame size, pixel format (already supplied) and the number of camera buffers specified,
	the driver will	calculate the size of the shared chunk this requires. It will create such
	a shared chunk containing the specified number of buffers within it (each with memory committed to them).
	This will replace a previous shared chunk created by this driver.
	A handle to the chunk will then be created for the client thread which will be assigned to the
	RChunk object supplied by the client.The configuration cannot be changed unless image capture is disabled.
	@param aCaptureMode	The capture mode that camera will be configured for.
	@param aNumBuffers	The desired number of buffers that is going to be used.
	@param aChunk		An RChunk object to which the chunk handle will be assigned.
	@return KErrNone if successful;
			KErrInUse if image capturing is already in progress;
			KErrArgument if any configuration values are invalid;
			KErrNoMemory if the driver failed allocate memory for the shared chunk specified;
			otherwise one of the other system-wide error codes.
	*/
	inline TInt SetBufConfigChunkCreate(TDevCamCaptureMode aCaptureMode, TInt aNumBuffers, RChunk& aChunk);

	/**
	Get the current camera configuration settings.
	@param aCaptureMode	The capture mode that the user requested the configuration settings of.
	@param aConfigBuf	A packaged TCameraConfigV02 object which will be filled with the configuration settings.
	@see TCameraConfigV02.
	*/
	inline void GetCamConfig(TDevCamCaptureMode aCaptureMode, TDes8& aConfigBuf);

	/**
	Get the current buffer configuration settings.
	@param aCaptureMode	The capture mode that the configuration request is for.
	@param aConfigBuf	A packaged TSharedChunkBufConfigBase derived object which will be filled with the configuration settings.
	@see TSharedChunkBufConfigBase.
	*/
	inline void GetBufferConfig(TDevCamCaptureMode aCaptureMode, TDes8& aConfigBuf);

	/**
	Set the current buffer configuration settings - using an existing shared chunk.
	The client supplies an existing shared chunk which is to be used by the driver as the play buffer.
	Any shared chunk previously created by the driver will be closed by it.
	The configuration cannot be changed unless image capture is disabled.
	@param aCaptureMode	The capture mode that the configuration is for.
	@param aBufferConfigBuf	A packaged TSharedChunkBufConfigBase derived object holding information on the buffer configuration
							settings of the chunk supplied.
	@param aChunk			A handle to the shared chunk which is to be used as the buffer. (This must be a
							valid handle for the calling thread).
	@return KErrNone if successful;
			KErrInUse if the playing of data is in progress;
			KErrBadHandle if the chunk handle was invalid;
			KErrArgument if any configuration values are invalid;
			otherwise one of the other system-wide error codes.
	@see TCameraConfigV02.
	*/
	inline TInt SetBufConfigChunkOpen(TDevCamCaptureMode aCaptureMode, const TDesC8& aBufferConfigBuf, RChunk& aChunk);

	/**
	Closes the chunk associated with a given capture mode, and frees the associated buffers.  The chunk being closed,
	should have been allocated by the device driver by a call to SetBufConfigChunkCreate().
	@param aCaptureMode The capture mode for which to close the chunk.
	@return	KErrNone if successful.
			KErrInUse if an attempt has been made to free the memory and chunk while they are in use.
			Otherwise one of the other system-wide error codes.
	*/
	inline TInt ChunkClose(TDevCamCaptureMode aCaptureMode);

	/**
	Start the image capture process in the capture mode supplied.
	If the driver is in free running mode then it will commence capturing frames - cycling between
	each buffer available to it until Stop() is called. In one shot mode the driver postpones the capturing
	of frames until a NotifyNewImage() request is received.
	@return KErrNone if successful;
			KErrNotReady if SetConfig() has not been previously called;
			otherwise one of the other system-wide error codes.
	@pre The driver must have been previousely initialised by a call to SetConfigNN()
	*/
	inline TInt Start();

	/**
	End the image capturing process.
	Also performs NotifyNewImageCancel()
	@return KErrGeneral if Start() has not been previously called;
			KErrNone otherwise.
	*/
	inline TInt Stop();

	/**
	Get the next available image.
	More than one request may be pending at any time.
	If the camera is enabled for image capture and image capture is not already in progress then the issuing
	of this request will start image capture mode.
	@param aStatus 	The request status which is signaled when an image is available (or an error occurs).
					If the request is successful then this result value is the offset within the shared chunk
					where the capture image resides. Alternatively, if an error occurs it will be set to one of
					the system wide error values:
				 	KErrNotReady if Start() hasn't been previousely called;
				 	KErrInUse if the client already has all the images buffers.
	@pre Image capturing must have been started with Start().
	*/
	inline void NotifyNewImage(TRequestStatus& aStatus);

	/**
	Cancels all outstanding NotifyNewImage() requests.
	All outstanding requests complete with KErrCancel.
	*/
	inline void NotifyNewImageCancel();

	/**
	Cancels a specific NotifyNewImage() request.
	The outstanding request completes with KErrCancel.
	@param aStatus The request status object associated with the request to be cancelled.
	*/
	inline void NotifyNewImageCancel(const TRequestStatus& aStatus);

	/**
	Release a buffer - making it available again for the driver to capture images into.
	@param aBufferId	The buffer id of the buffer that the image to be released is stored.
						This is a value returned by the NotifyNewImage() request when
						the buffer was claimed by the client.
	@return KErrNone if successful;
			KErrNotFound if the buffer id is invalid;
			otherwise one of the other system-wide error codes.
	*/
	inline TInt ReleaseBuffer(TInt aBufferId);

	/**
	Retrieves the offset to the start of the buffer using its id, in the specified capture mode.
	@param aCaptureMode	The capture mode that the buffer in the chunk is related to.
	@param aId			The id of the buffer in the chunk.
	@param aOffset		The offset, in bytes, of the start of the buffer within the chunk.
	@return KErrNone if successful;
			KErrNotFound if the id doesn't exist;
			otherwise one of the other system-wide error codes.
	*/
	inline TInt BufferIdToOffset(TDevCamCaptureMode aCaptureMode, TInt aId, TInt& aOffset);

	/**
	Sets the current capture mode. Submits the camera configuration	to the PDD.
	@param aCaptureMode	The capture mode that the camera switches to.	@see TDevCamCaptureMode.
	@return KErrNone if successful;
			otherwise one of the other system-wide error codes.
	*/
	inline TInt SetCaptureMode(TDevCamCaptureMode aCaptureMode);

	/**
	Queries the driver for the size of the structure to be passed to Caps().
	@return The size of the structure required to hold all capability information.
			otherwise one of the system-wide error codes.
	*/
	inline TInt CapsSize();

	/**
	Gets information regarding the frame sizes and frame rates supported for a given combination of capture mode and pixel format.
	The capture mode and pixel format chosen will be dependent upon the information returned by RDevCameraSc::Caps().
	@param	aCaptureMode The capture mode concerned.
	@param	aUidPixelFormat The UID of the pixel format mode concerned.
	@param	aFrameSizeCapsBuf	A packaged array of SDevCamFrameSize structures.
								This is a variable length array and must be large enough to hold all entries.
								Its size is determined by SDevCamPixelFormat::iNumFrameSizes, returned by RDevCameraSc::Caps(),
								multiplied by the size of SDevCamFrameSize.
	@return	KErrNone if successful;
			KErrArgument if an invalid capture mode or pixel format is specified, or if aFrameSizeCapsBuf is too small.
			otherwise one of the other system-wide error codes.
	*/
	inline TInt FrameSizeCaps(TDevCamCaptureMode aCaptureMode, TUidPixelFormat aUidPixelFormat, TDes8& aFrameSizeCapsBuf);
	

	/**
	Allows changing of the dynamic settings as specified in TDevCamDynamicAttributes.
	Checks locally the validity of the arguments passed so as to increase performance by not
	forcing a context switch.
	Check the allowable range of the settings via the TCameraCapsV02::iDynamicRange member.

	@param aAttribute An enum identifying the dynamic attribute to change.
	@param aValue The attributes value within a valid range.
	@return KErrNone if successful, KErrNotSupported if not supported, 
			KErrArgument if aValue out of range, KErrBadName is aAttribute not valid setting.
			Otherwise, one of the system wide error codes.
	@see TDevCamDynamicAttributes
	@see TCameraCapsV02
	*/
	inline TInt SetDynamicAttribute(TDevCamDynamicAttributes aAttribute, TUint aValue);

private:

	/** 
	Capability of Sensor. 
	Kept here for performance issues, i.e. to avoid context switches.
	*/
	TCameraCapsV02 *iCameraCaps;

	/** 
	Size of Camera Capabiliy struct. 
	Kept here for performance issues, i.e. to avoid context switches.
	*/
	TInt iCapsSize;
	
#endif	// __KERNEL_MODE__
	};




#include <d32camerasc.inl>

#endif	// __D32CAMERASC_H__
