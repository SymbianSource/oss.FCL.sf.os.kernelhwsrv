// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/main.cpp
// USB Test Program, main part.
// Device-side part, to work against t_usb_host running on the host.
// 
//

#include "general.h"
#include "config.h"
#include "activecontrol.h"
#include "activerw.h"

// --- Global Top Level Variables

CActiveControl* gActiveControl;
#ifdef USB_SC
RTest test(_L("T_USB_SCDEVICE"));
#else
RTest test(_L("T_USB_DEVICE"));
#endif

#ifdef USB_SC	
TBool gShareHandle = EFalse;
#endif
TBool gVerbose = EFalse;
TBool gSkip = EFalse;
TBool gTempTest = EFalse;
TBool gStopOnFail = ETrue;
TBool gAltSettingOnNotify = ETrue;
TInt8 gSettingNumber [128];
TInt32 gSoakCount = 1;
CActiveRW* gRW[KMaxConcurrentTests];							// the USB read/write active object
IFConfigPtr gInterfaceConfig [128] [KMaxInterfaceSettings];
TInt gActiveTestCount = 0;
#ifdef USB_SC
RChunk gChunk;
#endif
	

void RunAppL(TDes * aConfigFile, TDes * aScriptFile)
	{
	// Construct the active scheduler
	CActiveScheduler* myScheduler = new (ELeave) CActiveScheduler();

	// Push active scheduler onto the cleanup stack
	CleanupStack::PushL(myScheduler);

	// Install as the active scheduler
	CActiveScheduler::Install(myScheduler);

	// Create console handler
	#ifdef USB_SC
	CConsoleBase* myConsole = Console::NewL(_L("T_USB_SCDEVICE - USB Client Test"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(myConsole);

	myConsole->Printf(_L("T_USB_SCDEVICE v%d.%d.%d\n"),KDeviceVersionMajor,KDeviceVersionMinor,KDeviceVersionMicro);
	test.Printf(_L("T_USB_SCDEVICE v%d.%d.%d\n"),KDeviceVersionMajor,KDeviceVersionMinor,KDeviceVersionMicro);
	#else
	CConsoleBase* myConsole = Console::NewL(_L("T_USB_DEVICE - USB Client Test"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(myConsole);

	myConsole->Printf(_L("T_USB_DEVICE v%d.%d.%d\n"),KDeviceVersionMajor,KDeviceVersionMinor,KDeviceVersionMicro);
	test.Printf(_L("T_USB_DEVICE v%d.%d.%d\n"),KDeviceVersionMajor,KDeviceVersionMinor,KDeviceVersionMicro);
	#endif
	
	// outermost test begin
	test.Start(_L("Outermost test of t_usb_device\n"));

	do
		{
		gActiveControl = CActiveControl::NewL(myConsole, aConfigFile, aScriptFile);
		CleanupStack::PushL(gActiveControl);

	
		// Call request function
#ifdef USB_SC		
		if (!gShareHandle)
			gActiveControl->RequestEp0ControlPacket();
#else
		gActiveControl->RequestEp0ControlPacket();
#endif
		CActiveScheduler::Start();
		
		test.Printf (_L("Test Run Completed\n"));
		
		if (gSoakCount > 0)
			{
			gSoakCount--;
			}

		// Suspend thread for 2 seconds
		User::After(2000000);
		
		CleanupStack::PopAndDestroy(gActiveControl);

		}
	while ((gSoakCount > 0) || (gSoakCount == -1));

	// outermost test end
	test.End();
	test.Close();

	CleanupStack::PopAndDestroy(myConsole);

	CleanupStack::PopAndDestroy(myScheduler);

	return;
	}

void ParseCommandLine (TDes& aConfigFileName, TDes& aScriptFileName)
	{
	TBuf<64> c;
	
	User::CommandLine(c);
	c.LowerCase();

	aConfigFileName.SetLength(0);
	aScriptFileName.SetLength(0);
	if (c != KNullDesC)
		{
		TLex lex(c);
		TPtrC token;

		while (token.Set(lex.NextToken()), token != KNullDesC)
			{
			if (token == _L("/v"))
				{
				RDebug::Print(_L("Verbose output enabled\n"));
				gVerbose = ETrue;
				}
			else if (token == _L("/s"))
				{
				RDebug::Print(_L("Skipping some tests\n"));
				gSkip = ETrue;
				}
			else if (token == _L("/t"))
				{
				RDebug::Print(_L("Temporary Test\n"));
				gTempTest = ETrue;
				}
			else if (token == _L("/n"))
				{
				RDebug::Print(_L("Not Stopping on Test Fail\n"));
				gStopOnFail = EFalse;
				}
#ifdef USB_SC	
			else if (token == _L("/a"))
				{				
				RDebug::Print(_L("share handle test\n"));
				gShareHandle = ETrue;
				}
#endif
			else if (token.Left(5) == _L("/soak"))
				{
				TInt equalPos;
				gSoakCount = -1;
				equalPos = token.Locate('=');
				if ((equalPos+1) < token.Length())
					{
					TLex lexNum(token.Mid(equalPos+1));
					lexNum.Val(gSoakCount,EDecimal);	
					}
				RDebug::Print(_L("Soak test for %d iterations\n"),gSoakCount);
				}
			else if (token.Left(8) == _L("/script="))
				{
				aScriptFileName = token;
				}
			else
				{
				aConfigFileName = token;
				}
			}
		}
		
	}
	
TInt E32Main()
	{
	__UHEAP_MARK;
	
	CTrapCleanup* cleanup = CTrapCleanup::New();			// get clean-up stack

	TBuf<64> configFileName;
	TBuf<64> scriptFileName;
	ParseCommandLine (configFileName,scriptFileName);
	
	if (configFileName.Length() == 0)
		{
			RDebug::Print(_L("(T_USB: Warning - No Configuration File.)\n"));		
		}
	else
		{
		RDebug::Print(_L("T_USB: Config File Name %s\n"),configFileName.PtrZ());
		}

	if (scriptFileName.Length() != 0)
		{
		RDebug::Print(_L("T_USB: Script File Name %s\n"),scriptFileName.PtrZ());
		}

	TRAPD(error, RunAppL(& configFileName, &scriptFileName));

	__ASSERT_ALWAYS(!error, User::Panic(_L("T_USB_DEVICE: EPOC32EX"), error));

	delete cleanup;											// destroy clean-up stack

	__UHEAP_MARKEND;

	RDebug::Print(_L("Program exit: done.\n"));
	return 0;												// and return
	}


// -eof-
