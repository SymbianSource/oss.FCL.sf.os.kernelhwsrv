// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\nkern\nkern.h
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

class NThread;


/** Spin lock

	Used for protecting a code fragment against both interrupts and concurrent
	execution on another processor.
	
	@internalComponent
*/
class TSpinLock
	{
public:
	enum TOrder
		{
		// Bit 7 of order clear for locks used with interrupts disabled
		EOrderGenericIrqLow0	=0x00u,	// Device driver spin locks, low range
		EOrderGenericIrqLow1	=0x01u,	// Device driver spin locks, low range
		EOrderGenericIrqLow2	=0x02u,	// Device driver spin locks, low range
		EOrderGenericIrqLow3	=0x03u,	// Device driver spin locks, low range
		EOrderGenericIrqHigh0	=0x18u,	// Device driver spin locks, high range
		EOrderGenericIrqHigh1	=0x19u,	// Device driver spin locks, high range
		EOrderGenericIrqHigh2	=0x1Au,	// Device driver spin locks, high range
		EOrderGenericIrqHigh3	=0x1Bu,	// Device driver spin locks, high range

		// Bit 7 of order set for locks used with interrupts enabled, preemption disabled
		EOrderGenericPreLow0	=0x80u,		// Device driver spin locks, low range
		EOrderGenericPreLow1	=0x81u,		// Device driver spin locks, low range
		EOrderGenericPreHigh0	=0x9Eu,		// Device driver spin locks, high range
		EOrderGenericPreHigh1	=0x9Fu,		// Device driver spin locks, high range

		EOrderNone				=0xFFu	// No order check required (e.g. for dynamic ordering)
		};
public:
	IMPORT_C TSpinLock(TUint aOrder);
private:
	volatile TUint64 iLock;
	};

/** Macro to disable interrupts and acquire the lock.

@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQ(lock)					((void)NKern::DisableAllInterrupts())

/** Macro to release the lock and enable interrupts.

@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQ(lock)					(NKern::EnableAllInterrupts())

/** Macro to see if someone else is waiting for the lock, enabling IRQs 
    then disabling IRQs again.

@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQ(lock)					(NKern::EnableAllInterrupts(),(void)NKern::DisableAllInterrupts(),((TBool)TRUE))

/** Macro to remember original interrupt state then disable interrupts 
    and acquire the lock.
    
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQSAVE(lock)				(NKern::DisableAllInterrupts())

/** Macro to release the lock then restore original interrupt state to that 
	supplied.
	
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQRESTORE(lock,irq)		(NKern::RestoreInterrupts(irq))

/** Macro to see if someone else is waiting for the lock, enabling IRQs to
	the original state supplied then disabling IRQs again.
    
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQRESTORE(lock,irq)		(NKern::RestoreInterrupts(irq),((void)NKern::DisableAllInterrupts()),((TBool)TRUE))

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
#define __SPIN_LOCK(lock)

/** Macro to release the lock, don't change interrupt/preemption state.

@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK(lock)

/**
@internalComponent
*/
#define __SPIN_FLASH(lock)						((TBool)FALSE)

/** Macro to see if someone else is waiting for the lock, enabling preemption 
    then disabling it again.

@publishedPartner
@prototype
*/
#define __SPIN_FLASH_PREEMPT(lock)				((TBool)NKern::PreemptionPoint())


/** Read/Write Spin lock

	@internalComponent
*/
class TRWSpinLock
	{
public:
	IMPORT_C TRWSpinLock(TUint aOrder);		// Uses same order space as TSpinLock
private:
	volatile TUint64 iLock;
	};


/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQ_R(lock)					((void)NKern::DisableAllInterrupts())

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQ_R(lock)				(NKern::EnableAllInterrupts())

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQ_R(lock)				(NKern::EnableAllInterrupts(),(void)NKern::DisableAllInterrupts(),((TBool)TRUE))

/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQ_W(lock)					((void)NKern::DisableAllInterrupts())

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQ_W(lock)				(NKern::EnableAllInterrupts())

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQ_W(lock)				(NKern::EnableAllInterrupts(),(void)NKern::DisableAllInterrupts(),((TBool)TRUE))


/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_R(lock)						

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_R(lock)					

/**
@internalComponent
*/
#define __SPIN_FLASH_R(lock)					((TBool)FALSE)

/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_W(lock)						

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_W(lock)					

/**
@internalComponent
*/
#define __SPIN_FLASH_W(lock)					((TBool)FALSE)


/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQSAVE_R(lock)				(NKern::DisableAllInterrupts())

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQRESTORE_R(lock,irq)	(NKern::RestoreInterrupts(irq))

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQRESTORE_R(lock,irq)		(NKern::RestoreInterrupts(irq),((void)NKern::DisableAllInterrupts()),((TBool)TRUE))

/**
@publishedPartner
@prototype
*/
#define __SPIN_LOCK_IRQSAVE_W(lock)				(NKern::DisableAllInterrupts())

/**
@publishedPartner
@prototype
*/
#define __SPIN_UNLOCK_IRQRESTORE_W(lock,irq)	(NKern::RestoreInterrupts(irq))

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_IRQRESTORE_W(lock,irq)		(NKern::RestoreInterrupts(irq),((void)NKern::DisableAllInterrupts()),((TBool)TRUE))


/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_PREEMPT_R(lock)			((TBool)NKern::PreemptionPoint())

/**
@publishedPartner
@prototype
*/
#define __SPIN_FLASH_PREEMPT_W(lock)			((TBool)NKern::PreemptionPoint())


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
	@released
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
public:
	TInt iCount;				/**< @internalComponent */

	/** The thread allowed to wait on the semaphore
		@internalComponent
	*/
	NThreadBase* iOwningThread;	
	};

/** Create a fast semaphore

	@publishedPartner
	@released
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
	@released
*/
class NFastMutex
	{
public:
	IMPORT_C NFastMutex();
	IMPORT_C void Wait();
	IMPORT_C void Signal();
	IMPORT_C TBool HeldByCurrentThread();	/**< @internalComponent */
public:
	NThreadBase* iHoldingThread;	/**< @internalComponent */

	/** MUST ALWAYS BE 0 or 1
		@internalComponent
	*/
	TInt iWaiting;
	};


/**
@publishedPartner
@released

The type of the callback function used by the nanokernel timer. 

@see NTimer
*/
typedef void (*NTimerFn)(TAny*);




/**
@publishedPartner
@released

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
class NTimer : public SDblQueLink
	{
public:
	/**
	Default constructor.
	*/
	inline NTimer()
		: iState(EIdle)
		{}
	/**
	Constructor taking a callback function and a pointer to be passed
	to the callback function.
	
	@param aFunction The callback function.
	@param aPtr      A pointer to be passed to the callback function 
	                 when called.
	*/
	inline NTimer(NTimerFn aFunction, TAny* aPtr)
		: iPtr(aPtr), iFunction(aFunction), iState(EIdle)
		{}
	IMPORT_C TInt OneShot(TInt aTime);
	IMPORT_C TInt OneShot(TInt aTime, TBool aDfc);
	IMPORT_C TInt OneShot(TInt aTime, TDfc& aDfc);
	IMPORT_C TInt Again(TInt aTime);
	IMPORT_C TBool Cancel();
	IMPORT_C TBool IsPending();
public:
/**
	@internalComponent
*/
	enum TState
		{
		EIdle=0,			// not queued
		ETransferring=1,	// being transferred from holding to ordered queue
		EHolding=2,			// on holding queue
		EOrdered=3,			// on ordered queue
		ECritical=4,		// on ordered queue and in use by queue walk routine
		EFinal=5,			// on final queue
		};
public:
	/** Argument for callback function or the pointer to TDfc */
	TAny* iPtr;				/**< @internalComponent */

	/** Pointer to callback function. NULL value indicates that queuing of provided Dfc queue will be done
	instead of calling callback function on completion */
	NTimerFn iFunction;		/**< @internalComponent */

	TUint32 iTriggerTime;	/**< @internalComponent */
	TUint8 iCompleteInDfc;	/**< @internalComponent */
	TUint8 iState;			/**< @internalComponent */
	TUint8 iPad1;			/**< @internalComponent */

	/** Available for timer client to use.
		@internalTechnology */
	TUint8 iUserFlags;
	};

/**
@internalTechnology
*/
#define	i_NTimer_iUserFlags	iUserFlags

/**
@internalComponent
*/
#define	i_NTimer_iState		iState

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
	@released
*/
typedef void (*NThreadStateHandler)(NThread*,TInt,TInt);

/**
	@publishedPartner
	@released
*/
typedef void (*NThreadExceptionHandler)(TAny*,NThread*);

/**
	@publishedPartner
	@released
*/
typedef void (*NThreadTimeoutHandler)(NThread*,TInt);

/**
	@publishedPartner
	@released
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
	@released
*/
struct SFastExecTable
	{
	TInt iFastExecCount;			// includes implicit function#0
	TLinAddr iFunction[1];			// first entry is for call number 1
	};

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagClaim=0x80000000;		// claim system lock

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagRelease=0x40000000;		// release system lock

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagPreprocess=0x20000000;	// preprocess

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgMask=0x1C000000;	// 3 bits indicating additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs2=0x04000000;	// 2 additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs3=0x08000000;	// 3 additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs4=0x0C000000;	// 4 additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs5=0x10000000;	// 5 additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs6=0x14000000;	// 6 additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs7=0x18000000;	// 7 additional arguments

/**
	@publishedPartner
	@released
*/
const TUint32 KExecFlagExtraArgs8=0x1C000000;	// 8 additional arguments


/**
	@publishedPartner
	@released
*/
struct SSlowExecEntry
	{
	TUint32 iFlags;					// information about call
	TLinAddr iFunction;				// address of function to be called
	};


/**
	@publishedPartner
	@released
*/
struct SSlowExecTable
	{
	TInt iSlowExecCount;
	TLinAddr iInvalidExecHandler;	// used if call number invalid
	TLinAddr iPreprocessHandler;	// used for handle lookups
	SSlowExecEntry iEntries[1];		// first entry is for call number 0
	};

// Thread iAttributes Constants
const TUint8 KThreadAttImplicitSystemLock=1;	/**< @internalComponent */
const TUint8 KThreadAttAddressSpace=2;			/**< @internalComponent */
const TUint8 KThreadAttLoggable=4;				/**< @internalComponent */
const TUint8 KThreadAttDelayed=8;				/**< @internalComponent */


// Thread CPU
const TUint32 KCpuAffinityAny=0xffffffffu;		/**< @internalComponent */

/** Information needed for creating a nanothread.

	@publishedPartner
	@released
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

	@internalComponent
*/
typedef void (*TCpuIdleHandlerFn)(TAny* aPtr, TUint32 aStage);

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


/** Nanokernel functions

	@publishedPartner
	@released
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
	IMPORT_C static TInt ThreadCreate(NThread* aThread, SNThreadCreateInfo& anInfo);
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
	static void Init0(TAny* aVariantData);
	static void Init(NThread* aThread, SNThreadCreateInfo& anInfo);
	IMPORT_C static TBool KernelLocked(TInt aCount=0);						/**< @internalTechnology */
	IMPORT_C static NFastMutex* HeldFastMutex();							/**< @internalTechnology */
	static void Idle();	
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
	};


/** Create a fast semaphore

	@publishedPartner
	@released
*/
inline NFastSemaphore::NFastSemaphore(NThreadBase* aThread)
	:	iCount(0),
		iOwningThread(aThread ? aThread : (NThreadBase*)NKern::CurrentThread())
	{
	}


#endif
