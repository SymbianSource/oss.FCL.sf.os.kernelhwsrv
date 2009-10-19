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
// e32test\misc\d_rndtim.h
// 
//

#if !defined(__D_RNDTIM_H__)
#define __D_RNDTIM_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KRndTimLddName,"RndTim");

class TCapsRndTimV01
	{
public:
	TVersion	iVersion;
	};

class RRndTim : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlWait,
		EControlSetPriority,
		EControlStartTimer,
		EControlStopTimer,
		EControlCalibrate,
		};
public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KRndTimLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }
	inline void Wait()
		{ DoControl(EControlWait); }
	inline TInt SetPriority(RThread aThread, TInt aPriority)
		{ return DoControl(EControlSetPriority, (TAny*)aThread.Handle(), (TAny*)aPriority); }
	inline void StartTimer()
		{ DoControl(EControlStartTimer); }
	inline void StopTimer()
		{ DoControl(EControlStopTimer); }
	inline TInt Calibrate(TInt aMilliseconds)
		{ return DoControl(EControlCalibrate, (TAny*)aMilliseconds); }
#endif
	};

#endif
