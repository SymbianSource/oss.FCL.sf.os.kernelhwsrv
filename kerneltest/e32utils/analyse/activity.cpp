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

#include "analyse.h"
#include "activity.h"
#include "output.h"

#ifdef __MSVCDOTNET__
#include <ostream>
#include <iomanip>
#else //!__MSVCDOTNET__
#include <ostream.h>
#include <iomanip.h>
#endif //__MSVCDOTNET__

#include <algorithm>


// class Activity::Bucket

inline Activity::Bucket::Bucket(unsigned aPeriod,int aThread)
	:iPeriod(aPeriod),iThread(aThread)
	{}

bool Activity::Bucket::operator<(const Bucket& aRhs) const
	{
	if (iPeriod != aRhs.iPeriod)
		return iPeriod < aRhs.iPeriod;
	return iThread < aRhs.iThread;
	}

// class Activity::ThreadData

inline Activity::ThreadData::ThreadData(const Thread& aThread)
	:iThread(&aThread), iTotal(0)
	{}

inline Activity::ThreadData::ThreadData(unsigned aCutoff)
	:iTotal(aCutoff)
	{}

inline bool Activity::ThreadData::operator<(const ThreadData& aRhs) const
	{
	return iTotal < aRhs.iTotal;
	}


// class Activity


Activity::Activity(int aBucketSize, unsigned aBeginSample, double aCutOff)
	:iBucketSize(aBucketSize), iBeginSample(aBeginSample), iCutOff(aCutOff)
	{
	cout << "Execution activity\n\n";
	}

void Activity::Sample(unsigned aNumber, const Thread& aThread, PC)
	{
	if (aThread.iIndex == iThreads.size())
		iThreads.push_back(ThreadData(aThread));

	++iThreads[aThread.iIndex].iTotal;
	++iData[Bucket((aNumber - iBeginSample) / iBucketSize, aThread.iIndex)];
	} 

void Activity::Complete(unsigned aTotal, unsigned aActive)
	{
	cout.setf(ios::fixed, ios::floatfield);
	cout << setfill(' ');
	cout.precision(2);
	const char* emptySeparator;
	const char* separator;
//
	const unsigned ixCount = iThreads.size();
	int* remap = new int[ixCount];
	std::fill(remap, remap + ixCount, -1);

	std::sort(iThreads.begin(), iThreads.end());
	Threads::iterator cutoff = std::lower_bound(iThreads.begin(), iThreads.end(), ThreadData(iCutOff * aTotal * 0.01));
	iThreads.erase(iThreads.begin(), cutoff);

	const unsigned disCount = iThreads.size();
	for (int ix = 0; ix < disCount; ++ix)
		remap[iThreads[ix].iThread->iIndex] = ix;


	if (Analyse::Format() != Analyse::EExcel)
		{
		cout << "ID  Thread name\n";
		int ix;
		for (ix = 0; ix < disCount; ++ix)
			cout << char('A' + ix) << "  " << *iThreads[ix].iThread << '\n';

		cout << "\nsample  ";
		for (ix = 0; ix < disCount; ++ix)
			cout << setw(4) << char('A' + ix) << "    ";
		cout << "\n\n";
		separator = " ";
		emptySeparator = "        ";
		}
	else
		{
		cout << "sample";
		for (int ix = 0; ix < disCount; ++ix)
			cout << '\t' << *iThreads[ix].iThread;
		cout << '\n';
		separator = emptySeparator = "\t";
		}

	unsigned* totals = new unsigned[disCount];
	std::fill(totals, totals + disCount, 0);
	unsigned period = 0;
	bool values = false;
	for (Data::iterator p = iData.begin(), e = iData.end(); p != e; ++p)
		{
		while (period != p->first.iPeriod)
			{
			cout << setw(7) << (period * iBucketSize) + iBeginSample;
			if (values)
				{
				for (int ix = 0; ix < disCount; ++ix)
					cout <<	separator << Result(totals[ix], iBucketSize);
				std::fill(totals, totals + disCount, 0);
				values = false;
				}
			cout << '\n';
			++period;
			}
		int ix = remap[p->first.iThread];
		if (ix >= 0)
			{
			totals[ix] = p->second;
			values = true;
			}
		}
	if (values)
		{
		cout << setw(7) << (period * iBucketSize) + iBeginSample;
		for (int ix = 0; ix < disCount; ++ix)
			cout <<	separator << Result(totals[ix], iBucketSize);
		cout << '\n';
		}

	delete [] remap;
	delete [] totals;
	}
