// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32test.h>

#include "bm_suite.h"

class Overhead : public BMProgram
	{
	typedef void (*MeasurementFunc)(TBMResult*, TBMUInt64 aIter);
	struct Measurement 
		{
		MeasurementFunc iFunc;
		TPtrC			iName;

		Measurement(MeasurementFunc aFunc, const TDesC&	aName) : iFunc(aFunc), iName(aName) {}
		};
public :
	Overhead() : BMProgram(_L("Overhead"))
		{}
	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount);
private:
	static TBMResult iResults[];
	static Measurement iMeasurements[];

	static void TimerStampOverhead(TBMResult*, TBMUInt64 aIter);
	};

Overhead::Measurement Overhead::iMeasurements[] =
	{
	Measurement(&Overhead::TimerStampOverhead, _L("Getting Timer Stamp Overhead"))
	};
TBMResult Overhead::iResults[sizeof(Overhead::iMeasurements)/sizeof(Overhead::iMeasurements[0])];

static Overhead overhead;

void Overhead::TimerStampOverhead(TBMResult* aResult, TBMUInt64 aIter)
	{
	for (TBMUInt64 i = 0; i < aIter; ++i)
		{		
		TBMTicks t1, t2;
		::bmTimer.Stamp(&t1);
		::bmTimer.Stamp(&t2);
		aResult->Cumulate(TBMTicksDelta(t1, t2));
		//
		// Enable other threads to run
		//
		TInt prio = BMProgram::SetAbsPriority(RThread(), overhead.iOrigAbsPriority);
		BMProgram::SetAbsPriority(RThread(), prio);	
		}
	}
						
TBMResult* Overhead::Run(TBMUInt64 aIter, TInt* aCount)
	{
	TInt count = sizeof(iResults)/sizeof(iResults[0]);

	for (TInt i = 0; i < count; ++i)
		{
		iResults[i].Reset(iMeasurements[i].iName);
		iMeasurements[i].iFunc(&iResults[i], aIter);
		iResults[i].Update();
		}
	
	*aCount = count;
	return iResults;
	}

void AddOverhead()
	{
	BMProgram* next = bmSuite;
	bmSuite=(BMProgram*)&overhead;
	bmSuite->Next()=next;
	}
