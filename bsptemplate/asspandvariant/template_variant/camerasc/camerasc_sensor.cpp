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
// template\template_variant\camerasc\camerasc_sensor.cpp
// Implementation of the template shared chunk camera physical device driver (PDD).
// This file is part of the Template Base port
// 
//

/**
 @file
*/

#include "camerasc_plat.h"
#include <kernel/cache.h>

// XXX - Temporary structure containing a logo to be displayed.  Remove this when
//       changing this template into a "real" camera driver
#include "logoyuv2.cpp"


#define RGBtoBGR565(red, green, blue) (((blue & 0xf8) << 8) | ((green & 0xfc) << 3) | ((red & 0xf8) >> 3));

#define YUVtoYUV565(luma, blueC, redC) (((luma & 0xf8) << 8) | ((blueC & 0xfc) << 3) | ((redC & 0xf8) >> 3));

// Frame sizes and their associated frame rates supported by the Template sensor.  This selection was
// obtained by observation of typical formats supported by phones already on the market;  It is arbitrary
// and can be easily added to if desired
static const SDevCamFrameSize FrameSizes[] =
	{
		{ 320, 240, 1, 30 }	,		// QVGA - 0.075 MP
		// XXX: Although not used in this template driver, the following are suggested standard frame sizes
		// that should be implemented in your camera driver, as well as 320 x 240 above.  Remember to change
		// KNumFrameSizes below if you change the number of sizes defined in here!
		{ 640, 480, 1, 30 },		// VGA - 0.3 MP
		{ 800, 600, 1, 30 },		// SVGA - 0.5 MP
		{ 1024, 768, 1, 30 },		// XGA - 0.8 MP
		{ 2048, 1536, 1, 15 },	// QXGA - 3 MP
		//{ 2560, 1600, 1, 30 }		// WQXGA - 4.1 MP
	};

// This constant must be updated if the number of frame sizes listed above changes
static const TInt KNumFrameSizes = sizeof(FrameSizes) / sizeof(SDevCamFrameSize);

// Pixel formats supported by the three different capture modes.  These are mapped onto the appropriate
// array of supported frame rates by the FrameSizeCaps() function
static const SDevCamPixelFormat PixelFormats[] =
	{
		// Image pixel formats
		{ EUidPixelFormatYUV_422Interleaved, KNumFrameSizes, 2 },
		
		// Video pixel formats
		{ EUidPixelFormatYUV_422Interleaved, KNumFrameSizes, 2 },

		// View finder pixel formats
		{ EUidPixelFormatYUV_422Interleaved, KNumFrameSizes, 2 }

	};

// These constants must be updated if the number of pixel formats listed above changes
static const TInt KNumImagePixelFormats = 1;
static const TInt KNumVideoPixelFormats = 1;
static const TInt KNumViewFinderPixelFormats = 1;

// Alternate logo images after this many frames
static const TInt KAlternateLogoFrameInterval = 5;

static void ImageTimerCallback(TAny* aSensorIf)
	{
	DTemplateSensorIf* sensor = (DTemplateSensorIf*) aSensorIf;

	// XXX - Call the buffer done function in the sensor class.  In this case we are just emulating the
	// interrupt and DFC callback that would happen when an image is captured, so we always pass in KErrNone.
	// In a real driver, we would read the hardware here to check that the capture happened successfully and
	// would pass in the appropriate error code
	sensor->BufferDoneCallback(KErrNone);
	}

/**
Saves a configuration specifying such details as dimensions and pixel format in which the sensor should
capture images.
@param	aConfig	A TCameraConfigV02 structure containing the settings to be used.
@return	KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DSensorIf::SetConfig(const TCameraConfigV02& aConfig)
	{
	// Manual settings for flash mode, focus, white balance etc. are not supported by the sensor,
	// so check for these and return KErrNotSupported if they have been requested
	if ((aConfig.iFlashMode != ECamFlashNone) ||
		(aConfig.iExposureMode != ECamExposureAuto) ||
		(aConfig.iZoom != 0) /*||
        (aConfig.iWhiteBalanceMode != ECamWBAuto) ||
		(aConfig.iContrast != ECamContrastAuto) ||
		(aConfig.iBrightness != ECamBrightnessAuto)*/)
		{
		// XXX: Remove this once support is addded for these modes
		return KErrNotSupported;
		}

	// As well as saving the configuration, also save copies of the width and height for easy access,
	// as they are accessed frequently, as well as the offset in bytes between lines
	iConfig = aConfig;
	iWidth = aConfig.iFrameSize.iWidth;
	iHeight = aConfig.iFrameSize.iHeight;
	iLineOffset = (iWidth * iConfig.iPixelFormat.iPixelWidthInBytes);

	return KErrNone;
	}

/**
Constructor for the Template sensor class.
*/

DTemplateSensorIf::DTemplateSensorIf(MSensorObserver& aObserver, TDfcQue* aDFCQueue)
	: iDFCQueue(aDFCQueue)
	{
	iObserver = &aObserver;
	iXDirection = iYDirection = 1;
	
	iCounter = 0;
	iFlipSwitch = EFalse;
	}

/**
Second stage constructor for the Template sensor class.

@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSensorIf::DoCreate()
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateSensorIf::DoCreate()"));

	TInt r = KErrNone;

	for (TInt index = 0; index < KTotalCameraRequests; ++index)
		{
		if ((iImageTimerDFCs[index] = new TDfc(ImageTimerCallback, this, iDFCQueue, 0)) == NULL)
			{
			r = KErrNoMemory;

			break;
			}
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateSensorIf::DoCreate() => Returning %d", r));

	return r;
	}

/**
Destructor for the Template sensor class.
*/
DTemplateSensorIf::~DTemplateSensorIf()
	{
	for (TInt index = 0; index < KTotalCameraRequests; ++index)
		{
		iImageTimers[index].Cancel();
		delete iImageTimerDFCs[index];
		}
	}

/**
Called by the underlying sensor class when an image has been captured.
@param aResult	KErrNone if the image was captured successfully, otherwise one of
				the other system wide error codes.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSensorIf::BufferDoneCallback(TInt aResult)
	{
	TInt r = KErrNone;

	NKern::LockedDec(iPendingRequests);

	TLinAddr linAddr;
	TPhysAddr physAddr;

	// Call the LDD to let it know that an image capture has completed.  If the LDD needs more images
	// to be captured, then it will return KErrNone and the virtual and physical addresses of the
	// next buffer to be filled will be returned in linAddr and physAddr respectively.  Note that as
	// will as starting a capture of an image in here, the LDD may also call CaptureNextImage() to start
	// capture as well
	r = iObserver->NotifyImageCaptureEvent(aResult, linAddr, physAddr);

	if (r == KErrNone)
		{
		iNextRequest = ((iNextRequest + 1) % KTotalCameraRequests);
		NKern::LockedInc(iPendingRequests);

		// XXX: Temporary code to be removed in a real driver.  Fill the buffer for testing
		//      with user side code
		FillBuffer(linAddr);

		// XXX: Send buffer to sensor.  Normally the address of the buffer passed in in aLinAddr and
		//      aPhysAddr would be programmed into the sensor and/or bus hardware here and an interrupt
		//      would be generated when the iamge had been captured into the buffer.  In this simulated
		//      driver we will use a nanokernel timer to simulate this process
		iImageTimers[iNextRequest].OneShot(iImageTimerTicks, *iImageTimerDFCs[iNextRequest]);
		}

	return r;
	}

/**
Fills a buffer with a white background with a moving logo on top.
@param aBuffer	Pointer to the buffer to be filled.
*/
void DTemplateSensorIf::FillBuffer(TLinAddr aBuffer)
	{
	const TUint8* LogoData = Logo.iPixelData;
	const TUint8* LogoData2 = Logo.iPixelData2;
	TInt index = 0;
	TInt numPixels = (iConfig.iFrameSize.iWidth * iConfig.iFrameSize.iHeight);
	TUint yC, uC, vC;
	TUint16* buffer = (TUint16*) aBuffer;

    // Alternate between the two logos for cheesy animation effect
    if( ++iCounter == KAlternateLogoFrameInterval )
         {
         iFlipSwitch ^= 1;
         iCounter = 0;
         }
	
	
	// Set the "photo" background to be all white
	memset(buffer, 0xff, (numPixels * 2));

	// Point to the correct location in the buffer at which to render the logo
	buffer += ((iY * iConfig.iFrameSize.iWidth) + iX);

	// Iterate through the data for the logo and copy it into the "photo"
	for (TUint y = 0; y < Logo.iHeight; ++y)
		{
		for (TUint x = 0; x < Logo.iWidth; ++x)
			{
			// The logo is in 24 bit BGR format so read each pixel and convert it to 16 bit BGR565
			// before writing it into the "photo" buffer
			if( iFlipSwitch )
			    {
                yC = LogoData[index];
                uC = LogoData[index + 1];
                vC = LogoData[index + 2];
			    }
			else
                {
                yC = LogoData2[index];
                uC = LogoData2[index + 1];
                vC = LogoData2[index + 2];
                }

			*buffer++ = YUVtoYUV565(yC, uC, vC);
			// Point to the next source pixel
			index += 3;
			}

			// Point to the start of the next line in the buffer, taking into account that the logo
			// is narrower than the buffer
			buffer += (iConfig.iFrameSize.iWidth - Logo.iWidth);
		}

	// Bounce the logo around in the X direction.  This will take effect the next time this is called
	iX += iXDirection;

	if (iX <= 0)
		{
		iX = 0;
		iXDirection = -iXDirection;
		}
	else if (iX >= (TInt) (iConfig.iFrameSize.iWidth - Logo.iWidth))
		{
		iX = (iConfig.iFrameSize.iWidth - Logo.iWidth);
		iXDirection = -iXDirection;
		}

	// Bounce the logo around in the Y direction.  This will take effect the next time this is called
	iY += iYDirection;

	if (iY <= 0)
		{
		iY = 0;
		iYDirection = -iYDirection;
		}
	else if (iY >= (TInt) (iConfig.iFrameSize.iHeight - Logo.iHeight))
		{
		iY = (iConfig.iFrameSize.iHeight - Logo.iHeight);
		iYDirection = -iYDirection;
		}

	// Now flush the cache to memory, taking into account the size of each pixel.  This is not normally
	// necessary but given that we are emulating a camera driver in software we must ensure that the
	// cache is flushed to memory.  This is because in a real driver the buffer will have been filled
	// by DMA so upon return to the LDD, the LDD will discard the contents of the cache to ensure the
	// DMA-written data is ok.  In the case of filling the buffer using the CPU in this virtual camera
	// driver, that would result in the data being discarded!
	Cache::SyncMemoryBeforeDmaWrite((TLinAddr) aBuffer, (numPixels * iConfig.iPixelFormat.iPixelWidthInBytes));
	}

/**
Based on the capture mode and pixel format passed in, copies an array of supported SFrameSize
structures into a buffer supplied by the LDD.  These frame sizes and their associated frame rates
will reflect the capabilities of the given capture mode and pixel format.
@param aCaptureMode			The capture mode for which to obtain the supported frame sizes.
@param aUidPixelFormat		The UID of the pixel format (as defined in \epoc32\include\pixelformats.h)
							for which to obtain the supported frame sizes.
@param aFrameSizeCapsBuf	A reference to a descriptor that contains a buffer into which to place
							the frame size structures.  It is up to the LDD to ensure that this is
							large enough to hold all of the frame sizes.
@return Always KErrNone.
*/
TInt DTemplateSensorIf::FrameSizeCaps(TDevCamCaptureMode /*aCaptureMode*/, TUidPixelFormat /*aUidPixelFormat*/, TDes8& aFrameSizeCapsBuf)
	{
	TPtrC8 sourceFrameSizes((const TUint8*) FrameSizes, sizeof(FrameSizes));

	// Ensure the buffer passed in from the LDD is large enough and copy the requested frame sizes
	if (aFrameSizeCapsBuf.Size() < sourceFrameSizes.Size())
		{
		Kern::Printf("*** ECapsBufferTooSmall: %d vs %d",
				aFrameSizeCapsBuf.Size(),
				sourceFrameSizes.Size());
		Kern::Fault("camerasc", ECapsBufferTooSmall);
		}
	
	//__ASSERT_DEBUG((aFrameSizeCapsBuf.Size() >= sourceFrameSizes.Size()), Kern::Fault("camerasc", ECapsBufferTooSmall));
	aFrameSizeCapsBuf = sourceFrameSizes;

	return KErrNone;
	}

/**
Allocates a buffer large enough to hold the TCameraCapsV02 structure and its succeeding array of
pixel formats, and populates the structure and array with information about the capabilities of
the sensor.
@param aCameraCaps	Reference to a pointer into which to place the pointer to allocated buffer
@return	Size of the capabilities structure if successful, otherwise one of the other system wide
		error codes.
*/
TInt DTemplateSensorIf::GetCaps(TCameraCapsV02*& aCameraCaps)
	{
	// Allocate a buffer large enough to hold the TCameraCapsV02 structure and the array of pixel formats
	// that will follow it
	TInt r = (sizeof(TCameraCapsV02) + sizeof(PixelFormats));
	TUint8* capsBuffer = new TUint8[r];

	if (capsBuffer)
		{
		aCameraCaps = (TCameraCapsV02*) capsBuffer;

		// No special modes are supported at the moment
		aCameraCaps->iFlashModes = ECamFlashNone;
		aCameraCaps->iExposureModes = ECamExposureAuto; // or None?
		// do we still need whitebalance mode filed?
		aCameraCaps->iWhiteBalanceModes = ECamWBAuto | ECamWBDaylight | ECamWBCloudy | ECamWBTungsten | ECamWBFluorescent | ECamWBFlash | ECamWBSnow | ECamWBBeach;
		aCameraCaps->iMinZoom = 0;
		aCameraCaps->iMaxZoom = 0;
		aCameraCaps->iCapsMisc = KCamMiscContrast | KCamMiscBrightness | KCamMiscColorEffect;

		// There isn't really such thing as inwards or outwards orientation on an SDP, but we'll pretend it's
		// an outwards facing camera
		aCameraCaps->iOrientation = ECamOrientationOutwards;

		// Initialise the number of different pixel formats supported
		aCameraCaps->iNumImagePixelFormats = KNumImagePixelFormats;
		aCameraCaps->iNumVideoPixelFormats = KNumVideoPixelFormats;
		aCameraCaps->iNumViewFinderPixelFormats = KNumViewFinderPixelFormats;

		for (TInt i = 0; i < ECamAttributeMax; i++)
		    {
		    if (ECamAttributeColorEffect == (TDevCamDynamicAttribute)(i))
		        {
		        // WhiteBalance
		        // In case of white balance, we shouldn't use MIN and MAX values as some of them in between MIN and MAX can be missed out.
		        // As this is fake driver, There doesn't seem to be any major issue though.
		        aCameraCaps->iDynamicRange[i].iMin = ECamWBAuto;
		        aCameraCaps->iDynamicRange[i].iMax = ECamWBBeach;
		        aCameraCaps->iDynamicRange[i].iDefault = ECamWBAuto;
		        }
		    else
		        {    
		        // TBC :: Contrast, Brightness
		        aCameraCaps->iDynamicRange[i].iMin = 0;
		        aCameraCaps->iDynamicRange[i].iMax = 6;
		        aCameraCaps->iDynamicRange[i].iDefault = 3;
		        }
		    }

		// Setup some descriptors pointing to the pixel format array and the array passed in by the LDD
		// (located at the end of the TCameraCapsV02 structure) and copy the pixel format array
		TPtrC8 sourcePixelFormats((const TUint8*) PixelFormats, sizeof(PixelFormats));
		TPtr8 destPixelFormats((capsBuffer + sizeof(TCameraCapsV02)), sizeof(PixelFormats), sizeof(PixelFormats));
		destPixelFormats = sourcePixelFormats;
		}
	else
		{
		r = KErrNoMemory;
		}

	return r;
	}

/**
Powers up the sensor hardware.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSensorIf::RequestPower()
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateSensorIf::RequestPower()"));

	TInt r = KErrNone;

	__KTRACE_CAM(Kern::Printf("< DTemplateSensorIf::RequestPower() => Returning %d", r));

	return r;
	}

/**
Powers down the sensor hardware.
@return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DTemplateSensorIf::RelinquishPower()
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateSensorIf::RelinquishPower()"));

	TInt r = KErrNone;

	__KTRACE_CAM(Kern::Printf("< DTemplateSensorIf::RelinquishPower() => Returning %d", r));

	return r;
	}

/**
Begins capture of the next image into the buffer provided.  This function assumes that
Start() has already been called to start capture.  However, Stop() may also have been
subsequently called (for example to pause capture) and in this case, this function will
handle restarting the sensor.
@param aLinAddr		A virtual pointer to the buffer into which to capture the image.
@param aPhysAddr	A physical pointer to the buffer into which to capture the image.
					This points to the same memory as aLinAddr.
@return KErrNone if successful.
		KErrNotReady if there are no free requests to capture the image.
		Otherwise one of the other system wide error codes.
*/
TInt DTemplateSensorIf::CaptureNextImage(TLinAddr aLinAddr, TPhysAddr /*aPhysAddr*/)
	{
	TInt r = KErrNone;

	// Only start capturing the next image if there are any pending request slots available
	if (iPendingRequests < KTotalCameraRequests)
		{
		// Queue a transfer on the next available channel and indicate that the channel is
		// in use
		iNextRequest = ((iNextRequest + 1) % KTotalCameraRequests);
		NKern::LockedInc(iPendingRequests);

		// XXX: Temporary code to be removed in a real driver.  Fill the buffer for testing
		//      with user side code.  This is to simulate an image being captured into the buffer that
		//      has been passed in in aLinAddr.  As well as aLinAddr, which points to the virtual
		//      address of the buffer, the LDD will pass in the physical address as well, in aPhysAddr.
		//      Depending on the underlying sensor hardware and/or bus in use, you will have to choose
		//      which of these to use
		FillBuffer(aLinAddr);

		// XXX: Send buffer to sensor.  Normally the address of the buffer passed in in aLinAddr and
		//      aPhysAddr would be programmed into the sensor and/or bus hardware here and an interrupt
		//      would be generated when the iamge had been captured into the buffer.  In this simulated
		//      driver we will use a nanokernel timer to simulate this process
		iImageTimers[iNextRequest].OneShot(iImageTimerTicks, *iImageTimerDFCs[iNextRequest]);

		// If capturing has not yet started or has been paused by Stop(), start it
		if (!(iEnabled))
			{
			iEnabled = ETrue;
			}
		}
	else
		{
		r = KErrNotReady;
		}

	return r;
	}

/**
Begins capture of the first image into the buffer provided.  This function is similar to
CaptureNextImage(), except that it will perform any extra sensor intitialisation required
to start capture.
@param aCaptureMode	The capture mode for which to start capturing.
@param aLinAddr		A virtual pointer to the buffer into which to capture the image.
@param aPhysAddr	A physical pointer to the buffer into which to capture the image.
					This points to the same memory as aLinAddr.
@return KErrNone if successful
		KErrInUse if capture is already under way.
		KErrNotSupported if the frame size and/or frame rate are out of range.
		Otherwise one of the other system wide error codes.
*/
TInt DTemplateSensorIf::Start(TDevCamCaptureMode /*aCaptureMode*/, TLinAddr aLinAddr, TPhysAddr aPhysAddr)
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateSensorIf::Start()"));

	TInt r = KErrNone;

	// XXX - In a real camera driver, in here we would initialise start the capturing process in here.
	//       When an image is captured, the sensor hardware (or maybe the CSI bus) will generate an
	//       which will then be enqueued into the DFC queue that was passed into the constructor of
	//       the sensor class.  It is important to do the DFC processing in this DFC queue rather than
	//       a separate one because it ensures that fucntions in the PDD and LDD are called in a serialised
	//       manner, without the need for mutexts.  In this example camera driver we will convert the
	//       framerate into a nanokernel tick count and will use an NTimer.OneShot() call to simulate
	//       the sensor interrupt and DFC callback.  Divides are slow so we'll calculate the tick count
	//       here and will save it for later use
	iImageTimerTicks = ((1000000 / NKern::TickPeriod()) / iConfig.iFrameRate);

	// XXX - Once the one off hardware initialisation has been done for starting a new capture, then
	//       subsequent captures can usually reuse the same code in CaptureNextImage() for starting
	//       the next capture
	r = CaptureNextImage(aLinAddr, aPhysAddr);

	__KTRACE_CAM(Kern::Printf("< DTemplateSensorIf::Start() => Returning %d", r));

	return r;
	}

/**
Stops capturing any image capture that is currently in progress.  This function will act
more like a Pause() than a Stop() capturing can be restarted from where it was stopped.
*/
TInt DTemplateSensorIf::Stop()
	{
	__KTRACE_CAM(Kern::Printf("> DTemplateSensorIf::Stop()"));

	iEnabled = EFalse;
	iPendingRequests = iNextRequest = 0;

	// XXX - Cancel all of our pending image timer callbacks.  In a real driver we would write to the
	//       sensor and/or bus hardware here to cause them to cancel any pending image captures
	for (TInt index = 0; index < KTotalCameraRequests; ++index)
		{
		iImageTimers[index].Cancel();
		}

	__KTRACE_CAM(Kern::Printf("< DTemplateSensorIf::Stop()"));

	return KErrNone;
	}
