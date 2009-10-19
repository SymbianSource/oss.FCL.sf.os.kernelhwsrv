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

#ifndef __ACTIVITY__
#define __ACTIVITY__

#include "trace.h"
#include <map>
#include <vector>

class Activity : public Sampler
	{
	struct Bucket
		{
		inline Bucket(unsigned aPeriod,int aThread);
		bool operator<(const Bucket& aRhs) const;
		//
		unsigned iPeriod;
		int iThread;
		};
	struct ThreadData
		{
		inline ThreadData(const Thread& aThread);
		inline ThreadData(unsigned aCutoff);
		inline bool operator<(const ThreadData& aRhs) const;
		//
		const Thread* iThread;
		unsigned iTotal;
		};
	typedef std::map<Bucket,unsigned> Data;
	typedef std::vector<ThreadData> Threads;
public:
	Activity(int aBucketSize, unsigned aBeginSample, double aCutOff);
private:
	void Sample(unsigned aNumber, const Thread& aThread, PC aPc);
	void Complete(unsigned aTotal, unsigned aActive);
private:
	int iBucketSize;
	unsigned iBeginSample;
	double iCutOff;
	Threads iThreads;
	Data iData;
	};

#endif // __ACTIVITY__