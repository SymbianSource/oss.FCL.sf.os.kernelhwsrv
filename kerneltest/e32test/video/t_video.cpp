// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Overview:
// Test the video driver kernel extension
// API Information:
// HAL, UserSvr
// Details:
// - Get and report the value for brightness & max brightness. Adjust 
// the brightness. Verify results are as expected.
// - Get and report the value for contrast & max contrast. Adjust the 
// contrast. Verify results are as expected.
// - Get and set backlight status, verify results.
// - Get the number of display modes and the current display mode.
// - Get screen information for current display mode.
// - Get Bits per pixel for current display mode, for an illegal mode 
// and for all modes.
// - Switch display modes and verify results are as expected.
// - Get and set palette entries, verify the results.
// - Turn the display on and off.
// - If additional screens are supported, test each screen as above.
// - Test more devices than the kernel supports, verify the results.
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <videodriver.h>
#include <hal.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_VIDEO"));

LOCAL_C void RunTests(void);

#ifndef __WINS__
#define DUMP(x) test.Printf(_L(#x"= %d =0x%08x\n"), x, x)
#endif

LOCAL_C void RunTests(void)
	{
	TInt ret = KErrNone;
	TInt HALArg;
	TInt saved = 0;

/*	BRIGHTNESS	*/
	
	TBool HALMaxBrightnessSupported = EFalse;
	TBool HALGetBrightnessSupported = EFalse;
	TBool HALSetBrightnessSupported = EFalse;

	test.Next(_L("Max Brightness using HAL"));
	TInt maxBrightness=-1;
	ret = HAL::Get(HAL::EDisplayBrightnessMax, maxBrightness);
	test ((KErrNone == ret) || (KErrNotSupported == ret));

	if (KErrNone == ret)
		{
		HALMaxBrightnessSupported = ETrue;
		test.Printf(_L("Maximum brightness = %d\n"), maxBrightness);
		}
	else
		test.Printf(_L("Maximum brightness is NOT SUPPORTED by HAL\n"));


	test.Next(_L("Get Brightness using HAL"));
	HALArg = -1;
	ret = HAL::Get(HAL::EDisplayBrightness, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		test.Printf(_L("Brightness = %d\n"), HALArg);
		HALGetBrightnessSupported = ETrue;
		saved = HALArg;
		}
	else
		{
		test.Printf(_L("Get Brightness is NOT SUPPORTED by HAL\n"));
		}

	test.Next(_L("Test brightness is <= maxBrightness"));
	test(HALArg <= maxBrightness);

	test.Next(_L("Set Brightness using HAL"));
	ret = HAL::Set(HAL::EDisplayBrightness, 0);
	test ((KErrNone == ret) || (KErrNotSupported == ret) || (KErrArgument == ret));
	if ((KErrNone == ret) || (KErrArgument == ret))
		HALSetBrightnessSupported = ETrue;
	else
		test.Printf(_L("Set brightness is NOT SUPPORTED by HAL\n"));


		
	//if any of the brightness funcs are supported, test they all are
	//we've already tested Ldd/HAL are giving same support
	if (HALSetBrightnessSupported && HALGetBrightnessSupported && HALMaxBrightnessSupported)
		{
		//all supported
		//do more comprehensive set/gets
		test.Next(_L("Set Brightness using HAL to saved value"));
		ret = HAL::Set(HAL::EDisplayBrightness, saved);
		test (KErrNone == ret);

		test.Next(_L("Get Brightness using HAL"));
		HALArg = -1;
		ret = HAL::Get(HAL::EDisplayBrightness, HALArg);
		test (KErrNone == ret);
		test (saved == HALArg);
	
		test.Next(_L("Set Brightness to the max using HAL"));
		ret = HAL::Set(HAL::EDisplayBrightness, maxBrightness);
		test.Printf(_L("ret = %d maxbr = %d"),ret, maxBrightness);
		test (KErrNone == ret);

		test.Next(_L("Get Brightness using HAL"));
		HALArg = 0;
		ret = HAL::Get(HAL::EDisplayBrightness, HALArg);
		test (KErrNone == ret);
		test (maxBrightness == HALArg);

		test.Next(_L("Set Brightness using HAL"));
		ret = HAL::Set(HAL::EDisplayBrightness, saved);
		test (KErrNone == ret);


		//test some out of range values
		ret = HAL::Get(HAL::EDisplayBrightness, HALArg);
		test (KErrNone == ret);
		saved = HALArg;

		test.Next(_L("Set Brightness too large using HAL"));
		ret = HAL::Set(HAL::EDisplayBrightness, maxBrightness+1);
		test (KErrArgument == ret);

		test.Next(_L("Set Brightness too small using HAL"));
		ret = HAL::Set(HAL::EDisplayBrightness, -1);
		test (KErrArgument == ret);

		}
		else	//check none are supported
			test(!(HALSetBrightnessSupported || HALGetBrightnessSupported || HALMaxBrightnessSupported));


/*	CONTRAST	*/

	TBool HALMaxContrastSupported = EFalse;
	TBool HALGetContrastSupported = EFalse;
	TBool HALSetContrastSupported = EFalse;
	

	test.Next(_L("Max Contrast using HAL"));
	TInt maxContrast=-1;
	ret = HAL::Get(HAL::EDisplayContrastMax, maxContrast);
	test ((KErrNone == ret) || (KErrNotSupported == ret));

	if (KErrNone == ret)
		{
		HALMaxContrastSupported = ETrue;
		test.Printf(_L("Maximum Contrast = %d\n"), maxContrast);
		}
	else
		test.Printf(_L("Maximum Contrast is NOT SUPPORTED by HAL\n"));


	test.Next(_L("Get Contrast using HAL"));
	HALArg = -1;
	ret = HAL::Get(HAL::EDisplayContrast, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		test.Printf(_L("Contrast = %d\n"), HALArg);
		HALGetContrastSupported = ETrue;
		saved = HALArg;
		}
	else
		{
		test.Printf(_L("Get Contrast is NOT SUPPORTED by HAL\n"));
		}

	test.Next(_L("Test contrast is <= maxcontrast"));
	test(HALArg <= maxContrast);

	test.Next(_L("Set Contrast using HAL"));
	ret = HAL::Set(HAL::EDisplayContrast, 0);
	test ((KErrNone == ret) || (KErrNotSupported == ret) || (KErrArgument == ret));
	if ((KErrNone == ret) || (KErrArgument == ret))
		HALSetContrastSupported = ETrue;
	else
		test.Printf(_L("Set Contrast is NOT SUPPORTED by HAL\n"));


		
	//if any of the Contrast funcs are supported, test they all are
	//we've already tested Ldd/HAL are giving same support
	if (HALSetContrastSupported && HALGetContrastSupported && HALMaxContrastSupported)
		{
		//all supported
		//do more comprehensive set/gets
		test.Next(_L("Set Contrast using HAL to saved value"));
		ret = HAL::Set(HAL::EDisplayContrast, saved);
		test (KErrNone == ret);

		test.Next(_L("Get Contrast using HAL"));
		HALArg = -1;
		ret = HAL::Get(HAL::EDisplayContrast, HALArg);
		test (KErrNone == ret);
		test (saved == HALArg);
	
		test.Next(_L("Set Contrast to the max using HAL"));
		ret = HAL::Set(HAL::EDisplayContrast, maxContrast);
		test (KErrNone == ret);

		test.Next(_L("Get Contrast using HAL"));
		HALArg = 0;
		ret = HAL::Get(HAL::EDisplayContrast, HALArg);
		test (KErrNone == ret);
		test (maxContrast == HALArg);

		test.Next(_L("Set Contrast using HAL"));
		ret = HAL::Set(HAL::EDisplayContrast, saved);
		test (KErrNone == ret);


		//test some out of range values
		ret = HAL::Get(HAL::EDisplayContrast, HALArg);
		test (KErrNone == ret);
		saved = HALArg;

		test.Next(_L("Set Contrast too large using HAL"));
		ret = HAL::Set(HAL::EDisplayContrast, maxContrast+1);
		test (KErrArgument == ret);

		test.Next(_L("Set Contrast too small using HAL"));
		ret = HAL::Set(HAL::EDisplayContrast, -1);
		test (KErrArgument == ret);

		}
		else	//check none are supported
			test(!(HALSetContrastSupported || HALGetContrastSupported || HALMaxContrastSupported));

		
		
/*	BACKLIGHT	*/

	TBool HALGetBacklightSupported = EFalse;
	TBool HALSetBacklightSupported = EFalse;
	TBool lightSupported = EFalse;

	test.Next(_L("check if backlight supported using HAL"));
	HALArg = -1;
	ret = HAL::Get(HAL::EBacklight, lightSupported);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	test.Printf(_L("Backlight supported = %d"), lightSupported);



	test.Next(_L("Get Backlight state using HAL"));
	HALArg = -1;
	ret = HAL::Get(HAL::EBacklightState, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		HALGetBacklightSupported = ETrue;
		test.Printf(_L("Backlight is = %d from HAL\n"), HALArg);
		}
	else
		test.Printf(_L("Get Light is NOT SUPPORTED by HAL\n"));


	test.Next(_L("Set Backlight state using HAL"));
	HALArg = 0;
	ret = HAL::Set(HAL::EBacklightState, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		HALSetBacklightSupported = ETrue;
		test.Printf(_L("Backlight is set to = %d from HAL\n"), HALArg);
		}
	else
		test.Printf(_L("Set Light is NOT SUPPORTED by HAL\n"));




	if 	(HALGetBacklightSupported && HALSetBacklightSupported)
		{

		test.Next(_L("Get Backlight state using HAL (should be off)"));
		HALArg = -1;
		ret = HAL::Get(HAL::EBacklightState, HALArg);
		test (KErrNone == ret);
		test (0 == HALArg);

		test.Next(_L("Set Backlight state to on using HAL"));
		ret = HAL::Set(HAL::EBacklightState, 1);
		test (KErrNone == ret);

		}
	else
		test (!HALGetBacklightSupported == !HALSetBacklightSupported);


	/* maximum display colors*/
	test.Next(_L("Display Colors"));
	ret = HAL::Get(HAL::EDisplayColors, HALArg);
	test (KErrNone == ret);

	
	/*  DISPLAY MODE */
	test.Next(_L("Display Modes"));
	TInt totalModes;
	ret = HAL::Get(HAL::EDisplayNumModes, totalModes);
	test (KErrNone == ret);

	TInt displayMode;
	ret = HAL::Get(HAL::EDisplayMode, displayMode);
	test (KErrNone == ret);
	
	

	/* SCREEN INFORMATION*/

	test.Next(_L("Get Screen Info for current mode using driver"));


	test.Next(_L("Get screen info using usersvr"));
	TScreenInfoV01 screenInfo;
	TPckg<TScreenInfoV01> sI(screenInfo);
	UserSvr::ScreenInfo(sI);
	test (screenInfo.iScreenAddressValid != screenInfo.iWindowHandleValid);
	if (screenInfo.iScreenAddressValid)
		test (screenInfo.iScreenAddress != 0);
	if (screenInfo.iWindowHandleValid)
		test (screenInfo.iWindowHandle != 0);


	TInt val;
	test.Next(_L("Get DisplayXPixels using HAL"));
	ret = HAL::Get(HAL::EDisplayXPixels, val);
	test (KErrNone == ret);
	test (val == screenInfo.iScreenSize.iWidth);

	test.Next(_L("Get DisplayYPixels using HAL"));
	ret = HAL::Get(HAL::EDisplayYPixels, val);
	test (KErrNone == ret);
	test (val == screenInfo.iScreenSize.iHeight);

	TInt xtwips;
	test.Next(_L("Get DisplayXTwips using HAL"));
	ret = HAL::Get(HAL::EDisplayXTwips, xtwips);
	test (KErrNone == ret);

	TInt ytwips;
	test.Next(_L("Get DisplayYTwips using HAL"));
	ret = HAL::Get(HAL::EDisplayYTwips, ytwips);
	test (KErrNone == ret);

	TInt vaddr;
	test.Next(_L("Get video address using HAL"));
	ret = HAL::Get(HAL::EDisplayMemoryAddress, vaddr);
	test (KErrNone == ret);
#ifndef __WINS__
	test (vaddr == (TInt)screenInfo.iScreenAddress);
#else
	test (vaddr == (TInt)screenInfo.iWindowHandle);
#endif

	test.Next(_L("Use machine info"));

	TMachineInfoV1 mi;
	TMachineInfoV1Buf mib;

	UserHal::MachineInfo(mib);
	mi = mib();

	test.Printf(_L("si.iWidth = %d,  si.iHeight = %d,  mi.iWidth = %d, mi.iHeight = %d\n"),screenInfo.iScreenSize.iWidth, screenInfo.iScreenSize.iHeight, mi.iDisplaySizeInPixels.iWidth, mi.iDisplaySizeInPixels.iHeight);
	test.Printf(_L("xtwips = %d,  ytwips = %d,  iWidth = %d, iHeight = %d\n"),xtwips, ytwips, mi.iPhysicalScreenSize.iWidth, mi.iPhysicalScreenSize.iHeight);

	test (screenInfo.iScreenSize.iWidth == mi.iDisplaySizeInPixels.iWidth);
	test (screenInfo.iScreenSize.iHeight == mi.iDisplaySizeInPixels.iHeight);
	test (xtwips == mi.iPhysicalScreenSize.iWidth);
	test (ytwips == mi.iPhysicalScreenSize.iHeight);


/* BITS PER PIXEL */

	test.Next(_L("Get Bits per pixel for current display mode using HAL"));

	HALArg = displayMode;
	ret = HAL::Get(HAL::EDisplayBitsPerPixel, HALArg);
	test (KErrNone == ret);


	test.Next(_L("Get Bits per pixel with illegal mode using HAL"));
	HALArg = -1;
	ret = HAL::Get(HAL::EDisplayBitsPerPixel, HALArg);
	test (KErrArgument == ret);

	HALArg = totalModes;
	ret = HAL::Get(HAL::EDisplayBitsPerPixel, HALArg);
	test (KErrArgument == ret);

	
/*DISPLAY MODES*/
	test.Next(_L("loop through the display modes getting the info"));

	TInt count;


	for (count = 0; count < totalModes; count++)
		{

		test.Next(_L("Offset To first pixel"));
		HALArg = count;
		ret = HAL::Get(HAL::EDisplayOffsetToFirstPixel, HALArg);
		test (KErrNone == ret);
		test (HALArg >= 0);

		test.Next(_L("Test Offset between lines is > 0"));
		HALArg = count;
		ret = HAL::Get(HAL::EDisplayOffsetBetweenLines, HALArg);
		test (KErrNone == ret);
#ifndef __WINS__
		test (HALArg > 0);
#else		
		test.Printf(_L("WINS can return 0 here as it doesn't handle the buffer itself, target hardware must return > 0"));
		test (HALArg >= 0);
#endif
		test.Next(_L("is display mono"));
		HALArg = count;
		ret = HAL::Get(HAL::EDisplayIsMono, HALArg);
		test (KErrNone == ret);
		
		test.Next(_L("is display palettized"));
		HALArg = count;
		ret = HAL::Get(HAL::EDisplayIsPalettized, HALArg);
		test (KErrNone == ret);

		test.Next(_L("bits per pixel"));
		HALArg = count;
		ret = HAL::Get(HAL::EDisplayBitsPerPixel, HALArg);
		test (KErrNone == ret);

		}


	test.Next(_L("switch display modes must be supported if > 1 display mode"));

	TInt oldMode = displayMode;
#ifndef __X86__
	if (totalModes > 1)
		{
		HALArg = displayMode;
		ret = HAL::Set(HAL::EDisplayMode, HALArg);
		test.Printf(_L("ret is %d dmode is %d\n"),ret, HALArg);
		test (KErrNone == ret);

		ret = HAL::Get(HAL::EDisplayMode, HALArg);
		test (KErrNone == ret);
		test (HALArg == displayMode);

		}
#endif
	for (count = 0; count < totalModes; count++)
		{
			
#ifndef __X86__
		if (totalModes > 1)  //we must support mode change
			{
			test.Printf(_L("Setting Display Mode to %d\n"), count);

			ret = HAL::Set(HAL::EDisplayMode, count);
			test (KErrNone == ret);

			ret = HAL::Get(HAL::EDisplayMode, HALArg);
			test (KErrNone == ret);
			test (HALArg == count);
			}
#endif

		/* PALETTE */

		//get the palette entries
		//set a few to something else
		//set them again

		TInt palettized = count;
		test.Next(_L("Get if we are using a palette using HAL"));
		ret = HAL::Get(HAL::EDisplayIsPalettized, palettized);
		test (KErrNone == ret);

		if (palettized)
			{
			HALArg = count;
			ret = HAL::Get(HAL::EDisplayBitsPerPixel, HALArg);
			test (KErrNone == ret);
			test.Printf(_L("Bitsperpixel is  %d\n"),HALArg);
			test (HALArg <= 8);

			TInt max = (1 << HALArg) - 1;
			test.Printf(_L("number of palette entries is %d\n"),max);
			
			test.Next(_L("Get legal Palette entries using HAL and driver in loop"));
			for (TInt x = 0; x <= max; x++)
				{
				HALArg = x;
				ret = HAL::Get(HAL::EDisplayPaletteEntry, HALArg);
				test.Printf(_L("getting entry %d, ret is %d\n"),x, ret);
				test (KErrNone == ret);

				}


			//try a few sets
			TInt saved;

			test.Next(_L("Set Palette entry 0 to red using HAL"));

			saved = 0;
			ret = HAL::Get(HAL::EDisplayPaletteEntry, saved);
			test (KErrNone == ret);

			HALArg = 0xF80000;
			ret = HAL::Set(HAL::EDisplayPaletteEntry, HALArg);
			test (KErrNone == ret || KErrNotSupported == ret);
			
			if (KErrNone == ret)
				{
				HALArg = 0;
				ret = HAL::Get(HAL::EDisplayPaletteEntry, HALArg);
				test (KErrNone == ret);
				test ((HALArg & 0xF8FFFF)==0xF80000);

				ret = HAL::Set(HAL::EDisplayPaletteEntry, saved);
				test (KErrNone == ret);


				HALArg = 1;
				ret = HAL::Get(HAL::EDisplayPaletteEntry, HALArg);
				test (KErrNone == ret);


				HALArg = (7 << 24) || 0xFFFF00;
				ret = HAL::Set(HAL::EDisplayPaletteEntry, HALArg);
				test (KErrNone == ret);

			
				}
				

			
			HALArg = count;
			test (KErrNone == HAL::Get(HAL::EDisplayBitsPerPixel, HALArg));

			if (4 == HALArg)
				{
				test.Next(_L("Get Illegal palette entry using HAL"));
				HALArg = 18;
				ret = HAL::Get(HAL::EDisplayPaletteEntry, HALArg);
				test (KErrArgument == ret);


				test.Next(_L("Set Illegal palette entry using HAL"));
				HALArg = 0x12 << 24 ;
				ret = HAL::Set(HAL::EDisplayPaletteEntry, HALArg);
				test (KErrArgument == ret);
				}
		
			}
		else
			{
			//not palettized
			test.Next(_L("Get palette entry using HAL - should fail"));
			HALArg = 0;
			ret = HAL::Get(HAL::EDisplayPaletteEntry, HALArg);
			test (KErrNotSupported == ret);
			}

	
		}		

#ifndef __X86__
	if (totalModes > 1)  //we must support mode change
		{
		ret = HAL::Set(HAL::EDisplayMode, oldMode);
		test (KErrNone == ret);
		}
#endif
	
	
	
	
	/*	DISPLAY ON/OFF	*/

	TInt displayState;

	test.Next(_L("Check Display is on using HAL"));
	displayState = EFalse;
	ret = HAL::Get(HAL::EDisplayState, displayState);
	test (KErrNone == ret);
	test (displayState!=EFalse);

	test.Next(_L("Turn Display Off using HAL"));
	ret = HAL::Set(HAL::EDisplayState, 0);
	test (KErrNone == ret || KErrNotSupported == ret);

	if (KErrNone == ret)
		{
//		test.Next(_L("Check Display is off using HAL"));
		displayState = ETrue;
		ret = HAL::Get(HAL::EDisplayState, displayState);
		test (KErrNone == ret);
		test (displayState==EFalse);

	//	test.Next(_L("Display On using HAL"));
	//	ret = HAL::Set(HAL::EDisplayState, 1);
	//	test (KErrNotSupported == ret);
		

	//need some way of turning on the display!
		RTimer timer;
		test(timer.CreateLocal()==KErrNone);
		TTime now;
		now.HomeTime();
		TTime wakeup;
		wakeup=now+TTimeIntervalSeconds(10);
		TRequestStatus done;
		timer.At(done,wakeup);

		UserHal::SwitchOff();
		User::WaitForRequest(done);

		TRawEvent switchon;
		switchon.Set(TRawEvent::ESwitchOn);
		UserSvr::AddEvent(switchon);
		}
	else
		test.Printf(_L("Display On/Off not supported by HAL on this playform\n"));

	
	test.Next(_L("Check Display On using HAL"));
	displayState = EFalse;
	ret = HAL::Get(HAL::EDisplayState, displayState);
	test (KErrNone == ret);
	test (displayState!=EFalse);


// !!! Disable platform security tests until we get the new APIs
/*	
	test.Next(_L("Check if secure screen supported"));
	TInt secure = EFalse;
	ret = HAL::Get(HAL::ESecureDisplay, secure);
	test (KErrNone == ret || KErrNotSupported == ret);
	if (KErrNone == ret)
		{
		//get the secure address
		TInt addr = 0;
		ret = HAL::Get(HAL::ESecureDisplayMemoryAddress, addr);
		test (KErrNone == ret);

		//switch to secure screen
		ret = HAL::Set(HAL::ESecureDisplay, ETrue);
		test (KErrNone == ret);
		User::After(2000000);

		//switch to insecure screen
		ret = HAL::Set(HAL::ESecureDisplay, EFalse);
		test (KErrNone == ret);
		User::After(2000000);

		//switch to secure screen
		ret = HAL::Set(HAL::ESecureDisplay, ETrue);
		test (KErrNone == ret);
		User::After(2000000);

		//switch to insecure screen
		ret = HAL::Set(HAL::ESecureDisplay, EFalse);
		test (KErrNone == ret);
		}
	else
		test.Printf(_L("secure screen not supported on this platform\n"));
*/

	}


//the function above tests HAL APIs where no screen number is specified
//and implicitly screen 0 is assumed. This function runs only for additional
//screens if any (screen1, screen2 etc.)
//this uses the HAL APIs that take a screen number
LOCAL_C void RunTestsAdditionalScreens(TInt screen)
	{
	TInt ret = KErrNone;
	TInt HALArg;
	TInt saved = 0;

/*	BRIGHTNESS	*/
	
	TBool HALMaxBrightnessSupported = EFalse;
	TBool HALGetBrightnessSupported = EFalse;
	TBool HALSetBrightnessSupported = EFalse;
	

	test.Next(_L("Max Brightness using HAL"));
	TInt maxBrightness=-1;
	ret = HAL::Get(screen, HAL::EDisplayBrightnessMax, maxBrightness);
	test ((KErrNone == ret) || (KErrNotSupported == ret));

	if (KErrNone == ret)
		{
		HALMaxBrightnessSupported = ETrue;
		test.Printf(_L("Maximum brightness = %d\n"), maxBrightness);
		}
	else
		test.Printf(_L("Maximum brightness is NOT SUPPORTED by HAL\n"));


	test.Next(_L("Get Brightness using HAL"));
	HALArg = -1;
	ret = HAL::Get(screen, HAL::EDisplayBrightness, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		test.Printf(_L("Brightness = %d\n"), HALArg);
		HALGetBrightnessSupported = ETrue;
		saved = HALArg;
		}
	else
		{
		test.Printf(_L("Get Brightness is NOT SUPPORTED by HAL\n"));
		}

	test.Next(_L("Test brightness is <= maxBrightness"));
	test(HALArg <= maxBrightness);

	test.Next(_L("Set Brightness using HAL"));
	ret = HAL::Set(screen, HAL::EDisplayBrightness, 0);
	test ((KErrNone == ret) || (KErrNotSupported == ret) || (KErrArgument == ret));
	if ((KErrNone == ret) || (KErrArgument == ret))
		HALSetBrightnessSupported = ETrue;
	else
		test.Printf(_L("Set brightness is NOT SUPPORTED by HAL\n"));


		
	//if any of the brightness funcs are supported, test they all are
	//we've already tested Ldd/HAL are giving same support
	if (HALSetBrightnessSupported && HALGetBrightnessSupported && HALMaxBrightnessSupported)
		{
		//all supported
		//do more comprehensive set/gets
		test.Next(_L("Set Brightness using HAL to saved value"));
		ret = HAL::Set(screen, HAL::EDisplayBrightness, saved);
		test (KErrNone == ret);

		test.Next(_L("Get Brightness using HAL"));
		HALArg = -1;
		ret = HAL::Get(screen, HAL::EDisplayBrightness, HALArg);
		test (KErrNone == ret);
		test (saved == HALArg);
	
		test.Next(_L("Set Brightness to the max using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayBrightness, maxBrightness);
		test.Printf(_L("ret = %d maxbr = %d"),ret, maxBrightness);
		test (KErrNone == ret);

		test.Next(_L("Get Brightness using HAL"));
		HALArg = 0;
		ret = HAL::Get(screen, HAL::EDisplayBrightness, HALArg);
		test (KErrNone == ret);
		test (maxBrightness == HALArg);

		test.Next(_L("Set Brightness using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayBrightness, saved);
		test (KErrNone == ret);


		//test some out of range values
		ret = HAL::Get(screen, HAL::EDisplayBrightness, HALArg);
		test (KErrNone == ret);
		saved = HALArg;

		test.Next(_L("Set Brightness too large using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayBrightness, maxBrightness+1);
		test (KErrArgument == ret);

		test.Next(_L("Set Brightness too small using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayBrightness, -1);
		test (KErrArgument == ret);

		}
		else	//check none are supported
			test(!(HALSetBrightnessSupported || HALGetBrightnessSupported || HALMaxBrightnessSupported));


/*	CONTRAST	*/

	TBool HALMaxContrastSupported = EFalse;
	TBool HALGetContrastSupported = EFalse;
	TBool HALSetContrastSupported = EFalse;
	

	test.Next(_L("Max Contrast using HAL"));
	TInt maxContrast=-1;
	ret = HAL::Get(screen, HAL::EDisplayContrastMax, maxContrast);
	test ((KErrNone == ret) || (KErrNotSupported == ret));

	if (KErrNone == ret)
		{
		HALMaxContrastSupported = ETrue;
		test.Printf(_L("Maximum Contrast = %d\n"), maxContrast);
		}
	else
		test.Printf(_L("Maximum Contrast is NOT SUPPORTED by HAL\n"));


	test.Next(_L("Get Contrast using HAL"));
	HALArg = -1;
	ret = HAL::Get(screen, HAL::EDisplayContrast, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		test.Printf(_L("Contrast = %d\n"), HALArg);
		HALGetContrastSupported = ETrue;
		saved = HALArg;
		}
	else
		{
		test.Printf(_L("Get Contrast is NOT SUPPORTED by HAL\n"));
		}

	test.Next(_L("Test contrast is <= maxcontrast"));
	test(HALArg <= maxContrast);

	test.Next(_L("Set Contrast using HAL"));
	ret = HAL::Set(screen, HAL::EDisplayContrast, 0);
	test ((KErrNone == ret) || (KErrNotSupported == ret) || (KErrArgument == ret));
	if ((KErrNone == ret) || (KErrArgument == ret))
		HALSetContrastSupported = ETrue;
	else
		test.Printf(_L("Set Contrast is NOT SUPPORTED by HAL\n"));


		
	//if any of the Contrast funcs are supported, test they all are
	//we've already tested Ldd/HAL are giving same support
	if (HALSetContrastSupported && HALGetContrastSupported && HALMaxContrastSupported)
		{
		//all supported
		//do more comprehensive set/gets
		test.Next(_L("Set Contrast using HAL to saved value"));
		ret = HAL::Set(screen, HAL::EDisplayContrast, saved);
		test (KErrNone == ret);

		test.Next(_L("Get Contrast using HAL"));
		HALArg = -1;
		ret = HAL::Get(screen, HAL::EDisplayContrast, HALArg);
		test (KErrNone == ret);
		test (saved == HALArg);
	
		test.Next(_L("Set Contrast to the max using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayContrast, maxContrast);
		test (KErrNone == ret);

		test.Next(_L("Get Contrast using HAL"));
		HALArg = 0;
		ret = HAL::Get(screen, HAL::EDisplayContrast, HALArg);
		test (KErrNone == ret);
		test (maxContrast == HALArg);

		test.Next(_L("Set Contrast using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayContrast, saved);
		test (KErrNone == ret);


		//test some out of range values
		ret = HAL::Get(screen, HAL::EDisplayContrast, HALArg);
		test (KErrNone == ret);
		saved = HALArg;

		test.Next(_L("Set Contrast too large using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayContrast, maxContrast+1);
		test (KErrArgument == ret);

		test.Next(_L("Set Contrast too small using HAL"));
		ret = HAL::Set(screen, HAL::EDisplayContrast, -1);
		test (KErrArgument == ret);

		}
		else	//check none are supported
			test(!(HALSetContrastSupported || HALGetContrastSupported || HALMaxContrastSupported));

		
		
/*	BACKLIGHT	*/

	TBool HALGetBacklightSupported = EFalse;
	TBool HALSetBacklightSupported = EFalse;
	TBool lightSupported = EFalse;

	test.Next(_L("check if backlight supported using HAL"));
	HALArg = -1;
	ret = HAL::Get(screen, HAL::EBacklight, lightSupported);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	test.Printf(_L("Backlight supported = %d"), lightSupported);



	test.Next(_L("Get Backlight state using HAL"));
	HALArg = -1;
	ret = HAL::Get(screen, HAL::EBacklightState, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		HALGetBacklightSupported = ETrue;
		test.Printf(_L("Backlight is = %d from HAL\n"), HALArg);
		}
	else
		test.Printf(_L("Get Light is NOT SUPPORTED by HAL\n"));


	test.Next(_L("Set Backlight state using HAL"));
	HALArg = 0;
	ret = HAL::Set(screen, HAL::EBacklightState, HALArg);
	test ((KErrNone == ret) || (KErrNotSupported == ret));
	if (KErrNone == ret)
		{
		HALSetBacklightSupported = ETrue;
		test.Printf(_L("Backlight is set to = %d from HAL\n"), HALArg);
		}
	else
		test.Printf(_L("Set Light is NOT SUPPORTED by HAL\n"));




	if 	(HALGetBacklightSupported && HALSetBacklightSupported)
		{

		test.Next(_L("Get Backlight state using HAL (should be off)"));
		HALArg = -1;
		ret = HAL::Get(screen, HAL::EBacklightState, HALArg);
		test (KErrNone == ret);
		test (0 == HALArg);

		test.Next(_L("Set Backlight state to on using HAL"));
		ret = HAL::Set(screen, HAL::EBacklightState, 1);
		test (KErrNone == ret);

		}
	else
		test (!HALGetBacklightSupported == !HALSetBacklightSupported);


	/* maximum display colors*/
	test.Next(_L("Display Colors"));
	ret = HAL::Get(screen, HAL::EDisplayColors, HALArg);
	test (KErrNone == ret);

	
	/*  DISPLAY MODE */
	test.Next(_L("Display Modes"));
	TInt totalModes;
	ret = HAL::Get(screen, HAL::EDisplayNumModes, totalModes);
	test (KErrNone == ret);

	TInt displayMode;
	ret = HAL::Get(screen, HAL::EDisplayMode, displayMode);
	test (KErrNone == ret);
	
	TInt val;
	test.Next(_L("Get DisplayXPixels using HAL"));
	ret = HAL::Get(screen, HAL::EDisplayXPixels, val);
	test (KErrNone == ret);

	test.Next(_L("Get DisplayYPixels using HAL"));
	ret = HAL::Get(screen, HAL::EDisplayYPixels, val);
	test (KErrNone == ret);

	TInt xtwips;
	test.Next(_L("Get DisplayXTwips using HAL"));
	ret = HAL::Get(screen, HAL::EDisplayXTwips, xtwips);
	test (KErrNone == ret);

	TInt ytwips;
	test.Next(_L("Get DisplayYTwips using HAL"));
	ret = HAL::Get(screen, HAL::EDisplayYTwips, ytwips);
	test (KErrNone == ret);

	TInt vaddr;
	test.Next(_L("Get video address using HAL"));
	ret = HAL::Get(screen, HAL::EDisplayMemoryAddress, vaddr);
	test (KErrNone == ret);


/* BITS PER PIXEL */

	test.Next(_L("Get Bits per pixel for current display mode using HAL"));

	HALArg = displayMode;
	ret = HAL::Get(screen, HAL::EDisplayBitsPerPixel, HALArg);
	test (KErrNone == ret);


	test.Next(_L("Get Bits per pixel with illegal mode using HAL"));
	HALArg = -1;
	ret = HAL::Get(screen, HAL::EDisplayBitsPerPixel, HALArg);
	test (KErrArgument == ret);

	HALArg = totalModes;
	ret = HAL::Get(screen, HAL::EDisplayBitsPerPixel, HALArg);
	test (KErrArgument == ret);

	
/*DISPLAY MODES*/
	test.Next(_L("loop through the display modes getting the info"));

	TInt count;

	for (count = 0; count < totalModes; count++)
		{

		test.Next(_L("Offset To first pixel"));
		HALArg = count;
		ret = HAL::Get(screen, HAL::EDisplayOffsetToFirstPixel, HALArg);
		test (KErrNone == ret);
		test (HALArg >= 0);

		test.Next(_L("Test Offset between lines is > 0"));
		HALArg = count;
		ret = HAL::Get(screen, HAL::EDisplayOffsetBetweenLines, HALArg);
		test (KErrNone == ret);
#ifndef __WINS__
		test (HALArg > 0);
#else		
		test.Printf(_L("WINS can return 0 here as it doesn't handle the buffer itself, target hardware must return > 0"));
		test (HALArg >= 0);
#endif
		test.Next(_L("is display mono"));
		HALArg = count;
		ret = HAL::Get(screen, HAL::EDisplayIsMono, HALArg);
		test (KErrNone == ret);
		
		test.Next(_L("is display palettized"));
		HALArg = count;
		ret = HAL::Get(screen, HAL::EDisplayIsPalettized, HALArg);
		test (KErrNone == ret);

		test.Next(_L("bits per pixel"));
		HALArg = count;
		ret = HAL::Get(screen, HAL::EDisplayBitsPerPixel, HALArg);
		test (KErrNone == ret);

		}


	test.Next(_L("switch display modes must be supported if > 1 display mode"));

	TInt oldMode = displayMode;
#ifndef __X86__
	if (totalModes > 1)
		{
		HALArg = displayMode;
		ret = HAL::Set(screen, HAL::EDisplayMode, HALArg);
		test.Printf(_L("ret is %d dmode is %d\n"),ret, HALArg);
		test (KErrNone == ret);

		ret = HAL::Get(screen, HAL::EDisplayMode, HALArg);
		test (KErrNone == ret);
		test (HALArg == displayMode);

		}
#endif
	for (count = 0; count < totalModes; count++)
		{
			
#ifndef __X86__
		if (totalModes > 1)  //we must support mode change
			{
			test.Printf(_L("Setting Display Mode to %d\n"), count);

			ret = HAL::Set(screen, HAL::EDisplayMode, count);
			test (KErrNone == ret);

			ret = HAL::Get(screen, HAL::EDisplayMode, HALArg);
			test (KErrNone == ret);
			test (HALArg == count);
			}
#endif

		/* PALETTE */

		//get the palette entries
		//set a few to something else
		//set them again

		TInt palettized = count;
		test.Next(_L("Get if we are using a palette using HAL"));
		ret = HAL::Get(screen, HAL::EDisplayIsPalettized, palettized);
		test (KErrNone == ret);

		if (palettized)
			{
			HALArg = count;
			ret = HAL::Get(screen, HAL::EDisplayBitsPerPixel, HALArg);
			test (KErrNone == ret);
			test.Printf(_L("Bitsperpixel is  %d\n"),HALArg);
			test (HALArg <= 8);

			TInt max = (1 << HALArg) - 1;
			test.Printf(_L("number of palette entries is %d\n"),max);
			
			test.Next(_L("Get legal Palette entries using HAL and driver in loop"));
			for (TInt x = 0; x <= max; x++)
				{
				HALArg = x;
				ret = HAL::Get(screen, HAL::EDisplayPaletteEntry, HALArg);
				test.Printf(_L("getting entry %d, ret is %d\n"),x, ret);
				test (KErrNone == ret);

				}


			//try a few sets
			TInt saved;

			test.Next(_L("Set Palette entry 0 to red using HAL"));

			saved = 0;
			ret = HAL::Get(screen, HAL::EDisplayPaletteEntry, saved);
			test (KErrNone == ret);

			HALArg = 0xFF0000;
			ret = HAL::Set(screen, HAL::EDisplayPaletteEntry, HALArg);
			test (KErrNone == ret || KErrNotSupported == ret);
			
			if (KErrNone == ret)
				{
				HALArg = 0;
				ret = HAL::Get(screen, HAL::EDisplayPaletteEntry, HALArg);
				test (KErrNone == ret);
				test (HALArg = 0xFF0000);

				ret = HAL::Set(screen, HAL::EDisplayPaletteEntry, saved);
				test (KErrNone == ret);


				HALArg = 1;
				ret = HAL::Get(screen, HAL::EDisplayPaletteEntry, HALArg);
				test (KErrNone == ret);


				HALArg = (7 << 24) || 0xFFFF00;
				ret = HAL::Set(screen, HAL::EDisplayPaletteEntry, HALArg);
				test (KErrNone == ret);

			
				}
				

			
			HALArg = count;
			test (KErrNone == HAL::Get(screen, HAL::EDisplayBitsPerPixel, HALArg));

			if (4 == HALArg)
				{
				test.Next(_L("Get Illegal palette entry using HAL"));
				HALArg = 18;
				ret = HAL::Get(screen, HAL::EDisplayPaletteEntry, HALArg);
				test (KErrArgument == ret);


				test.Next(_L("Set Illegal palette entry using HAL"));
				HALArg = 0x12 << 24 ;
				ret = HAL::Set(screen, HAL::EDisplayPaletteEntry, HALArg);
				test (KErrArgument == ret);
				}
		
			}
		else
			{
			//not palettized
			test.Next(_L("Get palette entry using HAL - should fail"));
			HALArg = 0;
			ret = HAL::Get(screen, HAL::EDisplayPaletteEntry, HALArg);
			test (KErrNotSupported == ret);
			}

	
		}		

#ifndef __X86__
	if (totalModes > 1)  //we must support mode change
		{
		ret = HAL::Set(screen, HAL::EDisplayMode, oldMode);
		test (KErrNone == ret);
		}
#endif
	
	
	
	
	/*	DISPLAY ON/OFF	*/

	
	// get current display state
	TInt curDisplayState;
	ret = HAL::Get(screen, HAL::EDisplayState, curDisplayState);
	test (KErrNone == ret);

	test.Next(_L("Turn Display on using HAL"));
	ret = HAL::Set(screen, HAL::EDisplayState, 1);
	test (KErrNone == ret || KErrNotSupported == ret);

	TInt displayState;

	test.Next(_L("Check Display is on using HAL"));
	displayState = EFalse;
	ret = HAL::Get(screen, HAL::EDisplayState, displayState);
	test (KErrNone == ret);
	test (displayState!=EFalse);

	test.Next(_L("Turn Display Off using HAL"));
	ret = HAL::Set(screen, HAL::EDisplayState, 0);
	test (KErrNone == ret || KErrNotSupported == ret);

	if (KErrNone == ret)
		{
		test.Next(_L("Check Display is off using HAL"));
		displayState = ETrue;
		ret = HAL::Get(screen, HAL::EDisplayState, displayState);
		test (KErrNone == ret);
		test (displayState==EFalse);

	//	test.Next(_L("Display On using HAL"));
	//	ret = HAL::Set(screen, HAL::EDisplayState, 1);
	//	test (KErrNotSupported == ret);
		

	//need some way of turning on the display!
		RTimer timer;
		test(timer.CreateLocal()==KErrNone);
		TTime now;
		now.HomeTime();
		TTime wakeup;
		wakeup=now+TTimeIntervalSeconds(10);
		TRequestStatus done;
		timer.At(done,wakeup);

		UserHal::SwitchOff();
		User::WaitForRequest(done);

		TRawEvent switchon;
		switchon.Set(TRawEvent::ESwitchOn);
		UserSvr::AddEvent(switchon);
		}
	else
		test.Printf(_L("Display On/Off not supported by HAL on this playform\n"));

	// restore the original display state
	ret = HAL::Set(screen, HAL::EDisplayState, curDisplayState);
	test (KErrNone == ret || KErrNotSupported == ret);


// !!! Disable platform security tests until we get the new APIs
/*	
	test.Next(_L("Check if secure screen supported"));
	TInt secure = EFalse;
	ret = HAL::Get(screen, HAL::ESecureDisplay, secure);
	test (KErrNone == ret || KErrNotSupported == ret);
	if (KErrNone == ret)
		{
		//get the secure address
		TInt addr = 0;
		ret = HAL::Get(screen, HAL::ESecureDisplayMemoryAddress, addr);
		test (KErrNone == ret);

		//switch to secure screen
		ret = HAL::Set(screen, HAL::ESecureDisplay, ETrue);
		test (KErrNone == ret);
		User::After(2000000);

		//switch to insecure screen
		ret = HAL::Set(screen, HAL::ESecureDisplay, EFalse);
		test (KErrNone == ret);
		User::After(2000000);

		//switch to secure screen
		ret = HAL::Set(screen, HAL::ESecureDisplay, ETrue);
		test (KErrNone == ret);
		User::After(2000000);

		//switch to insecure screen
		ret = HAL::Set(screen, HAL::ESecureDisplay, EFalse);
		test (KErrNone == ret);
		}
	else
		test.Printf(_L("secure screen not supported on this platform\n"));
*/

	}


GLDEF_C TInt E32Main()
//
//
    {

	test.Title();
//
#if defined(__EPOC32__) && defined(__CPU_X86)
	test.Printf(_L("Doesn't run on X86\n"));
#else

	test.Start(_L("Testing Video extension"));

	RunTests();
	TInt screens=1;	// assume that we have at least 1 screen in case the HAL attr isn't supported
	TInt ret=HAL::Get(HAL::EDisplayNumberOfScreens, screens);
	test((ret==KErrNone) || (ret==KErrNotSupported));

	TInt i;
	for(i=1;i<screens;i++)
		RunTestsAdditionalScreens(i);

	test.Next(_L("Test more devices than the kernel can handle"));
	//this constant should have a value > KMaxHalEntries (defined in the kernel)
	const TInt KMoreThanKernelAllocates=10;
	for(i=screens;i<KMoreThanKernelAllocates;i++)
		{
		TInt val;
		test.Next(_L("Get DisplayXPixels using HAL"));
		ret = HAL::Get(i, HAL::EDisplayXPixels, val);
		test (KErrNotSupported == ret);

		test.Next(_L("Get DisplayYPixels using HAL"));
		ret = HAL::Get(i, HAL::EDisplayYPixels, val);
		test (KErrNotSupported == ret);

		TInt xtwips;
		test.Next(_L("Get DisplayXTwips using HAL"));
		ret = HAL::Get(i, HAL::EDisplayXTwips, xtwips);
		test (KErrNotSupported == ret);

		TInt ytwips;
		test.Next(_L("Get DisplayYTwips using HAL"));
		ret = HAL::Get(i, HAL::EDisplayYTwips, ytwips);
		test (KErrNotSupported == ret);

		TInt vaddr;
		test.Next(_L("Get video address using HAL"));
		ret = HAL::Get(i, HAL::EDisplayMemoryAddress, vaddr);
		test (KErrNotSupported == ret);
		}

	test.Next(_L("Test HAL::GetAll"));
	TInt numEntries;
	HAL::SEntry* entries;
	ret=HAL::GetAll(numEntries, entries);
	test(ret==KErrNone);

	test(numEntries==screens*(TInt)HAL::ENumHalAttributes);

	User::Free(entries);

	test.Next(_L("Test TRawEvent for multiple devices (screens)"));

	TRawEvent event1;
	test(event1.Type()==0);
	test(event1.DeviceNumber()==KUndefinedDeviceNumber);

	event1.SetDeviceNumber(0);
	test(event1.DeviceNumber()==0);

	event1.SetDeviceNumber(KUndefinedDeviceNumber);
	test(event1.DeviceNumber()==KUndefinedDeviceNumber);

	test(event1.Type()==0);

	event1.Set(TRawEvent::EKeyDown);
	test(event1.Type()==TRawEvent::EKeyDown);

	event1.Set(TRawEvent::EKeyUp,10);
	test(event1.Type()==TRawEvent::EKeyUp);

	event1.Set(TRawEvent::EKeyDown,10,10);
	test(event1.Type()==TRawEvent::EKeyDown);

	test.End();
	test.Close();

#endif

	return KErrNone;
    }

