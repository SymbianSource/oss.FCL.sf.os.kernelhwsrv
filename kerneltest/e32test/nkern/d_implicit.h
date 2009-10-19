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
// e32test\nkern\d_implicit.h
// 
//

#if !defined(__D_IMPLICIT_H__)
#define __D_IMPLICIT_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"ImpSysTest");

class TCapsImpSysTestV01
	{
public:
	TVersion	iVersion;
	};

struct SStats
	{
	TInt iFailCount;
	TInt iCount1;
	TInt iCount2;
	TInt iCount3;
	};

class RImpSysTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlStart,
		EControlStop
		};

	enum TTest
		{
		ETestPriority,
		ETestRoundRobin,
		ETestDummy
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline TInt Start(TInt aTestNum)
		{ return DoControl(EControlStart,(TAny*)aTestNum); }
	inline TInt Stop(SStats& aStats)
		{ return DoControl(EControlStop, &aStats); }
#endif
	};

#endif
