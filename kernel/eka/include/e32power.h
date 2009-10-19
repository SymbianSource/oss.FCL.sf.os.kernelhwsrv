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
// e32\include\e32power.h
// 
//

#ifndef __E32POWER_H__
#define __E32POWER_H__

#include <e32cmn.h>




/**
@publishedPartner
@released

System-wide power states
*/
enum TPowerState
	{
	/** The system is fully operational; could be busy or idle */ 
	EPwActive,
	/** The system sleeps keeping the full memory state in RAM */ 
	EPwStandby,
	/** System is off; can go to active by rebooting only */
	EPwOff,
	/** The system restarts - i.e. switches off and then back on */
	EPwRestart,
	/** An integer that strictly greater of any legal power state value */
	EPwLimit,
	};




/** 
@publishedPartner
@released

User-level domain manager's interface to Kernel-level power management.
*/
class Power

	{
public:
	IMPORT_C static TInt EnableWakeupEvents(TPowerState);
	IMPORT_C static void DisableWakeupEvents();
	IMPORT_C static void RequestWakeupEventNotification(TRequestStatus&);
	IMPORT_C static void CancelWakeupEventNotification();
	IMPORT_C static TInt PowerDown();
	};

#endif

