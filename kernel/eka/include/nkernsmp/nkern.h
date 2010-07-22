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
// e32\include\nkernsmp\nkern.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __NKERN_H__
#define __NKERN_H__

#ifdef	__STANDALONE_NANOKERNEL__
#undef	__IN_KERNEL__
#define	__IN_KERNEL__
#endif

#include <e32const.h>
#include <nklib.h>
#include <nk_event.h>
#include <dfcs.h>
#include <nk_trace.h>
#include <e32atomics.h>

extern "C" {
/** @internalComponent */
IMPORT_C void NKFault(const char* file, TInt line);
/** @internalComponent */
void NKIdle(TUint32 aStage);
}

/**
@publishedPartner
@released
*/
#define FAULT()		NKFault(__FILE__,__LINE__)

#ifdef _DEBUG

/**
@publishedPartner
@released
*/
#define __NK_ASSERT_DEBUG(c)	((void) ((c)||(FAULT(),0)) )

#else

#define __NK_ASSERT_DEBUG(c)

#endif

/**
@publishedPartner
@released
*/
#define __NK_ASSERT_ALWAYS(c)	((void) ((c)||(FAULT(),0)) )

/**
	@publishedPartner
	@released
*/
const TInt KNumPriorities=64;

const TInt KMaxCpus=8;

class NSchedulable;
class NThread;
class NThreadGroup;


/** Spin lock

	Used for protecting a code fragment against both interrupts and concurrent
	execution on another processor.

	List of spin locks in the nanokernel, in deadlock-prevention order:
	A	NEventHandler::TiedLock (preemption)
	B	NFastMutex spin locks (preemption)
	C	Thread spin locks (preemption)
	D	Thread group spin locks (preemption)
	E	Per-CPU ready list lock (preemption)

	a	Idle DFC list lock (interrupts)
	b	Per-CPU exogenous IDFC queue lock (interrupts)
	c	NTimerQ spin lock (interrupts)
	d	Generic IPI list locks (interrupts)
	e	NIrq spin locks (interrupts)
	f	Per-CPU event handler list lock (interrupts)
	z	BTrace lock (interrupts)

	z must be minimum since BTrace can appear anywhere

	interrupt-disabling spinlocks must be lower than preemption-disabling ones

	Nestings which actually occur are:
		A > C
		B > C > D > E
		c > f
		Nothing (except possibly z) nested inside a, b, d, f
		e is held while calling HW-poking functions (which might use other spinlocks)

@publishedPartner
@prototype
*/
class TSpinLock
	{
public:
	enum TOrder
		{
		// Bit 7 of order clear for locks used with interrupts disabled
		EOrderGenericIrqLow0	=0x00u,		// Device driver spin locks, low range
		EOrderGenericIrqLow1	=0x01u,		// Device driver spin locks, low range
		EOrderGenericIrqLow2	=0x02u,		// Device driver spin locks, low range
		EOrderGenericIrqLow3	=0x03u,		// Device driver spin locks, low range
		EOrderBTrace			=0x04u,		// BTrace lock
		EOrderEventHandlerList	=0x07u,		// Per-CPU event handler list lock
		EOrderCacheMaintenance  =0x08u,		// CacheMaintenance (for PL310)
		EOrderNIrq				=0x0Au,		// NIrq lock
		EOrderGenericIPIList	=0x0Du,		// Generic IPI list lock
		EOrderNTimerQ			=0x10u,		// Nanokernel timer queue lock
		EOrderExIDfcQ			=0x13u,		// Per-CPU exogenous IDFC queue list lock
		EOrderIdleDFCList		=0x16u,		// Idle DFC list lock
		EOrderGenericIrqHigh0	=0x18u,		// Device driver spin locks, high range
		EOrderGenericIrqHigh1	=0x19u,		// Device driver spin locks, high range
		EOrderGenericIrqHigh2	=0x1Au,		// Device driver spin locks, high range
		EOrderGenericIrqHigh3	=0x1Bu,		// Device driver spin locks, high range

		// Bit 7 of order set for locks used with interrupts enabled, preemption disabled
		EOrderGenericPreLow0	=0x80u,		// Device driver spin locks, low range
		EOrderGenericPreLow1	=0x81u,		// Device driver spin locks, low range
		EOrderReadyList			=0x88u,		// Per-CPU ready list lock
		EOrderThreadGroup		=0x90u,		// Thread group locks
		EOrderThread			=0x91u,		// Thread locks
		EOrderFastMutex			=0x98u,		// Fast mutex locks
		EOrderEventHandlerTied	=0x9Cu,		// Event handler tied lock
		EOrderEnumerate			=0x9Du,		// Thread/Group enumeration lists
		EOrderGenericPreHigh0	=0x9Eu,		// Device driver spin locks, high range
		EOrderGenericPreHigh1	=0x9Fu,		// Device driver spin locks, high range

		EOrderNone				=0xFFu		// No order check required (e.g. for dynamic ordering)
		};
public:
	IMPORT_C TSpinLock(TUint aOrder);
	IMPORT_C void LockIrq();				/**< @internalComponent disable interrupts and acquire the lock */
	IMPORT_C void UnlockIrq();				/**< @internalComponent release the lock and enable interrupts */
	IMPORT_C TBool FlashIrq();				/**< @internalComponent if someone else is waiting for the lock, UnlockIrq() then LockIrq() */
	IMPORT_C void LockOnly();				/**< @internalComponent acquire the lock, assuming interrupts/preemption already disabled */
	IMPORT_C void UnlockOnly();				/**< @internalComponent release the lock, don't change interrupt/preemption state */
	IMPORT_C TBool FlashOnly();				/**< @internalComponent if someone else is waiting for the lock, UnlockOnly() then LockOnly() */
	IMPORT_C TInt LockIrqSave();			/**< @internalComponent remember original interrupt state then disable interrupts and acquire the lock */
	IMPORT_C void UnlockIrqRestore(TInt);	/**< @internalComponent release the lock then restore original interrupt state */
	IMPORT_C TBool FlashIrqRestore(TInt);	/**< @internalComponent if someone else is waiting for the lock, UnlockIrqRestore() then LockIrq() */
	IMPORT_C TBool FlashPreempt();			/**< @internalComponent if someone else is waiting for the lock, UnlockOnly(); NKern::PreemptionPoint(); LockOnly(); */
private:
	volatile TUint64 iLock;
	};


/** Macro to disable interrupts and acquire the lock.

@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQ(lock)				((lock).LockIrq())

/** Macro to release the lock and enable interrupts.

@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQ(lock)				(lock).UnlockIrq()

/** Macro to see if someone else is waiting for the lock, enabling IRQs 
    then disabling IRQs again.

@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQ(lock)				(lock).FlashIrq()

/** Macro to remember original interrupt state then disable interrupts 
    and acquire the lock.
    
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQSAVE(lock)			((lock).LockIrqSave())

/** Macro to release the lock then restore original interrupt state to that 
	supplied.
	
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQRESTORE(lock,irq)	(lock).UnlockIrqRestore(irq)

/** Macro to see if someone else is waiting for the lock, enabling IRQs to
	the original state supplied then disabling IRQs again.
    
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQRESTORE(lock,irq)	(lock).FlashIrqRestore(irq)

/** Macro to acquire the lock. This assumes the caller has already disabled 
    interrupts/preemption. 
	
	If interrupts/preemption is not disabled a run-time assert will occur
	This is to protect against unsafe code that might lead to same core 
	deadlock.
	
    In device driver code it is safer to use __SPIN_LOCK_IRQSAVE() instead, 
	although not as efficient should interrupts aleady be disabled for the 
	duration the lock is held.
    
@publishedPartner
@prototype
*/
#define __SPIN_LOCK(lock)					((lock).LockOnly())

/** Macro to release the lock, don't change interrupt/preemption state.

@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK(lock)					(lock).UnlockOnly()

/**
@internalComponent
*/
#define __SPIN_FLASH(lock)					(lock).FlashOnly()

/** Macro to see if someone else is waiting for the lock, enabling preemption 
    then disabling it again.

@publishedPartner
@prototype
*/
#define __SPIN_FLASH_PREEMPT(lock)			(lock).FlashPreempt()


/** Read/Write Spin lock

@publishedPartner
@prototype
*/
class TRWSpinLock
	{
public:
	IMPORT_C TRWSpinLock(TUint aOrder);		// Uses same order space as TSpinLock

	IMPORT_C void LockIrqR();				/**< @internalComponent disable interrupts and acquire read lock */
	IMPORT_C void UnlockIrqR();				/**< @internalComponent release read lock and enable interrupts */
	IMPORT_C TBool FlashIrqR();				/**< @internalComponent if someone else is waiting for write lock, UnlockIrqR() then LockIrqR() */
	IMPORT_C void LockIrqW();				/**< @internalComponent disable interrupts and acquire write lock */
	IMPORT_C void UnlockIrqW();				/**< @internalComponent release write lock and enable interrupts */
	IMPORT_C TBool FlashIrqW();				/**< @internalComponent if someone else is waiting for the lock, UnlockIrqW() then LockIrqW() */
	IMPORT_C void LockOnlyR();				/**< @internalComponent acquire read lock, assuming interrupts/preemption already disabled */
	IMPORT_C void UnlockOnlyR();			/**< @internalComponent release read lock, don't change interrupt/preemption state */
	IMPORT_C TBool FlashOnlyR();			/**< @internalComponent if someone else is waiting for write lock, UnlockOnlyR() then LockOnlyR() */
	IMPORT_C void LockOnlyW();				/**< @internalComponent acquire write lock, assuming interrupts/preemption already disabled */
	IMPORT_C void UnlockOnlyW();			/**< @internalComponent release write lock, don't change interrupt/preemption state */
	IMPORT_C TBool FlashOnlyW();			/**< @internalComponent if someone else is waiting for the lock, UnlockOnlyW() then LockOnlyW() */
	IMPORT_C TInt LockIrqSaveR();			/**< @internalComponent disable interrupts and acquire read lock, return original interrupt state */
	IMPORT_C void UnlockIrqRestoreR(TInt);	/**< @internalComponent release read lock and reset original interrupt state */
	IMPORT_C TBool FlashIrqRestoreR(TInt);	/**< @internalComponent if someone else is waiting for write lock, UnlockIrqRestoreR() then LockIrqR() */
	IMPORT_C TInt LockIrqSaveW();			/**< @internalComponent disable interrupts and acquire write lock, return original interrupt state */
	IMPORT_C void UnlockIrqRestoreW(TInt);	/**< @internalComponent release write lock and reset original interrupt state */
	IMPORT_C TBool FlashIrqRestoreW(TInt);	/**< @internalComponent if someone else is waiting for the lock, UnlockIrqRestoreW() then LockIrqW() */
	IMPORT_C TBool FlashPreemptR();			/**< @internalComponent if someone else is waiting for write lock, UnlockOnlyR(); NKern::PreemptionPoint(); LockOnlyR(); */
	IMPORT_C TBool FlashPreemptW();			/**< @internalComponent if someone else is waiting for the lock, UnlockOnlyW(); NKern::PreemptionPoint(); LockOnlyW(); */
private:
	volatile TUint64 iLock;
	};


/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQ_R(lock)					(lock).LockIrqR()

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQ_R(lock)				(lock).UnlockIrqR()

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQ_R(lock)				((lock).FlashIrqR())

/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQ_W(lock)					(lock).LockIrqW()

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQ_W(lock)				(lock).UnlockIrqW()

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQ_W(lock)				((lock).FlashIrqW())


/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_R(lock)						(lock).LockOnlyR()

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_R(lock)					(lock).UnlockOnlyR()

/**
@internalComponent
*/
#define __SPIN_FLASH_R(lock)					((lock).FlashOnlyR())

/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_W(lock)						(lock).LockOnlyW()

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_W(lock)					(lock).UnlockOnlyW()

/**
@internalComponent
*/
#define __SPIN_FLASH_W(lock)					((lock).FlashOnlyW())


/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQSAVE_R(lock)				(lock).LockIrqSaveR()

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQRESTORE_R(lock,irq)	(lock).UnlockIrqRestoreR(irq)

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQRESTORE_R(lock,irq)		((lock).FlashIrqRestoreR(irq))

/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQSAVE_W(lock)				(lock).LockIrqSaveW()

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQRESTORE_W(lock,irq)	(lock).UnlockIrqRestoreW(irq)

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQRESTORE_W(lock,irq)		((lock).FlashIrqRestoreW(irq))


/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_PREEMPT_R(lock)			((lock).FlashPreemptR())

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_PREEMPT_W(lock)			((lock).FlashPreemptW())


#ifdef _DEBUG
#define __INCLUDE_SPIN_LOCK_CHECKS__
#endif


/** Nanokernel fast semaphore

	A light-weight semaphore class that only supports a single waiting thread,
	suitable for the Symbian OS thread I/O semaphore.
	
	Initialising a NFastSemaphore involves two steps:
	
	- Constructing the semaphore
	- Setting the semaphore owning thread (the one allowed to wait on it)
	
	For example, creating one for the current thread to wait on:
	
	@code
	NFastSemaphore sem;
	sem.iOwningThread = NKern::CurrentThread();
	@endcode
	
	@publishedPartner
	@prototype
*/
class NFastSemaphore
	{
public:
	inline NFastSemaphore();
	inline NFastSemaphore(NThreadBase* aThread);
	IMPORT_C void SetOwner(NThreadBase* aThread);
	IMPORT_C void Wait();
	IMPORT_C void Signal();
	IMPORT_C void SignalN(TInt aCount);
	IMPORT_C void Reset();
	void WaitCancel();

	TInt Dec(NThreadBase* aThread);	// does mb() if >0
	NThreadBase* Inc(TInt aCount);	// does mb()
	NThreadBase* DoReset();	// does mb()
public:
	/** If >=0 the semaphore count
		If <0, (thread>>2)|0x80000000
		@internalComponent
	*/
	TInt iCount;

	/** The thread allowed to wait on the semaphore
		@internalComponent
	*/
	NThreadBase* iOwningThread;	
	};

/** Create a fast semaphore

	@publishedPartner
	@prototype
*/
inline NFastSemaphore::NFastSemaphore()
	: iCount(0), iOwningThread(NULL)
	{}

/** Nanokernel fast mutex

	A light-weight priority-inheritance mutex that can be used if the following
	conditions apply:
	
	- Threads that hold the mutex never block.
	- The mutex is never acquired in a nested fashion
	
	If either of these conditions is not met, a DMutex object is more appropriate.
	
	@publishedPartner
	@prototype
*/
class NFastMutex
	{
public:
	IMPORT_C NFastMutex();
	IMPORT_C void Wait();
	IMPORT_C void Signal();
	IMPORT_C TBool HeldByCurrentThread();
private:
	void DoWaitL();
	void DoSignalL();

	friend class NKern;
public:
	/** @internalComponent

	If mutex is free and no-one is waiting, iHoldingThread=0
	If mutex is held and no-one is waiting, iHoldingThread points to holding thread
	If mutex is free but threads are waiting, iHoldingThread=1
	If mutex is held and threads are waiting, iHoldingThread points to holding thread but with bit 0 set
	*/
	NThreadBase* iHoldingThread;

	TUint32 i_NFastMutex_Pad1;	/**< @internalComponent */

	/** @internalComponent

	Spin lock to protect mutex
	*/
	TSpinLock iMutexLock;

	/** @internalComponent

	List of NThreads which are waiting for the mutex. The threads are linked via
	their iWaitLink members.
	*/
	TPriList<NThreadBase, KNumPriorities> iWaitQ;
	};

__ASSERT_COMPILE(!(_FOFF(NFastMutex,iMutexLock)&7));


/**
@publishedPartner
@prototype

The type of the callback function used by the nanokernel timer. 

@see NTimer
*/
typedef NEventFn NTimerFn;




/**
@publishedPartner
@prototype

A basic relative timer provided by the nanokernel.

It can generate either a one-shot interrupt or periodic interrupts.

A timeout handler is called when the timer expires, either:
- from the timer ISR - if the timer is queued via OneShot(TInt aTime) or OneShot(TInt aTime, TBool EFalse), or
- from the nanokernel timer dfc1 thread - if the timer is queued via OneShot(TInt aTime, TBool ETrue) call, or
- from any other dfc thread that provided DFC belongs to - if the timer is queued via OneShot(TInt aTime, TDfc& aDfc) call.
Call-back mechanism cannot be changed in the life time of a timer.

These timer objects may be manipulated from any context.
The timers are driven from a periodic system tick interrupt,
usually a 1ms period.

@see NTimerFn
*/
class NTimerQ;
class NTimer : public NEventHandler
	{
public:
	/**
	Default constructor.
	*/
	inline NTimer()
		{
		iHType = EEventHandlerNTimer;
		i8888.iHState1 = EIdle;
		}
	/**
	Constructor taking a callback function and a pointer to be passed
	to the callback function.
	
	@param aFunction The callback function.
	@param aPtr      A pointer to be passed to the callback function 
	                 when called.
	*/
	inline NTimer(NTimerFn aFunction, TAny* aPtr)
		{
		iPtr = aPtr;
		iFn = aFunction;
		iHType = EEventHandlerNTimer;
		i8888.iHState1 = EIdle;
		}
	IMPORT_C NTimer(NSchedulable* aTied, NTimerFn aFunction, TAny* aPtr);
	IMPORT_C NTimer(TDfcFn aFunction, TAny* aPtr, TInt aPriority);					// create DFC, queue to be set later
	IMPORT_C NTimer(TDfcFn aFunction, TAny* aPtr, TDfcQue* aDfcQ, TInt aPriority);	// create DFC
	IMPORT_C void SetDfcQ(TDfcQue* aDfcQ);
	IMPORT_C ~NTimer();
	IMPORT_C TInt SetTied(NSchedulable* aTied);
	IMPORT_C TInt OneShot(TInt aTime);
	IMPORT_C TInt OneShot(TInt aTime, TBool aDfc);
	IMPORT_C TInt OneShot(TInt aTime, TDfc& aDfc);
	IMPORT_C TInt Again(TInt aTime);
	IMPORT_C TBool Cancel();
	IMPORT_C TBool IsPending();
private:
	enum { ECancelDestroy=1 };
private:
	inline TBool IsNormal()
		{ return iHType==EEventHandlerNTimer; }
	inline TBool IsMutating()
		{ return iHType<KNumDfcPriorities; }
	inline TBool IsValid()
		{ return iHType<KNumDfcPriorities || iHType==EEventHandlerNTimer; }
	void AddAsDFC();
	TUint DoCancel(TUint aFlags);
	void DoCancel0(TUint aState);
	TBool DoCancelMutating(TUint aFlags);
public:
/**
	@internalComponent
*/
	enum TState
		{
		EIdle=0,			// not queued
							// 1 skipped so as not to clash with DFC states
		ETransferring=2,	// being transferred from holding to ordered queue
		EHolding=3,			// on holding queue
		EOrdered=4,			// on ordered queue
		ECritical=5,		// on ordered queue and in use by queue walk routine
		EFinal=6,			// on final queue
		EEventQ=32,			// 32+n = on event queue of CPU n (for tied timers)
		};
public:
	TUint32 iTriggerTime;	/**< @internalComponent */
	TUint32	iNTimerSpare1;	/**< @internalComponent */

	/** This field is available for use by the timer client provided that
		the timer isn't a mutating-into-DFC timer.
		@internalTechnology */
//	TUint8 iUserFlags;									// i8888.iHState0
//	TUint8 iState;			/**< @internalComponent */	// i8888.iHState1
//	TUint8 iCompleteInDfc;	/**< @internalComponent */	// i8888.iHState2


	friend class NTimerQ;
	friend class NSchedulable;
	};

/**
@internalTechnology
*/
#define	i_NTimer_iUserFlags	i8888.iHState0

/**
@internalComponent
*/
#define	i_NTimer_iState		i8888.iHState1

/**
	@publishedPartner
	@released
*/
typedef void (*NThreadFunction)(TAny*);

/**
	@publishedPartner
	@released
*/
typedef TDfc* (*NThreadExitHandler)(NThread*);

/**
	@publishedPartner
	@prototype
*/
typedef void (*NThreadStateHandler)(NThread*,TInt,TInt);

/**
	@publishedPartner
	@prototype
*/
typedef void (*NThreadExceptionHandler)(TAny*,NThread*);

/**
	@publishedPartner
	@prototype
*/
typedef void (*NThreadTimeoutHandler)(NThread*,TInt);

/**
	@publishedPartner
	@prototype
*/
struct SNThreadHandlers
	{
	NThreadExitHandler iExitHandler;
	NThreadStateHandler iStateHandler;
	NThreadExceptionHandler iExceptionHandler;
	NThreadTimeoutHandler iTimeoutHandler;
	};

/** @internalComponent */
extern void NThread_Default_State_Handler(NThread*, TInt, TInt);

/** @internalComponent */
extern void NThread_Default_Exception_Handler(TAny*, NThread*);

/** @internalComponent */
#define NTHREAD_DEFAULT_EXIT_HANDLER		((NThreadExitHandler)0)

/** @internalComponent */
#define	NTHREAD_DEFAULT_STATE_HANDLER		(&NThread_Default_State_Handler)

/** @internalComponent */
#define	NTHREAD_DEFAULT_EXCEPTION_HANDLER	(&NThread_Default_Exception_Handler)

/** @internalComponent */
#define	NTHREAD_DEFAULT_TIMEOUT_HANDLER		((NThreadTimeoutHandler)0)


/**
	@publishedPartner
	@prototype
*/
struct SFastExecTable
	{
	TInt iFastExecCount;			// includes implicit function#0
	TLinAddr iFunction[1];			// first entry is for call number 1
	};

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagClaim=0x80000000;		// claim system lock

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagRelease=0x40000000;		// release system lock

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagPreprocess=0x20000000;	// preprocess

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgMask=0x1C000000;	// 3 bits indicating additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs2=0x04000000;	// 2 additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs3=0x08000000;	// 3 additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs4=0x0C000000;	// 4 additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs5=0x10000000;	// 5 additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs6=0x14000000;	// 6 additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs7=0x18000000;	// 7 additional arguments

/**
	@publishedPartner
	@prototype
*/
const TUint32 KExecFlagExtraArgs8=0x1C000000;	// 8 additional arguments


/**
	@publishedPartner
	@prototype
*/
struct SSlowExecEntry
	{
	TUint32 iFlags;					// information about call
	TLinAddr iFunction;				// address of function to be called
	};


/**
	@publishedPartner
	@prototype
*/
struct SSlowExecTable
	{
	TInt iSlowExecCount;
	TLinAddr iInvalidExecHandler;	// used if call number invalid
	TLinAddr iPreprocessHandler;	// used for handle lookups
	SSlowExecEntry iEntries[1];		// first entry is for call number 0
	};

// Thread iAttributes Constants
const TUint8 KThreadAttImplicitSystemLock=1;		/**< @internalComponent */
const TUint8 KThreadAttAddressSpace=2;				/**< @internalComponent */
const TUint8 KThreadAttLoggable=4;					/**< @internalComponent */


// Thread CPU
const TUint32 KCpuAffinityAny=0xffffffffu;			/**< @internalComponent */
const TUint32 KCpuAffinityPref=0x40000000u;			/**< @internalComponent */
const TUint32 KCpuAffinityTransient=0x20000000u;	/**< @internalComponent */

/** Information needed for creating a nanothread.

	@publishedPartner
	@prototype
*/
struct SNThreadCreateInfo
	{
	NThreadFunction iFunction;
	TAny* iStackBase;
	TInt iStackSize;
	TInt iPriority;
	TInt iTimeslice;
	TUint8 iAttributes;
	TUint32 iCpuAffinity;
	const SNThreadHandlers* iHandlers;
	const SFastExecTable* iFastExecTable;
	const SSlowExecTable* iSlowExecTable;
	const TUint32* iParameterBlock;
	TInt iParameterBlockSize;		// if zero, iParameterBlock _is_ the initial data
									// otherwise it points to n bytes of initial data
	NThreadGroup* iGroup;			// NULL for lone thread
	};

/** Information needed for creating a nanothread group.

	@publishedPartner
	@prototype
*/
struct SNThreadGroupCreateInfo
	{
	TUint32 iCpuAffinity;
	TDfc* iDestructionDfc;
	};

/**	Constant for use with NKern:: functions which release a fast mutex as well
	as performing some other operations.

	@publishedPartner
	@released
*/
#define	SYSTEM_LOCK		(NFastMutex*)0

/** Idle handler function
	Pointer to a function which is called whenever a CPU goes idle

	@param	aPtr	The iPtr stored in the SCpuIdleHandler structure
	@param	aStage	Bits 0-7 give a bitmask of CPUs now active, i.e. 0 means all processors now idle
					Bit 31 set indicates that the current core can now be powered down
					Bit 30 set indicates that other cores still remain to be retired
					Bit 29 set indicates that postamble processing is required after waking up
	@param	aU		Points to some per-CPU uncached memory used for handshaking during power down/power up

	@internalComponent
*/
typedef void (*TCpuIdleHandlerFn)(TAny* aPtr, TUint32 aStage, volatile TAny* aU);

/** Idle handler structure

	@internalComponent
*/
struct SCpuIdleHandler
	{
	/**
	Defined flag bits in aStage parameter
	*/
	enum
		{
		EActiveCpuMask=0xFFu,
		EPostamble=1u<<29,		// postamble needed
		EMore=1u<<30,			// more cores still to be retired
		ERetire=1u<<31,			// this core can now be retired
		};

	TCpuIdleHandlerFn	iHandler;
	TAny*				iPtr;
	volatile TBool		iPostambleRequired;
	};


/**
@internalComponent
*/
enum TUserModeCallbackReason
	{
	EUserModeCallbackRun,
	EUserModeCallbackCancel,
	};


/**
A callback function executed when a thread returns to user mode.

@internalComponent
*/
typedef void (*TUserModeCallbackFunc)(TAny* aThisPtr, TUserModeCallbackReason aReasonCode);


/**
An object representing a queued callback to be executed when a thread returns to user mode.

@internalComponent
*/
class TUserModeCallback
	{
public:
	TUserModeCallback(TUserModeCallbackFunc);
	~TUserModeCallback();

public:
	TUserModeCallback* volatile iNext;
	TUserModeCallbackFunc iFunc;
	};

TUserModeCallback* const KUserModeCallbackUnqueued = ((TUserModeCallback*)1);


/** Main function for AP

@internalTechnology
*/
struct SAPBootInfo;
typedef void (*TAPBootFunc)(volatile SAPBootInfo*);


/** Information needed to boot an AP

@internalTechnology
*/
struct SAPBootInfo
	{
	TUint32				iCpu;				// Hardware CPU ID
	TUint32				iInitStackSize;		// Size of initial stack
	TLinAddr			iInitStackBase;		// Base of initial stack
	TAPBootFunc			iMain;				// Address of initial function to call
	TAny*				iArgs[4];
	};

typedef void (*NIsr)(TAny*);

/** Nanokernel functions

	@publishedPartner
	@prototype
*/
class NKern
	{
public:
	/** Bitmask values used when blocking a nanothread.
		@see NKern::Block()
	 */
	enum TBlockMode 
		{
		EEnterCS=1,		/**< Enter thread critical section before blocking */
		ERelease=2,		/**< Release specified fast mutex before blocking */
		EClaim=4,		/**< Re-acquire specified fast mutex when unblocked */
		EObstruct=8,	/**< Signifies obstruction of thread rather than lack of work to do */
		};

	/** Values that specify the context of the processor.
		@see NKern::CurrentContext()
	*/
	enum TContext
		{
		EThread=0,			/**< The processor is in a thread context*/
		EIDFC=1,			/**< The processor is in an IDFC context*/
		EInterrupt=2,		/**< The processor is in an interrupt context*/
		EEscaped=KMaxTInt	/**< Not valid a process context on target hardware*/
		};

public:
	// Threads
	IMPORT_C static TInt ThreadCreate(NThread* aThread, SNThreadCreateInfo& aInfo);
	IMPORT_C static TBool ThreadSuspend(NThread* aThread, TInt aCount);
	IMPORT_C static TBool ThreadResume(NThread* aThread);
	IMPORT_C static TBool ThreadResume(NThread* aThread, NFastMutex* aMutex);
	IMPORT_C static TBool ThreadForceResume(NThread* aThread);
	IMPORT_C static TBool ThreadForceResume(NThread* aThread, NFastMutex* aMutex);
	IMPORT_C static void ThreadRelease(NThread* aThread, TInt aReturnValue);
	IMPORT_C static void ThreadRelease(NThread* aThread, TInt aReturnValue, NFastMutex* aMutex);
	IMPORT_C static void ThreadSetPriority(NThread* aThread, TInt aPriority);
	IMPORT_C static void ThreadSetPriority(NThread* aThread, TInt aPriority, NFastMutex* aMutex);
	static void ThreadSetNominalPriority(NThread* aThread, TInt aPriority);
	IMPORT_C static void ThreadRequestSignal(NThread* aThread);
	IMPORT_C static void ThreadRequestSignal(NThread* aThread, NFastMutex* aMutex);
	IMPORT_C static void ThreadRequestSignal(NThread* aThread, TInt aCount);
	IMPORT_C static void ThreadKill(NThread* aThread);
	IMPORT_C static void ThreadKill(NThread* aThread, NFastMutex* aMutex);
	IMPORT_C static void ThreadEnterCS();
	IMPORT_C static void ThreadLeaveCS();
	static NThread* _ThreadEnterCS();		/**< @internalComponent */
	static void _ThreadLeaveCS();			/**< @internalComponent */
	IMPORT_C static TInt Block(TUint32 aTimeout, TUint aMode, NFastMutex* aMutex);
	IMPORT_C static TInt Block(TUint32 aTimeout, TUint aMode);
	IMPORT_C static void NanoBlock(TUint32 aTimeout, TUint aState, TAny* aWaitObj);
	IMPORT_C static void ThreadGetUserContext(NThread* aThread, TAny* aContext, TUint32& aAvailRegistersMask);
	IMPORT_C static void ThreadSetUserContext(NThread* aThread, TAny* aContext);
	IMPORT_C static void ThreadGetSystemContext(NThread* aThread, TAny* aContext, TUint32& aAvailRegistersMask);
	static void ThreadModifyUsp(NThread* aThread, TLinAddr aUsp);
	IMPORT_C static TInt FreezeCpu();													/**< @internalComponent */
	IMPORT_C static void EndFreezeCpu(TInt aCookie);									/**< @internalComponent */
	IMPORT_C static TUint32 ThreadSetCpuAffinity(NThread* aThread, TUint32 aAffinity);	/**< @internalComponent */
	IMPORT_C static void ThreadSetTimeslice(NThread* aThread, TInt aTimeslice);			/**< @internalComponent */
	IMPORT_C static TUint64 ThreadCpuTime(NThread* aThread);							/**< @internalComponent */
	IMPORT_C static TUint32 CpuTimeMeasFreq();											/**< @internalComponent */
	static TInt QueueUserModeCallback(NThreadBase* aThread, TUserModeCallback* aCallback);	/**< @internalComponent */
	static void MoveUserModeCallbacks(NThreadBase* aSrcThread, NThreadBase* aDestThread);	/**< @internalComponent */
	static void CancelUserModeCallbacks();												/**< @internalComponent */
	static void JumpTo(TInt aCpu);														/**< @internalComponent */

	// Thread Groups
	IMPORT_C static TInt GroupCreate(NThreadGroup* aGroup, SNThreadGroupCreateInfo& aInfo);
	IMPORT_C static void GroupDestroy(NThreadGroup* aGroup);
	IMPORT_C static NThreadGroup* CurrentGroup();
	IMPORT_C static NThreadGroup* LeaveGroup();
	IMPORT_C static void JoinGroup(NThreadGroup* aGroup);
	IMPORT_C static TUint32 GroupSetCpuAffinity(NThreadGroup* aGroup, TUint32 aAffinity);

	// Fast semaphores
	IMPORT_C static void FSSetOwner(NFastSemaphore* aSem,NThreadBase* aThread);
	IMPORT_C static void FSWait(NFastSemaphore* aSem);
	IMPORT_C static void FSSignal(NFastSemaphore* aSem);
	IMPORT_C static void FSSignal(NFastSemaphore* aSem, NFastMutex* aMutex);
	IMPORT_C static void FSSignalN(NFastSemaphore* aSem, TInt aCount);
	IMPORT_C static void FSSignalN(NFastSemaphore* aSem, TInt aCount, NFastMutex* aMutex);

	// Fast mutexes
	IMPORT_C static void FMWait(NFastMutex* aMutex);
	IMPORT_C static void FMSignal(NFastMutex* aMutex);
	IMPORT_C static TBool FMFlash(NFastMutex* aMutex);

	// Scheduler
	IMPORT_C static void Lock();
	IMPORT_C static NThread* LockC();
	IMPORT_C static void Unlock();
	IMPORT_C static TInt PreemptionPoint();

	// Interrupts
	IMPORT_C static TInt DisableAllInterrupts();
	IMPORT_C static TInt DisableInterrupts(TInt aLevel);
	IMPORT_C static void RestoreInterrupts(TInt aRestoreData);
	IMPORT_C static void EnableAllInterrupts();

	// Read-modify-write
	inline static TInt LockedInc(TInt& aCount)
		{ return __e32_atomic_add_ord32(&aCount,1); }
	inline static TInt LockedDec(TInt& aCount)
		{ return __e32_atomic_add_ord32(&aCount,0xffffffff); }
	inline static TInt LockedAdd(TInt& aDest, TInt aSrc)
		{ return __e32_atomic_add_ord32(&aDest,aSrc); }
	inline static TInt64 LockedInc(TInt64& aCount)
		{ return __e32_atomic_add_ord64(&aCount,1); }
	inline static TInt64 LockedDec(TInt64& aCount)
		{ return __e32_atomic_add_ord64(&aCount,TUint64(TInt64(-1))); }
	inline static TInt64 LockedAdd(TInt64& aDest, TInt64 aSrc)		/**< @internalComponent */
		{ return __e32_atomic_add_ord64(&aDest,aSrc); }
	inline static TUint32 LockedSetClear(TUint32& aDest, TUint32 aClearMask, TUint32 aSetMask)
		{ return __e32_atomic_axo_ord32(&aDest,~(aClearMask|aSetMask),aSetMask); }
	inline static TUint16 LockedSetClear16(TUint16& aDest, TUint16 aClearMask, TUint16 aSetMask)	/**< @internalComponent */
		{ return __e32_atomic_axo_ord16(&aDest,TUint16(~(aClearMask|aSetMask)),aSetMask); }
	inline static TUint8 LockedSetClear8(TUint8& aDest, TUint8 aClearMask, TUint8 aSetMask)
		{ return __e32_atomic_axo_ord8(&aDest,TUint8(~(aClearMask|aSetMask)),aSetMask); }
	inline static TInt SafeInc(TInt& aCount)
		{ return __e32_atomic_tas_ord32(&aCount,1,1,0); }
	inline static TInt SafeDec(TInt& aCount)
		{ return __e32_atomic_tas_ord32(&aCount,1,-1,0); }
	inline static TInt AddIfGe(TInt& aCount, TInt aLimit, TInt aInc)	/**< @internalComponent */
		{ return __e32_atomic_tas_ord32(&aCount,aLimit,aInc,0); }
	inline static TInt AddIfLt(TInt& aCount, TInt aLimit, TInt aInc)	/**< @internalComponent */
		{ return __e32_atomic_tas_ord32(&aCount,aLimit,0,aInc); }
	inline static TAny* SafeSwap(TAny* aNewValue, TAny*& aPtr)
		{ return __e32_atomic_swp_ord_ptr(&aPtr, aNewValue); }
	inline static TUint8 SafeSwap8(TUint8 aNewValue, TUint8& aPtr)
		{ return __e32_atomic_swp_ord8(&aPtr, aNewValue); }
	inline static TUint16 SafeSwap16(TUint16 aNewValue, TUint16& aPtr)						/**< @internalComponent */
		{ return __e32_atomic_swp_ord16(&aPtr, aNewValue); }
	inline static TBool CompareAndSwap(TAny*& aPtr, TAny* aExpected, TAny* aNew)			/**< @internalComponent */
		{ return __e32_atomic_cas_ord_ptr(&aPtr, &aExpected, aNew); }
	inline static TBool CompareAndSwap8(TUint8& aPtr, TUint8 aExpected, TUint8 aNew)		/**< @internalComponent */
		{ return __e32_atomic_cas_ord8(&aPtr, (TUint8*)&aExpected, (TUint8)aNew); }
	inline static TBool CompareAndSwap16(TUint16& aPtr, TUint16 aExpected, TUint16 aNew)	/**< @internalComponent */
		{ return __e32_atomic_cas_ord16(&aPtr, (TUint16*)&aExpected, (TUint16)aNew); }
	inline static TUint32 SafeSwap(TUint32 aNewValue, TUint32& aPtr)						/**< @internalComponent */
		{ return __e32_atomic_swp_ord32(&aPtr, aNewValue); }
	inline static TUint SafeSwap(TUint aNewValue, TUint& aPtr)								/**< @internalComponent */
		{ return __e32_atomic_swp_ord32(&aPtr, aNewValue); }
	inline static TInt SafeSwap(TInt aNewValue, TInt& aPtr)									/**< @internalComponent */
		{ return __e32_atomic_swp_ord32(&aPtr, aNewValue); }
	inline static TBool CompareAndSwap(TUint32& aPtr, TUint32 aExpected, TUint32 aNew)		/**< @internalComponent */
		{ return __e32_atomic_cas_ord32(&aPtr, &aExpected, aNew); }
	inline static TBool CompareAndSwap(TUint& aPtr, TUint aExpected, TUint aNew)			/**< @internalComponent */
		{ return __e32_atomic_cas_ord32(&aPtr, (TUint32*)&aExpected, (TUint32)aNew); }
	inline static TBool CompareAndSwap(TInt& aPtr, TInt aExpected, TInt aNew)				/**< @internalComponent */
		{ return __e32_atomic_cas_ord32(&aPtr, (TUint32*)&aExpected, (TUint32)aNew); }


	// Miscellaneous
	IMPORT_C static NThread* CurrentThread();
	IMPORT_C static TInt CurrentCpu();										/**< @internalComponent */
	IMPORT_C static TInt NumberOfCpus();									/**< @internalComponent */
	IMPORT_C static void SetNumberOfActiveCpus(TInt aNumber);
	IMPORT_C static void LockSystem();
	IMPORT_C static void UnlockSystem();
	IMPORT_C static TBool FlashSystem();
	IMPORT_C static void WaitForAnyRequest();
	IMPORT_C static void Sleep(TUint32 aTime);
	IMPORT_C static void Exit();
	IMPORT_C static void DeferredExit();
	IMPORT_C static void YieldTimeslice();									/**< @internalComponent */
	IMPORT_C static void RotateReadyList(TInt aPriority);					
	IMPORT_C static void RotateReadyList(TInt aPriority, TInt aCpu);		/**< @internalTechnology */
	IMPORT_C static void RecordIntLatency(TInt aLatency, TInt aIntMask);	/**< @internalTechnology */
	IMPORT_C static void RecordThreadLatency(TInt aLatency);				/**< @internalTechnology */
	IMPORT_C static TUint32 TickCount();
	IMPORT_C static TInt TickPeriod();
	IMPORT_C static TInt TimerTicks(TInt aMilliseconds);
	IMPORT_C static TInt TimesliceTicks(TUint32 aMicroseconds);				/**< @internalTechnology */
	IMPORT_C static TInt CurrentContext();
	IMPORT_C static TUint32 FastCounter();
	IMPORT_C static TInt FastCounterFrequency();
	IMPORT_C static TUint64 Timestamp();
	IMPORT_C static TUint32 TimestampFrequency();
	static void Init0(TAny* aVariantData);
	static void Init(NThread* aThread, SNThreadCreateInfo& aInfo);
	static TInt BootAP(volatile SAPBootInfo* aInfo);
	IMPORT_C static TBool KernelLocked(TInt aCount=0);						/**< @internalTechnology */
	IMPORT_C static NFastMutex* HeldFastMutex();							/**< @internalTechnology */
	static void Idle();
	static void DoIdle();
	IMPORT_C static SCpuIdleHandler* CpuIdleHandler();						/**< @internalTechnology */
	static void NotifyCrash(const TAny* a0, TInt a1);						/**< @internalTechnology */
	IMPORT_C static TBool Crashed();
	static TUint32 IdleGenerationCount();

	// Debugger support
	typedef void (*TRescheduleCallback)(NThread*);
	IMPORT_C static void SchedulerHooks(TLinAddr& aStart, TLinAddr& aEnd);
	IMPORT_C static void InsertSchedulerHooks();
	IMPORT_C static void RemoveSchedulerHooks();
	IMPORT_C static void SetRescheduleCallback(TRescheduleCallback aCallback);

	// Interrupts
	enum TIrqInitFlags
		{
		EIrqInit_FallingEdge=0,
		EIrqInit_RisingEdge=2,
		EIrqInit_LevelLow=1,
		EIrqInit_LevelHigh=3,
		EIrqInit_Shared=0x10,
		EIrqInit_Count=0x20,
		};

	enum TIrqBindFlags
		{
		EIrqBind_Raw=1,
		EIrqBind_Count=2,
		EIrqBind_Exclusive=4,
		EIrqBind_Tied=8
		};

	enum TIrqIdBits
		{
		EIrqIndexMask = 0x0000ffff,	// bottom 16 bits is IRQ number if top 16 bits all zero
									// otherwise is IRQ handler index
		EIrqCookieMask = 0x7fff0000,
		EIrqCookieShift = 16
		};

	static void InterruptInit0();
	IMPORT_C static TInt InterruptInit(TInt aId, TUint32 aFlags, TInt aVector, TUint32 aHwId, TAny* aExt=0);
	IMPORT_C static TInt InterruptBind(TInt aId, NIsr aIsr, TAny* aPtr, TUint32 aFlags, NSchedulable* aTied);
	IMPORT_C static TInt InterruptUnbind(TInt aId);
	IMPORT_C static TInt InterruptEnable(TInt aId);
	IMPORT_C static TInt InterruptDisable(TInt aId);
	IMPORT_C static TInt InterruptClear(TInt aId);
	IMPORT_C static TInt InterruptSetPriority(TInt aId, TInt aPri);
	IMPORT_C static TInt InterruptSetCpuMask(TInt aId, TUint32 aMask);
	IMPORT_C static void Interrupt(TInt aIrqNo);
	};


/** Create a fast semaphore

	@publishedPartner
	@prototype
*/
inline NFastSemaphore::NFastSemaphore(NThreadBase* aThread)
	:	iCount(0),
		iOwningThread(aThread ? aThread : (NThreadBase*)NKern::CurrentThread())
	{
	}


class TGenericIPI;

/**
@internalComponent
*/
typedef void (*TGenericIPIFn)(TGenericIPI*);

/**
@internalComponent
*/
class TGenericIPI : public SDblQueLink
	{
public:
	void Queue(TGenericIPIFn aFunc, TUint32 aCpuMask);
	void QueueAll(TGenericIPIFn aFunc);
	void QueueAllOther(TGenericIPIFn aFunc);
	void WaitEntry();
	void WaitCompletion();
public:
	TGenericIPIFn			iFunc;
	volatile TUint32		iCpusIn;
	volatile TUint32		iCpusOut;
	};

/**
@internalComponent
*/
class TStopIPI : public TGenericIPI
	{
public:
	TUint32 StopCPUs();
	void ReleaseCPUs();
	static void Isr(TGenericIPI*);
public:
	volatile TInt iFlag;
	};


/**
@internalComponent
*/
class TCoreCycler
	{
public:
	TCoreCycler();
	TInt Next();
private:
	void Init();
private:
	TUint32			iCores;
	TUint32			iRemain;
	TInt			iInitialCpu;
	TInt			iCurrentCpu;
	NThreadGroup*	iG;
	TInt			iFrz;
	};



#include <ncern.h>
#endif
