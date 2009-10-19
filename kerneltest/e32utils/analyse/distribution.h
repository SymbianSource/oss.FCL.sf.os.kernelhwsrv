// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#ifndef __DISTRIBUTION__
#define __DISTRIBUTION__

#include "trace.h"
#include "codespace.h"
#include <vector>

class Distribution : public Sampler
	{
public:
	struct ThreadData
		{
		ThreadData(const Thread& aThread, int aSize);
		ThreadData(int aCutoff);
		bool operator<(const ThreadData& aRhs) const;
		//
		const Thread* iThread;
		int* iBuckets;
		int iBucketsLength;
		int iTotal;
		};
	typedef std::vector<ThreadData> Data;
public:
	Distribution(CodeSpace& aCodeSpace, double aCutOff);
private:
	void Sample(unsigned aNumber, const Thread& aThread, PC aPc);
	void Complete(unsigned aTotal, unsigned aActive);
private:
	CodeSpace& iCodeSpace;
	double iCutOff;
	Data iData;
	};

#endif // __DISTRIBUTION__