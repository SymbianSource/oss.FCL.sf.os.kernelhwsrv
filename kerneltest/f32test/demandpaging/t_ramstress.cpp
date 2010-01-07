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
// f32test\demandpaging\t_ramstress.cpp
// Following this, it requests that the driver continuously attempt 
// to move pages directly.
// Platforms/Drives/Compatibility:
// Hardware only. No defrag support on emulator. 	
// 
//

//! @SYMTestCaseID				KBASE-t_ramstress-0606
//! @SYMTestType				CT
//! @SYMTestCaseDesc			RAM Defrag background defrag stresser 
//! @SYMPREQ					PREQ308
//! @SYMTestPriority			High
//! @SYMTestActions				t_ramstress loads and opens the logical device driver ("D_RAMSTRESS"). 
//! @SYMTestExpectedResults		Finishes if the system behaves as expected, panics otherwise
//! @SYMTestPriority			High
//! @SYMTestStatus				Implemented
//-------------------------------------------------------------------------------------------------
#include <e32test.h>
#include <u32hal.h>
#include <f32file.h>
#include <e32svr.h>
#include <hal.h>
#include "t_ramstress.h"

RTest test(_L("T_RAMSTRESS"));

RRamStressTestLdd  RamstressLdd;


//
// E32Main
//
// Main entry point.
//

TInt E32Main()
	{
	test.Title();
	test.Start(_L("RAM Defrag background stress testing..."));

	TBuf<256> args;
	User::CommandLine(args);
	TLex	lex(args);
	TPtrC  token=lex.NextToken();
	if (token.Length() != 0)
		{
		// exit immediately if we are run as part of autotest
		test.End();
		return KErrNone;
		}

	TInt r = User::LoadLogicalDevice(KRamStressTestLddName);
	test(r==KErrNone || r==KErrAlreadyExists);
	test(RamstressLdd.Open()==KErrNone);

	TUint zoneCount = 0;
	TInt ret = UserSvr::HalFunction(EHalGroupRam,ERamHalGetZoneCount,&zoneCount,0);
	test(ret == KErrNone);
	test(zoneCount != 0);

	TUint index;
	TInt iters = 0;
	while (1)
		{
		for (index = 0; index < zoneCount; index ++)
			{
			RamstressLdd.DoMovePagesInZone(index);
			User::AfterHighRes(1000);
			}
		//## mDH make it run forever iters ++;
		if (iters > 1000000)
			{
			break;
			}
		}
	RamstressLdd.Close();
	test.Next(_L("Ram Defrag : Done"));
	test.End();
	return KErrNone;
	}
