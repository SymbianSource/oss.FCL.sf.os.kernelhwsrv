// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\window\t_wsimp.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32test.h>
#include <e32ver.h>
#include <e32hal.h>
#include <e32twin.h>
#include <e32svr.h>
#include <hal.h>

LOCAL_D RConsole theConsole;

LOCAL_C void readKeys()
//
// Read keys until ESC pressed
//
    {

	TRequestStatus s;
	TConsoleKey k;
    do
        {
       	theConsole.Read(k,s);
       	User::WaitForRequest(s);
        } while((TInt)k.Code()!=EKeyEscape);
    }

LOCAL_C void printf(TRefByValue<const TDesC> aFmt,...)
//
// Print to the console
//
	{

	if (theConsole.Handle()==KNullHandle)
		{
		TInt r=theConsole.Init(_L("T_SWIMP"),TSize(KConsFullScreen,KConsFullScreen));
		__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Open-Console"),0));
		r=theConsole.Control(_L("+Maximize +NewLine -Lock -Wrap"));
		__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Config-Console"),0));
		}
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<0x100> aBuf;
	aBuf.AppendFormatList(aFmt,list);
	TInt r=theConsole.Write(aBuf);
	__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Write-Console"),0));
	}


GLDEF_C TInt E32Main()
//
// Test the various kernel types.
//
    {

	TVersion v(KE32MajorVersionNumber,KE32MinorVersionNumber,KE32BuildVersionNumber);
	TName vName = v.Name();
	printf(_L("T_SWIMP %S\n"),&vName);
	TName uvName = User::Version().Name();
	printf(_L("Epoc/32 %S\n"),&uvName);
//
	TRequestStatus s;
	TConsoleKey k;
	theConsole.Read(k,s);
	theConsole.ReadCancel();
	User::WaitForRequest(s);
	__ASSERT_ALWAYS(s.Int()==KErrCancel || s.Int()==KErrNone, User::Panic(_L("Test Fail"),1));
//
	TInt r=theConsole.Control(_L("+Raw"));
	__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Set Raw Mode"),0));
//
	printf(_L("Raw mode - hit ESC to continue\r\n"));
//
    FOREVER
        {
       	theConsole.Read(k,s);
       	User::WaitForRequest(s);
		if (k.Type()==TRawEvent::EKeyDown)
			{
			if (k.Code()==4)
				break;
			printf(_L("Down %d\r\n"),k.Code());
			}
		else if (k.Type()==TRawEvent::EKeyUp)
			printf(_L("Up %d\r\n"),k.Code());
        }
//
	r=theConsole.Control(_L("-Raw"));
	__ASSERT_ALWAYS(r==KErrNone,User::Panic(_L("Reset Raw Mode"),0));
//
	printf(_L("Normal mode - hit ESC to continue\r\n"));
//
    FOREVER
        {
       	theConsole.Read(k,s);
       	User::WaitForRequest(s);
		if (k.Code()==EKeyEscape)
			break;
		printf(_L("Key %d\r\n"),k.Code());
		}
//
    printf(_L("Completed OK\n"));
//
    printf(_L("Key click set to LOUD - press keys, ESC to continue...\n"));

	TInt max;
	HAL::Get(HAL::EKeyboardClickVolumeMax, max);
	HAL::Set(HAL::EKeyboardClickVolume, max);
	HAL::Set(HAL::EKeyboardClickState, ETrue);

    readKeys();

    printf(_L("Key click set to SOFT...\n"));

	HAL::Set(HAL::EKeyboardClickVolume, 0);
	HAL::Set(HAL::EKeyboardClickState, ETrue);

    readKeys();

    printf(_L("Key click set to LOUD...\n"));

	HAL::Set(HAL::EKeyboardClickVolume, max);
	HAL::Set(HAL::EKeyboardClickState, ETrue);

    readKeys();

    printf(_L("Key click set to LOUD and OFF...\n"));

	HAL::Set(HAL::EKeyboardClickVolume, max);
	HAL::Set(HAL::EKeyboardClickState, EFalse);

    readKeys();

	return(KErrNone);
    }

