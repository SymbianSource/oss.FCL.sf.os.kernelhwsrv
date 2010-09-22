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
// e32test\window\d_keyrepeat.h
// 
//

#if !defined(__D_KEYREPEAT_H__)
#define __D_KEYREPEAT_H__

#include <e32cmn.h>
#include <e32keys.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"D_KEYREPEAT.LDD");


// class RKeyEvent
class RKeyEvent
	{
public:
	RKeyEvent();
	RKeyEvent(TStdScanCode aKey, TInt aRepeatCount);
public:
	TStdScanCode iKey;
	TInt iRepeatCount;
	};

// class RTestKeyRepeatLdd
class RTestKeyRepeatLdd : public RBusLogicalChannel
	{
public:

	enum TControl
		{
		ESetRepeat=1,
		ERepeats
		};

public:
	inline TInt Open();
	inline TInt SetRepeat(RKeyEvent &aEvent);
	inline TInt Repeats();
	};

// inlines
#ifndef __KERNEL_MODE__
inline TInt RTestKeyRepeatLdd::Open()
	{
	return DoCreate(KLddName,TVersion(0,1,0),KNullUnit,NULL,NULL);
	}

inline TInt RTestKeyRepeatLdd::SetRepeat(RKeyEvent &aEvent)
	{
    return DoControl(ESetRepeat,&aEvent);
	}

inline TInt RTestKeyRepeatLdd::Repeats()
	{
    return DoControl(ERepeats);
	}

#endif //__KERNEL_MODE__

#endif   //__D_KEYREPEAT_H__
