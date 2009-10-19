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
// e32test\power\d_lddpowerseqtest.h
// 
//

#if !defined(__DLDDPOWERSEQTEST_H__)
#define __DLDDPOWERSEQTEST_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"D_LDDPOWERSEQTEST.LDD");

class RLddTest1 : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		ESET_SLEEPTIME = 0
		};
	enum TRequest
		{
		EPOWERDOWN_POWER1 = 1,
		EPOWERDOWN_POWER2 = 2,
		EPOWERUP_POWER1 = 3,
		EPOWERUP_POWER2 = 4
		};

public:
	inline TInt Open();
	inline void Test_power1down(TRequestStatus &aStatus, TUint &time);
	inline void Test_power2down(TRequestStatus &aStatus, TUint &time);
	inline void Test_power1up(TRequestStatus &aStatus, TUint &time);
	inline void Test_power2up(TRequestStatus &aStatus, TUint &time);
	inline TInt Test_setSleepTime(TUint sleepTime);
	static inline TInt Unload();
	};

#include "d_lddpowerseqtest.inl"

#endif   //__DLDDPOWERSEQTEST_H__
