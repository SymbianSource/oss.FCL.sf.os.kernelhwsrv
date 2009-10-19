// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\system\t_reason.cpp
// Debugging aid which prints the ESHELL startup banner down the serial port, as an
// aid to debugging repeated bleep-bleep-bleep restarting. 
// 
//

#include "e32std.h"
#include "e32std_private.h"
#include "e32svr.h"
#include "e32hal.h"

TUint j;

TInt E32Main()
	{

	FOREVER
		{
		TMachineStartupType reason;
		UserHal::StartupReason(reason);
		switch (reason)
			{
			case EStartupCold:		RDebug::Print(_L("Cold Start    ")); break;
			case EStartupColdReset: 	RDebug::Print(_L("Cold Reset    ")); break;
			case EStartupNewOs: 		RDebug::Print(_L("New OS        ")); break;
			case EStartupPowerFail:		RDebug::Print(_L("Power failed  ")); break;
			case EStartupWarmReset:		RDebug::Print(_L("Warm Reset    ")); break;
			case EStartupKernelFault:	
				TInt faultno;
				UserHal::FaultReason(faultno);
				if (faultno == 0x10000000)
					RDebug::Print(_L("Kernel Exception  "));
				else
				if (faultno >= 0x10000)
					RDebug::Print(_L("Kernel PANIC: %d  "), faultno-0x10000);
				else
					RDebug::Print(_L("Kernel FAULT: %d  "), faultno);
				break;
			case EStartupSafeReset:		RDebug::Print(_L("Safe Reset    ")); break;
			default:
				RDebug::Print(_L("<?reason=%d>  "), reason);
				break;
			}

		if (reason==EStartupWarmReset || reason==EStartupPowerFail || reason==EStartupKernelFault)
			{
			TInt exceptno;
			TExcInfo exceptInfo;
			UserHal::ExceptionId(exceptno);
			UserHal::ExceptionInfo(exceptInfo);
			RDebug::Print(_L("(last exception %d: code %08x data %08x) "), exceptno, exceptInfo.iCodeAddress,exceptInfo.iDataAddress);
			}

		RDebug::Print(_L("\r\n\nCopyright (C) 1997-1999 Symbian Ltd\r\n\n"));

		for (TInt i=0; i<1000000; i++)
			j=i%17;	// waste some time
		}
	}
