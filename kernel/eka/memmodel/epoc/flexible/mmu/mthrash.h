// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @file
 @internalComponent
*/

#ifndef MTHRASH_H
#define MTHRASH_H

class DThrashMonitor
	{
public:
	
	DThrashMonitor();
	void Start();
	TInt ThrashLevel();
	TInt SetThresholds(TUint thrashing, TUint good);

	void NotifyStartPaging();  			// The current thread has started a paging operation
	void NotifyEndPaging();    			// The current thread has completed a paging operation

private:

	enum TCount
		{
		ECountThreadsPaging,
		EMaxCount
		};

	struct TCountData
		{
		TInt iCount;					// Current value of counter
		TUint32 iLastUpdateTime;		// Time counter was last updated
		TInt64 iTotal;					// Accumulator for ticks * value
		TInt iAverage;					// Average value of counter for last time period * 100
		};

private: 

	void UpdateCount(TCount aCount, TInt aDelta);	
	void RecalculateThrashLevel();		// Update internal state, called periodically from timer DFC
	static void UpdateDfcFunc(TAny* aPtr);

private:

	TBool iRunning;
	NTimer iUpdateTimer;
	TDfc iUpdateDfc;
	TUint32 iLastUpdateTime;
	NFastMutex iMutex;

	TInt iThrashLevel;
	TBool iIsThrashing;
	TInt iThresholdThrashing;
	TInt iThresholdGood;

	TCountData iCount[EMaxCount];
	};

extern DThrashMonitor TheThrashMonitor;

#endif
