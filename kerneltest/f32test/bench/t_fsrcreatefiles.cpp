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
// f32test\bench\t_fsrcreatefiles.cpp
// 
//


#include <f32file.h>
#include <e32test.h>
#include "t_benchmain.h"

GLDEF_D RTest test(_L("File Server Benchmarks, create files"));
LOCAL_D TDriveList gDriveList;

/** Creates files

	@param aSelector Configuration in case of manual execution
*/
LOCAL_C TInt TestAll(TAny* aSelector)
	{
	TInt r = 0;
	TTime startTime;
	TTime endTime;
	TTimeIntervalSeconds timeTaken;

	Validate(aSelector);
	
	gFormat = EFalse; 	// The card won't be formatted after this test execution
	
	startTime.HomeTime();
	
	TestFileCreate(aSelector);
	
	endTime.HomeTime();
	r = endTime.SecondsFrom(startTime, timeTaken);
	FailIfError(r);
	test.Printf(_L("#~TS_Timing_%d,%d=%d\n"), gTestHarness, gTestCase, timeTaken.Int());

	return KErrNone;
	}

/** Call all tests

*/
GLDEF_C void CallTestsL()
	{

	CSelectionBox* TheSelector = CSelectionBox::NewL(test.Console());
	// Each test case of the suite has an identifyer for parsing purposes of the results
	gTestHarness = 0; 	

	
	if(gMode == 0) 
		{ // Manual
		gSessionPath=_L("?:\\");
		TCallBack createFiles(TestFileCreate,TheSelector);
		TheSelector->AddDriveSelectorL(TheFs);
		TheSelector->AddLineL(_L("Create all files"),createFiles);
		TheSelector->Run();
		}
	else 
		{ // Automatic
		TestAll(TheSelector);
		}
		
	delete TheSelector;
	}
