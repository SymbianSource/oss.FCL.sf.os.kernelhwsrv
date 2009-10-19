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
// e32test\bench\t_asmbm.cpp
// Common header for assmbler benchmark tests
// 
//

#ifndef __T_ASMBM_H__
#define __T_ASMBM_H__

#include <e32cmn.h>

const TUint KCategoryGeneral = 1<<0;	///< Benchmarks that get run by default
const TUint KCategoryMemory = 1<<1;		///< Memory alignement benchmarks run with -m option
const TUint KCategoryExtra = 1<<2;		///< Extra benchmarks run with -x option

struct TBmInfo
	{
	TBuf8<64> iName;	///< Name of this benchmark
	TUint iCategories;	///< Set of applicable categories
	TUint iAlignStep;	///< For memory benchmarks only, the alignment step 
	};

struct TBmParams
	{
	TInt iIts;			///< NUmber of iterations to run benchmark for
	TInt iSourceAlign;	///< For memory benchmarks only, the source alignment
	TInt iDestAlign;	///< For memory benchmarks only, the destination alignment
	};

struct MBenchmarkList
	{
	virtual TInt Count() = 0;
	virtual TInt Info(TInt aIndex, TBmInfo& aInfoOut) = 0;
	virtual TInt Run(TInt aIndex, const TBmParams& aParams, TInt& aDeltaOut) = 0;
	};

/// Implementation of main program
void RunBenchmarkTestsL(MBenchmarkList& aBenchmarks);

/// Caller-supplied function to initialise any data used by benchmarks
extern void InitDataL();

#endif
