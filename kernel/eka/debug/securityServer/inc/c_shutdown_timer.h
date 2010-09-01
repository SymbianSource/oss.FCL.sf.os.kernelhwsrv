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
// Definitions for the security server's shutdown timer.
// 
//

#ifndef C_SHUTDOWN_TIMER_H
#define C_SHUTDOWN_TIMER_H

/**
@file
@internalTechnology
@released
*/

#include <e32base.h>

const TInt KShutdownDelay = 5000000; // approx 5 seconds
const TInt KActivePriorityShutdown = -1; // priority for shutdown AO

/**
Timer class used to manage shutdown of the DSS
*/
class CShutdownTimer : public CTimer
	{
public:
	CShutdownTimer();
	void ConstructL();
	void Start();
private:
	void RunL();
	};

#endif // C_SHUTDOWN_TIMER_H

