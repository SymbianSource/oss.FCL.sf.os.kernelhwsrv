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
// This test should not depend on any external data files and should work with
// the default (empty) epoc.ini file.
// The test should be run without the Graphics GCE enabled but with the Base GCE
// driver enabled. On the emulator this can be done by launcing with -Dtextshell --
// and on the H4 build a textshell ROM with -DSYMBIAN_BASE_USE_GCE but NOT 
// -DSYMBIAN_GRAPHICS_USE_GCE
// In the visual tests some flickering may occur due to updates to the console. On
// the emulator it is possible to configure a second screen so that the console updates
// will only happen on one screen. The test automatically runs on every screen available.
// 
//

#define __E32TEST_EXTENSION__

#include <dispchannel.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32def.h>
#include <e32def_private.h>
#include <e32svr.h>
#include <e32test.h>
#include <pixelformats.h>
#include <hal.h>

RTest test(_L("Display Channel device driver unit tests"));

/** Maximum probable pixel resolution width or height */
static const TInt KMaxExpectedPixelRes = 10000;

/** Unlikely to ever have a > 1000 Hz refresh rate */
static const TInt KMaxExpectedRefreshRate = 1000;

/** Time (in microseconds) for each visual test */
static const TInt KDrawWaitTime = 1000000;

/** Array of supported rotations */
static const RDisplayChannel::TDisplayRotation KRotations[] = {
		RDisplayChannel::ERotationNormal, 
		RDisplayChannel::ERotation90CW,
		RDisplayChannel::ERotation180,
		RDisplayChannel::ERotation270CW};
static const TInt KNumRotations = sizeof(KRotations) / sizeof(RDisplayChannel::TDisplayRotation);

/** Array of pixel formats to try for visual test */
static const RDisplayChannel::TPixelFormat KPixelFormats[] = {
	EUidPixelFormatYUV_422Interleaved16bit,	// not supported on emulator but should not be a fatal error
	EUidPixelFormatXRGB_4444,	
	EUidPixelFormatARGB_4444,
	EUidPixelFormatRGB_565,
	EUidPixelFormatXRGB_8888,
	EUidPixelFormatARGB_8888,
	EUidPixelFormatARGB_8888_PRE	
};
static const TInt KNumPixelFormats = sizeof(KPixelFormats) / sizeof(RDisplayChannel::TPixelFormat);

/**
Encapsulates display related HAL information. 
*/
class THalDisplayInfo 
	{
public:
	TBool iIsMono;
	TBool iIsPalettized;
	TInt iBitsPerPixel;
	TInt iMemoryAddress;
	TInt iMemoryHandle;
	TInt iState;
	TInt iColors;
	TInt iXPixels;
	TInt iYPixels;
	TInt iXTwips;
	TInt iYTwips;
	TInt iNumModes;	
	TInt iMode;
	TInt iOffsetBetweenLines;
	TInt iOffsetToFirstPixel;
	TBool iIsPixelOrderRGB;
	TBool iIsPixelOrderLandscape;
	};

/**
Helper class that waits for RDisplayChannel asynchronous requests and
can cancel them if necessary. The purpose of this class is so that the main 
test class can create an asynchronous request and also simulate the completion
of that request e.g. faking a display change event.
*/
class CAsyncHelper : public CActive
	{
public:
	inline CAsyncHelper(RDisplayChannel& aDisp);
	inline ~CAsyncHelper();
	inline TRequestStatus& Status();
	void WaitForOperation(TInt* aResult);
private:
	// From CActive
	inline void DoCancel();
	inline void RunL();	
private:
	RDisplayChannel& iDisp;
	TInt* iResult;
	};

inline CAsyncHelper::CAsyncHelper(RDisplayChannel& aDisp) : CActive(EPriorityHigh), iDisp(aDisp) {CActiveScheduler::Add(this);}
inline CAsyncHelper::~CAsyncHelper() {Deque();}
inline TRequestStatus& CAsyncHelper::Status() {return iStatus;}
// Writes the iStatus.Int() to the address defined by the client of this AO
inline void CAsyncHelper::RunL() {*iResult = iStatus.Int();}

void CAsyncHelper::WaitForOperation(TInt* aResult)
/**
Invokes SetActive() to wait for the asynchronous operation to complete. The completion
code is copied to the aResult when RunL is invoked.
@param aResult	out parameter that will be set to iStatus.Int()
*/
	{
	iResult = aResult; 	
	// Set the result to default value that is unlikely to be returned by the real API
	*aResult = KMaxTInt;
	SetActive();
	}

void CAsyncHelper::DoCancel()
	{
	// Driver should fail if cancel is called when there is not standing request
	// so cancel just attempts to cancel everything.
	iDisp.CancelGetCompositionBuffer();
	iDisp.CancelPostUserBuffer();
	iDisp.CancelWaitForPost();
	iDisp.NotifyOnDisplayChangeCancel();
	}

/**
Class to test device driver for RDisplayChannel
*/
class CDisplayChannelTest : public CActive
	{
	enum TTestState {
		ETestDisplayInfo,
		ETestCompositionBuffers,
		ETestUserBuffers,
		ETestRotations,
		ETestDisplayChange,
		ETestDisplayChangeDoCancel,
		ETestDisplayChangeCheckCancel,
		ETestGetCompositionBufferDoCancel,
		ETestGetCompositionBufferCheckCancel,
		ETestWaitForPostDoCancel,
		ETestWaitForPostCheckCancel,
		ETestResolutions,
		ETestPixelFormats,
		ETestBufferFormats,
		ETestV11inV10,
		EVisualTest,
		ETestSecondHandle,
		ETestBufferTransitions,
		ETestFinished
	};
	
	public:
		static CDisplayChannelTest* NewLC(TInt aScreenId);
		void Start();
		~CDisplayChannelTest();
		
	private:		
		// From CActive
		void DoCancel();
		TInt RunError(TInt aError);
		void RunL();

	private:		
		CDisplayChannelTest(TInt aScreenId);
		void CompleteSelf(TTestState aNextState);

		// The tests
		void CheckDisplayInfo();
		void CheckResolutions();
		void CheckPixelFormats();
		void CheckDisplayChange();
		void CheckCompositionBuffers();
		void CheckBufferFormat();
		void CheckUserBuffers();
		void CheckRotations();
		TBool IsValidRotation(RDisplayChannel::TDisplayRotation aRotation);
		TBool IsValidPixelFormat(RDisplayChannel::TPixelFormat aPixelFormat);		
		void CheckSetRotation(TUint aSupported, RDisplayChannel::TDisplayRotation aNewRotation);
		void CheckV11inV10();
		void VisualTest();
		void DrawLegacyBuffer(TInt aStep);
		void DrawFillToMemory(TUint8* aFirstPixelAddr, TInt aOffsetBetweenLines, 
				RDisplayChannel::TPixelFormat aPixelFormat, TInt aWidth, TInt aHeight, TInt aStep);
		void DrawCompositionBuffer(
				RDisplayChannel::TPostCount& aPostCount, 
				RDisplayChannel::TBufferFormat aBufferFormat,
				RDisplayChannel::TDisplayRotation aRotation, TInt aStep);
		void GetHalDisplayInfo();
		void CheckSecondHandle();
		void TestBufferTransitions();

	private:
		RDisplayChannel iDisp;			/// handle to display channel device driver
		TVersion iVersion;				/// version number of disp channel driver interface
		THalDisplayInfo iHalInfo;		/// info about legacy buffer from HAL
		TInt iScreenId;					/// run tests on each screen
		TTestState iState;				/// the current test
		CAsyncHelper *iAsyncHelper;				
		TInt iAsyncHelperResult;		/// set to iAyncHelper::iStatus.Int()
		RArray<RDisplayChannel::TResolution> iResolutions;
		RArray<RDisplayChannel::TPixelFormat> iPixelFormats;
		TInt iVisualTestFormatIndex;		/// index of the current pixel format in visual test 
		TInt iVisualTestRotationIndex;		/// index of the current rotation in the visual test
		TUint iDummyCompositionBuffer;		/// dummy var used to test cancel of GetCompositionBuffer
		RDisplayChannel::TPostCount iDummyPostCount;	/// dummy var used to test CancelWaitForPost 
	};

// Gets a HAL value, logs the result and errors if HAL::Get failed
#define DBG_HAL(DEVICE, ATT, VAL, ERR, IN) \
	{ \
	VAL = IN;\
	ERR = HAL::Get(DEVICE, ATT, VAL); \
	test.Printf(_L(#ATT)); \
	test.Printf(_L(" device %d err = %d, val = %d\n"), DEVICE, ERR, VAL); \
	test_KErrNone(ERR); \
	}

void CDisplayChannelTest::GetHalDisplayInfo()
/**
Retrieves display related HAL settings. This also initialises the legacy buffer by retrieving
HAL::EDisplayMemoryAddress
*/
	{
	TInt err = KErrNotSupported;	
		
	DBG_HAL(iScreenId, HAL::EDisplayMemoryAddress, iHalInfo.iMemoryAddress, err, 0);
			
	iHalInfo.iMemoryHandle = 0;
	err = HAL::Get(iScreenId, HAL::EDisplayMemoryHandle, iHalInfo.iMemoryHandle);
	test(err == KErrNone || err == KErrNotSupported);
	test.Printf(_L("HAL::EDisplayMemoryHandle returned err %d\n"), err);
	if (err == KErrNone)
		{
		// Handle is not needed so don't leak it
		RHandleBase h;
		h.SetHandle(iHalInfo.iMemoryHandle);
		h.Close();
		}
	
	// This is mostly for information purposes to ensure the legacy buffer is sane.
	DBG_HAL(iScreenId, HAL::EDisplayState, iHalInfo.iState, err, 0);
	DBG_HAL(iScreenId, HAL::EDisplayColors, iHalInfo.iColors, err, 0);
	DBG_HAL(iScreenId, HAL::EDisplayXPixels, iHalInfo.iXPixels, err, 0);
	DBG_HAL(iScreenId, HAL::EDisplayYPixels, iHalInfo.iYPixels, err, 0);
	DBG_HAL(iScreenId, HAL::EDisplayXTwips, iHalInfo.iXTwips, err, 0);
	DBG_HAL(iScreenId, HAL::EDisplayYTwips, iHalInfo.iYTwips, err, 0);	
	DBG_HAL(iScreenId, HAL::EDisplayIsPixelOrderRGB, iHalInfo.iIsPixelOrderRGB, err, 0);
	DBG_HAL(iScreenId, HAL::EDisplayIsPixelOrderLandscape, iHalInfo.iIsPixelOrderLandscape, err, 0);
	
	DBG_HAL(iScreenId, HAL::EDisplayNumModes, iHalInfo.iNumModes, err, 0);		
	DBG_HAL(iScreenId, HAL::EDisplayMode, iHalInfo.iMode, err, 0);
	
	// Get info for current display mode
	DBG_HAL(iScreenId, HAL::EDisplayIsMono, iHalInfo.iIsMono, err, iHalInfo.iMode);
	DBG_HAL(iScreenId, HAL::EDisplayBitsPerPixel, iHalInfo.iBitsPerPixel, err, iHalInfo.iMode);
	DBG_HAL(iScreenId, HAL::EDisplayOffsetBetweenLines, iHalInfo.iOffsetBetweenLines, err, iHalInfo.iMode);
	DBG_HAL(iScreenId, HAL::EDisplayOffsetToFirstPixel, iHalInfo.iOffsetToFirstPixel, err, iHalInfo.iMode);
	DBG_HAL(iScreenId, HAL::EDisplayIsPalettized, iHalInfo.iIsPalettized, err, iHalInfo.iMode);
	}

CDisplayChannelTest::CDisplayChannelTest(TInt aScreenId)
/**
Constructor
@param aScreenId	the screen number to run the test on
*/
	: CActive(EPriorityStandard), iScreenId(aScreenId)
	{	
	TVersion versionRequired = iDisp.VersionRequired();
	test.Printf(_L("*** Opening display channel for screen %d. Test compiled against version %d.%d.%d ***\n"),
			iScreenId, versionRequired.iMajor, versionRequired.iMinor, versionRequired.iBuild);
	TInt err = iDisp.Open(iScreenId);
	test_KErrNone(err);
	
	test.Printf(_L("Successfully opened display channel for screen %d\n"), iScreenId);

	// This test should be updated if a change to the driver requires a version change
	err = iDisp.Version(iVersion);
	if (err == KErrNotSupported)
		{
		test.Printf(_L("Version API not supported. Assuming v1.0.0\n"));
		iVersion.iMajor = 1;
		iVersion.iMinor = 0;
		iVersion.iBuild = 0;
		}
	else
		{
		test.Printf(_L("Display channel driver version %d.%d.%d\n"), 
				iVersion.iMajor, iVersion.iMinor, iVersion.iBuild);
		test_KErrNone(err);
		}
	test(iVersion.iMajor >= 1 && iVersion.iMinor >= 0);		
	GetHalDisplayInfo();
	CActiveScheduler::Add(this);
	
	iAsyncHelper = new CAsyncHelper(iDisp);
	test_NotNull(iAsyncHelper);
	}

CDisplayChannelTest::~CDisplayChannelTest()
/**
Destructor
*/
	{
	Deque();
	delete iAsyncHelper;
	iPixelFormats.Close();
	iResolutions.Close();
	iDisp.Close();	
	}

CDisplayChannelTest* CDisplayChannelTest::NewLC(TInt aScreenId)
/**
Factory method that creates a new instance of the screen
display channel unit test object and places a pointer to this on the cleanup stack

@param aScreenId	the screen number to run the test on
@return	a pointer to the new CDisplayTest object.
*/
	{
	CDisplayChannelTest* self = new(ELeave) CDisplayChannelTest(aScreenId);
	CleanupStack::PushL(self);
	return self;
	}

void CDisplayChannelTest::CheckDisplayInfo()
/**
Check the values returned by CheckDisplayInfo
*/
	{
	test.Next(_L("Test GetDisplayInfo"));
	TPckgBuf<RDisplayChannel::TDisplayInfo> infoPkg;

	test_KErrNone(iDisp.GetDisplayInfo(infoPkg));

	// This test only works with 24 and 32 BPP displays and crashes otherwise.  Test for this and display
	// a nice human readable message rather than just crashing
	if ((infoPkg().iBitsPerPixel != 24) && (infoPkg().iBitsPerPixel != 32))
		{
		TBuf<256> message;

		message.Format(_L("*** Error! %d bits per pixel displays are not supported. ***\n*** Please configure your ROM to use 24 or 32 bits per pixel.  ***\n"), infoPkg().iBitsPerPixel);
		test.Printf(message);

		// And fail the test for the benefit of automated ONB tests
		test_Equal(infoPkg().iBitsPerPixel, 24);
		}

	test_Compare(infoPkg().iBitsPerPixel, >=, 1);
	test_Compare(infoPkg().iAvailableRotations, !=, 0);

	// check for invalid rotations i.e. those not defined by TRotation
	test((infoPkg().iAvailableRotations & 0xFFF0) == 0);

	// Check that the refresh rate field isn't garbage
	test_Compare(infoPkg().iRefreshRateHz, >=, 1);
	test_Compare(infoPkg().iRefreshRateHz, <=, KMaxExpectedRefreshRate);

	// Should always be at least one composition buffer
	test_Compare(infoPkg().iNumCompositionBuffers, >=, 1);
	}

void CDisplayChannelTest::CheckResolutions()
/**
 Validate that the APIs to get / set resolutions.

 Tests<br>
 NumberOfResolutions, GetResolutions, GetResolution, GetRotation
 */
	{
	test.Next(_L("Test NumberOfResolutions, GetResolutions, GetResolution, GetRotation"));

	// Get and reserve space for expected number of resolutions
	TInt n = iDisp.NumberOfResolutions();
	test_Compare(n, >=, 1);
	
	iResolutions.Reset();
	test_KErrNone(iResolutions.Reserve(n));
	for (TInt i = 0; i < n; ++i)
		{
		test_KErrNone(iResolutions.Append(RDisplayChannel::TResolution(TSize(0,0), TSize(0,0), 0)));
		}
	
	// Retrieve the resolutions and make sure the number of resolutions returned matches the 
	// expected number. It is assumed that the display state won't be changed during the execution
	// of this test.
	TInt actualResolutions = 0;	
	TPtr8 resPtr(reinterpret_cast<TUint8*>(&iResolutions[0]), 
			sizeof(RDisplayChannel::TResolution) * n, sizeof(RDisplayChannel::TResolution) * n);
	test_KErrNone(iDisp.GetResolutions(resPtr, actualResolutions));
	test_Equal(n, actualResolutions);	

	test.Printf(_L("Supported resolutions"));
	for (TInt res = 0; res < n; ++res)
		{
		RDisplayChannel::TResolution& r = iResolutions[res];
		test.Printf(_L("pixelX = %d heightX = %d twipsX = %d twipsY = %d flags = 0x%08x\n"), 
				r.iPixelSize.iWidth, r.iPixelSize.iHeight, r.iTwipsSize.iWidth, r.iTwipsSize.iHeight, r.iFlags);		
		
		// If either pixel height or pixel width is zero then both must be zero
		// If either pixel height or pixel width is non-zero then both must be positive
		test((r.iPixelSize.iHeight == 0 && r.iPixelSize.iWidth == 0) ||
			 (r.iPixelSize.iHeight > 0 && r.iPixelSize.iWidth > 0));

		// Test resolutions are sane
		test(r.iPixelSize.iHeight <= KMaxExpectedPixelRes && r.iPixelSize.iWidth <= KMaxExpectedPixelRes);

		// If either twips height or pixel width is zero then both must be zero
		// If either twips height or pixel width is non-zero then both must be positive
		test((r.iTwipsSize.iHeight == 0 && r.iTwipsSize.iWidth == 0) ||
			 (r.iTwipsSize.iHeight > 0 && r.iTwipsSize.iWidth > 0));
		
		// twips resolution can be zero iff pixel resolution is also zero
		test((r.iPixelSize.iHeight == 0 && r.iTwipsSize.iHeight == 0) ||
			 (r.iPixelSize.iHeight > 0  && r.iTwipsSize.iHeight > 0));

		// At least one rotation must be supported. Ignore other bits in the flags field
		test(r.iFlags & RDisplayChannel::ERotationAll != 0);
		}

	// Get current resolution in pixels
	TSize currentResolution;
	test_KErrNone(iDisp.GetResolution(currentResolution));

	// Get current resolution in twips
	TSize currentTwips;
	test_KErrNone(iDisp.GetTwips(currentTwips));

	RDisplayChannel::TDisplayRotation currentRotation = iDisp.CurrentRotation();
	test(IsValidRotation(currentRotation));

	// The current resolution and rotation must be in the list of supported resolutions
	TBool foundCurrentRes = EFalse;
	for (TInt j = iResolutions.Count() - 1; j >= 0; --j)
		{
		if (iResolutions[j].iPixelSize == currentResolution &&
			iResolutions[j].iTwipsSize == currentTwips &&
			iResolutions[j].iFlags & currentRotation)
			{
			foundCurrentRes = ETrue;
			break;
			}
		}
	test(foundCurrentRes);
	
	// Now and try every supported resolution
	TInt err;
	for (TInt k = iResolutions.Count() - 1; k >= 0; --k)
		{
		err = iDisp.SetResolution(iResolutions[k].iPixelSize);
		test(err == KErrNone || err == KErrNotSupported);
		}
	// attempt to set back to original resolution, this could fail
	err = iDisp.SetResolution(currentResolution);
	test(err == KErrNone || err == KErrNotSupported);
	}

void CDisplayChannelTest::CheckPixelFormats()
/**
 Validates that the pixel format APIs are sane/consistent.

 In version 1.1 the APIs are just stubs that return KErrNotSupported
 */
	{
	test.Next(_L("Test NumberOfPixelFormats, GetPixelFormats"));

	// At least one pixel format must be supported
	TInt n = iDisp.NumberOfPixelFormats();

	if (iVersion.iMajor == 1 && iVersion.iMinor <= 1)
		{
		test_Compare(n, ==, KErrNotSupported);
		n = 1; // Override return to test stub for GetPixelFormats
		}
	else
		{
		test_Compare(n, >=, 1);
		}

	TInt err = iPixelFormats.Reserve(n);
	test_KErrNone(err);
	for (TInt i = 0; i < n; ++i)
		{
		test_KErrNone(iPixelFormats.Append(-1));
		}
	TPtr8 pixelFormatsPtr(reinterpret_cast<TUint8*>(&iPixelFormats[0]),
			sizeof(RDisplayChannel::TPixelFormat) * n, sizeof(RDisplayChannel::TPixelFormat) * n);		

	TInt actualFormats = -1;	
	if (iVersion.iMajor == 1 && iVersion.iMinor <= 1)
		{
		test_Compare(iDisp.GetPixelFormats(pixelFormatsPtr, actualFormats), ==, KErrNotSupported);
		}
	else
		{		
		test_KErrNone(iDisp.GetPixelFormats(pixelFormatsPtr, actualFormats));
		
		// The number of formats shouldn't have changed whilst this test is running
		test_Equal(n, actualFormats);			
		RArray<RDisplayChannel::TPixelFormat> pixelFormatsArray(
				reinterpret_cast<RDisplayChannel::TPixelFormat*>(&pixelFormatsPtr[0]), actualFormats);
		
		// Check the pixel formats returned are all valid
		for (TInt pf = pixelFormatsArray.Count() - 1; pf >= 0; --pf)
			{
			IsValidPixelFormat(pixelFormatsArray[pf]);
			}		
		}
	}

void CDisplayChannelTest::CheckDisplayChange()
/**
 Register for display change notification then immediately cancel.
 */
	{
	test.Next(_L("Test NotifyOnDisplayChange, NotifyOnDisplayChangeCancel"));
	// Cancel should be allowed even if NotifyOnyDisplayChange has not been called
	iDisp.NotifyOnDisplayChangeCancel();

	iDisp.NotifyOnDisplayChange(iAsyncHelper->Status());
	iAsyncHelper->WaitForOperation(&iAsyncHelperResult);
	}

void CDisplayChannelTest::DrawFillToMemory(
		TUint8* aFirstPixelAddr, 
		TInt aOffsetBetweenLines, 
		RDisplayChannel::TPixelFormat aPixelFormat, 
		TInt aWidth, 
		TInt aHeight, 
		TInt aStep)
/** 
 Draws a shaded fill to a memory region
 @param	aFirstPixelAddr			the address of the first pixel in the buffer.
 @param	aOffsetBetweenLines		offset between pixels at the start of each line
 @param aBpp					bits per pixel
 @param	aWidth					width of the region in pixels
 @param aHeight					height of the region in pixels
 @aStep	aStep					integer >= 1 to vary the pattern by test number 
 */	{
	test.Printf(_L("DrawFileToMemory\npixelformat = 0x%08x offsetbetweenlines = %d pixel address = 0x%08x width=%d height = %d\n"), 
			aPixelFormat, aOffsetBetweenLines, aFirstPixelAddr, aWidth, aHeight);	


	TInt xShadeMax = 0xFF;
	TInt yShadeMax = 0xFF;
	
	if (aPixelFormat == EUidPixelFormatRGB_565)
		{
		xShadeMax = 0x3F;	// 6 bits for green
		yShadeMax = 0x1F;
		}
	else if (aPixelFormat == EUidPixelFormatARGB_4444 || aPixelFormat == EUidPixelFormatXRGB_4444)
		{
		xShadeMax = 0x0F;
		yShadeMax = 0x0F;
		}
	
	aStep = Max(1, aStep);
	TUint8* lineAddr = aFirstPixelAddr;
	for (TInt y = 0; y < aHeight; ++y)
		{
		TInt yShade = (y * yShadeMax) / aHeight;
		TUint8* pixelAddr = lineAddr;
		for (TInt x = 0; x < aWidth; ++x)
			{
			TInt xShade = (x * xShadeMax) / aWidth;			
			TUint8 red = 0;
			TUint8 green = 0;
			TUint8 blue = 0;
			
			if ( aStep == 0 || y > aStep * 10)
				{
				// Green top left, blue bottom right
				green = static_cast<TUint8>(xShadeMax - xShade);
				blue = static_cast<TUint8>(yShade);
				}							
			else
				{
				// The size of the red band indicates different test steps			
				red = static_cast<TUint8>((yShadeMax * x) / aWidth);
				}
			
			if (aPixelFormat == EUidPixelFormatRGB_565)
				{
				*pixelAddr++ = static_cast<TUint8>(blue | (green << 5));
				*pixelAddr++ = static_cast<TUint8>((green >> 3) | (red << 3));
				}
			else if (aPixelFormat == EUidPixelFormatARGB_4444 || aPixelFormat == EUidPixelFormatXRGB_4444)
				{
				*pixelAddr++ = static_cast<TUint8>(blue | (green << 4));
				*pixelAddr++ = red;					
				}
			else if (aPixelFormat == EUidPixelFormatXRGB_8888 || aPixelFormat == EUidPixelFormatARGB_8888 
					|| aPixelFormat == EUidPixelFormatARGB_8888)
				{
				*pixelAddr++ = blue;
				*pixelAddr++ = green;
				*pixelAddr++ = red;
				*pixelAddr++ = 0xFF;	// unused
				}
			}
		lineAddr += aOffsetBetweenLines;
		}
	}

void CDisplayChannelTest::DrawLegacyBuffer(TInt aStep)
	{
	test.Printf(_L("DrawLegacyBuffer\n"));
	TInt oldMode;
	TInt err;
	err = HAL::Get(iScreenId, HAL::EDisplayMode, oldMode);
	for (TInt i = 0; i < iHalInfo.iNumModes; ++i)
		{
		// Attempt to set the legacy buffer to a mode supporting 32bit (RGBA or RGBX)
		TInt modeBpp = i;
		err = HAL::Get(iScreenId, HAL::EDisplayBitsPerPixel, modeBpp);
				
		test_KErrNone(err);				
		if ((modeBpp == 24 || modeBpp == 32))
			{
			TInt newMode = i;
			err = HAL::Set(iScreenId, HAL::EDisplayMode, newMode);
			break;
			}
		}
		
	GetHalDisplayInfo();
	err = HAL::Set(iScreenId, HAL::EDisplayMode, oldMode);
	TUint8* firstPixelAddr = reinterpret_cast<TUint8*>(iHalInfo.iMemoryAddress + iHalInfo.iOffsetToFirstPixel);
	TInt offsetBetweenLines = iHalInfo.iOffsetBetweenLines;
	TInt width = iHalInfo.iXPixels;
	TInt height = iHalInfo.iYPixels;
	
	if ((! iHalInfo.iIsPalettized) && iHalInfo.iIsPixelOrderRGB)
		{		
		DrawFillToMemory(firstPixelAddr, offsetBetweenLines,
				EUidPixelFormatXRGB_8888, width, height, aStep);
		}
	}

void CDisplayChannelTest::DrawCompositionBuffer(
		RDisplayChannel::TPostCount& aPostCount,
		RDisplayChannel::TBufferFormat aBufferFormat,
		RDisplayChannel::TDisplayRotation aRotation, TInt aStep)
/**
Attempts to set the requested buffer format and rotation then draws a shaded fill 
to the buffer returned by RDisplayChannel::GetCompositionBuffer.
If it is not possible to set the desired buffer format then the actual buffer format
is used.

@param	aPostCount		out parameter that is set to the post count returned by PostCompositionBuffer
@param	aBufferFormat	the buffer format to use for this test step
@param	aRotation		the rotation to set for this test step
@param	aStep			test step number
*/
	{
	test.Printf(_L("DrawCompositionBuffer\n"));
		
	TBool configChanged;
	TInt err;
	
	RDisplayChannel::TBufferFormat actualBufferFormat(TSize(0,0),0);
	if (iVersion.iMajor > 1 || iVersion.iMinor > 0)
		{
		// It should be possible to set the rotation and the buffer format in either order.
		// To test this the order is swapped every test step
		if (aStep % 2 == 0)
			{
			test_KErrNone(iDisp.SetRotation(aRotation, configChanged));
			err = iDisp.SetBufferFormat(aBufferFormat);
			}
		else
			{
			err = iDisp.SetBufferFormat(aBufferFormat);		
			test_KErrNone(iDisp.SetRotation(aRotation, configChanged));		
			}
		if (err != KErrNone)
			{
			test.Printf(_L("Unable to set buffer format 0x%08x width %d height %d"),
					aBufferFormat.iPixelFormat, aBufferFormat.iSize.iWidth, aBufferFormat.iSize.iHeight);
			}			
		test_KErrNone(iDisp.GetBufferFormat(actualBufferFormat));
		}
	else
		{
		// buffer format not switched in v1.1 so test just validates post / wait for post
		TPckgBuf<RDisplayChannel::TDisplayInfo> infoPkg;
		test_KErrNone(iDisp.GetDisplayInfo(infoPkg));

		err = iDisp.SetRotation(aRotation, configChanged);
		TInt expectedErr = KErrNone;
		if ((!IsValidRotation(aRotation)) || ((infoPkg().iAvailableRotations & aRotation) == 0))
			{
			expectedErr = KErrArgument;
			}
		test(err == expectedErr);

		actualBufferFormat = aBufferFormat;
		}
	
	// Get the composition buffer index
	TUint bufferIndex;
	TRequestStatus status;
	iDisp.GetCompositionBuffer(bufferIndex, status);
	User::WaitForRequest(status);
	test(status == KErrNone);

	// Now get access to the composition buffer
	RChunk compChunk;
	TInt offset = 0;
	err = iDisp.GetCompositionBufferInfo(bufferIndex, compChunk, offset);
	test_KErrNone(err);	
	
	TUint8* baseAddr = compChunk.Base();
	TPckgBuf<RDisplayChannel::TDisplayInfo> infoPkg;
	err = iDisp.GetDisplayInfo(infoPkg);
	test_KErrNone(err);
	
	test.Printf(_L("DrawCompositionBuffer::GetCompositionBufferInfo index = 0x%08x base = 0x%08x offset = 0x%08x\n"),
			bufferIndex, baseAddr, offset);
	
	// Find out structure of the buffer
	TUint8* firstPixelAddr = baseAddr + offset;	
		
	// Find out current display dimensions
	TInt width;
	TInt height;	
	RDisplayChannel::TDisplayRotation currentRotation = iDisp.CurrentRotation();
	test(IsValidRotation(currentRotation));
	if (currentRotation == RDisplayChannel::ERotationNormal ||
		currentRotation == RDisplayChannel::ERotation180)
		{
		width = actualBufferFormat.iSize.iWidth;
		height = actualBufferFormat.iSize.iHeight;
		}
	else
		{
		height = actualBufferFormat.iSize.iWidth;
		width = actualBufferFormat.iSize.iHeight;
		}	
	
	TInt offsetBetweenLines;
	if (iVersion.iMajor > 1 || iVersion.iMinor > 0)
		{
		 offsetBetweenLines = iDisp.NextLineOffset(actualBufferFormat, 0);
		}
	else
		{
		// NextLineOffset not supported in v1.0 and displayinfo offset doesn't work on H4
		offsetBetweenLines = 4 * width;
		}
	DrawFillToMemory(firstPixelAddr, offsetBetweenLines, actualBufferFormat.iPixelFormat, 
			width, height, aStep);

	err = iDisp.PostCompositionBuffer(NULL, aPostCount);
	test_KErrNone(err);
	User::After(KDrawWaitTime);
	
	compChunk.Close();
	}

void CDisplayChannelTest::CheckCompositionBuffers()
/**
 Retrieves the current composition buffer index and checks the information about
 this buffer.
 */
	{
	test.Next(_L("Test GetCompositionBuffer, CancelGetCompositionBuffer, GetCompositionBufferInfo, PostLegacyBuffer"));

	iDisp.CancelGetCompositionBuffer();	// Test cancel without an outstanding call

	TUint bufferIndex;
	TRequestStatus status;
	// Get with immediate cancel
	iDisp.GetCompositionBuffer(bufferIndex, status);
	iDisp.CancelGetCompositionBuffer();
	test(status == KErrNone || status == KErrCancel);

	iDisp.GetCompositionBuffer(bufferIndex, status);	// Get, no cancel
	User::WaitForRequest(status);
	test(status == KErrNone);

	RChunk compChunk;
	TInt offset = 0;
	test_KErrNone(iDisp.GetCompositionBufferInfo(bufferIndex, compChunk, offset));

	// client must be able to read and write to the chunk
	test(compChunk.IsReadable());
	test(compChunk.IsWritable());
	test_Compare(offset, >=, 0);

	RDisplayChannel::TPostCount postCountA;
	RDisplayChannel::TPostCount postCountB;
	test_KErrNone(iDisp.PostCompositionBuffer(NULL, postCountA));
	test_KErrNone(iDisp.PostCompositionBuffer(NULL, postCountB));
	test_Compare(postCountB - postCountA, >=, 1);
	
	// Wait for first postcount value
	iDisp.WaitForPost(postCountA, status);
	User::WaitForRequest(status);
	test(status == KErrNone);

	// It should be possible to wait again on postCountA
	// and this should complete immediately with KErrNone. However, there
	// there is bug in the emulator causes this to wait forever.
	
	compChunk.Close();

	// Legacy buffer should have been initialised by retrieval of HAL::EDisplayMemoryAddress 
	test_KErrNone(iDisp.PostLegacyBuffer(NULL, postCountA));
	test_Compare(postCountA - postCountB, >=, 1);	
	}

void CDisplayChannelTest::VisualTest()
/**
Iterates over the arrays of pixel formats and rotations attempting to 
draw a shaded fill to the composition buffer
*/
	{
	test.Next(_L("Visual test"));	
	
	RDisplayChannel::TPostCount postCount;	
	if (iVisualTestFormatIndex < KNumPixelFormats)
		{
		RDisplayChannel::TBufferFormat bufferFormat(TSize(0,0), 0);		
		if (iVersion.iMajor == 1 && iVersion.iMinor == 0)
			{
			// only one format supported in v1.0 so only one loop needed
			bufferFormat.iPixelFormat = EUidPixelFormatXRGB_8888;
			iVisualTestFormatIndex = KNumPixelFormats - 1; 	
			TPckgBuf<RDisplayChannel::TDisplayInfo> infoPkg;
			iDisp.GetDisplayInfo(infoPkg);
			bufferFormat.iSize.iWidth = infoPkg().iNormal.iWidth;
			bufferFormat.iSize.iHeight = infoPkg().iNormal.iHeight;
			}
		else
			{			
			test_KErrNone(iDisp.GetBufferFormat(bufferFormat));
			bufferFormat.iPixelFormat = KPixelFormats[iVisualTestFormatIndex];			
			}
		DrawCompositionBuffer(postCount, bufferFormat, KRotations[iVisualTestRotationIndex], iVisualTestRotationIndex);	
		iVisualTestRotationIndex++;
		if (iVisualTestRotationIndex >= KNumRotations)
			{
			iVisualTestRotationIndex = 0;
			iVisualTestFormatIndex++;
			}
		iDisp.WaitForPost(postCount, iStatus);
		SetActive();
		}
	else
		{
		// Test drawing to the legacy buffer
		test.Printf(_L("Drawing to legacy buffer\n"));
		
		TBool configChanged;
		iDisp.SetRotation(KRotations[0], configChanged);		
		DrawLegacyBuffer(20); // Make legacy buffer obviously different
		test_KErrNone(iDisp.PostLegacyBuffer(NULL, postCount));
		CompleteSelf(ETestSecondHandle);
		User::After(KDrawWaitTime);
		}
	}

void CDisplayChannelTest::CheckBufferFormat()
/**
 Tests the APIs for getting and setting the buffer format.
  In version 1.1 these APIs are only stubs that return KErrNotSupported
 
 @pre CheckResolutions must have called prior to calling this method
 @pre CheckPixelFormats must have been called prior to calling this method.
 */
	{
	test.Next(_L("Test GetBufferFormat, SetBufferFormat, NextLineOffset, NextPlaneOffset"));

	RDisplayChannel::TBufferFormat bufferFormat(TSize(0,0), 0);
	TInt err = iDisp.GetBufferFormat(bufferFormat);
	if (iVersion.iMajor == 1 && iVersion.iMinor <= 1)
		{
		test_Compare(err, ==, KErrNotSupported);
		}
	else
		{
		test(IsValidPixelFormat(bufferFormat.iPixelFormat));
		test(bufferFormat.iSize.iHeight > 0 && bufferFormat.iSize.iHeight > 0);
		// Check that the buffer is at least as large as the current pixel resolution
		TSize resSize;
 		test_KErrNone(iDisp.GetResolution(resSize));
		test(bufferFormat.iSize.iHeight >= resSize.iHeight && bufferFormat.iSize.iWidth >= resSize.iWidth);
		}
	
	RDisplayChannel::TBufferFormat newBufferFormat(TSize(iHalInfo.iXPixels, iHalInfo.iYPixels), 0);
	if (iVersion.iMajor == 1 && iVersion.iMinor <= 1)
		{
		// API not support in 1.1
		test_Compare(iDisp.SetBufferFormat(newBufferFormat), ==, KErrNotSupported);
		}
	else
		{
		// Tests assumes that 32bpp XRGB888 is supported on most hardware
		RDisplayChannel::TBufferFormat newBufferFormat(TSize(0,0), EUidPixelFormatXRGB_8888);
		test_Compare(iDisp.SetBufferFormat(newBufferFormat), ==, KErrArgument);	// buffer must be large enough for resolution
						
		// Should be able to current buffer format
		test_KErrNone(iDisp.SetBufferFormat(bufferFormat));
		}

	// Get current information and check this against new APIs that give 
	// line and plane information for any mode.
	TSize currentPixelRes;
	TSize currentTwipRes;
	RDisplayChannel::TDisplayRotation currentRotation = iDisp.CurrentRotation();
	RDisplayChannel::TBufferFormat currentBufferFormat(TSize(0,0), 0);
	
	test_KErrNone(iDisp.GetResolution(currentPixelRes));
	test_KErrNone(iDisp.GetTwips(currentTwipRes));
	test_KErrNone(iDisp.GetBufferFormat(currentBufferFormat));
	RDisplayChannel::TResolution res(currentPixelRes, currentTwipRes, currentRotation);

	TInt planeOffset = iDisp.NextPlaneOffset(currentBufferFormat, 0);
	if (iVersion.iMajor == 1 && iVersion.iMinor <= 1)
		{
		test_Compare(planeOffset, ==, KErrNotSupported);
		}
	else
		{
		// Supported in v1.1
		test_Compare(planeOffset, >=, 0);
		
		if (iVersion.iMajor > 1 || iVersion.iMinor > 1)
			{
			// Extended API in v1.2
			test.Printf(_L("Check that planeoffset APIs match"));
			TInt planeOffset2 = iDisp.NextPlaneOffset(currentBufferFormat, res, currentRotation, 0);
			test_Compare(planeOffset, ==, planeOffset2);		
							
			// check that invalid buffer formats are rejected
			RDisplayChannel::TBufferFormat badBufferFormat(currentBufferFormat);
			badBufferFormat.iPixelFormat = -1;
			test(iDisp.NextPlaneOffset(badBufferFormat, res, currentRotation, 0) == KErrArgument);
			}
		}

	TInt lineOffset = iDisp.NextLineOffset(currentBufferFormat, 0);	
	if (iVersion.iMajor == 1 && iVersion.iMinor <= 1)
		{
		test_Compare(lineOffset, ==, KErrNotSupported);
		}
	else
		{
		test_Compare(lineOffset, >, 0);	// supported in v1.1
		
		if (iVersion.iMajor > 1 || iVersion.iMinor > 1)
			{
			// Extended API in v1.2
			test.Printf(_L("Check that lineoffset APIs match"));
			TInt lineOffset2 = iDisp.NextLineOffset(currentBufferFormat, res, currentRotation, 0);
			// stride values must be the same and > 0 for any non-zero resolution and the current
			// resolution should not be zero in size.
			
			test_Compare(lineOffset, ==, lineOffset2);
			// check that invalid buffer formats are rejected
			RDisplayChannel::TBufferFormat badBufferFormat(currentBufferFormat);
			badBufferFormat.iPixelFormat = -1;
			test(iDisp.NextLineOffset(badBufferFormat, res, currentRotation, 0) == KErrArgument);
			}
		}
	}

void CDisplayChannelTest::CheckUserBuffers()
/**
 Test APIs that manage user composition buffers. Since this unit test doesn't
 have access to the real surfaces the tests are mostly robustness tests.
 */
	{
	test.Next(_L("Test WaitForPost, DeRegisterUserBuffer"));

	// Cancel should not fail even if WaitForPost has not been called
	iDisp.CancelWaitForPost();
	
	// Check that cancelling a non-existent post request doesn't fail
	iDisp.CancelPostUserBuffer();

	// Make sure wait immediately followed by cancel doesn't crash
	TRequestStatus status;
	RDisplayChannel::TPostCount postCount = 0;
    iDisp.WaitForPost(postCount, status);
    iDisp.CancelWaitForPost();
	test(status == KErrNone || status == KErrCancel);

	// De-register a non-existent buffer id
	RDisplayChannel::TBufferId badBufferId(42);
	TInt err = iDisp.DeregisterUserBuffer(badBufferId);
	// emulator KErrArugment but on H4 KErrNotFound	
	test(err == KErrArgument || err == KErrNotFound);

	// Create and use a new buffer, should fail because chunk must be a SHARED chunk
	RChunk myChunk;
	const TInt chunkSize = 320 * 200 * 4; // actual size is not important because this should fail
	err =  myChunk.CreateGlobal(KNullDesC, chunkSize, chunkSize, EOwnerProcess);
	test_KErrNone(err); // Allocation should not fail under normal conditions
	RDisplayChannel::TBufferId myBufferId;
	err = iDisp.RegisterUserBuffer(myBufferId, myChunk, 0);
	// emulator KErrBadHandle but on H4 KErrArgument
	test(err == KErrBadHandle || err == KErrArgument);
	myChunk.Close();

	// Try to post a request from a bad buffer id
	iDisp.PostUserBuffer(badBufferId, status, NULL, postCount);
	User::WaitForRequest(status);
	// Emulator KErrArgument H4 KErrNotFound
	test(status.Int() == KErrArgument || status.Int() == KErrNotFound);
	
	// Attempt to register an already existing buffer as a user buffer
	TUint compId;
	iDisp.GetCompositionBuffer(compId, status);
	User::WaitForRequest(status);
	RChunk compChunk;
	TInt offset;
	test_KErrNone(iDisp.GetCompositionBufferInfo(compId, compChunk, offset));
	test_KErrNone(iDisp.RegisterUserBuffer(myBufferId, compChunk, offset));
	err = iDisp.DeregisterUserBuffer(myBufferId);
	test(err == KErrNone || err == KErrInUse);
	compChunk.Close();
	}

TBool CDisplayChannelTest::IsValidPixelFormat(RDisplayChannel::TPixelFormat aPixelFormat)
/**
Validates whether the value of aPixelFormat corresponds to a valid enum in TUidPixelFormat
@param	aPixelFormat	the pixel format value to test
@return	ETrue if aPixelFormat is valid; otherwise, EFalse is returned.
*/
	{
	switch (aPixelFormat)
		{
		case EUidPixelFormatUnknown:
		case EUidPixelFormatXRGB_8888:
		case EUidPixelFormatBGRX_8888:
		case EUidPixelFormatXBGR_8888:
		case EUidPixelFormatBGRA_8888:
		case EUidPixelFormatARGB_8888:
		case EUidPixelFormatABGR_8888:
		case EUidPixelFormatARGB_8888_PRE:
		case EUidPixelFormatABGR_8888_PRE:
		case EUidPixelFormatBGRA_8888_PRE:
		case EUidPixelFormatARGB_2101010:
		case EUidPixelFormatABGR_2101010:
		case EUidPixelFormatBGR_888:
		case EUidPixelFormatRGB_888:
		case EUidPixelFormatRGB_565:
		case EUidPixelFormatBGR_565:
		case EUidPixelFormatARGB_1555:
		case EUidPixelFormatXRGB_1555:
		case EUidPixelFormatARGB_4444:
		case EUidPixelFormatARGB_8332:
		case EUidPixelFormatBGRX_5551:
		case EUidPixelFormatBGRA_5551:
		case EUidPixelFormatBGRA_4444:
		case EUidPixelFormatBGRX_4444:
		case EUidPixelFormatAP_88:
		case EUidPixelFormatXRGB_4444:
		case EUidPixelFormatXBGR_4444:
		case EUidPixelFormatRGB_332:
		case EUidPixelFormatA_8:
		case EUidPixelFormatBGR_332:
		case EUidPixelFormatP_8:
		case EUidPixelFormatP_4:
		case EUidPixelFormatP_2:
		case EUidPixelFormatP_1:
		case EUidPixelFormatYUV_420Interleaved:
		case EUidPixelFormatYUV_420Planar:
		case EUidPixelFormatYUV_420PlanarReversed:
		case EUidPixelFormatYUV_420SemiPlanar:
		case EUidPixelFormatYUV_422Interleaved:
		case EUidPixelFormatYUV_422Planar:
		case EUidPixelFormatYUV_422Reversed:
		case EUidPixelFormatYUV_422SemiPlanar:
		case EUidPixelFormatYUV_422InterleavedReversed:
		case EUidPixelFormatYUV_422Interleaved16bit:
		case EUidPixelFormatYUV_444Interleaved:
		case EUidPixelFormatYUV_444Planar:
		case EUidPixelFormatL_8:
		case EUidPixelFormatL_4:
		case EUidPixelFormatL_2:
		case EUidPixelFormatL_1:
		case EUidPixelFormatSpeedTaggedJPEG:
		case EUidPixelFormatJPEG:
			return ETrue;
		default:
			return EFalse;
		}
	}

TBool CDisplayChannelTest::IsValidRotation(RDisplayChannel::TDisplayRotation aRotation)
/**
Checks whether the supplied rotation is a valid rotation. <br>
N.B. Only single rotations are accepted so EFalse is returned for ERotationAll.
@param	aRotation	the rotation to validate
@return ETrue if the supplied rotation is valid; otherwise, EFalse is returned.
*/
	{
	switch (aRotation)
		{
		case RDisplayChannel::ERotationNormal:
		case RDisplayChannel::ERotation90CW:
		case RDisplayChannel::ERotation180:
		case RDisplayChannel::ERotation270CW:
			return ETrue;
		default:
			return EFalse;
		}
	}

void CDisplayChannelTest::CheckSetRotation(TUint aSupported, RDisplayChannel::TDisplayRotation aNewRotation)
/**
Tests the SetRotation API attempting to set the requested resolution. If the resolution is supported
then SetRotation should succeed and the CurrentRotation should change. Otherwise, SetResolution should
fail and the current rotation should be unchanged.

@param	aSupported		The set of supported resolutions for TDisplayInfo
@param	aNewRotation	The new rotation to set
*/
	{
	RDisplayChannel::TDisplayRotation currentRotation = iDisp.CurrentRotation();
	test(IsValidRotation(currentRotation));

	TBool displayConfigChanged = EFalse;
	TInt err = iDisp.SetRotation(aNewRotation, displayConfigChanged);
	TInt expectedErr = KErrNone;
	if ((!IsValidRotation(aNewRotation)) || ((aSupported & aNewRotation) == 0))
		{
		expectedErr = KErrArgument;
		}
	test(err == expectedErr);

	// Check whether the rotation should / shouldn't have changed
	test (iDisp.CurrentRotation() == (err == KErrNone ? aNewRotation : currentRotation));
	}

void CDisplayChannelTest::CheckRotations()
/**
Tests the SetRotation and GetRotation APIs by attempting to set each valid rotation
plus some invalid rotation values.
If a rotation is valid but not supported then KErrNotSupported should be returned.
*/
	{
	test.Next(_L("Test CurrentRotation, SetRotation"));

	// Find out supported resolutions
	TPckgBuf<RDisplayChannel::TDisplayInfo> infoPkg;
	test_KErrNone(iDisp.GetDisplayInfo(infoPkg));

	CheckSetRotation(infoPkg().iAvailableRotations, RDisplayChannel::ERotationNormal);
	CheckSetRotation(infoPkg().iAvailableRotations, RDisplayChannel::ERotation90CW);
	CheckSetRotation(infoPkg().iAvailableRotations, RDisplayChannel::ERotation180);
	CheckSetRotation(infoPkg().iAvailableRotations, RDisplayChannel::ERotation270CW);
	CheckSetRotation(infoPkg().iAvailableRotations, RDisplayChannel::ERotationNormal);
	CheckSetRotation(infoPkg().iAvailableRotations, static_cast<RDisplayChannel::TDisplayRotation>(-1));
	CheckSetRotation(infoPkg().iAvailableRotations, static_cast<RDisplayChannel::TDisplayRotation>(0));
	}

void CDisplayChannelTest::CheckV11inV10()
/**
The purpose of this test is to verify that v1.0 of the display channel driver
returns KErrNotSupported for methods that only exist in newer versions as opposed
to panicking.
To run this test for real t_display needs to be built against v1.1 and then copied
to a v1.0 environment.

If the version number is > 1.0 then this method does nothing.
*/
	{
	if (iVersion.iMajor > 1 || iVersion.iMinor > 0)
		{
		return;
		}
	
	test.Next(_L("Test check v1.1 functions fail gracefully in v1.0"));

	// APIs should fail before evaluating parameters
	TInt intDummy;
	TInt err;
	TBuf8<256> buf;
	TSize size;
	
	test.Printf(_L("Testing display change APIs\n"));
	iDisp.NotifyOnDisplayChangeCancel();
	TRequestStatus status;	
	iDisp.NotifyOnDisplayChange(status);
	test(status == KErrNotSupported);

	err = iDisp.NumberOfResolutions();
	test(err == KErrNotSupported);

	err = iDisp.GetResolutions(buf, intDummy);
	test(err == KErrNotSupported);

	err = iDisp.GetResolution(size);
	test(err == KErrNotSupported);

	err = iDisp.GetTwips(size);
	test(err == KErrNotSupported);

	err = iDisp.NumberOfPixelFormats();
	test(err == KErrNotSupported);

	err = iDisp.GetPixelFormats(buf, intDummy);
	test(err == KErrNotSupported);

	RDisplayChannel::TBufferFormat bufferFormat(TSize(0,0),0);
	err = iDisp.GetBufferFormat(bufferFormat);
	test(err == KErrNotSupported);
	
	err = iDisp.SetBufferFormat(bufferFormat);
	test(err == KErrNotSupported);

	err = iDisp.NextPlaneOffset(bufferFormat, 0);
	test(err == KErrNotSupported);

	err = iDisp.NextLineOffset(bufferFormat, 0);
	test(err == KErrNotSupported);
	}

void CDisplayChannelTest::CheckSecondHandle()
/**
Opens a second RDisplayChannel. 
The driver may not support this but must not crash. 
*/
	{
	test.Next(_L("Open a second handle"));
	RDisplayChannel disp2;
	TInt err = disp2.Open(iScreenId);
	test(err == KErrNone || err == KErrInUse);
	disp2.Close();
	}

void CDisplayChannelTest::TestBufferTransitions()
/**
Because different buffer types (ie. composition, legacy and user) complete differently, we must test
switching between those different types of buffers to ensure that this is taken into account.
*/
	{
	// The support code required for this test exists only in the separated GCE display LDD, not in the
	// legacy monolithic WINSCW LDD
#if defined(_DEBUG) && !defined(__WINS__)
	test.Next(_L("Test transitions between buffer types"));

	TPckgBuf<RDisplayChannel::TDisplayInfo> displayInfo;
	test_KErrNone(iDisp.GetDisplayInfo(displayInfo));

	RChunk chunk;
	RDisplayChannel::TBufferFormat bufferFormat(TSize(iHalInfo.iXPixels, iHalInfo.iYPixels), displayInfo().iPixelFormat);

	test.Next(_L("Get the LDD to create a user buffer"));
	TInt err = iDisp.CreateUserBuffer(bufferFormat, chunk);
	test_KErrNone(err);

	test.Next(_L("Register a user buffer"));
	RDisplayChannel::TBufferId bufferId;
	err = iDisp.RegisterUserBuffer(bufferId, chunk, 0);
	test_KErrNone(err);

	test.Next(_L("Post a user buffer"));
	TRequestStatus status;
	RDisplayChannel::TPostCount postCount;
	iDisp.PostUserBuffer(bufferId, status, NULL, postCount);
	iDisp.PostLegacyBuffer(NULL, postCount);

	test.Printf(_L("Waiting for user buffer\n"));
	User::WaitForRequest(status);
	test(status.Int() == KErrNone || status.Int() == KErrCancel);
	test.Printf(_L("Waiting for legacy buffer\n"));
	iDisp.WaitForPost(postCount, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	test.Printf(_L("Getting composition buffer\n"));
	TUint bufferIndex;
	iDisp.GetCompositionBuffer(bufferIndex, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	iDisp.PostUserBuffer(bufferId, status, NULL, postCount);
	iDisp.PostCompositionBuffer(NULL, postCount);

	test.Printf(_L("Waiting for user buffer\n"));
	User::WaitForRequest(status);
	test(status.Int() == KErrNone || status.Int() == KErrCancel);
	test.Printf(_L("Waiting for composition buffer\n"));
	iDisp.WaitForPost(postCount, status);
	User::WaitForRequest(status);
	test_KErrNone(status.Int());

	test.Printf(_L("Deregistering user buffers\n"));
	err = iDisp.DeregisterUserBuffer(bufferId);
	test_KErrNone(err);

	test.Printf(_L("Done, closing shared chunk\n"));
	chunk.Close();
#endif // defined(_DEBUG) && !defined(__WINS__)
	}

void CDisplayChannelTest::Start()
/**
 Run all of the test cases
 */
	{
	CompleteSelf(ETestDisplayInfo);
	}

void CDisplayChannelTest::CompleteSelf(TTestState aNextState)
/*
Advances to the next test state for test steps that don't invoke
asynchronous requests on this AO. 
*/
	{	
	iState = aNextState;
	TRequestStatus* status = &iStatus;	
	SetActive();
	User::RequestComplete(status, KErrNone);
	}

void CDisplayChannelTest::DoCancel()
	{	
	iAsyncHelper->Cancel();
	}

TInt CDisplayChannelTest::RunError(TInt aError)
	{
	test_KErrNone(aError);
	return KErrNone;
	}

void CDisplayChannelTest::RunL()
/**
 Run all of the tests where the API is defined for that version.
 */
	{
	test_KErrNone(iStatus.Int());
	
	test.Printf(_L("Test state %d\n"), iState);
	switch (iState)
		{
		case ETestDisplayInfo:
			CheckDisplayInfo();
			CompleteSelf(ETestCompositionBuffers);
			break;
		case ETestCompositionBuffers:
			CheckCompositionBuffers();
			CompleteSelf(ETestUserBuffers);
			break;
		case ETestUserBuffers:
			CheckUserBuffers();
			CompleteSelf(ETestRotations);
			break;
		case ETestRotations:
			CheckRotations();			
			CompleteSelf(ETestWaitForPostDoCancel);				
			break;
		case ETestWaitForPostDoCancel:
			// Post the composition buffer, register wait and cancel wait.
			iDisp.PostCompositionBuffer(NULL, iDummyPostCount);
			iDisp.WaitForPost(iDummyPostCount, iAsyncHelper->Status());
			iAsyncHelper->WaitForOperation(&iAsyncHelperResult);
			iDisp.CancelWaitForPost();
			CompleteSelf(ETestWaitForPostCheckCancel);
			break;
		case ETestWaitForPostCheckCancel:
			test(iAsyncHelperResult == KErrCancel || iAsyncHelperResult == KErrNone);
			CompleteSelf(ETestGetCompositionBufferDoCancel);
			break;
		case ETestGetCompositionBufferDoCancel:
			iDisp.GetCompositionBuffer(iDummyCompositionBuffer, iAsyncHelper->Status());
			iAsyncHelper->WaitForOperation(&iAsyncHelperResult);
			iDisp.CancelGetCompositionBuffer();
			CompleteSelf(ETestGetCompositionBufferCheckCancel);
			break;
		case ETestGetCompositionBufferCheckCancel:
			test(iAsyncHelperResult == KErrCancel || iAsyncHelperResult == KErrNone);
			
			if (iVersion.iMajor == 1 && iVersion.iMinor == 0)
				{
				CompleteSelf(ETestV11inV10);
				}
			else
				{
				CompleteSelf(ETestDisplayChange);
				}
			break;
		case ETestDisplayChange:	// API in v1.1 +
			CheckDisplayChange();
			CompleteSelf(ETestDisplayChangeDoCancel);
			break;			
		case ETestDisplayChangeDoCancel:	// API in v1.1 +
			iDisp.NotifyOnDisplayChangeCancel();
			CompleteSelf(ETestDisplayChangeCheckCancel);
			break;
		case ETestDisplayChangeCheckCancel:	// API in v1.1 +
			test(iAsyncHelperResult == KErrCancel);	// display should not have changed
			CompleteSelf(ETestResolutions);
			break;
		case ETestResolutions:	// API in v1.1 +
			CheckResolutions();
			CompleteSelf(ETestPixelFormats);
			break;
		case ETestPixelFormats:	// API in v1.1 +
			CheckPixelFormats();
			CompleteSelf(ETestBufferFormats);
			break;
		case ETestBufferFormats:	// API in v1.1 +
			CheckBufferFormat();
			CompleteSelf(EVisualTest);
			break;
		case ETestV11inV10:			
			CheckV11inV10();
			CompleteSelf(EVisualTest);
			break;
		case EVisualTest:
			VisualTest();	// visual test is async because of WaitForPost
			break;
		case ETestSecondHandle:
			CheckSecondHandle();
			CompleteSelf(ETestBufferTransitions);
			break;
		case ETestBufferTransitions:
			TestBufferTransitions();
			CompleteSelf(ETestFinished);
			break;
		case ETestFinished:
			CActiveScheduler::Stop();
			break;
		default:
			test(EFalse);		
		}
	}

void MainL()
/**
 Initialise RTest and run the tests
 */
	{
	test.Start(_L("Testing display channel driver"));
	
	// If the device driver does not exist then this is considered a pass
	// because the display channel is not a mandatory part of the base port
	_LIT(KLdd, "display0.ldd");
	test.Printf(_L("Loading logical %S\n"), &KLdd);
	TInt err = User::LoadLogicalDevice(KLdd);	
	test(err == KErrNone || err == KErrAlreadyExists || err == KErrNotFound);		
	
	// Only test for kenel memory leaks for non WINSCW builds as the WINSCW LDD is obsolete and would
	// take forever to debug
#ifndef __WINS__
	__KHEAP_MARK;
#endif // ! __WINS__

	if (err == KErrNone || err == KErrAlreadyExists)
		{
		TInt numberOfScreens;
		User::LeaveIfError(HAL::Get(HAL::EDisplayNumberOfScreens, numberOfScreens));
		for (TInt screenNum  = 0; screenNum < numberOfScreens; ++screenNum)
			{
			CActiveScheduler* s = new(ELeave) CActiveScheduler();
			CActiveScheduler::Install(s);
			CleanupStack::PushL(s);
			CDisplayChannelTest* displayTest = CDisplayChannelTest::NewLC(screenNum);
			displayTest->Start();
			s->Start();
			CleanupStack::PopAndDestroy(2, s);	// s, displayTest 
			}
		}
	else
		{
		test.Printf(_L("display0.ldd not present. Finishing test.\n"));
		}
	
#ifndef __WINS__
	__KHEAP_MARKEND;
#endif // ! __WINS__

	test.End();
	}

TInt E32Main()
/**
 Create cleanup stack, initialise memory checks and run the tests.
 */
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (!cleanup)
		{
		return KErrNoMemory;
		}
	__UHEAP_MARK;
	test.Title();
	TRAPD(err, MainL());
	test.Close();
	__UHEAP_MARKEND;
	delete cleanup;
	return err;
	}
