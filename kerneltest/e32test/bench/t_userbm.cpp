// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_userbm.cpp
// 
//

#include "t_userbm.h"
#include <hal.h>

RPointerArray<TUserBenchmark> UserBenchmarks;

TInt TUserBenchmarkList::Count()
	{
	return UserBenchmarks.Count();
	}

TInt TUserBenchmarkList::Info(TInt aIndex, TBmInfo& aInfoOut)
	{
	aInfoOut = *(TBmInfo*)UserBenchmarks[aIndex];
	return KErrNone;
	}

TInt TUserBenchmarkList::Run(TInt aIndex, const TBmParams& aParams, TInt& aDeltaOut)
	{
	RThread thread;
	thread.SetPriority(EPriorityAbsoluteHigh);

	TUserBenchmark& bm = *UserBenchmarks[aIndex];
	TUint init, final;
	init = User::FastCounter();
	bm.Run(aParams.iIts);
	final = User::FastCounter();
	aDeltaOut = final - init;

	thread.SetPriority(EPriorityNormal);
	return KErrNone;
	}

TUserBenchmark::TUserBenchmark(const TDesC8& aName, TUint aCategory)
	{
	iName = aName;
	iCategories = aCategory;
	iAlignStep = 0;
	__ASSERT_ALWAYS(UserBenchmarks.Append(this) == KErrNone, User::Invariant());
	}
