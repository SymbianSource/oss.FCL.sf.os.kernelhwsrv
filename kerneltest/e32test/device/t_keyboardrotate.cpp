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
// e32test\device\t_keyboardrotate.cpp
// Keyboard Rotation test file
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32event.h>
#include <e32event_private.h>
#include <hal.h>

 //---------------------------------------------
 //! @SYMTestCaseID PBASE_KEYBOARDROTATE
 //! @SYMTestType UT
 //! @SYMTestCaseDesc Manual test of keyboard rotation modifiers.
 //! @SYMREQ CR - APOS-666C3S 
 //! @SYMTestActions Select each of 4 possible keyboard rotations and verify correct behaviour.
 //! @SYMTestExpectedResults The described section of the keyboard is rotated as displayed by the test.
 //! @SYMTestPriority High
 //---------------------------------------------

LOCAL_D RTest test(_L("T_KEYBOARDROTATE"));

GLDEF_C TInt E32Main()	
	{

	test.Title();
	test.Start(_L("Test Keyboard Rotation Modifiers\n"));
	test.Printf(_L("The following 3x3 grid of keys can be rotated: \n\n"));
	test.Printf(_L("Q W E\n\n"));
	test.Printf(_L("A S D\n\n"));
	test.Printf(_L("Z X C\n\n"));

	TKeyCode response;
	TRawEvent event;

	TInt machineuid;
	HAL::Get(HAL::EMachineUid, machineuid);
	if(machineuid != HAL::EMachineUid_Lubbock && machineuid != HAL::EMachineUid_Win32Emulator)
		{
		test.Printf(_L("Test only supports Lubbock platform and emulator\n"));
		test.End();
		return(0);
		}

	for(;;)
		{
		test.Printf(_L("Select desired option: \n\n"));
		test.Printf(_L("(1) Rotate 90 deg clockwise\n"));
		test.Printf(_L("(2) Rotate 180 deg clockwise\n"));
		test.Printf(_L("(3) Rotate 270 deg clockwise\n"));
		test.Printf(_L("(4) Return to default layout\n\n"));
		test.Printf(_L("Press any other key to exit\n\n"));

		response=test.Getch();

		switch((TInt)response)
			{
			case '1':
				event.Set(TRawEvent::EUpdateModifiers, EModifierRotateBy90);
				UserSvr::AddEvent(event);
				test.Printf(_L("The keyboard layout is now as follows: \n\n"));
				test.Printf(_L("Z A Q\n\n"));
				test.Printf(_L("X S W\n\n"));
				test.Printf(_L("C D E\n\n"));
				break;

			case '2':
				event.Set(TRawEvent::EUpdateModifiers, EModifierRotateBy180);
				UserSvr::AddEvent(event);
				test.Printf(_L("The keyboard layout is now as follows: \n\n"));
				test.Printf(_L("C X Z\n\n"));
				test.Printf(_L("D S A\n\n"));
				test.Printf(_L("E W Q\n\n"));
				break;

			case '3':
				event.Set(TRawEvent::EUpdateModifiers, EModifierRotateBy270);
				UserSvr::AddEvent(event);
				test.Printf(_L("The keyboard layout is now as follows: \n\n"));
				test.Printf(_L("E D C\n\n"));
				test.Printf(_L("W S X\n\n"));
				test.Printf(_L("Q A Z\n\n"));
				break;

			case '4':
				event.Set(TRawEvent::EUpdateModifiers, EModifierCancelRotation);
				UserSvr::AddEvent(event);
				test.Printf(_L("The keyboard layout is now as follows: \n\n"));
				test.Printf(_L("Q W E\n\n"));
				test.Printf(_L("A S D\n\n"));
				test.Printf(_L("Z X C\n\n"));
				break;

			default:
				event.Set(TRawEvent::EUpdateModifiers, EModifierCancelRotation);
				UserSvr::AddEvent(event);
				test.End();
				return(0);
			}
		
		test.Printf(_L("Test the new keyboard layout below and press enter when finished:\n"));

		while(response!=EKeyEnter)
			{
			response=test.Getch();
			test.Printf(TPtrC((TUint16 *)&response, 1));
			}

		test.Printf(_L("\n\n"));
		}
    }

