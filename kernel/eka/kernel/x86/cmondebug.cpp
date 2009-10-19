// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\x86\cmondebug.cpp
// 
//

#include <kernel/monitor.h>

//
// Startup
//
void CrashDebugger::QuadrupleBeepAndPowerDown()
	{
//	Kern::Beep(262);
//	P::Wait(127);
//	Kern::Beep(294);
//	P::Wait(127);
//	Kern::Beep(311);
//	P::Wait(127);
//	Kern::Beep(349);
//	P::Wait(127);
//	Kern::Beep(0);
	CheckPower();
	}

