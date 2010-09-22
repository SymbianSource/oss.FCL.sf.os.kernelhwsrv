// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_reason2.cpp
// Overview:
// Tests system startup reason HAL functions and exception info functions:
// API Information:
// UserHal::StartupReason() (deprecated), HAL:.Get(ESystemStartupReason,..),
// UserHal::FaultReason, UserHal::ExceptionId and UserHal::ExceptionInfo
// Details:
// - Asks system startup reason from user hal
// - Asks system startup reason with replacing hal::get method
// - Asks Exception info
// Platforms/Drives/Compatibility:
// All.
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//
#define __E32TEST_EXTENSION__

#include <e32hal.h>
#include <e32test.h>
#include <hal.h>
#include <u32hal.h>
#include <e32svr.h>

LOCAL_D RTest test(_L("T_REASON2"));

TInt gSysReason=KErrNotSupported;

void GetSysStartupReason()
	{
	test.Next(_L("Get startup reason using (deprecated) UserHal::StartupReason API"));

	TMachineStartupType reason;

	test_KErrNone(UserHal::StartupReason(reason));
	switch (reason)
		{
		case EStartupCold:			RDebug::Print(_L("Cold Start    ")); break;
		case EStartupColdReset: 	RDebug::Print(_L("Cold Reset    ")); break;
		case EStartupNewOs: 		RDebug::Print(_L("New OS        ")); break;
		case EStartupPowerFail:		RDebug::Print(_L("Power failed  ")); break;
		case EStartupWarmReset:		RDebug::Print(_L("Warm Reset    ")); break;
		case EStartupKernelFault:	RDebug::Print(_L("Kernel fault  ")); break;
		case EStartupSafeReset:		RDebug::Print(_L("Safe Reset    ")); break;
		default:
			RDebug::Print(_L("<?reason=%d>  "), reason);
			test(EFalse); // fail, unknown reason returned
			break;
		}

	// test the replacing API
	TInt r=KErrNone;
	test.Next(_L("Get system startup reason using HAL::Get()"));
	r=HAL::Get(HAL::ESystemStartupReason,gSysReason);
#if defined(__WINS__)
	test_Equal(KErrNotSupported,r);
#else
	test_KErrNone(r);
	switch (gSysReason)
		{
		case HAL::ESystemStartupReason_Cold:	RDebug::Print(_L("reason:Cold ")); break;
		case HAL::ESystemStartupReason_Warm:	RDebug::Print(_L("reason:Warm ")); break;
		case HAL::ESystemStartupReason_Fault: 	RDebug::Print(_L("reason:Fault")); break;
		default:
			RDebug::Print(_L("<?reason=%d>  "), gSysReason);
			test(EFalse); // fail, unknown reason returned
			break;
		}
#endif
	}

void GetExceptionInfo()
	{
	test.Next(_L("Get exception ID"));
	TInt exceptno;
	TInt faultno;

	TExcInfo exceptInfo;
	test_KErrNone(UserHal::ExceptionId(exceptno));
				
	test.Next(_L("Get exception info"));
	test_KErrNone(UserHal::ExceptionInfo(exceptInfo));

	test.Next(_L("Get fault reason"));
	test_KErrNone(UserHal::FaultReason(faultno));

	if (gSysReason==HAL::ESystemStartupReason_Warm || gSysReason==HAL::ESystemStartupReason_Fault)
		{
		RDebug::Print(_L("(last exception %d: code %08x data %08x) "), exceptno, exceptInfo.iCodeAddress,exceptInfo.iDataAddress);
		}
				
	if (gSysReason==HAL::ESystemStartupReason_Fault)
		{
		if (faultno == 0x10000000)
			{
			RDebug::Print(_L("Kernel Exception  "));
			}
		else
			{
			if (faultno >= 0x10000)
				{
				RDebug::Print(_L("Kernel PANIC: %d  "), faultno-0x10000);
				}
			else
				{
				RDebug::Print(_L("Kernel FAULT: %d  "), faultno);
				}
			}
		}
	}

TInt E32Main()
	{
	test.Title();

	test.Start(_L("Test startup reasons from Hal"));

	// test startup reason
	GetSysStartupReason();

	// test exception and fault info UserHal functions
	GetExceptionInfo();

	test.End();

	return KErrNone;
	}
