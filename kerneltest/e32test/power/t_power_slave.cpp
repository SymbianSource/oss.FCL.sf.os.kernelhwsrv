// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\power\t_power_slave.cpp
// 
//

#include <e32power.h>
#include <e32test.h>

GLDEF_C TInt E32Main()
	{
	// Get arguments from the command line
	TInt len = User::CommandLineLength();
	TInt size = len * sizeof(TUint16);
	__ASSERT_ALWAYS(size == sizeof(TInt),User::Invariant());
	TInt arg;
	TPtr cmd((TUint16*) &arg, len);
	User::CommandLine(cmd);

	TBool jit = User::JustInTime();
	User::SetJustInTime(EFalse);

	TRequestStatus status;
	switch(arg)
		{
	case 0: 
		Power::PowerDown();
		break;
	case 1:
		Power::EnableWakeupEvents(EPwStandby);
		break;
	case 2:
		Power::DisableWakeupEvents();	
		break;
	case 3:
		Power::RequestWakeupEventNotification(status);
		break;
	case 4:
		Power::CancelWakeupEventNotification();
		break;
	default:
		break;
		}

	User::SetJustInTime(jit);
	return KErrNone;
	}
