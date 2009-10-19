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
// e32test\bench\d_kernbm.h
// Kernel side header containg internal definitions for d_kernasmbm ldd
// 
//

#ifndef __D_KERNBM_H__
#define __D_KERNBM_H__

#include "d_kernasmbm.h"

/// Base class for kernel benchmarks
class TKernelBenchmark
	{
public:
	const TBmInfo& Info() const;
	virtual TInt Run(const TBmParams& aParams, TInt& aResult);
protected:
	TKernelBenchmark(const TDesC8& aName);
	TKernelBenchmark(const TDesC8& aName, TInt aAlignStep);
private:
	virtual void DoRun(const TBmParams& aParams) = 0;
private:
	TBmInfo iInfo;
	};

#define CALL_10_TIMES(x) x; x; x; x; x; x; x; x; x; x

/// Macro to define a benchmark
#define DEFINE_BENCHMARK(name, pre, test, post)                  \
_LIT(KName_##name, #name);                                       \
class TGeneralBenchmark_##name : public TKernelBenchmark         \
	{                                                            \
	public:                                                      \
	TGeneralBenchmark_##name():TKernelBenchmark(KName_##name){}  \
	virtual void DoRun(const TBmParams& aParams)                 \
		{                                                        \
		TInt its = aParams.iIts;                                 \
		pre;                                                     \
		for (TInt j = 0 ; j < its ; ++j)                         \
			{                                                    \
			CALL_10_TIMES(test);                                 \
			}                                                    \
		post;                                                    \
		}                                                        \
	} Instance_##name

/// Macro to define a memory benchmark
#define DEFINE_MEMORY_BENCHMARK(name, step, srcBase, destBase, pre, test, post) \
_LIT(KName_##name, #name);                                       \
class TMemoryBenchmark_##name : public TKernelBenchmark          \
	{                                                            \
public:                                                          \
	TMemoryBenchmark_##name():                                   \
		TKernelBenchmark(KName_##name, step){}                   \
	virtual void DoRun(const TBmParams& aParams)                 \
		{                                                        \
		TInt its = aParams.iIts;                                 \
		const TUint8* src = srcBase + aParams.iSourceAlign;      \
		TUint8* dest = destBase + aParams.iDestAlign;            \
		pre;                                                     \
		for (TInt j = 0 ; j < its ; ++j)                         \
			{                                                    \
			CALL_10_TIMES(test);                                 \
			}                                                    \
		post;                                                    \
		}                                                        \
	} Instance_##name

/// Base class for benchmarks using a second thread
class TThreadedBenchmark : public TKernelBenchmark
	{
public:
	virtual TInt Run(const TBmParams& aParams, TInt& aResult);
protected:
	TThreadedBenchmark(const TDesC8& aName, TInt aRelPri);
private:
	static TInt Thread2Func(TAny *aPtr);
	virtual void DoRun2(TInt aIts) = 0;
protected:
	DThread* iThread1;
	DThread* iThread2;
private:
	TInt iRelPri;
	TInt iIts;
	};

/// Macro to define a thread benchmark easily
#define DEFINE_THREADED_BENCHMARK(name, relPri, pre, test1, test2, post) \
_LIT(KName_##name, #name);                                               \
class TKernelBenchmark_##name : public TThreadedBenchmark                \
	{                                                                    \
public:                                                                  \
	TKernelBenchmark_##name():TThreadedBenchmark(KName_##name, relPri){} \
	virtual void DoRun(const TBmParams& aParams)                         \
		{                                                                \
		TInt its = aParams.iIts;                                         \
		pre;                                                             \
		for (TInt j = 0 ; j < its*10 ; ++j)                              \
			{                                                            \
			test1;                                                       \
			}                                                            \
		post;                                                            \
		}                                                                \
	virtual void DoRun2(TInt aIts)                                       \
		{                                                                \
		for (TInt j = 0 ; j < aIts*10 ; ++j)                             \
			{                                                            \
			test2;                                                       \
			}                                                            \
		}                                                                \
	} Instance_##name

/// Initialise data used by benchmarks
TInt InitData();

/// Clean up data used by benchmarks
void CloseData();

/// Pointer to user-side buffer, needed by some tests
extern TUint8* UserPtr;

/// List of defined benchmarks
extern RPointerArray<TKernelBenchmark> KernelBenchmarks;

/// Macro to 32-byte align addresses
#define ALIGN_ADDR(a) ((TAny*)((((TInt)a) & ~0x1f) + 0x20))

#endif
