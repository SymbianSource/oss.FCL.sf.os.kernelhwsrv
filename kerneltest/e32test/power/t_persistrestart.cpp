// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\t_persistrestart.cpp
// 
//

#include <e32hal.h>
#include <e32modes.h>
#include <e32power.h>
#include <e32test.h>
#include <hal.h>
#include <f32file.h>
#include <e32uid.h>

#define TEST_STARTUP_MODE
#define TEST_RESTART

const TInt KStartupModeLess = -1;

GLDEF_D RTest test(_L("Persist startup mode test"));

void DoTests()
	{
	TInt r;
	TInt startupMode = -1;
	TInt maxStartupModes = -1;
	TInt maxCustomRestartReasons = -1;

	// Read the largest possible persistable value (via HAL Custom Restart)
	r = HAL::Get( HALData::EMaximumCustomRestartReasons, maxCustomRestartReasons );
	test( r == KErrNone );
	test.Printf(_L("Fetching largest possible persistable value (via HAL Custom Restart)..\nmaxCustomRestartReasons = %d\n"), maxCustomRestartReasons);

	// Read the largest possible persistable value
	r = HAL::Get( HALData::EMaximumRestartStartupModes, maxStartupModes );
	test( r == KErrNone );
	test.Printf(_L("Fetching largest possible persistable value..\nmaxStartupModes = %d\n"), maxStartupModes);

	// Read the restart reason
	r = RProperty::Get(KUidSystemCategory, KSystemStartupModeKey, startupMode);
	test( r == KErrNone );
	test.Printf(_L("Reading the stored restart value..\nstartupMode = %d\n"), startupMode);

	// If the restart reason is a default value then it means that the board wasn't restarted with a restart reason.
	if( startupMode == EStartupModeUndefined )
		{
		if ( maxStartupModes != (TInt)0xffffffff )
			{
			// Persist the startup mode
#ifdef TEST_STARTUP_MODE
			// Test erroneous values first
			r = HAL::Set( HALData::EPersistStartupModeKernel, KStartupModeLess );
			test( r == KErrArgument );
	
			TUint StartupModeMore = maxStartupModes + 1;
			r = HAL::Set( HALData::EPersistStartupModeKernel, StartupModeMore );
			test( r == KErrArgument );		
#endif
			}
		// Then give a proper value
		r = HAL::Set( HALData::EPersistStartupModeKernel, maxStartupModes );
		test( r == KErrNone );

		// Persist contents of HAL file via HALSettings.exe
		RProcess process;
		r = process.Create(_L("HALSettings.exe"), _L("PERSIST"));
		test(r == KErrNone);
		TRequestStatus status;
		process.Logon(status);
		process.Resume();
		User::WaitForRequest(status);
		process.Close();

#ifdef TEST_RESTART
		// Go ahead and restart the board using a restart reason
		test.Printf(_L("Enabling wake up power events to restart..\n"));
		r = Power::EnableWakeupEvents(EPwRestart);
		test( r == KErrNone );

		test.Printf(_L("Restarting..\n"));
		r = Power::PowerDown();
		test( r == KErrNone );
#endif
		}

	// If the restart reason is within an allowed range it means that the board has indeed been restarted with a restart reason.
	if( startupMode >= 0 && startupMode <= maxStartupModes )
		{
		// Report the restart reason and check whether it's of the same value with the original restart reason (a constant)
		if ( startupMode != maxStartupModes )
			test.Printf(_L("\nStartup mode was NOT successfully persisted across system restart.\nStored startup mode = %d"), startupMode);
		else
			test.Printf(_L("\nStartup mode (%d) was successfully persisted across system restart.\n"), startupMode);
		// If the comparison is successful, then exit. Otherwise return the error code.
		return;
		}
	}

GLDEF_C TInt E32Main()
//
// Test restarting and persisting a startup mode
//
    {
	test.Start(_L("Test restarting and persisting a startup mode"));
	DoTests();
 	return(KErrNone);
    }
