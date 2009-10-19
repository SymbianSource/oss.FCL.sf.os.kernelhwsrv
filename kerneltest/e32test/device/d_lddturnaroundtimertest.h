// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\d_lddturnaroundtimetest.h
// 
//

#if !defined(__DLDDTURNAROUNDTIMETEST_H__)
#define __DLDDTURNAROUNDTIMETEST_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"TurnarountTimeTest");

class RLddTest1 : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EGET_TIMERTICKCOUNT = 0,
		EGET_TIMERTICKS = 1
		};

public:
	inline TInt Open();
	inline TInt Test_getTimerTicks(TUint &time);
	inline TInt Test_getTimerCount(TUint &time);
	static inline TInt Unload();
	};

#include "d_lddturnaroundtimertest.inl"

#endif   //__DLDDTURNAROUNDTIMETEST_H__
