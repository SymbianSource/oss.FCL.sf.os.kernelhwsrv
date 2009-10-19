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
// e32test\bench\t_userbm.h
// 
//

#ifndef __T_USERBM_H__
#define __T_USERBM_H__

#include "t_asmbm.h"
#include <e32std.h>

/// Base class for user benchmarks
class TUserBenchmark : public TBmInfo
	{
public:
	virtual void Run(TInt aIts) = 0;
protected:
	TUserBenchmark(const TDesC8& aName, TUint aCategory);
	};

class TUserBenchmarkList : public MBenchmarkList
	{
	virtual TInt Count();
	virtual TInt Info(TInt aIndex, TBmInfo& aInfoOut);
	virtual TInt Run(TInt aIndex, const TBmParams& aParams, TInt& aDeltaOut);
	};

/// List containing all defined benchmarks
extern RPointerArray<TUserBenchmark> UserBenchmarks;

#define CALL_10_TIMES(x) x; x; x; x; x; x; x; x; x; x

/// Macro to define a benchmark easily
#define DEFINE_USER_BENCHMARK(name, pre, test)         \
_LIT8(KName_##name, #name);                            \
class TBenchmark_##name : public TUserBenchmark        \
	{                                                  \
public:                                                \
	TBenchmark_##name() :                              \
         TUserBenchmark(KName_##name, KCategoryGeneral)\
		 {}                                            \
	virtual void Run(TInt aIts)                        \
		{                                              \
		pre;                                           \
		for (TInt j = 0 ; j < aIts ; ++j)              \
			{                                          \
			CALL_10_TIMES(test);                       \
			}                                          \
		}                                              \
	} Instance_##name

/// Macro to define a benchmark in category 'extra'
#define DEFINE_EXTRA_BENCHMARK(name, pre, test)        \
_LIT8(KName_##name, #name);                            \
class TBenchmark_##name : public TUserBenchmark        \
	{                                                  \
public:                                                \
	TBenchmark_##name() :                              \
         TUserBenchmark(KName_##name, KCategoryExtra)  \
		 {}                                            \
	virtual void Run(TInt aIts)                        \
		{                                              \
		pre;                                           \
		for (TInt j = 0 ; j < aIts ; ++j)              \
			{                                          \
			CALL_10_TIMES(test);                       \
			}                                          \
		}                                              \
	} Instance_##name

#endif
