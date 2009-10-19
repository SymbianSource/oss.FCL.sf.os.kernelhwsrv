// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// template\Template_Variant\Specific\lcd.cpp
// Implementation of an LCD driver. 
// This file is part of the Template Base port
// N.B. This sample code assumes that the display supports setting the backlight on or off, 
// as well as adjusting the contrast and the brightness.
// 
//



#include <videodriver.h>
#include "platform.h"
#include <nkern.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <kernel/kpower.h>
#include <template_assp_priv.h>


// TO DO: (mandatory)
// If the display supports Contrast and/or Brightness control then supply the following defines:
// This is only example code... you may need to modify it for your hardware
const TInt KConfigInitialDisplayContrast	= 128;
const TInt KConfigLcdMinDisplayContrast		= 1;
const TInt KConfigLcdMaxDisplayContrast		= 255;
const TInt KConfigInitialDisplayBrightness	= 128;
const TInt KConfigLcdMinDisplayBrightness	= 1;
const TInt KConfigLcdMaxDisplayBrightness	= 255;

// TO DO: (mandatory)
// define a macro to calculate the screen buffer size
// This is only example code... you may need to modify it for your hardware
// aBpp is the number of bits-per-pixel, aPpl is the number of pixels per line and 
// aLpp number of lines per panel
#define FRAME_BUFFER_SIZE(aBpp,aPpl,aLpp)	(aBpp*aPpl*aLpp)/8	
																

// TO DO: (mandatory)
// define the physical screen dimensions
// This is only example code... you need to modify it for your hardware
const TUint	KConfigLcdWidth					= 640;		// 640 pixels per line
const TUint	KConfigLcdHeight				= 480;		// 480 lines per panel

// TO DO: (mandatory)
// define the characteristics of the LCD display
// This is only example code... you need to modify it for your hardware
const TBool	KConfigLcdIsMono				= EFalse;
const TBool	KConfigLcdPixelOrderLandscape	= ETrue;
const TBool	KConfigLcdPixelOrderRGB			= ETrue;
const TInt	KConfigLcdMaxDisplayColors		= 65536;


// TO DO: (mandatory)
// define the display dimensions in TWIPs
// A TWIP is a 20th of a point.  A point is a 72nd of an inch
// Therefore a TWIP is a 1440th of an inch
// This is only example code... you need to modify it for your hardware
const TInt	KConfigLcdWidthInTwips			= 9638;		// = 6.69 inches
const TInt	KConfigLcdHeightInTwips			= 7370;		// = 5.11 inches

// TO DO: (mandatory)
// define the available display modes
// This is only example code... you need to modify it for your hardware
const TInt  KConfigLcdNumberOfDisplayModes	= 1;
const TInt  KConfigLcdInitialDisplayMode	= 0;
struct SLcdConfig
	{
	TInt iMode;
	TInt iOffsetToFirstVideoBuffer;
	TInt iLenghtOfVideoBufferInBytes;
	TInt iOffsetBetweenLines;
	TBool iIsPalettized;
	TInt iBitsPerPixel;
	};
static const SLcdConfig Lcd_Mode_Config[KConfigLcdNumberOfDisplayModes]=
	{
		{
		0,								// iMode
		0,								// iOffsetToFirstVideoBuffer
		FRAME_BUFFER_SIZE(8, KConfigLcdWidth, KConfigLcdHeight),	// iLenghtOfVideoBufferInBytes
		KConfigLcdWidth,				// iOffsetBetweenLines
		ETrue,							// iIsPalettized
		8								// iBitsPerPixel
		}
	};	



_LIT(KLitLcd,"LCD");

//
// TO DO: (optional)
//
// Add any private functions and data you require
//
NONSHARABLE_CLASS(DLcdPowerHandler) : public DPowerHandler
	{
public: 
	DLcdPowerHandler();
	~DLcdPowerHandler();
	
	// from DPowerHandler
	void PowerDown(TPowerState);
	void PowerUp();

	void PowerUpDfc();
	void PowerDownDfc();

	TInt Create();
	void DisplayOn();
	void DisplayOff();
	TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);

	void PowerUpLcd(TBool aSecure);
	void PowerDownLcd();

	void ScreenInfo(TScreenInfoV01& aInfo);
	void WsSwitchOnScreen();
	void WsSwitchOffScreen();
	void HandleMsg();
	void SwitchDisplay(TBool aSecure);

	void SetBacklightState(TBool aState);
	void BacklightOn();
	void BacklightOff();
	TInt SetContrast(TInt aContrast);
	TInt SetBrightness(TInt aBrightness);

private:
	TInt SetPaletteEntry(TInt aEntry, TInt aColor);
	TInt GetPaletteEntry(TInt aEntry, TInt* aColor);
	TInt NumberOfPaletteEntries();
	TInt GetCurrentDisplayModeInfo(TVideoInfoV01& aInfo, TBool aSecure);
	TInt GetSpecifiedDisplayModeInfo(TInt aMode, TVideoInfoV01& aInfo);
	TInt SetDisplayMode(TInt aMode);
	void SplashScreen();
	TInt GetDisplayColors(TInt* aColors);

private:
	TBool iIsPalettized;
	TBool iDisplayOn;				// to prevent a race condition with WServer trying to power up/down at the same time
	DPlatChunkHw* iChunk;
	DPlatChunkHw* iSecureChunk;
	TBool iWsSwitchOnScreen;
 	TBool iSecureDisplay;
	TDynamicDfcQue* iDfcQ;
	TMessageQue iMsgQ;
	TDfc iPowerUpDfc;
	TDfc iPowerDownDfc;	
	TVideoInfoV01 iVideoInfo;
	TVideoInfoV01 iSecureVideoInfo;
	NFastMutex iLock;				// protects against being preempted whilst manipulating iVideoInfo/iSecureVideoInfo
	TPhysAddr ivRamPhys;
	TPhysAddr iSecurevRamPhys;

	TBool iBacklightOn;
	TInt iContrast;
	TInt iBrightness;
	};


/**
HAL handler function

@param	aPtr a pointer to an instance of DLcdPowerHandler
@param	aFunction the function number
@param	a1 an arbitrary parameter
@param	a2 an arbitrary parameter
*/
LOCAL_C TInt halFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
	{
	DLcdPowerHandler* pH=(DLcdPowerHandler*)aPtr;
	return pH->HalFunction(aFunction,a1,a2);
	}

/**
DFC for receiving messages from the power handler
@param	aPtr a pointer to an instance of DLcdPowerHandler
*/
void rxMsg(TAny* aPtr)
	{
	DLcdPowerHandler& h=*(DLcdPowerHandler*)aPtr;
	h.HandleMsg();
	}

/**
DFC for powering up the device

@param aPtr	aPtr a pointer to an instance of DLcdPowerHandler
*/
void power_up_dfc(TAny* aPtr)
	{
	((DLcdPowerHandler*)aPtr)->PowerUpDfc();
	}

/**
DFC for powering down the device

@param aPtr	aPtr a pointer to an instance of DLcdPowerHandler
*/
void power_down_dfc(TAny* aPtr)
	{
	((DLcdPowerHandler*)aPtr)->PowerDownDfc();
	}


/**
Default constructor
*/
DLcdPowerHandler::DLcdPowerHandler() :
		DPowerHandler(KLitLcd),
		iMsgQ(rxMsg,this,NULL,1),
		iPowerUpDfc(&power_up_dfc,this,6),
		iPowerDownDfc(&power_down_dfc,this,7),
		iBacklightOn(EFalse),
		iContrast(KConfigInitialDisplayContrast),
		iBrightness(KConfigInitialDisplayBrightness)
	{
	}

DLcdPowerHandler::~DLcdPowerHandler()
	{
	if (iDfcQ)
		iDfcQ->Destroy();
	}

/**
Second-phase constructor 

Called by factory function at ordinal 0
*/
TInt DLcdPowerHandler::Create()
	{
	const TInt KLCDDfcQPriority=27; // Equal to Kern::DfcQue0() priority
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KLCDDfcQPriority, _L("LCDPowerHandler"));
	if (r != KErrNone)
		{
		return r;
		}

	// map the video RAM
	TInt vSize = ((TemplateAssp*)Arch::TheAsic())->VideoRamSize();
	ivRamPhys = TTemplate::VideoRamPhys();				// EXAMPLE ONLY: assume TTemplate interface class
	r = DPlatChunkHw::New(iChunk,ivRamPhys,vSize,EMapAttrUserRw|EMapAttrBufferedC);
	if (r != KErrNone)
		return r;
	
	//create "secure" screen immediately after normal one
	iSecurevRamPhys =  ivRamPhys + vSize;
	TInt r2 = DPlatChunkHw::New(iSecureChunk,iSecurevRamPhys,vSize,EMapAttrUserRw|EMapAttrBufferedC);
	if (r2 != KErrNone)
		return r2;

	TUint* pV=(TUint*)iChunk->LinearAddress();

	__KTRACE_OPT(KEXTENSION,Kern::Printf("DLcdPowerHandler::Create: VideoRamSize=%x, VideoRamPhys=%08x, VideoRamLin=%08x",vSize,ivRamPhys,pV));

	// TO DO: (mandatory)
	// initialise the palette for the initial display mode
	// NOTE: the palette could either be a buffer allocated in system RAM (usually contiguous to Video buffer)
	//		 or could be offered as part of the hardware block that implemenst the lcd control
	//

	TUint* pV2=(TUint*)iSecureChunk->LinearAddress();

	__KTRACE_OPT(KEXTENSION,Kern::Printf("DLcdPowerHandler::Create: Secure display VideoRamSize=%x, VideoRamPhys=%08x, VideoRamLin=%08x",vSize,iSecurevRamPhys,pV2));

	// TO DO: (mandatory)
	// initialise the secure screen's palette for the initial display mode
	//
	
	// setup the video info structure, this'll be used to remember the video settings
	iVideoInfo.iDisplayMode = KConfigLcdInitialDisplayMode;
	iVideoInfo.iOffsetToFirstPixel = Lcd_Mode_Config[KConfigLcdInitialDisplayMode].iOffsetToFirstVideoBuffer;
	iVideoInfo.iIsPalettized = Lcd_Mode_Config[KConfigLcdInitialDisplayMode].iIsPalettized;
	iVideoInfo.iOffsetBetweenLines = Lcd_Mode_Config[KConfigLcdInitialDisplayMode].iOffsetBetweenLines;
	iVideoInfo.iBitsPerPixel = Lcd_Mode_Config[KConfigLcdInitialDisplayMode].iBitsPerPixel;

	iVideoInfo.iSizeInPixels.iWidth = KConfigLcdWidth;
	iVideoInfo.iSizeInPixels.iHeight = KConfigLcdHeight;
	iVideoInfo.iSizeInTwips.iWidth = KConfigLcdWidthInTwips;
	iVideoInfo.iSizeInTwips.iHeight = KConfigLcdHeightInTwips;
	iVideoInfo.iIsMono = KConfigLcdIsMono;
	iVideoInfo.iVideoAddress=(TInt)pV;
	iVideoInfo.iIsPixelOrderLandscape = KConfigLcdPixelOrderLandscape;
	iVideoInfo.iIsPixelOrderRGB = KConfigLcdPixelOrderRGB;

	iSecureVideoInfo = iVideoInfo;
	iSecureVideoInfo.iVideoAddress = (TInt)pV2;

	iDisplayOn = EFalse;
	iSecureDisplay = EFalse;

	// install the HAL function
	r=Kern::AddHalEntry(EHalGroupDisplay, halFunction, this);
	if (r!=KErrNone)
		return r;

	iPowerUpDfc.SetDfcQ(iDfcQ);
	iPowerDownDfc.SetDfcQ(iDfcQ);
	iMsgQ.SetDfcQ(iDfcQ);
	iMsgQ.Receive();

	// install the power handler
	// power up the screen
	Add();
	DisplayOn();

	SplashScreen();
	
	return KErrNone;
	}

/**
Turn the display on
May be called as a result of a power transition or from the HAL
If called from HAL, then the display may be already be on (iDisplayOn == ETrue)
*/
void DLcdPowerHandler::DisplayOn()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DisplayOn %d", iDisplayOn));
	if (!iDisplayOn)				// may have been powered up already
		{
		iDisplayOn = ETrue;
		PowerUpLcd(iSecureDisplay);
		SetContrast(iContrast);
		SetBrightness(iBrightness);
		}
	}

/**
Turn the display off
May be called as a result of a power transition or from the HAL
If called from Power Manager, then the display may be already be off (iDisplayOn == EFalse)
if the platform is in silent running mode
*/
void DLcdPowerHandler::DisplayOff()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("DisplayOff %d", iDisplayOn));
	if (iDisplayOn)
		{
		iDisplayOn = EFalse;
		PowerDownLcd();
		}
	}

/**
Switch between secure and non-secure displays

@param aSecure ETrue if switching to secure display
*/
void DLcdPowerHandler::SwitchDisplay(TBool aSecure)
 	{
 	if (aSecure)
 		{
 		if (!iSecureDisplay)
 			{
 			//switch to secure display
 			DisplayOff();
 			iSecureDisplay = ETrue;
 			DisplayOn();
 			}
 		}
 	else
 		{
 		if (iSecureDisplay)
 			{
 			//switch from secure display
 			DisplayOff();
 			iSecureDisplay = EFalse;
 			DisplayOn();
 			}
 		}
 	}

/**
DFC to power up the display
*/
void DLcdPowerHandler::PowerUpDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("PowerUpDfc"));
	DisplayOn();

	PowerUpDone();				// must be called from a different thread than PowerUp()
	}

/**
DFC to power down the display
*/
void DLcdPowerHandler::PowerDownDfc()
	{
	__KTRACE_OPT(KPOWER, Kern::Printf("PowerDownDfc"));
	DisplayOff();
	PowerDownDone();			// must be called from a different thread than PowerUp()
	}

/**
Schedule the power-down DFC
*/
void DLcdPowerHandler::PowerDown(TPowerState)
	{
	iPowerDownDfc.Enque();		// schedules DFC to execute on this driver's thread
	}

/**
Schedule the power-up DFC
*/
void DLcdPowerHandler::PowerUp()
	{
	iPowerUpDfc.Enque();		// schedules DFC to execute on this driver's thread
	}

/**
Power up the display

@param aSecure ETrue if powering up the secure display
*/
void DLcdPowerHandler::PowerUpLcd(TBool aSecure)
    {

	// TO DO: (mandatory)
	// Power up the display and configure it ready for use
	// Examples of things that may need initializing are:
	// panel dimensions
	// line & frame timings
	// bits-per-pixel
	// Configuring the DMA engine to map the video buffer in ivRamPhys or iSecurevRamPhys
	// contrast, brightness, backlight
	// power
	// etc. etc.
	//
    }

/**
Power down the display and the backlight
*/
void DLcdPowerHandler::PowerDownLcd()
    {
	SetBacklightState(EFalse);

	// TO DO: (mandatory)
	// Power down the display & disable LCD DMA.
	// May need to wait until the current frame has been output
	//
    }

/**
Set the Lcd contrast

@param aValue the contrast setting
*/
TInt DLcdPowerHandler::SetContrast(TInt aValue)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("SetContrast(%d)", aValue));

	if (aValue >= KConfigLcdMinDisplayContrast && aValue <= KConfigLcdMaxDisplayContrast)
		{
		iContrast=aValue;
		
		// TO DO: (mandatory)
		// set the contrast
		//
		return KErrNone;
		}

	return KErrArgument;
	}

/**
Set the Lcd brightness

@param aValue the brightness setting
*/
TInt DLcdPowerHandler::SetBrightness(TInt aValue)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("SetBrightness(%d)", aValue));

	if (aValue >= KConfigLcdMinDisplayBrightness && aValue <= KConfigLcdMaxDisplayBrightness)
		{
		iBrightness=aValue;

		// TO DO: (mandatory)
		// set the brightness
		//
		return KErrNone;
		}
	return KErrArgument;
	}

/**
Turn the backlight on
*/
void DLcdPowerHandler::BacklightOn()
    {
	// TO DO: (mandatory)
	// turn the backlight on
	//
    }

/**
Turn the backlight off
*/
void DLcdPowerHandler::BacklightOff()
    {
	// TO DO: (mandatory)
	// turn the backlight off
	//
    }

/**
Set the state of the backlight

@param aState ETrue if setting the backlight on
*/
void DLcdPowerHandler::SetBacklightState(TBool aState)
	{
	iBacklightOn=aState;
	if (iBacklightOn)
		BacklightOn();
	else
		BacklightOff();
	}

void DLcdPowerHandler::ScreenInfo(TScreenInfoV01& anInfo)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("DLcdPowerHandler::ScreenInfo"));
	anInfo.iWindowHandleValid=EFalse;
	anInfo.iWindowHandle=NULL;
	anInfo.iScreenAddressValid=ETrue;
	anInfo.iScreenAddress=(TAny *)(iChunk->LinearAddress());
	anInfo.iScreenSize.iWidth=KConfigLcdWidth;
	anInfo.iScreenSize.iHeight=KConfigLcdHeight;
	}

/**
Handle a message from the power handler
*/
void DLcdPowerHandler::HandleMsg(void)
	{
	
	TMessageBase* msg = iMsgQ.iMessage;
	if (msg == NULL)
		return;

	if (msg->iValue)
		DisplayOn();
	else
		DisplayOff();
	msg->Complete(KErrNone,ETrue);
	}

/**
Send a message to the power-handler message queue to turn the display on
*/
void DLcdPowerHandler::WsSwitchOnScreen()
	{
	TThreadMessage& m=Kern::Message();
	m.iValue = ETrue;
	m.SendReceive(&iMsgQ);		// send a message and block Client thread until keyboard has been powered up
	}

/**
Send a message to the power-handler message queue to turn the display off
*/
void DLcdPowerHandler::WsSwitchOffScreen()
	{
	TThreadMessage& m=Kern::Message();
	m.iValue = EFalse;
	m.SendReceive(&iMsgQ);		// send a message and block Client thread until keyboard has been powered down
	}

/**
Return information about the current display mode

@param	aInfo a structure supplied by the caller to be filled by this function.
@param	aSecure ETrue if requesting information about the secure display
@return	KErrNone if successful
*/
TInt DLcdPowerHandler::GetCurrentDisplayModeInfo(TVideoInfoV01& aInfo, TBool aSecure)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("GetCurrentDisplayModeInfo"));
	NKern::FMWait(&iLock);
	if (aSecure)
 		aInfo = iSecureVideoInfo;
 	else
 		aInfo = iVideoInfo;
	NKern::FMSignal(&iLock);
	return KErrNone;
	}

/**
Return information about the specified display mode

@param	aMode the display mode to query
@param	aInfo a structure supplied by the caller to be filled by this function.
@return	KErrNone if successful
*/
TInt DLcdPowerHandler::GetSpecifiedDisplayModeInfo(TInt aMode, TVideoInfoV01& aInfo)
	{
	__KTRACE_OPT(KEXTENSION,Kern::Printf("GetSpecifiedDisplayModeInfo mode is %d",aMode));

	if (aMode < 0 || aMode >= KConfigLcdNumberOfDisplayModes)
		return KErrArgument;

	NKern::FMWait(&iLock);
	aInfo = iVideoInfo;
	NKern::FMSignal(&iLock);

	if (aMode != aInfo.iDisplayMode)
		{
		aInfo.iOffsetToFirstPixel=Lcd_Mode_Config[aMode].iOffsetToFirstVideoBuffer;
		aInfo.iIsPalettized = Lcd_Mode_Config[aMode].iIsPalettized;
		aInfo.iOffsetBetweenLines=Lcd_Mode_Config[aMode].iOffsetBetweenLines;
		aInfo.iBitsPerPixel = Lcd_Mode_Config[aMode].iBitsPerPixel;
		}
	return KErrNone;
	}

/**
Set the display mode

@param	aMode the display mode to set
*/
TInt DLcdPowerHandler::SetDisplayMode(TInt aMode)
	{

	__KTRACE_OPT(KEXTENSION,Kern::Printf("SetDisplayMode = %d", aMode));

	if (aMode < 0 || aMode >= KConfigLcdNumberOfDisplayModes)
		return KErrArgument;

	NKern::FMWait(&iLock);

	// store the current mode
	iVideoInfo.iDisplayMode = aMode;
	iVideoInfo.iOffsetToFirstPixel = Lcd_Mode_Config[aMode].iOffsetToFirstVideoBuffer;
	iVideoInfo.iIsPalettized = Lcd_Mode_Config[aMode].iIsPalettized;
	iVideoInfo.iOffsetBetweenLines = Lcd_Mode_Config[aMode].iOffsetBetweenLines;
	iVideoInfo.iBitsPerPixel = Lcd_Mode_Config[aMode].iBitsPerPixel;

	// store the current mode for secure screen
	iSecureVideoInfo.iDisplayMode = aMode;
	iSecureVideoInfo.iOffsetToFirstPixel = Lcd_Mode_Config[aMode].iOffsetToFirstVideoBuffer;
	iSecureVideoInfo.iIsPalettized = Lcd_Mode_Config[aMode].iIsPalettized;
	iSecureVideoInfo.iOffsetBetweenLines = Lcd_Mode_Config[aMode].iOffsetBetweenLines;
	iSecureVideoInfo.iBitsPerPixel = Lcd_Mode_Config[aMode].iBitsPerPixel;
	
	// TO DO: (mandatory)
	// set bits per pixel on hardware
	// May need to reconfigure DMA if video buffer size and location have changed
	//
	NKern::FMSignal(&iLock);

	__KTRACE_OPT(KEXTENSION,Kern::Printf("SetDisplayMode mode = %d, otfp = %d, palettized = %d, bpp = %d, obl = %d",
		aMode, iVideoInfo.iOffsetToFirstPixel, iVideoInfo.iIsPalettized, iVideoInfo.iBitsPerPixel, iVideoInfo.iOffsetBetweenLines));

	return KErrNone;
	}

/**
Fill the video memory with an initial pattern or image
This will be displayed on boot-up
*/
void DLcdPowerHandler::SplashScreen()
	{
	// TO DO: (optional)
	// replace the example code below to display a different spash screen

	// initialise the video ram to be a splash screen
	__KTRACE_OPT(KEXTENSION,Kern::Printf("SplashScreen"));

	TUint x,y;
	TUint8 * p = (TUint8*)(iVideoInfo.iVideoAddress + iVideoInfo.iOffsetToFirstPixel);

	//draw >< on screen
	TUint rsh = KConfigLcdHeight;
	TUint rsw = KConfigLcdWidth;
	for (y = 0; y < rsh>>1; y++)
		{
		TUint8* q = p;
		for (x = 0; x < rsw; x++)
			*p++ = (x < y || (rsw-x<y)) ? 1 : 2;
		p = q + iVideoInfo.iOffsetBetweenLines;
		}
	for (y = rsh>>1; y < rsh; y++)
		{
		TUint8* q = p;
		for (x = 0; x < rsw; x++)
			*p++ = ((x < rsh-y) || (rsw-x<rsh-y)) ? 1 : 2;
		p = q + iVideoInfo.iOffsetBetweenLines;
		}

	p = (TUint8*)(iSecureVideoInfo.iVideoAddress + iSecureVideoInfo.iOffsetToFirstPixel);

	//draw >< on secure screen
	for (y = 0; y < rsh>>1; y++)
		{
		TUint8* q = p;
		for (x = 0; x < rsw; x++)
			*p++ = (x < y || (rsw-x<y)) ? 1 : 2;
		p = q + iSecureVideoInfo.iOffsetBetweenLines;
		}
	for (y = rsh>>1; y < rsh; y++)
		{
		TUint8* q = p;
		for (x = 0; x < rsw; x++)
			*p++ = ((x < rsh-y) || (rsw-x<rsh-y)) ? 1 : 2;
		p = q + iSecureVideoInfo.iOffsetBetweenLines;
		}
	}


/**
Get the size of the pallete

@return	the number of pallete entries
*/
TInt DLcdPowerHandler::NumberOfPaletteEntries()		//only call when holding mutex
	{
	// TO DO: (mandatory)
	// Calculate the number of Palette entries - this is normally 
	// calculated from the bits per-pixel.
	// This is only example code... you may need to modify it for your hardware
	//
	TInt num = iVideoInfo.iIsPalettized ? 1<<iVideoInfo.iBitsPerPixel : 0;

	__KTRACE_OPT(KEXTENSION,Kern::Printf("NumberOfPaletteEntries = %d", num));

	return num;
	}


/** 
Retrieve the palette entry at a particular offset

@param	aEntry the palette index
@param	aColor a caller-supplied pointer to a location where the returned RGB color is to be stored
@return	KErrNone if successful
		KErrNotSupported if the current vide mode does not support a palette
		KErrArgument if aEntry is out of range
*/
TInt DLcdPowerHandler::GetPaletteEntry(TInt aEntry, TInt* aColor)
	{
	NKern::FMWait(&iLock);
	if (!iVideoInfo.iIsPalettized)
		{
		NKern::FMSignal(&iLock);
		return KErrNotSupported;
		}

	if ((aEntry < 0) || (aEntry >= NumberOfPaletteEntries()))
		{
		NKern::FMSignal(&iLock);
		return KErrArgument;
		}

	// TO DO: (mandatory)
	// read the RGB value of the palette entry into aColor
	// NOTE: the palette could either be a buffer allocated in system RAM (usually contiguous to Video buffer)
	//		 or could be offered as part of the hardware block that implemenst the lcd control
	//
	NKern::FMSignal(&iLock);

	__KTRACE_OPT(KEXTENSION,Kern::Printf("GetPaletteEntry %d color 0x%x", aEntry, aColor));

	return KErrNone;
	}

/** 
Set the palette entry at a particular offset

@param	aEntry the palette index
@param	aColor the RGB color to store
@return	KErrNone if successful
		KErrNotSupported if the current vide mode does not support a palette
		KErrArgument if aEntry is out of range
*/
TInt DLcdPowerHandler::SetPaletteEntry(TInt aEntry, TInt aColor)
	{

	NKern::FMWait(&iLock);
	if (!iVideoInfo.iIsPalettized)
		{
		NKern::FMSignal(&iLock);
		return KErrNotSupported;
		}

	if ((aEntry < 0) || (aEntry >= NumberOfPaletteEntries()))	//check entry in range
		{
		NKern::FMSignal(&iLock);
		return KErrArgument;
		}

	// TO DO: (mandatory)
	// update the palette entry for the secure and non-secure screen
	// NOTE: the palette could either be a buffer allocated in system RAM (usually contiguous to Video buffer)
	//		 or could be offered as part of the hardware block that implemenst the lcd control
	//
	__KTRACE_OPT(KEXTENSION,Kern::Printf("SetPaletteEntry %d to 0x%x", aEntry, aColor ));

	return KErrNone;
	}

/**
a HAL entry handling function for HAL group attribute EHalGroupDisplay

@param	a1 an arbitrary argument
@param	a2 an arbitrary argument
@return	KErrNone if successful
*/
TInt DLcdPowerHandler::HalFunction(TInt aFunction, TAny* a1, TAny* a2)
	{
	__e32_memory_barrier(); // Ensure changes from other clients are picked up

	TInt r=KErrNone;
	switch(aFunction)
		{
		case EDisplayHalScreenInfo:
			{
			TPckgBuf<TScreenInfoV01> vPckg;
			ScreenInfo(vPckg());
			Kern::InfoCopy(*(TDes8*)a1,vPckg);
			break;
			}

		case EDisplayHalWsRegisterSwitchOnScreenHandling:
			iWsSwitchOnScreen=(TBool)a1;
			break;
		
		case EDisplayHalWsSwitchOnScreen:
			WsSwitchOnScreen();
			break;

		case EDisplayHalMaxDisplayContrast:
			{
			TInt mc=KConfigLcdMaxDisplayContrast;
			kumemput32(a1,&mc,sizeof(mc));
			break;
			}
		case EDisplayHalSetDisplayContrast:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetDisplayContrast")))
				return KErrPermissionDenied;
			r=SetContrast(TInt(a1));
			break;
		
		case EDisplayHalDisplayContrast:
			kumemput32(a1,&iContrast,sizeof(iContrast));
			break;

		case EDisplayHalMaxDisplayBrightness:
			{
			TInt mc=KConfigLcdMaxDisplayBrightness;
			kumemput32(a1,&mc,sizeof(mc));
			break;
			}
		
		case EDisplayHalSetDisplayBrightness:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetDisplayBrightness")))
				return KErrPermissionDenied;
			r=SetBrightness(TInt(a1));
			break;
		
		case EDisplayHalDisplayBrightness:
			kumemput32(a1,&iBrightness,sizeof(iBrightness));
			break;
		
		case EDisplayHalSetBacklightOn:
			if(!Kern::CurrentThreadHasCapability(ECapabilityWriteDeviceData,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetBacklightOn")))
				return KErrPermissionDenied;
			if (Kern::MachinePowerStatus()<ELow)
				r=KErrBadPower;
			else
				SetBacklightState(TBool(a1));
			break;
		
		case EDisplayHalBacklightOn:
			kumemput32(a1,&iBacklightOn,sizeof(TInt));
			break;

		case EDisplayHalModeCount:
			{
			TInt ndm = KConfigLcdNumberOfDisplayModes;
			kumemput32(a1, &ndm, sizeof(ndm));
			break;
			}
		
		case EDisplayHalSetMode:
			if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetMode")))
				return KErrPermissionDenied;
			r = SetDisplayMode((TInt)a1);
			break;
		
		case EDisplayHalMode:
			kumemput32(a1, &iVideoInfo.iDisplayMode, sizeof(iVideoInfo.iDisplayMode));
			break;

		case EDisplayHalSetPaletteEntry:
			if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetPaletteEntry")))
				return KErrPermissionDenied;
			r = SetPaletteEntry((TInt)a1, (TInt)a2);
			break;
		
		case EDisplayHalPaletteEntry:
			{
			TInt entry;
			kumemget32(&entry, a1, sizeof(TInt));
			TInt x;
			r = GetPaletteEntry(entry, &x);
			if (r == KErrNone)
				kumemput32(a2, &x, sizeof(x));
			break;
			}
		
		case EDisplayHalSetState:
			{
			if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetState")))
				return KErrPermissionDenied;
			if ((TBool)a1)
				{
				WsSwitchOnScreen();
				}
			else
				{
				WsSwitchOffScreen();
				}
			break;
			}

		case EDisplayHalState:
			kumemput32(a1, &iDisplayOn, sizeof(TBool));
			break;

		case EDisplayHalColors:
			{
			TInt mdc = KConfigLcdMaxDisplayColors;
			kumemput32(a1, &mdc, sizeof(mdc));
			break;
			}

		case EDisplayHalCurrentModeInfo:
			{
			TPckgBuf<TVideoInfoV01> vPckg;
			r = GetCurrentDisplayModeInfo(vPckg(), (TBool)a2);
			if (KErrNone == r)
				Kern::InfoCopy(*(TDes8*)a1,vPckg);
			}
			break;

		case EDisplayHalSpecifiedModeInfo:
			{
			TPckgBuf<TVideoInfoV01> vPckg;
			TInt mode;
			kumemget32(&mode, a1, sizeof(mode));
			r = GetSpecifiedDisplayModeInfo(mode, vPckg());
			if (KErrNone == r)
				Kern::InfoCopy(*(TDes8*)a2,vPckg);
			}
			break;
			
		case EDisplayHalSecure:
			kumemput32(a1, &iSecureDisplay, sizeof(TBool));
			break;

		case EDisplayHalSetSecure:
			{
			if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetSecure")))
				return KErrPermissionDenied;
			SwitchDisplay((TBool)a1);
			}
			break;

		default:
			r=KErrNotSupported;
			break;
		}

	__e32_memory_barrier(); // Ensure any changes are propagated to other clients

	return r;
	}


DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KPOWER,Kern::Printf("Starting LCD power manager"));

	// create LCD power handler
	TInt r=KErrNoMemory;
	DLcdPowerHandler* pH=new DLcdPowerHandler;
	if (pH)
		r=pH->Create();

	__KTRACE_OPT(KPOWER,Kern::Printf("Returns %d",r));
	return r;
	}

