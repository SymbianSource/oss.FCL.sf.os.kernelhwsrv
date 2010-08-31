// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\KernCoreStats.h
// 
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/** @file
@internalComponent
@prototype
*/

#ifndef __KERNCORESTATS_H__
#define __KERNCORESTATS_H__

#include <kernel/kernel.h>

// Kernel side stats (also used when obtaining stats from Kernel)
const TInt KStatsCoreNumTimesInIdle		 = 0x0001;
const TInt KStatsCoreTotalTimeInIdle		 = 0x0002;
const TInt KStatsTimeCrossIdleAndActive		 = 0x0004;
const TInt KStatsReadyStateChanges		 = 0x0008;
const TInt KStatsNumTimeSliceExpire		 = 0x0010;
const TInt KStatsNumEvents			 = 0x0020;

extern "C" {
extern  TUint KernCoreStats_EnterIdle(TUint aCore);
extern  void KernCoreStats_LeaveIdle(TInt aCookie,TUint aCore);
}

class KernCoreStats
	{

public:

IMPORT_C static TInt Stats(TAny* aBuffer);
IMPORT_C static TInt Configure(TUint aStatSelection);

IMPORT_C static TInt Retire(TInt aRetired, TInt aInfoMask=0);
IMPORT_C static TInt Engage(TInt aEngage);

#ifndef __WINS__
#ifndef __SMP__

IMPORT_C static TUint EnterIdle();
IMPORT_C static void LeaveIdle(TUint aCookie);

#endif
#endif

	static void AddEvent();

	enum {KStatDisabled = 1};  // Thas value is used in place of an structure offset, to indicate the stat is'nt enabled.

private:
	TInt StatsCopy(TAny* aBuffer);
	void Construct(TUint aBitField);

	void DoEnterIdle(TUint aCore);
	void DoLeaveIdle(TUint aCore);

private:

	static KernCoreStats* StatsData;
	static TSpinLock Lock;	// We use a spinlock here becouse we cant use a FastMutex - as there is
       				// a good chance a FastMutex will be held when the core goes
	
	inline TUint32* Value32(TAny* aBuffer, TUint16 aOffset);
	inline TUint64* Value64(TAny* aBuffer, TUint16 aOffset);
private:


	// Here stored geometry information about data to be collected internally.

	TUint64* iTotalTimeIdle;	// Pointer to Array of Time spent Idle, per core.
	TUint64* iLastTimeCore;
	TUint64* iTimesCIA;		//Pointer to Array of Time spent in permutions of idle.
	TUint* iNumberIdles;	//Pointer of Array of number of times idle, per core. 
	TUint iNumEvents;
	TUint *iLastAddReadyCount;
	TUint *iLastSubReadyCount;
	TUint *iLastSlicesCount;


	// The stored gemetry of where the colected data will be copied too, in offsets
	TUint16 iOffsTotalTimeIdle;
	TUint16 iOffsTimesCIA;

	TUint16 iOffsNumberIdles;
	TUint16 iOffsNumEvents;

	TUint16 iOffsReadyStateAdd;
	TUint16 iOffsReadyStateRemove;

	TUint16 iOffsNumTimeSliceExpire;
	// Idle Tracking Data
	TUint8 iCoresIdle;
	TUint8 iPad; // Not used.

	TUint iCoreMask;			// indicates which cores are idle
	TUint64 iLastTime;			// used for TimesCIA
friend TUint KernCoreStats_EnterIdle(TUint aCore);
friend void KernCoreStats_LeaveIdle(TInt aCookie,TUint aCore);
	};

inline TUint32* KernCoreStats::Value32(TAny* aBuffer, TUint16 aOffset)
{ return (TUint32*) ((TUint8*) aBuffer + aOffset); };

inline TUint64* KernCoreStats::Value64(TAny* aBuffer, TUint16 aOffset)
{ return (TUint64*) ((TUint8*) aBuffer + aOffset); };


#endif // __KERNCORESTATS_H__
