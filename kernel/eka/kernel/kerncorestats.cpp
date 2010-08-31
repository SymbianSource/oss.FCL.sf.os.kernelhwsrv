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
// e32\kernel\kerncorestats.cpp
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/** @file
@internalComponent
@prototype
*/

#include <kerncorestats.h>
#include <nk_priv.h>

// The following differ between unicore and smp, so use define to unify.
#ifdef __SMP__
#define FASTTIMER NKern::Timestamp()
#define FASTTIMERFREQ NKern::TimestampFrequency()
#define NUMCPUS TheScheduler.iNumCpus
#else
#define FASTTIMER (TUint64) NKern::FastCounter()
#define FASTTIMERFREQ NKern::FastCounterFrequency()
#define NUMCPUS 1
#endif

KernCoreStats* KernCoreStats::StatsData=NULL;
TSpinLock KernCoreStats::Lock(TSpinLock::EOrderGenericIrqLow0);

/**
This method is called to initiate the KernCoreStats system.
It can only be called once, and the is no way to disable the system again afterwards. 

@param aStatSelection

The bitfield defining which statistics to be collected.

@return KErrNone on success. KErrInUse is the system has already been configured.
*/
EXPORT_C TInt KernCoreStats::Configure(TUint aStatSelection)
	{
	if (StatsData)
		return KErrInUse;

	TInt cores = NUMCPUS;

// Calculate size needed

	TInt dataSize = 0;
	dataSize+= (aStatSelection & KStatsCoreTotalTimeInIdle)?	sizeof(TUint64)*cores*2 :0;
	dataSize+= (aStatSelection & KStatsTimeCrossIdleAndActive)?	sizeof(TUint64)*(cores+1) :0;
	dataSize+= (aStatSelection & KStatsCoreNumTimesInIdle)?		sizeof(TUint)*cores :0;
	dataSize+= (aStatSelection & KStatsCoreTotalTimeInIdle)?	sizeof(TUint)*cores :0;
	dataSize+= (aStatSelection & KStatsReadyStateChanges)?		sizeof(TUint)*2*cores:0;
	dataSize+= (aStatSelection & KStatsNumTimeSliceExpire)?		sizeof(TUint)*cores:0;
	
// Create stats object object

	KernCoreStats* kks = (KernCoreStats*) Kern::Alloc(sizeof(KernCoreStats) + dataSize); //coreTimesSize + offset);
	if (kks==NULL)
		return KErrNoMemory;

	kks->Construct(aStatSelection);

	__e32_memory_barrier();
	StatsData = kks;	
	return KErrNone;
	};



/**

  This is a utility class, for calculating address offsets within kerncorestats, and offsets where the data will be copied
  to at a later point.  This is made more complicated by every item being optional.

@internal
*/ 

class DynamicObject
	{
public:
	DynamicObject(TAny* aPointer);
	void Add(TBool, TUint, TAny**, TUint, TUint16&);
private:
	TUint iOffset;
	TUint* iPointer;
	};


/**
 
Contructor which takes the address of the allocated memory object, for which items are to be added into.

@internal
*/

DynamicObject::DynamicObject(TAny* aPointer)
	:iOffset(0),
	iPointer((TUint*) aPointer)
	{};

/**
Workes out the address of a data item within the alloceted memory object, placeing it after the previosely added one.
It also works out the target offset, for which the data will be used too fill, within a later provided data block.

@param aFeatureEnabled

A bool used to indicate if the feature is enabled or not.  If not, the refranced offsets are set to indicate the feature is disabled.

@param aSizeInWordsP

The amount of memory for which the object shall use within the internal data object.

@aPointer

The pointer to hold the location of the data item, within the internal object.
If the feature is disabled, this is set to NULL.

@param aSizeInWordsO

The amount of memory for which the object shall use within the exteral data object.

@param aOffset

The offset to point to the location of the data item, within the external object.  
This offset can be added to the provided pointer, to form a pointer to it.
If the feature is disabled, this is set to KStatDisabled.


@internal
*/

void DynamicObject::Add(TBool aFeatureEnabled, TUint aSizeInWordsP,TAny** aPointer, TUint aSizeInWordsO, TUint16& aOffset)
	{
	if (aFeatureEnabled)
		{	
		*aPointer = iPointer;
		aOffset = (TUint16) iOffset;
		iPointer+= aSizeInWordsP;
		iOffset+=sizeof(TInt)* aSizeInWordsO;
		}
	else
		{
		*aPointer=NULL;
		aOffset=KernCoreStats::KStatDisabled;
		}
	}	      

/**
@internal

This second stage constructor is used initialise the KernCoreStats object.
It is called by KernCoreStats::Configure.

@param aBF

The bitfield defining which statistics to be collected.

@param aSizeToCopy

The size of the section of the data buffer used for holding the stats, which
can be directly copied into the destination buffer.  The remaining must be calculated.

@pre If user thread, it must be in a critical section.

*/
void KernCoreStats::Construct(TUint aBF)
	{
	TInt cores = NUMCPUS;

	// Here we calculate the offsets and pointers of the data we are collecting.
	// We need pointers for internal data collection, and offsets for where we will put the data in the structure at collection.

	DynamicObject o((TUint8*) this + sizeof(KernCoreStats));
	TUint16 dummy=0;

	//	Flag to turn feature on,		size of/and location of internal data,	size of/and location of target data,
	o.Add(aBF & KStatsCoreTotalTimeInIdle, 		cores*2, 	(TAny**)& iTotalTimeIdle, 	cores,	iOffsTotalTimeIdle);
	o.Add(aBF & KStatsCoreTotalTimeInIdle, 		cores*2, 	(TAny**)& iLastTimeCore,	0,	dummy);
	o.Add(aBF & KStatsTimeCrossIdleAndActive, 	(cores+1)*2,	(TAny**)& iTimesCIA,		cores+1, iOffsTimesCIA);
	o.Add(aBF & KStatsCoreNumTimesInIdle, 		cores,		(TAny**)& iNumberIdles,		cores,	iOffsNumberIdles);
	o.Add(aBF & KStatsNumEvents, 			0,		(TAny**)& dummy,		1,	iOffsNumEvents);
	o.Add(aBF & KStatsReadyStateChanges,		cores,		(TAny**)& iLastAddReadyCount,	1, 	iOffsReadyStateAdd);
	o.Add(aBF & KStatsReadyStateChanges,		cores,		(TAny**)& iLastSubReadyCount,	1,	iOffsReadyStateRemove);
	o.Add(aBF & KStatsNumTimeSliceExpire,		cores,		(TAny**)& iLastSlicesCount,	1, 	iOffsNumTimeSliceExpire);


	TUint64 timeNow = FASTTIMER;
	TInt core;

	// set up varables needed for CoreTotalTimeInIdle
	if (iLastTimeCore)
		{
		for (core=0; core<cores; core++) // Start core times time now.
			iLastTimeCore[core]=timeNow;
		}

	// Set up variables used for KTimeCrossIdleAndActive (iTimesCIA) calculations.
	// (Could make conditional, but wouldnt save time)

	iLastTime=timeNow;
	iCoresIdle= 0;
	};


/**
This method fills the provided buffer with the kernel statistics that have been collected.

@param aBuffer

The pre-allocated buffer for which the collected data should be copied to.

@return KErrNone on success.
*/

EXPORT_C TInt KernCoreStats::Stats(TAny* aBuffer)
	{
	if (StatsData)
		return StatsData->StatsCopy(aBuffer);
	else
		return KErrNotReady;
	};



/**
@internal
  
This method implements the functionality of KernCoreStats::Stats.
This method fills the provided buffer with the kernel statistics that have been collected.

@param aBuffer

The pre-allocated buffer for which the collected data should be copied to.

@return KErrNone on success.
*/

TInt KernCoreStats::StatsCopy(TAny* aBuffer)
	{
	TUint32 timerFrequency = FASTTIMERFREQ;
	TUint32 targetFrequency=1000000;
	TInt cores = NUMCPUS;
	
	TInt core;
	TInt ints = __SPIN_LOCK_IRQSAVE(Lock); // This spinlock is doubley important, as it acts as memory barrier for "if (StatsData)"

	TUint64 timeNow = FASTTIMER;

	// For the core idle times, we must add in the time for any core which is currently idle,
	// so that the values are properly framed.
	if (iTotalTimeIdle)
		{
		TUint u;
		TUint64 idletime;
		for(core=0, u = iCoreMask; core<cores; u>>=1, core++)
			{
			if (u & 0x1u)
				{
				iTotalTimeIdle[core]+=timeNow-iLastTimeCore[core];
				iLastTimeCore[core]=timeNow;
				}

			idletime = (iTotalTimeIdle[core]*targetFrequency)/timerFrequency;
			Value32(aBuffer, iOffsTotalTimeIdle)[core] = ((idletime>>32)?KMaxTUint32:(TUint32)idletime);

			iTotalTimeIdle[core]=0;
			}

		__SPIN_FLASH_IRQRESTORE(Lock,ints); 
		timeNow = FASTTIMER;
		}
	

	// For times across idle and active, we must add in the time for the current state of the cores
	// so that the values are properly framed.
	if (iTimesCIA)
		{
		iTimesCIA[iCoresIdle]+= timeNow-iLastTime;
		iLastTime=timeNow;

		// Copy to structure
		TUint64 timecia;
		for(core=0; core<cores+1; core++) // here we are not actally talking about cores, but CIA.
			{
			timecia =  (iTimesCIA[core]*targetFrequency)/timerFrequency;

			Value32(aBuffer, iOffsTimesCIA)[core] = (timecia>>32)?KMaxTUint32:(TUint32)timecia;
			iTimesCIA[core]=0;
			}
		__SPIN_FLASH_IRQRESTORE(Lock,ints);
		}
	
	if (iNumberIdles)
		{
		for(core=0; core<cores; core++)
			{
			Value32(aBuffer, iOffsNumberIdles)[core] =iNumberIdles[core];
			iNumberIdles[core]=0;
			}
		}

	if (iOffsNumEvents!=KStatDisabled)
		{
		*Value32(aBuffer, iOffsNumEvents)= iNumEvents;
		iNumEvents=0;
		}

	__SPIN_UNLOCK_IRQRESTORE(Lock, ints);

	// Calculate the MadeReady couter, and the MadeUnready counter.

	if (iOffsReadyStateAdd!=KStatDisabled)
		{
		TUint addTot=0;
		TUint subTot=0;

		TUint addReadyCount;
		TUint subReadyCount;

#ifndef __X86__
#ifdef __SMP__
		// for each core, the previous ready and unready values are subtracted from
		// the current counter value, and then these values added together.

		for (core=0; core<cores; core++)
			{
			addReadyCount= TheSubSchedulers[core].iMadeReadyCounter;
			subReadyCount= TheSubSchedulers[core].iMadeUnReadyCounter;
			
			addTot+= addReadyCount - iLastAddReadyCount[core];
			iLastAddReadyCount[core]=addReadyCount;
			
			subTot+= subReadyCount - iLastSubReadyCount[core];
			iLastSubReadyCount[core]=subReadyCount;
			}
#else

		addReadyCount= TheScheduler.iMadeReadyCounter;
		subReadyCount= TheScheduler.iMadeUnReadyCounter;
			
		addTot= addReadyCount - *iLastAddReadyCount;
		*iLastAddReadyCount=addReadyCount;
			
		subTot= subReadyCount - *iLastSubReadyCount;
		*iLastSubReadyCount=subReadyCount;
#endif
#endif
		// write the calculated values into the provided data buffer.
		*Value32(aBuffer, iOffsReadyStateAdd)=addTot; 
		*Value32(aBuffer, iOffsReadyStateRemove)=subTot; 
		}


	// Calculate the time slice expire value
	if (iOffsNumTimeSliceExpire != KStatDisabled)
		{
		TUint slicesTot=0;
		TUint slicesCount;

#ifndef __X86__
#ifdef __SMP__	

		// for each core, the previous time slice values are subtracted from
		// the current counter values, and then these values added together.
		for (core=0; core<cores ;core++)
			{
			slicesCount= TheSubSchedulers[core].iTimeSliceExpireCounter;
			slicesTot+= slicesCount - iLastSlicesCount[core];
			iLastSlicesCount[core]=slicesCount;
			}
#else
		slicesCount= TheScheduler.iTimeSliceExpireCounter;
		slicesTot= slicesCount - *iLastSlicesCount;
		*iLastSlicesCount=slicesCount;
#endif
#endif	
		// write the calculated values into the provided data buffer.
		*Value32(aBuffer, iOffsNumTimeSliceExpire) = slicesTot;
		}

	return KErrNone;
	}


/**
@internal
   
This is called from idle thread, before it entered its idle state.
For SMP kernels, this is done by the kernel, and the base ports should not call EnterIdle.

@param aCore

The core for which this code in executing.

@pre Interrupts must be disabled
*/

extern "C" TUint KernCoreStats_EnterIdle(TUint aCore)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED , "KernCoreStats_EnterIdle");
	if (KernCoreStats::StatsData)
		{
			KernCoreStats::StatsData->DoEnterIdle(aCore); 
		return (TUint) ETrue;
		}
	return (TUint) EFalse;	
	}
#ifndef __WINS__
#ifndef __SMP__

/**
This is called from idle thread, before it enters its idle state.
For non-SMP kernels, the base port must call this explicitly.
SMP base ports should not call this, as its called from within the kernel.

@pre Interrupts must be disabled

@return a cookie that should be passed to LeaveIdle(), after the core leaves idle.
*/

EXPORT_C TUint KernCoreStats::EnterIdle()
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED , "KernCoreStats::EnterIdle");
	if (StatsData)
		{
		StatsData->DoEnterIdle(0); 
		return (TUint) ETrue;
		}
	return (TUint) EFalse;
	}

#endif
#endif

/**
@internal

The implementation code for KernCoreStats::EnterIdle and KernCoreStats_EnterIdle.

@param aCore

The core for which this code in executing.

@pre Interrupts must be disabled

*/

void KernCoreStats::DoEnterIdle(TUint aCore)
	{
	__SPIN_LOCK(Lock); // This spinlock is doubley important, as it acts as memory barrier for "if (StatsData)"
	TUint64 timeNow = FASTTIMER;

	__ASSERT_DEBUG( ((iCoreMask & (1<<aCore) )==0), FAULT() );


	if (iLastTimeCore)
		{
		iLastTimeCore[aCore]=timeNow;
		iCoreMask|=1<<aCore;
		}

	if (iNumberIdles)
		iNumberIdles[aCore]++;

	if (iTimesCIA)
		{
		iTimesCIA[iCoresIdle]+= timeNow-iLastTime;

#ifdef __SMP__
		__ASSERT_DEBUG((iCoresIdle<TheScheduler.iNumCpus), FAULT() );
#else
		__ASSERT_DEBUG((iCoresIdle==0), FAULT() );
#endif

		iLastTime=timeNow;
		iCoresIdle++;
		}

	__SPIN_UNLOCK(Lock);
	}

/**
This is called from idle thread, after it comes out of its idle state.
For SMP kernels, this is done by the kernel, and the base ports should not call LeaveIdle.

@param aCookie

The value returned from EnterIdle should be passed here.

@param aCore

The core for which this code in executing.

@pre Interrupts must be disabled
*/

extern "C" void KernCoreStats_LeaveIdle(TInt aCookie,TUint aCore)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED , "KernCoreStats_LeaveIdle");
	if (aCookie)
		KernCoreStats::StatsData->DoLeaveIdle(aCore);
	}

#ifndef __WINS__
#ifndef __SMP__

/**
This is called from idle thread, after it comes out of its idle state.
For non-SMP kernels, the base port must call this explicitly.
SMP base ports should not call this, as its called from within the kernel.

@param aCookie

The value returned from EnterIdle should be passed here.

@pre Interrupts must be disabled
*/

EXPORT_C void KernCoreStats::LeaveIdle(TUint aCookie)
	{
	CHECK_PRECONDITIONS(MASK_INTERRUPTS_DISABLED , "KernCoreStats::LeaveIdle");
	if (aCookie)
		StatsData->DoLeaveIdle(0);
	}

#endif
#endif

/**
@internal

The implementation code for KernCoreStats::LeaveIdle and KernCoreStats_LeaveIdle.

@param aCore

The core for which this code in executing.

@pre Interrupts must be disabled
*/

void KernCoreStats::DoLeaveIdle(TUint aCore)
	{
	__SPIN_LOCK(Lock);// This spinlock is doubley important, as it acts as memory barrier for "if (StatsData)"

	TUint64 timeNow = FASTTIMER;

	if (iTotalTimeIdle)
		{
		__ASSERT_DEBUG(iCoreMask&(1<<aCore), FAULT() );
		iCoreMask&= ~(1<<aCore);
		iTotalTimeIdle[aCore] += timeNow - iLastTimeCore[aCore];
		}
	if (iTimesCIA)
		{
		__ASSERT_DEBUG((StatsData->iCoresIdle>0), FAULT() );
		iTimesCIA[iCoresIdle]+= timeNow-iLastTime;
		iLastTime=timeNow;
		iCoresIdle--;
		}
	__SPIN_UNLOCK(Lock);
	}


/**
@internal

Called from Kern::AddEvent

This is used to increment the KernCoreStats event counter.
*/
void KernCoreStats::AddEvent()
	{
	if (StatsData)
		{
		TInt ints = __SPIN_LOCK_IRQSAVE(Lock);
		if (StatsData->iOffsNumEvents!= KStatDisabled)
			StatsData->iNumEvents++;
		__SPIN_UNLOCK_IRQRESTORE(Lock, ints);
		}
	};


EXPORT_C TInt KernCoreStats::Retire(TInt, TInt)
	{
	// DUMMY METHOD
	return KErrNone;
	};

EXPORT_C TInt KernCoreStats::Engage(TInt)
	{
	// DUMMY METHOD

	return KErrNone;
	}
