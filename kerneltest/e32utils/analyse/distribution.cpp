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
#include "distribution.h"
#include "output.h"

#ifdef __MSVCDOTNET__
#include <ostream>
#include <iomanip>
#else //!__MSVCDOTNET__
#include <ostream.h>
#include <iomanip.h>
#endif //__MSVCDOTNET__

#include <algorithm>

// class Distribution

namespace {

int* BucketBox(int aSize)
	{
	int* buckets = new int[aSize];
	memset(buckets, 0, sizeof(int) * aSize);
	return buckets;
	}

class BucketTotals
	{
private:
	class Generator
		{
	public:
		inline Generator()
			:iValue(0)
			{}
		inline int operator()()
			{return iValue++;}
	private:
		int iValue;
		};
	class Sorter
		{
	public:
		inline Sorter(const int* aTotals)
			:iTotals(aTotals)
			{}
		inline bool operator()(int aLhs, int aRhs) const
			{return (iTotals[aLhs] > iTotals[aRhs]);}
	private:
		const int* iTotals;
		};
public:
	BucketTotals(Distribution::Data& aData, int aSize);
	~BucketTotals();
	void Sort();
//
	inline int Total() const;
	inline int Map(int aBucket) const;
	inline int operator[](int aBucket) const;
	inline Result Sample(int aSample) const;
private:
	void Total(Distribution::Data& aData);
private:
	int iSize;
	int* iTotals;
	int* iOrdering;
	int iTotal;
	};

BucketTotals::BucketTotals(Distribution::Data& aData, int aSize)
	:iSize(aSize), iTotals(BucketBox(aSize)), iOrdering(BucketBox(aSize))
	{
	Total(aData);
	std::generate_n(iOrdering, iSize, Generator());
	}

BucketTotals::~BucketTotals()
	{
	delete [] iTotals;
	delete [] iOrdering;
	}

void BucketTotals::Total(Distribution::Data& aData)
	{
	int total = 0;
	for (Distribution::Data::iterator p = aData.begin(), e = aData.end(); p != e; ++p)
		{
		const int* box = p->iBuckets;
		int threadTotal = 0;
		for (int j = p->iBucketsLength; --j >=0;)
			{
			threadTotal += box[j];
			iTotals[j] += box[j];
			}
		p->iTotal = threadTotal;
		total += threadTotal;
		}
	iTotal = total;
	}

void BucketTotals::Sort()
	{
	std::sort(iOrdering, iOrdering + iSize, Sorter(iTotals));
	}

inline int BucketTotals::Total() const
	{return iTotal;}

inline int BucketTotals::Map(int aBucket) const
	{return iOrdering[aBucket];}

inline int BucketTotals::operator[](int aBucket) const
	{return iTotals[iOrdering[aBucket]];}

inline Result BucketTotals::Sample(int aSample) const
	{return Result(aSample, iTotal);}

};

inline Distribution::ThreadData::ThreadData(const Thread& aThread, int aSize)
	:iThread(&aThread), iBuckets(BucketBox(aSize)), iTotal(0), iBucketsLength(aSize)
	{}

inline Distribution::ThreadData::ThreadData(int aCutoff)
	:iTotal(aCutoff)
	{}

inline bool Distribution::ThreadData::operator<(const Distribution::ThreadData& aRhs) const
	{return (iTotal > aRhs.iTotal);}

Distribution::Distribution(CodeSpace& aCodeSpace, double aCutOff)
	:iCodeSpace(aCodeSpace), iCutOff(aCutOff)
	{
	cout << "Profile distribution\n\n";
	}

void Distribution::Sample(unsigned , const Thread& aThread, PC aPc)
//
// Collect the sample in the bucket as allocated by the code space
//
	{
	if (aThread.iIndex == iData.size())
		iData.push_back(ThreadData(aThread,iCodeSpace.Size()));

	///
	int bucket = iCodeSpace.Bucket(aPc);
	if (bucket >= iData[aThread.iIndex].iBucketsLength)
		{
		int* new_buckets = new int[bucket+1];
		memset(new_buckets, 0, sizeof(int) * (bucket+1));
		memcpy(new_buckets, iData[aThread.iIndex].iBuckets, sizeof(int) * iData[aThread.iIndex].iBucketsLength);
		delete [] iData[aThread.iIndex].iBuckets;
		iData[aThread.iIndex].iBuckets = new_buckets;
		iData[aThread.iIndex].iBucketsLength = bucket+1;
		}

	++iData[aThread.iIndex].iBuckets[bucket];
	}

void Distribution::Complete(unsigned aTotal, unsigned aActive)
	{
	// accumulate thread and bucket totals
	const int nbuckets = iCodeSpace.Size();
	BucketTotals totals(iData, nbuckets);
	if (iCodeSpace.Ordering() == CodeSpace::ERandom)
		totals.Sort();

	int cutoff = int(iCutOff * totals.Total() * 0.01);
	std::sort(iData.begin(), iData.end());
	iData.erase(std::upper_bound(iData.begin(), iData.end(), ThreadData(cutoff)), iData.end());
//
	cout.setf(ios::fixed, ios::floatfield);
	cout.precision(2);
//
	cout << "Samples: " << aTotal << '\n';
	cout << " Active: " << aActive << "  (" << double(aActive*100)/aTotal << "%)\n";
	cout << "Counted: " << totals.Total() << "  (" << double(totals.Total()*100)/aTotal << "%)\n\n";
	cout << setfill(' ');
//
	Data::iterator p, e;
	if (!Analyse::Option(Analyse::ETotalOnly))
		{
		if (Analyse::Format() != Analyse::EExcel)
			{
			cout << "ID  Thread name\n";
			char id = 'A';
			for (p = iData.begin(), e = iData.end(); p != e; ++p)
				cout << id++ << "   " << *p->iThread << '\n';
			cout << '\n';

			id = 'A';
			for (p = iData.begin(), e = iData.end(); p != e; ++p)
				cout << setw(4) << id++ << "    ";
			cout << "  total\n\n";

			for (p = iData.begin(), e = iData.end(); p != e; ++p)
				cout << totals.Sample(p->iTotal) << " ";
			cout << "  " << totals.Sample(totals.Total()) << "  total\n\n";
			}
		else
			{
			cout.precision(5);
			for (p = iData.begin(), e = iData.end(); p != e; ++p)
				cout << '\t' << *p->iThread;
			cout << "\ttotal\n";
			}
		}

	if (cutoff == 0 && iCodeSpace.Ordering() != CodeSpace::ELinear)
		cutoff = 1;

	for (int ix = 0; ix< nbuckets; ++ix)
		{
		int total = totals[ix];
		if (total >= cutoff)
			{
			int jx = totals.Map(ix);
			if (jx == CodeSpace::KOtherBucket && Analyse::Option(Analyse::ENoOther))
				continue;
			if (Analyse::Format() == Analyse::EExcel)
				{
				cout << iCodeSpace.Name(jx);
				if (!Analyse::Option(Analyse::ETotalOnly))
					{
					for (p = iData.begin(), e = iData.end(); p != e; ++p)
						cout << '\t' << totals.Sample(jx>=p->iBucketsLength ? 0 : p->iBuckets[jx]);
					}
				cout << '\t' << totals.Sample(total) << '\n';
				}
			else
				{
				if (!Analyse::Option(Analyse::ETotalOnly))
					{
					for (p = iData.begin(), e = iData.end(); p != e; ++p)
						cout << totals.Sample(jx>=p->iBucketsLength ? 0 : p->iBuckets[jx]) << " ";
					cout << "  ";
					}
				cout << totals.Sample(total) << "  " << iCodeSpace.Name(jx) << '\n';
				}
			}
		}

	if (!Analyse::Option(Analyse::ETotalOnly))
		{
		if (Analyse::Format() == Analyse::EExcel)
			{
			cout << "total";
			for (p = iData.begin(), e = iData.end(); p != e; ++p)
				cout << '\t' << totals.Sample(p->iTotal);
			cout << '\t' << totals.Sample(totals.Total()) << '\n';
			}
		}
	}
