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
// e32test\misc\t_destruct.h
// 
//

#ifndef __T_DESTRUCT_H__

// Passed to slave process to indicate which test to run, also used as the exit reason of slave
// process to check that the correct code path executed.
enum TTestType
	{
	ETestMainThreadReturn=1,
	ETestMainThreadExit,
	ETestChildThreadReturn,
	ETestOtherThreadExit,
	ETestOtherThreadPanic,
	ETestOtherThreadRunning,
	ETestPermanentThreadExit,
	ETestRecursive,
	ETestDestructorExits,
	ETestLastThreadPanic,
	
	ENumTestTypes
	};

// Messages passed from slave process to main test process to signal when destruction has happened.
enum TMessage
	{
	EMessageConstruct,		  // construction of exe global data
	EMessageConstructStatic,  // construction of statically-linked dll data
	EMessageConstructDynamic, // construction of dynamically-loaded dll data
	EMessageConstructStatic3, // construction of statically-linked dll data in dll 3
	EMessagePreDestruct,	  // destruct shouldn't happen before here
	EMessageDestruct,		  // destruction of exe global data
	EMessageDestructStatic,   // destruction of statically-linked dll data
	EMessageDestructDynamic,  // destruction of dynamically-loaded dll data
	EMessageDestructStatic3,  // destruction of statically-linked dll data in dll 3

	ENumMessges
	};

_LIT(KMessageQueueName, "t_destruct queue");

IMPORT_C void StaticMain();
IMPORT_C void StaticMain3();

#endif
