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
// e32\include\nkernsmp\nk_priv.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __NK_PRIV_H__
#define __NK_PRIV_H__
#include <cpudefs.h>
#include <nkern.h>

#define __USE_BTRACE_LOCK__

class Monitor;

/********************************************
 * Schedulable = thread or thread group
 ********************************************/

/**
@publishedPartner
@prototype

Base class for a nanokernel thread or group
*/
class NThreadGroup;
class NSchedulable : public TPriListLink
	{
public:
	enum
		{
		EReadyGroup=1,
		EReadyCpuMask=0x7f,
		EReadyOffset=0x80,
		};

	enum NReadyFlags
		{
		ENewTimeslice=1,
		EPreferSameCpu=2,
		EUnPause=4,
		};

	enum NEventState
		{
		EEventCountShift=16u,
		EEventCountMask=0xffff0000u,
		EEventCountInc=0x10000u,
		EEventCpuShift=0u,
		EEventCpuMask=0x1fu,
		EThreadCpuShift=8u,
		EThreadCpuMask=0x1f00u,
		EDeferredReady=0x4000u,
		EEventParent=0x8000u,
		};
public:
	NSchedulable();
	void AcqSLock();
	void RelSLock();
	void LAcqSLock();
	void RelSLockU();
	void ReadyT(TUint aMode);					// make ready, assumes lock held
	TInt BeginTiedEvent();
	void EndTiedEvent();
	TInt AddTiedEvent(NEventHandler* aEvent);
	TBool TiedEventReadyInterlock(TInt aCpu);
	void UnPauseT();							// decrement pause count and make ready if necessary
	static void DeferredReadyIDfcFn(TAny*);
	void DetachTiedEvents();
public:
	inline TBool IsGroup()			{return !iParent;}
	inline TBool IsLoneThread()		{return iParent==this;}
	inline TBool IsGroupThread()	{return iParent && iParent!=this;}
public:
//	TUint8				iReady;					/**< @internalComponent */	// flag indicating thread on ready list = cpu number | EReadyOffset
//	TUint8				iCurrent;				/**< @internalComponent */	// flag indicating thread is running
//	TUint8				iLastCpu;				/**< @internalComponent */	// CPU on which this thread last ran
	TUint8				iPauseCount;			/**< @internalComponent */	// count of externally requested pauses extending a voluntary wait
	TUint8				iSuspended;				/**< @internalComponent */	// flag indicating active external suspend (Not used for groups)
	TUint8				iNSchedulableSpare1;	/**< @internalComponent */
	TUint8				iNSchedulableSpare2;	/**< @internalComponent */

	TUint8				iCpuChange;				/**< @internalComponent */	// flag showing CPU migration outstanding
	TUint8				iStopping;				/**< @internalComponent */	// thread is exiting, thread group is being destroyed
	TUint16				iFreezeCpu;				/**< @internalComponent */	// flag set if CPU frozen - count for groups
	NSchedulable*		iParent;				/**< @internalComponent */	// Pointer to group containing thread, =this for normal thread, =0 for group

	TUint32				iCpuAffinity;			/**< @internalComponent */
	volatile TUint32	iEventState;			/**< @internalComponent */	// bits 16-31=count, 0-4=event CPU, 5-9=thread CPU, 10=defer, 11=parent

	TSpinLock			iSSpinLock;				/**< @internalComponent */

	SDblQue				iEvents;				/**< @internalComponent */	// doubly-linked list of tied events

	TUint32				i_IDfcMem[sizeof(TDfc)/sizeof(TUint32)];	/**< @internalComponent */	// IDFC used to make thread ready after last tied event completes
//	TDfc				iDeferredReadyIDfc;		/**< @internalComponent */	// IDFC used to make thread ready after last tied event completes

	union
		{
		TUint64			iRunCount64;
		TUint32			iRunCount32[2];
		};
	union
		{
		TUint64			iTotalCpuTime64;		/**< @internalComponent */	// total time spent running, in hi-res timer ticks
		TUint32			iTotalCpuTime32[2];		/**< @internalComponent */	// total time spent running, in hi-res timer ticks
		};
	};

__ASSERT_COMPILE(!(_FOFF(NSchedulable,iSSpinLock)&7));
__ASSERT_COMPILE(!(_FOFF(NSchedulable,iRunCount64)&7));
__ASSERT_COMPILE(!(_FOFF(NSchedulable,iTotalCpuTime64)&7));
__ASSERT_COMPILE(!(sizeof(NSchedulable)&7));


/**
@internalComponent
*/
inline TBool TDfc::IsValid()
	{
	if (iHType < KNumDfcPriorities)
		return TRUE;
	if (iHType != EEventHandlerIDFC)
		return FALSE;
	return !iTied || !iTied->iStopping;
	}

/********************************************
 * Thread
 ********************************************/

/**
@internalComponent
*/
class NThreadWaitState
	{
private:
	enum TWtStFlags
		{
		EWtStWaitPending		=0x01u,		// thread is about to wait
		EWtStWaitActive			=0x02u,		// thread is actually blocked
		EWtStTimeout			=0x04u,		// timeout is active on this wait
		EWtStObstructed			=0x08u,		// wait is due to obstruction (e.g. mutex) rather than lack of work to do
		EWtStDead				=0x80u,		// thread is dead
		};
private:
	NThreadWaitState();
	void SetUpWait(TUint aType, TUint aFlags, TAny* aWaitObj);
	void SetUpWait(TUint aType, TUint aFlags, TAny* aWaitObj, TUint32 aTimeout);
	void SetDead(TDfc* aKillDfc);
	void CancelWait();
	TInt DoWait();
	static void TimerExpired(TAny*);
	TInt UnBlockT(TUint aType, TAny* aWaitObj, TInt aReturnValue);
	TUint32 ReleaseT(TAny*& aWaitObj, TInt aReturnValue);
	void CancelTimerT();
private:
	inline NThreadBase* Thread();
	inline TBool WaitPending()
		{ return iWtC.iWtStFlags & (EWtStWaitPending|EWtStDead); }
	inline TBool ThreadIsBlocked()
		{ return iWtC.iWtStFlags & (EWtStWaitActive|EWtStDead); }
	inline TBool ThreadIsDead()
		{ return iWtC.iWtStFlags & EWtStDead; }
private:
	struct S
		{
		volatile TUint8			iWtStFlags;
		volatile TUint8			iWtObjType;
		volatile TUint8			iWtStSpare1;
		volatile TUint8			iWtStSpare2;
		union
			{
			TAny* volatile		iWtObj;
			volatile TInt		iRetVal;
			TDfc* volatile		iKillDfc;
			};
		};
	union
		{
		S						iWtC;
		volatile TUint32		iWtSt32[2];
		volatile TUint64		iWtSt64;
		};
	NTimer						iTimer;
private:
	friend class NSchedulable;
	friend class NThreadBase;
	friend class NThread;
	friend class TScheduler;
	friend class TSubScheduler;
	friend class TDfc;
	friend class TDfcQue;
	friend class NFastSemaphore;
	friend class NFastMutex;
	friend class NTimer;
	friend class NTimerQ;
	friend class NKern;
	friend class Monitor;
	friend class NKTest;
	};

/**
@publishedPartner
@prototype

Base class for a nanokernel thread.
*/
class TSubScheduler;
class NThreadBase : public NSchedulable
	{
public:
    /**
    Defines the possible types of wait object
    */
	enum NThreadWaitType
		{
		EWaitNone,
		EWaitFastSemaphore,
		EWaitFastMutex,
		EWaitSleep,
		EWaitBlocked,
		EWaitDfc,
		
		ENumWaitTypes
		};

		
	/**
	@internalComponent
	*/
	enum NThreadCSFunction
		{
		ECSExitPending=-1,
		ECSExitInProgress=-2,
		ECSDivertPending=-3,
		};

	/**
	@internalComponent
	*/
	enum NThreadTimeoutOp
		{
		ETimeoutPreamble=0,
		ETimeoutPostamble=1,
		ETimeoutSpurious=2,
		};
public:
	NThreadBase();
	TInt Create(SNThreadCreateInfo& anInfo,	TBool aInitial);
	void UnReadyT();
	TBool SuspendOrKill(TInt aCount);
	TBool DoSuspendOrKillT(TInt aCount, TSubScheduler* aS);
	TBool CancelTimerT();
	void DoReleaseT(TInt aReturnCode, TUint aMode);
	TBool CheckFastMutexDefer();
	void DoCsFunctionT();
	TBool Resume(TBool aForce);
	IMPORT_C TBool Suspend(TInt aCount);		/**< @internalComponent */
	IMPORT_C TBool Resume();					/**< @internalComponent */
	IMPORT_C TBool ForceResume();				/**< @internalComponent */
	IMPORT_C void Release(TInt aReturnCode, TUint aMode);	/**< @internalComponent */
	IMPORT_C void RequestSignal();				/**< @internalComponent */
	IMPORT_C void SetPriority(TInt aPriority);	/**< @internalComponent */
	void SetMutexPriority(NFastMutex* aMutex);
	void LoseInheritedPriorityT();
	void ChangeReadyThreadPriority();
	TUint32 SetCpuAffinity(TUint32 aAffinity);
	TBool TiedEventLeaveInterlock();
	TBool TiedEventJoinInterlock();
	IMPORT_C void Kill();						/**< @internalComponent */
	void Exit();
	// hooks for platform-specific code
	void OnKill(); 
	void OnExit();
public:
	static void TimerExpired(TAny* aPtr);

	/** @internalComponent */
	inline void UnknownState(TInt aOp, TInt aParam)
		{ (*iHandlers->iStateHandler)((NThread*)this,aOp,aParam); }

	/** @internalComponent */
	inline TUint8 Attributes()
		{ return i_ThrdAttr; }

	/** @internalComponent */
	inline TUint8 SetAttributes(TUint8 aNewAtt)
		{ return __e32_atomic_swp_ord8(&i_ThrdAttr, aNewAtt); }

	/** @internalComponent */
	inline TUint8 ModifyAttributes(TUint8 aClearMask, TUint8 aSetMask)
		{ return __e32_atomic_axo_ord8(&i_ThrdAttr, (TUint8)~(aClearMask|aSetMask), aSetMask); }

	/** @internalComponent */
	inline void SetAddressSpace(TAny* a)
		{ iAddressSpace=a; }

	/** @internalComponent */
	inline void SetExtraContext(TAny* a, TInt aSize)
		{ iExtraContext = a; iExtraContextSize = aSize; }

	/** @internalTechnology */
	inline TBool IsDead()
		{ return iWaitState.ThreadIsDead(); }
public:
	TPriListLink		iWaitLink;				/**< @internalComponent */	// used to link thread into a wait queue
//	TUint8				iBasePri;				/**< @internalComponent */	// priority with no fast mutex held
//	TUint8				iMutexPri;				/**< @internalComponent */	// priority from held fast mutex
//	TUint8				iInitial;				/**< @internalComponent */	// TRUE if this is an initial thread
	TUint8				iLinkedObjType;
	TUint8				i_ThrdAttr;				/**< @internalComponent */
	TUint8				iNThreadBaseSpare10;
	TUint8				iFastMutexDefer;		/**< @internalComponent */

	NFastSemaphore		iRequestSemaphore;		/**< @internalComponent */

	TInt				iTime;					/**< @internalComponent */	// time remaining, 0 if expired
	TInt				iTimeslice;				/**< @internalComponent */	// timeslice for this thread, -ve = no timeslicing

	TLinAddr			iSavedSP;				/**< @internalComponent */
	TAny*				iAddressSpace;			/**< @internalComponent */

	NFastMutex* volatile iHeldFastMutex;		/**< @internalComponent */	// fast mutex held by this thread
	TUserModeCallback* volatile iUserModeCallbacks;	/**< @internalComponent */	// Head of singly-linked list of callbacks
	TAny* volatile		iLinkedObj;				/**< @internalComponent */	// object to which this thread is linked
	NThreadGroup*		iNewParent;				/**< @internalComponent */	// group to join

	const SFastExecTable* iFastExecTable;		/**< @internalComponent */
	const SSlowExecEntry* iSlowExecTable;		/**< @internalComponent */	// points to first entry iEntries[0]

	volatile TInt		iCsCount;				/**< @internalComponent */	// critical section count
	volatile TInt		iCsFunction;			/**< @internalComponent */	// what to do on leaving CS: +n=suspend n times, 0=nothing, -1=exit

	NThreadWaitState	iWaitState;				/**< @internalComponent */

	const SNThreadHandlers* iHandlers;			/**< @internalComponent */	// additional thread event handlers
	TInt				iSuspendCount;			/**< @internalComponent */	// -how many times we have been suspended

	TLinAddr			iStackBase;				/**< @internalComponent */
	TInt				iStackSize;				/**< @internalComponent */

	TAny*				iExtraContext;			/**< @internalComponent */	// parent FPSCR value (iExtraContextSize == -1), coprocessor context (iExtraContextSize > 0) or NULL
	TInt				iExtraContextSize;		/**< @internalComponent */	// +ve=dynamically allocated, 0=none, -1=iExtraContext stores parent FPSCR value

	TUint32				iNThreadBaseSpare6;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare7;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare8;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare9;		/**< @internalComponent */	// spare to allow growth while preserving BC

	// For EMI support - HOPEFULLY THIS CAN DIE
	TUint32	iTag;							/**< @internalComponent */	// User defined set of bits which is ANDed with a mask when the thread is scheduled, and indicates if a DFC should be scheduled.
	TAny* iVemsData;						/**< @internalComponent */	// This pointer can be used by any VEMS to store any data associated with the thread.  This data must be clean up before the Thread Exit Monitor completes.
	};

__ASSERT_COMPILE(!(_FOFF(NThreadBase,iWaitLink)&7));
__ASSERT_COMPILE(!(sizeof(NThreadBase)&7));

#ifdef __INCLUDE_NTHREADBASE_DEFINES__
#define	iReady				iSpare1				/**< @internalComponent */
#define	iCurrent			iSpare2				/**< @internalComponent */
#define	iLastCpu			iSpare3				/**< @internalComponent */

#define iBasePri			iWaitLink.iSpare1	/**< @internalComponent */
#define	iMutexPri			iWaitLink.iSpare2	/**< @internalComponent */
#define	i_NThread_Initial	iWaitLink.iSpare3	/**< @internalComponent */

#endif

/** @internalComponent */
#define	i_NThread_BasePri	iWaitLink.iSpare1

/** @internalComponent */
#define	NTHREADBASE_CPU_AFFINITY_MASK	0x80000000

/** @internalComponent */
inline NThreadBase* NThreadWaitState::Thread()
	{ return _LOFF(this, NThreadBase, iWaitState); }

/********************************************
 * Thread group
 ********************************************/

/**
@publishedPartner
@prototype

Base class for a nanokernel thread or group
*/
class NThreadGroup : public NSchedulable
	{
public:
	NThreadGroup();
public:
	TInt iThreadCount;										/**< @internalComponent */
	TPriList<NThreadBase, KNumPriorities> iNThreadList;		/**< @internalComponent */
	};

/********************************************
 * Scheduler
 ********************************************/

/**
@internalComponent
*/
class TScheduler;
class NThread;
class NIrqHandler;
class TSubScheduler : public TPriListBase
	{
public:
	TSubScheduler();
	void QueueDfcs();
	void RotateReadyList(TInt aPriority);
	NThread* SelectNextThread();
	TBool QueueEvent(NEventHandler* aEvent);
	void QueueEventAndKick(NEventHandler* aEvent);
	void SaveTimesliceTimer(NThreadBase* aThread);
	void UpdateThreadTimes(NThreadBase* aOld, NThreadBase* aNew);
private:
	SDblQueLink*	iExtraQueues[KNumPriorities-1];
public:
	TSpinLock		iExIDfcLock;				// lock to protect exogenous IDFC queue

	SDblQue			iExIDfcs;					// list of pending exogenous IDFCs (i.e. ones punted over from another CPU)

	SDblQue			iDfcs;						// normal IDFC/DFC pending queue (only accessed by this CPU)

	TDfc* volatile	iCurrentIDFC;				// pointer to IDFC currently running on this CPU
	NThread*		iCurrentThread;				// the thread currently running on this CPU

	TUint32			iCpuNum;
	TUint32			iCpuMask;

	TSpinLock		iReadyListLock;

	volatile TUint8	iRescheduleNeededFlag;		// TRUE if a thread reschedule is pending
	TUint8			iSubSchedulerSBZ1;			// always zero
	volatile TUint8	iDfcPendingFlag;			// TRUE if a normal IDFC is pending
	volatile TUint8	iExIDfcPendingFlag;			// TRUE if an exogenous IDFC is pending
	TInt			iKernLockCount;				// how many times the current CPU has locked the kernel

	TUint8			iInIDFC;					// TRUE if IDFCs are currently being run on this CPU
	volatile TUint8	iEventHandlersPending;		// TRUE if an event handler is pending on this CPU
	TUint8			iSubSchedulerSpare4;
	TUint8			iSubSchedulerSpare5;
	TAny*			iAddressSpace;

	TUint32			iReschedIPIs;
	TScheduler*		iScheduler;

	union
		{
		TUint64		iLastTimestamp64;			// NKern::Timestamp() value at last reschedule or timestamp sync
		TUint32		iLastTimestamp32[2];
		};
	union
		{
		TUint64		iReschedCount64;
		TUint32		iReschedCount32[2];
		};

	TAny*			iExtras[24];				// Space for platform-specific extras

	TGenericIPI*	iNextIPI;					// next generic IPI to run on this CPU
	NThread*		iInitialThread;				// Initial (idle) thread on this CPU

	TSpinLock		iEventHandlerLock;			// lock to protect event handler queue

	SDblQue			iEventHandlers;				// queue of pending event handlers on this CPU

	TUint64			iSpinLockOrderCheck;		// bitmask showing which spinlock orders currently held

	TUint32			iSubSchedulerPadding[8];
	};

__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iExIDfcLock)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iEventHandlerLock)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iReadyListLock)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iLastTimestamp64)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iReschedCount64)&7));
__ASSERT_COMPILE(sizeof(TSubScheduler)==512);	// make it a nice power of 2 size for easy indexing

/**
@internalComponent
*/
class TScheduler
	{
public:
	TScheduler();
	static void Reschedule();
	IMPORT_C static TScheduler* Ptr();
	inline void SetProcessHandler(TLinAddr aHandler) {iProcessHandler=aHandler;}
public:
	TLinAddr		iMonitorExceptionHandler;
	TLinAddr		iProcessHandler;

	TLinAddr		iRescheduleHook;
	TUint32			iActiveCpus1;				// bit n set if CPU n is accepting unlocked threads

	TUint32			iActiveCpus2;				// bit n set if CPU n is accepting generic IPIs
	TInt			iNumCpus;					// number of CPUs under the kernel's control

	TSubScheduler*	iSub[KMaxCpus];				// one subscheduler per CPU

	TAny*			iExtras[24];				// Space for platform-specific extras

	NFastMutex		iLock;						// the 'system lock' fast mutex

	TSpinLock		iIdleSpinLock;				// lock to protect list of DFCs to be run on idle

	SDblQue			iIdleDfcs;					// list of DFCs to run when all CPUs go idle

	TUint32			iCpusNotIdle;				// bitmask - Bit n set => CPU n is not idle
	TUint8			iIdleGeneration;			// Toggles between 0 and 1 each time iIdleDfcs list is spilled to a CPU IDFC queue
	TUint8			iIdleSpillCpu;				// Which CPU last spilled the iIdleDfcs list to its IDFC queue
	TUint8			iTSchedulerSpare1;
	TUint8			iTSchedulerSpare2;

	TUint32			iIdleGenerationCount;		// Incremented each time iIdleDfcs list is spilled to a CPU IDFC queue
	TUint32			i_Scheduler_Padding[3];

	// For EMI support - HOPEFULLY THIS CAN DIE
	NThread* iSigma;	
	TDfc* iEmiDfc;
	TUint32 iEmiMask;
	TUint32 iEmiState;
	TUint32 iEmiDfcTrigger;
	TBool iLogging;
	TAny* iBufferStart;
	TAny* iBufferEnd;
	TAny* iBufferTail;
	TAny* iBufferHead;
	};

__ASSERT_COMPILE(!(_FOFF(TScheduler,iIdleSpinLock)&7));
__ASSERT_COMPILE(sizeof(TScheduler)==512);

extern TScheduler TheScheduler;
extern TSubScheduler TheSubSchedulers[KMaxCpus];

#ifdef __USE_BTRACE_LOCK__
extern TSpinLock BTraceLock;

#define	__ACQUIRE_BTRACE_LOCK()			TInt _btrace_irq = BTraceLock.LockIrqSave()
#define	__RELEASE_BTRACE_LOCK()			BTraceLock.UnlockIrqRestore(_btrace_irq)

#else

#define	__ACQUIRE_BTRACE_LOCK()
#define	__RELEASE_BTRACE_LOCK()

#endif

/**
@internalComponent
*/
extern "C" TSubScheduler& SubScheduler();

/**
@internalComponent
*/
extern "C" void send_resched_ipis(TUint32 aMask);

/**
@internalComponent
*/
extern "C" void send_resched_ipi(TInt aCpu);

/**
@internalComponent
*/
extern "C" void send_resched_ipi_and_wait(TInt aCpu);


#include <nk_plat.h>

/**
Call with kernel locked

@internalComponent
*/
inline void RescheduleNeeded()
	{ SubScheduler().iRescheduleNeededFlag = 1; }


/**
@internalComponent
*/
#define	NCurrentThread()	NKern::CurrentThread()

/** Optimised current thread function which can only be called from places where
	CPU migration is not possible - i.e. with interrupts disabled or preemption
	disabled.

@internalComponent
*/
extern "C" NThread* NCurrentThreadL();

/** @internalComponent */
inline TBool CheckCpuAgainstAffinity(TInt aCpu, TUint32 aAffinity)
	{
	if (aAffinity & NTHREADBASE_CPU_AFFINITY_MASK)
		return aAffinity & (1<<aCpu);
	return aAffinity==(TUint32)aCpu;
	}

/**
@internalComponent
*/
#define __NK_ASSERT_UNLOCKED	__NK_ASSERT_DEBUG(!NKern::KernelLocked())

/**
@internalComponent
*/
#define __NK_ASSERT_LOCKED		__NK_ASSERT_DEBUG(NKern::KernelLocked())

#ifdef _DEBUG
/**
@publishedPartner
@released
*/
#define __ASSERT_NO_FAST_MUTEX	__NK_ASSERT_DEBUG(!NKern::HeldFastMutex());

/**
@publishedPartner
@released
*/
#define __ASSERT_FAST_MUTEX(m)	__NK_ASSERT_DEBUG((m)->HeldByCurrentThread());

/**
@publishedPartner
@released
*/
#define __ASSERT_SYSTEM_LOCK	__NK_ASSERT_DEBUG(TScheduler::Ptr()->iLock.HeldByCurrentThread());

#define __ASSERT_NOT_ISR		__NK_ASSERT_DEBUG(NKern::CurrentContext()!=NKern::EInterrupt)

#else
#define __ASSERT_NO_FAST_MUTEX
#define __ASSERT_FAST_MUTEX(m)
#define	__ASSERT_SYSTEM_LOCK
#define __ASSERT_NOT_ISR
#endif

/********************************************
 * System timer queue
 ********************************************/

/**
@publishedPartner
@prototype
*/
class NTimerQ
	{
	friend class NTimer;
public:
	typedef void (*TDebugFn)(TAny* aPtr, TInt aPos);	/**< @internalComponent */
	enum { ETimerQMask=31, ENumTimerQueues=32 };		/**< @internalComponent */	// these are not easily modifiable

	/** @internalComponent */
	struct STimerQ
		{
		SDblQue iIntQ;
		SDblQue iDfcQ;
		};
public:
	NTimerQ();
	static void Init1(TInt aTickPeriod);
	static void Init3(TDfcQue* aDfcQ);
	IMPORT_C static TAny* TimerAddress();
	IMPORT_C void Tick();
	IMPORT_C static TInt IdleTime();
	IMPORT_C static void Advance(TInt aTicks);
private:
	static void DfcFn(TAny* aPtr);
	void Dfc();
	void Add(NTimer* aTimer);
	void AddFinal(NTimer* aTimer);
public:
	STimerQ			iTickQ[ENumTimerQueues];	/**< @internalComponent */	// NOTE: the order of member data is important
	TUint32			iPresent;					/**< @internalComponent */	// The assembler code relies on it
	TUint32			iMsCount;					/**< @internalComponent */
	SDblQue			iHoldingQ;					/**< @internalComponent */
	SDblQue			iOrderedQ;					/**< @internalComponent */
	SDblQue			iCompletedQ;				/**< @internalComponent */
	TDfc			iDfc;						/**< @internalComponent */
	TUint8			iTransferringCancelled;		/**< @internalComponent */
	TUint8			iCriticalCancelled;			/**< @internalComponent */
	TUint8			iPad1;						/**< @internalComponent */
	TUint8			iPad2;						/**< @internalComponent */
	TDebugFn		iDebugFn;					/**< @internalComponent */
	TAny*			iDebugPtr;					/**< @internalComponent */
	TInt			iTickPeriod;				/**< @internalComponent */	// in microseconds

	/**
	This member is intended for use by ASSP/variant interrupt code as a convenient
	location to store rounding error information where hardware interrupts are not
	exactly one millisecond. The Symbian kernel does not make any use of this member.
	@publishedPartner
	@prototype
	*/
	TInt			iRounding;
	TInt			iDfcCompleteCount;			/**< @internalComponent */
	TSpinLock		iTimerSpinLock;				/**< @internalComponent */
	};

__ASSERT_COMPILE(!(_FOFF(NTimerQ,iTimerSpinLock)&7));


GLREF_D NTimerQ TheTimerQ;

/**
@internalComponent
*/
inline TUint32 NTickCount()
	{return TheTimerQ.iMsCount;}

/**
@internalComponent
*/
inline TInt NTickPeriod()
	{return TheTimerQ.iTickPeriod;}


extern "C" {
/**
@internalComponent
*/
extern void NKCrashHandler(TInt aPhase, const TAny* a0, TInt a1);

/**
@internalComponent
*/
extern TUint32 CrashState;
}


/**
@internalComponent
*/
class TGenIPIList : public SDblQue
	{
public:
	TGenIPIList();
public:
	TSpinLock			iGenIPILock;
	};

/**
@internalComponent
*/
class TCancelIPI : public TGenericIPI
	{
public:
	void Send(TDfc* aDfc, TInt aCpu);
	static void Isr(TGenericIPI*);
public:
	TDfc* volatile iDfc;
	};


/**
@internalComponent
*/
TBool InterruptsStatus(TBool aRequest);


//declarations for the checking of kernel preconditions

/**
@internalComponent

PRECOND_FUNCTION_CALLER is needed for __ASSERT_WITH_MESSAGE_ALWAYS(),
so is outside the #ifdef _DEBUG.
*/
#ifndef PRECOND_FUNCTION_CALLER
#define PRECOND_FUNCTION_CALLER		0
#endif

#ifdef _DEBUG

/**
@internalComponent
*/
#define MASK_NO_FAST_MUTEX 0x1
#define MASK_CRITICAL 0x2
#define MASK_NO_CRITICAL 0x4
#define MASK_KERNEL_LOCKED 0x8
#define MASK_KERNEL_UNLOCKED 0x10
#define MASK_KERNEL_LOCKED_ONCE 0x20
#define MASK_INTERRUPTS_ENABLED 0x40
#define MASK_INTERRUPTS_DISABLED 0x80
#define MASK_SYSTEM_LOCKED 0x100
#define MASK_NOT_ISR 0x400
#define MASK_NOT_IDFC 0x800 
#define MASK_NOT_THREAD 0x1000
#define MASK_NO_CRITICAL_IF_USER 0x2000
#define MASK_THREAD_STANDARD ( MASK_NO_FAST_MUTEX | MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC )
#define MASK_THREAD_CRITICAL ( MASK_THREAD_STANDARD | MASK_CRITICAL )
#define MASK_ALWAYS_FAIL 0x4000
#define	MASK_NO_RESCHED 0x8000

#if defined(__STANDALONE_NANOKERNEL__) || (!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
#define CHECK_PRECONDITIONS(mask,function)
#define __ASSERT_WITH_MESSAGE_DEBUG(cond,message,function)

#else
/**
@internalComponent
*/
extern "C" TInt CheckPreconditions(TUint32 aConditionMask, const char* aFunction, TLinAddr aAddr);
/**
@internalComponent
*/
#define CHECK_PRECONDITIONS(mask,function) CheckPreconditions(mask,function,PRECOND_FUNCTION_CALLER)

#ifdef __KERNEL_APIS_CONTEXT_CHECKS_FAULT__

/**
@internalComponent
*/
#define __ASSERT_WITH_MESSAGE_DEBUG(cond,message,function) \
			__ASSERT_DEBUG( (cond), ( \
			DEBUGPRINT("Assertion failed: %s\nFunction: %s; called from: %08x\n",message,function,PRECOND_FUNCTION_CALLER),\
			NKFault(function, 0)))

#else//!__KERNEL_APIS_CONTEXT_CHECKS_FAULT__
/**
@internalComponent
*/
#define __ASSERT_WITH_MESSAGE_DEBUG(cond,message,function) \
			__ASSERT_DEBUG( (cond), \
			DEBUGPRINT("Assertion failed: %s\nFunction: %s; called from: %08x\n",message,function,PRECOND_FUNCTION_CALLER))


#endif//__KERNEL_APIS_CONTEXT_CHECKS_FAULT__
#endif//(!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))

#else//if !DEBUG

#define CHECK_PRECONDITIONS(mask,function)
#define __ASSERT_WITH_MESSAGE_DEBUG(cond,message,function )

#endif//_DEBUG

#if (!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
#define __ASSERT_WITH_MESSAGE_ALWAYS(cond,message,function )
#else
#ifdef __KERNEL_APIS_CONTEXT_CHECKS_FAULT__
/**
@internalComponent
*/
#define __ASSERT_WITH_MESSAGE_ALWAYS(cond,message,function) \
			__ASSERT_ALWAYS( (cond), ( \
			DEBUGPRINT("Assertion failed: %s\nFunction: %s; called from: %08x\n",message,function,PRECOND_FUNCTION_CALLER),\
			NKFault(function, 0)))
#else
/**
@internalComponent
*/
#define __ASSERT_WITH_MESSAGE_ALWAYS(cond,message,function) \
			__ASSERT_ALWAYS( (cond), \
			DEBUGPRINT("Assertion failed: %s\nFunction: %s; called from: %08x\n",message,function,PRECOND_FUNCTION_CALLER))
#endif//__KERNEL_APIS_CONTEXT_CHECKS_FAULT__
#endif//(!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))

#endif
