// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nkernsmp\dfcs.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __DFCS_H__
#define __DFCS_H__

#include <nklib.h>
#include <nk_event.h>

class NTimer;
class NThreadBase;
class NThread;
class NFastSemaphore;
class NFastMutex;
class TSubScheduler;
class TCancelIPI;

/********************************************
 * Delayed function call queue
 ********************************************/

/**
@publishedPartner
@released

The number of DFC priorities the system has, which range from 0
to KNumDfcPriorities - 1.
*/
const TInt KNumDfcPriorities=8;

/**
@publishedPartner
@released

The highest priority level for a DFC, which is equal to KNumDfcPriorities + 1.
*/
const TInt KMaxDfcPriority=KNumDfcPriorities-1;

class TDfc;
/**
@publishedPartner
@prototype

Defines a DFC queue.

Each DFC queue is associated with a thread.

@see TDfc
*/
class TDfcQue : public TPriList<TDfc,KNumDfcPriorities>
	{
public:
	IMPORT_C TDfcQue();

	inline TBool IsEmpty();		/**< @internalComponent */
	static void ThreadFunction(TAny* aDfcQ);
public:
	NThreadBase* iThread;		/**< @internalComponent */
	};

/**
@internalComponent
*/
inline TBool TDfcQue::IsEmpty()
	{ return (iPresent[0]==0); }

/********************************************
 * Delayed function call
 ********************************************/

/**
@publishedPartner
@released

The function type that can be set to run as a DFC or IDFC.

@see TDfc
*/
typedef NEventFn TDfcFn;

/**
@publishedPartner
@prototype

Defines a Deferred Function Call (DFC) or Immediate Deferred Function Call (IDFC).

A DFC is a kernel object that specifies a function to be run in a thread,
which is processing a DFC queue. A DFC is added to a DFC queue that is 
associated with a given thread, where it is cooperatively scheduled with other
DFCs on that queue.  Queued DFCs are run in order of their priority, followed
by the order they where queued.  When the DFC gets to run, the function is run
kernel side, and no other DFC in this queue will get to run until it 
completes. A DFC can be queued from any context.

An IDFC is run as soon as the scheduler is next run, which is during the IRQ
postamble if queued from an ISR; when the currently-running IDFC completes if
queued from an IDFC; or when the kernel is next unlocked if queued from thread
context.  Unlike a DFC, the IDFC is not run from a thread context, and its
execution time must be much smaller.  For these reasons, IDFCs are rarely used
directly, but are used for implementation of the kernel and RTOS personality
layers.  An important use of IDFCs is in the implementation of queuing DFCs from
an ISR context.  IDFCs are run with interrupts enabled but the kernel locked.
*/
class TDfc : public NEventHandler
	{
	// iPriority<KNumDfcPriorities => DFC, otherwise IDFC
	//
	// iHState2	= 0 normally
	//			= Bit n is set if CPU n is waiting to cancel this DFC
	// iHState1	= 0 if not on any list
	//			= 100nnnnn if on CPU n endogenous IDFC/DFC queue
	//			= 101nnnnn if on CPU n exogenous IDFC queue
	//			= 110nnnnn if running on CPU n
	//			= 111nnnnn if running on CPU n and a second execution is also pending (Add() was called while running)
	//			= 011nnnnn if running on CPU n and a second idle queue is also pending (QueueOnIdle() was called while running)
	//			= 0010000g if idle DFC generation g (could be either on idle queue or CPU endogenous IDFC/DFC queue)
	//			= 00000001 if on final DFC queue
	// iHState1 and iHState2 are accessed together as a single 16 bit iDfcState
	//
	// iHState0 is set to 0 when a DFC/IDFC is added to a CPUs endogenous IDFC
	// queue or to the idle queue. It is set to 1 if and when BeginTiedEvent()
	// is subsequently called (thus only for tied IDFCs).
	//
	// For IDFC iHType = EEventHandlerIDFC
	// For DFC iHType = priority (0 to 7) and iTied points to TDfcQue (since DFCs can't be tied)
	//
public:
	IMPORT_C TDfc(TDfcFn aFunction, TAny* aPtr);									// create IDFC
	IMPORT_C TDfc(NSchedulable* aTied, TDfcFn aFunction, TAny* aPtr);				// create IDFC tied to a thread or group
	IMPORT_C TDfc(TDfcFn aFunction, TAny* aPtr, TInt aPriority);					// create DFC, queue to be set later
	IMPORT_C TDfc(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority);	// create DFC
	IMPORT_C ~TDfc();
	IMPORT_C TBool Add();						// call from ISR or IDFC or thread with kernel locked
	IMPORT_C TBool Cancel();					// call from anywhere except ISR
	IMPORT_C TBool Enque();						// call from thread
	IMPORT_C TBool Enque(NFastMutex* aMutex);	// call from thread, signal fast mutex (anti-thrash)
	IMPORT_C TBool DoEnque();					// call from IDFC or thread with kernel locked
	IMPORT_C TBool RawAdd();					// same as Add() but without checks for 'correct' usage or other instrumentation
	IMPORT_C TBool QueueOnIdle();				// queue the DFC to be run when the system goes idle
	IMPORT_C TInt SetTied(NSchedulable* aTied);	// tie an IDFC to a thread or group
	IMPORT_C NThreadBase* Thread();				// thread on which DFC runs, NULL for IDFC
public:
	inline TBool Queued();
	inline TBool IsIDFC();
	inline TBool IsDFC();
	inline void SetDfcQ(TDfcQue* aDfcQ);
	inline void SetFunction(TDfcFn aDfcFn);
	inline void SetPriority(TInt aPriority);			/**< @internalComponent */
private:
	inline TBool IsValid();								/**< @internalComponent */
private:
	TUint32 AddStateChange();							/**< @internalComponent */
	TUint32 MoveToFinalQStateChange();					/**< @internalComponent */
	TUint32 TransferIDFCStateChange(TInt aCpu);			/**< @internalComponent */
	TUint32 RunIDFCStateChange();						/**< @internalComponent */
	TUint32 EndIDFCStateChange(TSubScheduler*);			/**< @internalComponent */
	TUint32 EndIDFCStateChange2();						/**< @internalComponent */
	TUint32 CancelInitialStateChange();					/**< @internalComponent */
	TUint32 CancelFinalStateChange();					/**< @internalComponent */
	TUint32 QueueOnIdleStateChange();					/**< @internalComponent */
	void ResetState();									/**< @internalComponent */

	friend class TSubScheduler;
	friend class TCancelIPI;
	friend class TDfcQue;
	friend class NTimer;
	};

/**
@publishedPartner
@prototype

Used to find out if the DFC/IDFC is queued on either the pending or final DFC queue.

@return TRUE if the DFC/IDFC is queued, otherwise FALSE.

*/
inline TBool TDfc::Queued()
	{ TUint32 state = i8816.iHState16; return state && (state&0xE0)!=0xC0; }

/**
@publishedPartner
@prototype

Determines if the object represents an IDFC rather than a DFC.

@return TRUE if this represents an IDFC, otherwise FALSE.
*/
inline TBool TDfc::IsIDFC()
	{ return iHType == EEventHandlerIDFC; }

/**
@publishedPartner
@prototype

Determines if the object represents a DFC rather than an IDFC.

@return TRUE if this represents a DFC, otherwise FALSE.
*/
inline TBool TDfc::IsDFC()
	{ return iHType < KNumDfcPriorities; }


/**
@publishedPartner
@prototype

Sets the DFC queue that the DFC is to added to and executed by.

Note that this function should only be used in the initialisation of the DFC, 
when it is not on any queue.  This function does not move the DFC from one 
queue to another.

@param aDfcQ

	The DFC queue that the DFC is to be added to and executed by.

*/
inline void TDfc::SetDfcQ(TDfcQue* aDfcQ)
	{ iDfcQ = aDfcQ; }

/**
@publishedPartner
@prototype

Sets the function that is run when the DFC/IDFC is scheduled.

@param aDfcFn

	The function that the DFC/IDFC runs when it is scheduled.

*/
inline void TDfc::SetFunction(TDfcFn aDfcFn)
	{ iFn = aDfcFn; }

/**
@internalComponent
*/
inline void TDfc::SetPriority(TInt aPriority)
	{ iHState = (TUint8)aPriority; }

#ifdef __INCLUDE_TDFC_DEFINES__
/**
@internalComponent
*/
#define	iDfcState		(i8816.iHState16)

/**
@internalComponent
*/
#define DFC_STATE(p)	((p)->i8816.iHState16)
#endif


/********************************************
 * Kernel-side asynchronous request,
 * based on DFC queueing
 ********************************************/

/**
@internalComponent
*/
class TAsyncRequest : protected TDfc
	{
public:
	IMPORT_C void Send(TDfc* aCompletionDfc);
	IMPORT_C void Send(NFastSemaphore* aCompletionSemaphore);
	IMPORT_C TInt SendReceive();
	IMPORT_C void Cancel();
	IMPORT_C void Complete(TInt aResult);
	inline TBool PollForCancel()
		{ return iCancel; }
protected:
	IMPORT_C TAsyncRequest(TDfcFn aFunction, TDfcQue* aDfcQ, TInt aPriority);
protected:
	TAny*	iCompletionObject;
	volatile TBool	iCancel;
	TInt	iResult;
	};


#endif
