// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// UI menu file
// 
//

/**
 @file
*/

#include <e32const.h>
#include <e32const_private.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32cons.h>
#include <f32file.h>
#include <hal.h>
#include <u32hal.h>
#include "bootloader_variantconfig.h"
#include <nkern/nk_trace.h>
#include <e32twin.h>

#define FILE_ID	0x01
#include "bootldr.h"
#include <d32comm.h>

// Console screen
RConsole TheConsole;
RThread TheKeyThread;


////////////////////////////////////////////////////////
void menu_displayfn();
LOCAL_C void menu_processkey(TConsoleKey&);
void autoload_displayfn();
LOCAL_C void autoload_processkey(TConsoleKey &aKey);

struct display_process
	{
	void (*iDisplayFn)();
	void (*iProcessKey)(TConsoleKey&);
	};

display_process screens[] =
	{
	{autoload_displayfn, autoload_processkey},
	{menu_displayfn, menu_processkey}
	};

#define SCREEN_AUTOLOAD 0
#define SCREEN_MENU 1
display_process* gCurrentScreen = &screens[SCREEN_AUTOLOAD];
TBool gScreenUpdate;	// Controls whether to redraw the screen

void mainmenu_usbboot(TUint32, TUint32)
	{
	// Turn off raw mode so app receives key events
	TheConsole.Control(_L("-Raw"));
	if(StartUSBMS() == EFalse)
		{
		// Didn't manage to start
		TheConsole.Control(_L("+Raw"));
		TheConsole.SetCursorPosAbs(TPoint(0,15));
		TheConsole.Write(_L("Insert MMC card first!!"));
		TheConsole.SetCursorPosAbs(TPoint(0,16));
		TheConsole.Write(_L("Card required for USB boot"));
		TheConsole.SetCursorPosAbs(TPoint(0,17));
		TheConsole.Write(_L("mode"));
		}
	return;
	}

void mainmenu_serial(TUint32 aPort, TUint32 aBaud)
	{
	TBps baudRate;

	TheConsole.SetCursorPosAbs(TPoint(0,15));
	TBuf<64> outBuf;

	switch (aBaud)
		{
		case 115200:	baudRate = EBps115200; break;
		case 230400:    baudRate = EBps230400; break;
		default:
			outBuf.AppendFormat(_L("Invalid Baud Rate (COM%d,%d)"), aPort, aBaud);
			TheConsole.Write(outBuf);
			return;
		}
	outBuf.AppendFormat(_L("Setting COM%d baud %d"), aPort, aBaud);
	TheConsole.Write(outBuf);

	SerialDownloadPort = aPort;
	SerialBaud         = baudRate;
	WriteConfig();
	Restart(KtRestartReasonHardRestart);
	}

#ifdef __SUPPORT_FLASH_REPRO__
void mainmenu_bootnor(TUint32, TUint32)
	{
	TheConsole.SetCursorPosAbs(TPoint(0,15));
	TheConsole.Write(_L("Attempting to boot from"));
	TheConsole.SetCursorPosAbs(TPoint(0,16));
	TheConsole.Write(_L("onboard NOR flash..."));
	// This will restart the board very quickly - pause a second to let the LCD
	// controler display the message
	User::After(10000);
	Restart(KtRestartReasonBootRestart | KtRestartReasonNORImage);
	}
#endif

#ifdef __SUPPORT_MEMORY_TEST__
void mainmenu_memtest(TUint32, TUint32)
	{
	TheConsole.SetCursorPosAbs(TPoint(0,15));
	TheConsole.Write(_L("Starting Memory Test"));
	TheConsole.SetCursorPosAbs(TPoint(0,16));
	TheConsole.Write(_L("screen will decay..."));
	// This will restart the board very quickly - pause a second to let the LCD
	// controler display the message
	User::After(10000);
	Restart(KtRestartCustomRestartMemCheck);
	}
#endif

#ifdef __SUPPORT_FLASH_NAND__
void mainmenu_bootnand(TUint32, TUint32)
	{
	TheConsole.SetCursorPosAbs(TPoint(0,15));
	TheConsole.Write(_L("Attempting to boot from"));
	TheConsole.SetCursorPosAbs(TPoint(0,16));
	TheConsole.Write(_L("onboard NAND flash..."));
	// This will restart the board very quickly - pause a moment to let the LCD
	// controler display the message
	User::After(10000);
	Restart(KtRestartReasonBootRestart | KtRestartReasonNANDImage);
	}
#endif

#ifdef __SUPPORT_FLASH_ONENAND__
LOCAL_C void mainmenu_bootonenand(TUint32, TUint32)
	{
	TheConsole.SetCursorPosAbs(TPoint(0,15));
	TheConsole.Write(_L("Attempting to boot from"));
	TheConsole.SetCursorPosAbs(TPoint(0,16));
	TheConsole.Write(_L("onboard OneNAND flash..."));
	// This will restart the board very quickly - pause a moment to let the LCD
	// controller display the message
	User::After(10000);
	Restart(KtRestartReasonBootRestart | KtRestartReasonONENANDImage);
	}
#endif

#ifdef __USE_EMBEDDED_MMC_SD__
LOCAL_C void mainmenu_bootmmc(TUint32, TUint32)
	{
	TheConsole.SetCursorPosAbs(TPoint(0,15));
	TheConsole.Write(_L("Attempting to boot from"));
	TheConsole.SetCursorPosAbs(TPoint(0,16));
	TheConsole.Write(_L("Embedded MMC..."));
	// This will restart the board very quickly - pause a moment to let the LCD
	// controller display the message
	User::After(10000);
	Restart(KtRestartReasonBootRestart | KtRestartReasonMMCSDImage);
	}
#endif

// Example of switching screens
//
// void mainmenu_gotoautoload()
//	{
//	// Change screen
//	gCurrentScreen = &screens[SCREEN_AUTOLOAD];
//	gScreenUpdate=ETrue;
//	}

struct menu_item
	{
	TPtrC iTitle;
	void (*iFn)(TUint32, TUint32);
	TUint32 iParam1;
	TUint32 iParam2;
	};

menu_item mainmenu[] =
	{
//	{_L("Autoload mode"), mainmenu_gotoautoload},
#ifdef __USE_USBMS__
	{_L("USB Mass Storage mode"), mainmenu_usbboot, 0, 0},
#endif
#ifdef __SUPPORT_COM0_115200__
	{_L("COM0 @ 115200"), mainmenu_serial, 0, 115200},
#endif
#ifdef __SUPPORT_COM0_230400__
	{_L("COM0 @ 230400"), mainmenu_serial, 0, 230400},
#endif
#ifdef __SUPPORT_COM1_115200__
	{_L("COM1 @ 115200"), mainmenu_serial, 1, 115200},
#endif
#ifdef __SUPPORT_COM1_230400__
	{_L("COM1 @ 230400"), mainmenu_serial, 1, 230400},
#endif
#ifdef __SUPPORT_COM2_115200__
	{_L("COM2 @ 115200"), mainmenu_serial, 2, 115200},
#endif
#ifdef __SUPPORT_COM2_230400__
	{_L("COM2 @ 230400"), mainmenu_serial, 2, 230400},
#endif
#ifdef __SUPPORT_COM3_115200__
	{_L("COM3 @ 115200"), mainmenu_serial, 3, 115200},
#endif
#ifdef __SUPPORT_COM3_230400__
	{_L("COM3 @ 230400"), mainmenu_serial, 3, 230400},
#endif
#ifdef __SUPPORT_COM7_115200__
	{_L("COM7 @ 115200"), mainmenu_serial, 7, 115200},
#endif

#ifdef __SUPPORT_FLASH_REPRO__
	{_L("Try boot NOR flash"), mainmenu_bootnor, 0, 0},
#endif
#ifdef __SUPPORT_FLASH_NAND__
	{_L("Try boot NAND flash"), mainmenu_bootnand, 0, 0},
#endif
#ifdef __SUPPORT_FLASH_ONENAND__
	{_L("Try boot OneNAND flash"), mainmenu_bootonenand, 0, 0},
#endif
#ifdef __USE_EMBEDDED_MMC_SD__
	{_L("Try boot Embedded MMC"), mainmenu_bootmmc, 0, 0},
#endif	
#ifdef __SUPPORT_MEMORY_TEST__
	{_L("Run Memory Test"), mainmenu_memtest, 0, 0},
#endif
	{_L("The End"), NULL},		// DO NOT REMOVE - array terminator
	};

TInt mainmenu_items=0;	// The number of items in the menu is set up when the menu is drawn
TInt mainmenu_idx=0;

// autoload screen functions
void autoload_displayfn()
	{
	TheConsole.ClearScreen();
	TheConsole.SetCursorPosAbs(TPoint(0,0));
	TheConsole.SetTitle(_L("AUTOLOAD"));
	TheConsole.Write(_L("AUTOLOADING on Serial....."));
	}

void autoload_processkey(TConsoleKey &aKey)
{
	TUint code = aKey.Code();
	if (aKey.Type() == 0x3) // key down
		{
		if ((code == 0x3)||(code == 0x4)) // Direction key enter, or Esc
			{
			// Change to menu mode
			gCurrentScreen = &screens[SCREEN_MENU];
			gScreenUpdate=ETrue;
			}
		}
	}

// main menu screen functions
void menu_displayfn()
	{
	TheConsole.ClearScreen();
	TheConsole.SetTitle(_L("Bootloader menu"));
	mainmenu_items=0;
	for (TInt i=0; mainmenu[i].iFn!=NULL ; i++)
		{
		TheConsole.SetCursorPosAbs(TPoint(0,i+1));
		if (mainmenu_idx==i)
			TheConsole.Write(_L("==> "));
		else
			TheConsole.Write(_L("    "));

		TBuf<0x2> buf;
		buf.AppendNum(i);
		TheConsole.Write(buf);
		TheConsole.Write(_L("."));
		
		TheConsole.Write(mainmenu[i].iTitle);
		mainmenu_items++;
		}
	TheConsole.SetCursorPosAbs(TPoint(0,14));
	TheConsole.Write(_L("============================"));
	}

void menu_processkey(TConsoleKey &aKey)
	{

	TUint code=aKey.Code();
	TUint type=aKey.Type();
	if (type == 0x3) // key down
		{
		if (code == 0x3) // Direction key enter
			{
			mainmenu[mainmenu_idx].iFn(mainmenu[mainmenu_idx].iParam1, mainmenu[mainmenu_idx].iParam2);
			}
		else if (code == 0x11) // Direction key down (portrait)
			{
			if (++mainmenu_idx == mainmenu_items)
				mainmenu_idx=0;
			gScreenUpdate=ETrue;
			}
		else if (code == 0x10) // Direction key up (portrait)
			{
			if (--mainmenu_idx<0)
				mainmenu_idx=mainmenu_items-1;
			gScreenUpdate=ETrue;
			}
		else if ((code >= 0x30) && (code <= 0x39))
			{
			// If a number is entered, select the relevant menu option
			TUint num=code-0x30;
			if (num < TUint(mainmenu_items))
				{
				// coverity[overrun-local]
				// Coverity doesn't consider the comparison against mainmenu_items
				mainmenu[num].iFn(mainmenu[num].iParam1, mainmenu[num].iParam2);
				}
			}
		}
	}

// Generic key press processor
void keystate_processor(TConsoleKey &aKey)
	{
	gScreenUpdate=EFalse;
	gCurrentScreen->iProcessKey(aKey);

	if (gScreenUpdate)
		gCurrentScreen->iDisplayFn();
	}

TInt KeyThreadFn(TAny*)
	{
	// Create a console window
	TheConsole.Init(_L(""),TSize(KConsFullScreen,KConsFullScreen));
	// Suppress cursor, stop any resizing, ensure that we get RAW events from
	// the console which means people can't dick with the screen
	TheConsole.Control(_L("-Cursor -Aresize +Raw"));

	while(1)
		{
		TConsoleKey key;
		TheConsole.Read(key);
		//RDebug::Print(_L("code 0x%x\n"), key.Code());
		//RDebug::Print(_L("type 0x%x\n"), key.Type());
		//RDebug::Print(_L("mods 0x%x\n"), key.Modifiers());
		keystate_processor(key);
		}
	}

//////////////////////////////////////////////////////////////////////////////
//
// Application entry point
//
//////////////////////////////////////////////////////////////////////////////
GLDEF_C void StartMenu()
	{
    TInt r = KErrUnknown;
	
	// Create thread for console and key events
	r = TheKeyThread.Create(_L("keythread"),KeyThreadFn,KDefaultStackSize,0x200,0x200,(TAny*)NULL,EOwnerProcess);
	if (r != KErrNone)
		{
		RDebug::Print(_L("FAULT: thread create %d\r\n"),r);
		BOOT_FAULT();
		}
	TheKeyThread.Resume();

	// Pause slightly to let the console draw before continuing
	User::After(100000);
	}

GLDEF_C void EnableMenu()
	{
	// RDebug::Print(_L("Enable\r\n"));
	// Touching TheConsole is not allowed from this context
	// TheConsole.Control(_L("+Raw"));
	}

GLDEF_C void DisableMenu()
	{
	// RDebug::Print(_L("Disable\r\n"));
	// Touching TheConsole is not allowed from this context
	// TheConsole.Control(_L("-Raw"));
	}
