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
// e32test\system\d_nanowait.h
// 
//

#if !defined(__D_NANOWAIT_H__)
#define __D_NANOWAIT_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KNanoWaitLddName,"NanoWait");

class TCapsNanoWaitV01
	{
public:
	TVersion	iVersion;
	};


class RNanoWait : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlStartNanoWait,
		};

public:
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{ return DoCreate(KNanoWaitLddName(),TVersion(0,1,1),KNullUnit,NULL,NULL); }

	inline TInt StartNanoWait(TInt aLoopCount, TInt aInterval)
		{ return DoControl(EControlStartNanoWait, (TAny*)aLoopCount, (TAny*)aInterval); }
#endif
	};

#endif
