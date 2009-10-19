// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/debug/rventtracker.h
// 
//

#ifndef __REVENTTRACKER_H__
#define __REVENTTRACKER_H__

#include <e32cmn.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif // __KERNEL_MODE__

_LIT(KTestLddName, "TestEventTracker");
_LIT8(KUseStopModeHook, "UseStopModeHook");

/** Session with event tracking debug agent */

class REventTracker: public RBusLogicalChannel
	{
public:
	enum 
		{
		EStart,
		EStop,
		};
public:
	static inline TVersion Version() { return TVersion(1, 0, 1); }
#ifndef __KERNEL_MODE__
public:
	inline TInt Open(TBool aUseHook);
	inline TInt Start();
	inline TInt Stop();
#endif
	};


#ifndef __KERNEL_MODE__

inline TInt REventTracker::Open(TBool aUseHook)
	{
	return DoCreate(KTestLddName, Version(), aUseHook, NULL, NULL, EOwnerThread);
	}

inline TInt REventTracker::Start()
	{
	return DoControl(EStart);
	}

inline TInt REventTracker::Stop()
	{
	return DoControl(EStop);
	}

#endif // __KERNEL_MODE__

#endif // __REVENTTRACKER_H__
