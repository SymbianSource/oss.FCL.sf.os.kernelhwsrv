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
// e32\include\nkern\nk_priv.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __NK_PRIV_H__
#define __NK_PRIV_H__
#include <cpudefs.h>
#include <nkern.h>

/********************************************
 * DFCs
 ********************************************/

/**
@internalComponent
*/
inline TBool TDfc::TestAndSetQueued()
	{ return __e32_atomic_swp_ord8(&iSpare3, 1); }

/********************************************
 * Thread
 ********************************************/

class TUserModeCallback;

/**
@publishedPartner
@released

Base class for a nanokernel thread.
*/
class NThreadBase : public TPriListLink
	{
public:
    /**
    Defines the possible states of a nanokernel thread.
    */
	enum NThreadState
		{
		/**
		The thread is eligible for execution.
		
		Threads in this state are linked into the ready list.
		The highest priority READY thread is the one that will run, unless it
		is blocked on a fast mutex.
		*/
		EReady,
		
		/**
		The thread is explicitly suspended (rather than blocking on
		a wait object).
		*/
		ESuspended,
		
		/**
		The thread is blocked waiting for a fast semaphore to be signalled.
		*/
		EWaitFastSemaphore,
		
		/**
		The thread is blocked waiting for a specific time period to elapse.
		*/
		ESleep,
		
		/**
		The thread is blocked on a wait object implemented in a layer above
		the nanokernel.
		
		In practice, this means that it is blocked on a Symbian OS
		semaphore or mutex.
		*/
		EBlocked,
		
		/**
		The thread has terminated and will not execute again.
		*/
		EDead,
		
		/**
		The thread is a DFC-handling thread and it is blocked waiting for
		the DFC to be queued.
		*/
		EWaitDfc,
		
		/**
		Not a thread state, but defines the maximum number of states.
		*/
		ENumNStates
		};




    /**
    Defines a set of values that, when passed to a nanokernel state handler,
    indicates which operation is being performed on the thread.
    
    Every thread that can use a new type of wait object, must have a nanokernel
    state handler installed to handle operations on that thread while it is
    waiting on that wait object.
    
    A wait handler has the signature:
    @code
    void StateHandler(NThread* aThread, TInt aOp, TInt aParam);
    @endcode
    
    where aOp is one of these enum values.
    
    The state handler is always called with preemption disabled.
    */
	enum NThreadOperation
		{
		/**
		Indicates that the thread is suspended while not in a critical section,
		and not holding a fast mutex.
		
		StateHandler() is called in whichever context
		NThreadBase::Suspend() is called from.
		
		Note that the third parameter passed to StateHandler() contains
		the requested suspension count.
		*/
		ESuspend=0,
		
		/**
		Indicates that the thread is being resumed while suspended, and
		the last suspension has been removed.
		
		StateHandler() is called in whichever context
		NThreadBase::Resume() is called from.
		*/
		EResume=1,
		
		/**
		Indicates that the thread has all suspensions cancelled while
		actually suspended.
		
		Statehandler() is called in whichever context
		NThreadBase::ForceResume() is called from.
		*/
		EForceResume=2,
		
		/**
		Indicates that the thread is being released from its wait.
		
		Statehandler() is called in whichever context
		NThreadBase::Release() is called from.
		*/
		ERelease=3,
		
		/**
		Indicates that the thread's priority is being changed.
		
		StateHandler() is called in whichever context
		NThreadBase::SetPriority() is called from.
		*/
		EChangePriority=4,
		
		/**
		Indicates that the thread has called NKern::ThreadLeaveCS() with
		an unknown NThreadBase::iCsFunction that is negative, but not equal
		to NThreadBase::ECsExitPending.
		
		Note that NThreadBase::iCsFunction is internal to Symbian OS.
		*/
		ELeaveCS=5,
		
		/**
		Indicates that the thread's wait timeout has expired, and no timeout
		handler has been defined for that thread.
	    
	    StateHandler() is called in the context of the nanokernel
	    timer thread, DfcThread1.
	    */
		ETimeout=6,
		};
		
	enum NThreadCSFunction
		{
		ECSExitPending=-1,
		ECSExitInProgress=-2
		};

	enum NThreadTimeoutOp
		{
		ETimeoutPreamble=0,
		ETimeoutPostamble=1,
		ETimeoutSpurious=2,
		};
public:
	NThreadBase();
	TInt Create(SNThreadCreateInfo& anInfo,	TBool aInitial);
	IMPORT_C void CheckSuspendThenReady();
	IMPORT_C void Ready();
	void DoReady();
	void DoCsFunction();
	IMPORT_C TBool Suspend(TInt aCount);
	IMPORT_C TBool Resume();
	IMPORT_C TBool ForceResume();
	IMPORT_C void Release(TInt aReturnCode);
	IMPORT_C void RequestSignal();
	IMPORT_C void SetPriority(TInt aPriority);
	void SetEntry(NThreadFunction aFunction);
	IMPORT_C void Kill();
	void Exit();
	void ForceExit();
	// hooks for platform-specific code
	void OnKill(); 
	void OnExit();
public:
	static void TimerExpired(TAny* aPtr);
	inline void UnknownState(TInt aOp, TInt aParam)
		{ (*iHandlers->iStateHandler)((NThread*)this,aOp,aParam); }

	/** @internalComponent */
	inline TUint8 Attributes()
		{ return iSpare2; }

	/** @internalComponent */
	inline TUint8 SetAttributes(TUint8 aNewAtt)
		{ return __e32_atomic_swp_ord8(&iSpare2, aNewAtt); }

	/** @internalComponent */
	inline TUint8 ModifyAttributes(TUint8 aClearMask, TUint8 aSetMask)
		{ return __e32_atomic_axo_ord8(&iSpare2, (TUint8)~(aClearMask|aSetMask), aSetMask); }

	/** @internalComponent */
	inline void SetAddressSpace(TAny* a)
		{ iAddressSpace=a; }

	inline void SetReturnValue(TInt aValue)
		{ iReturnValue=aValue; }
	inline void SetExtraContext(TAny* a, TInt aSize)
		{ iExtraContext = a; iExtraContextSize = aSize; }

	/** @internalComponent */
	void CallUserModeCallbacks();
public:
//	TUint8 iNState;														// use iSpare1 for state
//	TUint8 i_ThrdAttr;						/**< @internalComponent */	// use iSpare2 for attributes
//	TUint8 iUserContextType;											// use iSpare3
	NFastMutex* iHeldFastMutex;				/**< @internalComponent */	// fast mutex held by this thread
	NFastMutex* iWaitFastMutex;				/**< @internalComponent */	// fast mutex on which this thread is blocked
	TAny* iAddressSpace;					/**< @internalComponent */
	TInt iTime;															// time remaining
	TInt iTimeslice;													// timeslice for this thread
	NFastSemaphore iRequestSemaphore;		/**< @internalComponent */
	TAny* iWaitObj;														// object on which this thread is waiting
	TInt iSuspendCount;						/**< @internalComponent */	// -how many times we have been suspended
	TInt iCsCount;							/**< @internalComponent */	// critical section count
	TInt iCsFunction;						/**< @internalComponent */	// what to do on leaving CS: +n=suspend n times, 0=nothing, -1=exit
	NTimer iTimer;							/**< @internalComponent */
	TInt iReturnValue;
	TLinAddr iStackBase;					/**< @internalComponent */
	TInt iStackSize;						/**< @internalComponent */
	const SNThreadHandlers* iHandlers;		/**< @internalComponent */	// additional thread event handlers
	const SFastExecTable* iFastExecTable;	/**< @internalComponent */
	const SSlowExecEntry* iSlowExecTable;	/**< @internalComponent */	// points to first entry iEntries[0]
	TLinAddr iSavedSP;						/**< @internalComponent */
	TAny* iExtraContext;					/**< @internalComponent */	// parent FPSCR value (iExtraContextSize == -1), coprocessor context (iExtraContextSize > 0) or NULL
	TInt iExtraContextSize;					/**< @internalComponent */	// +ve=dynamically allocated, 0=none, -1=iExtraContext stores parent FPSCR value
	TUint iLastStartTime;					/**< @internalComponent */	// last start of execution timestamp
	TUint64 iTotalCpuTime;					/**< @internalComponent */	// total time spent running, in hi-res timer ticks
	TUint32 iTag;							/**< @internalComponent */	// User defined set of bits which is ANDed with a mask when the thread is scheduled, and indicates if a DFC should be scheduled.
	TAny* iVemsData;						/**< @internalComponent */	// This pointer can be used by any VEMS to store any data associated with the thread.  This data must be clean up before the Thread Exit Monitor completes.
	TUserModeCallback* volatile iUserModeCallbacks;	/**< @internalComponent */	// Head of singly-linked list of callbacks
	TUint32 iSpare7;						/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32 iSpare8;						/**< @internalComponent */	// spare to allow growth while preserving BC
	};

__ASSERT_COMPILE(!(_FOFF(NThreadBase,iTotalCpuTime)&7));

#ifdef __INCLUDE_NTHREADBASE_DEFINES__
#define iNState				iSpare1
#define	i_ThrdAttr			iSpare2
#define iUserContextType	iSpare3
#endif

#define	i_NThread_BasePri	iPriority

/********************************************
 * Scheduler
 ********************************************/

/**
@internalComponent
*/
class TScheduler : public TPriListBase
	{
public:
	TScheduler();
	void Remove(NThreadBase* aThread);
	void RotateReadyList(TInt aPriority);
	void QueueDfcs();
	static void Reschedule();
	static void YieldTo(NThreadBase* aThread);
	void TimesliceTick();
	IMPORT_C static TScheduler* Ptr();
	inline void SetProcessHandler(TLinAddr aHandler) {iProcessHandler=aHandler;}
private:
	SDblQueLink* iExtraQueues[KNumPriorities-1];
public:
	TUint8 iRescheduleNeededFlag;
	TUint8 iDfcPendingFlag;
	TInt iKernCSLocked;
	SDblQue iDfcs;
	TLinAddr iMonitorExceptionHandler;
	TLinAddr iProcessHandler;
	TLinAddr iRescheduleHook;
	TUint8 iInIDFC;
	NFastMutex iLock;
	NThreadBase* iCurrentThread;
	TAny* iAddressSpace;
	TAny* iExtras[16];
	// For EMI support
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
	// For BTrace suport
	TUint8 iCpuUsageFilter;
	TUint8 iFastMutexFilter;
	BTrace::THandler iBTraceHandler;
	// Idle notification
	SDblQue iIdleDfcs;
	TUint32 iIdleGenerationCount;
	// Delayed threads
	SDblQue iDelayedQ;
	TDfc iDelayDfc;
	// KernCoreStats
	TUint iMadeReadyCounter;	// Number of times threads' where made ready.
	TUint iMadeUnReadyCounter;	// Number of times threads' where made unready.
	TUint iTimeSliceExpireCounter;	// Number of times threads' time slice expire, resulting in round robin.
	};

GLREF_D TScheduler TheScheduler;

/**
@internalComponent
*/
inline void RescheduleNeeded()
	{TheScheduler.iRescheduleNeededFlag=TRUE;}

#include <nk_plat.h>

/**
@internalComponent
*/
inline NThread* NCurrentThread()
	{ return (NThread*)TheScheduler.iCurrentThread; }


/**
@internalComponent
*/
#define __NK_ASSERT_UNLOCKED	__NK_ASSERT_DEBUG(TheScheduler.iKernCSLocked==0)

/**
@internalComponent
*/
#define __NK_ASSERT_LOCKED		__NK_ASSERT_DEBUG(TheScheduler.iKernCSLocked!=0)

#ifdef _DEBUG
/**
@publishedPartner
@released
*/
#define __ASSERT_NO_FAST_MUTEX	{	\
								NThread* nt=NKern::CurrentThread();	\
								__NK_ASSERT_DEBUG(!nt->iHeldFastMutex); \
								}

/**
@publishedPartner
@released
*/
#define __ASSERT_FAST_MUTEX(m)	{	\
								NThread* nt=NKern::CurrentThread();	\
								__NK_ASSERT_DEBUG(nt->iHeldFastMutex==(m) && (m)->iHoldingThread==nt); \
								}

/**
@publishedPartner
@released
*/
#define __ASSERT_SYSTEM_LOCK	{	\
								NThread* nt=NKern::CurrentThread();	\
								NFastMutex& m=TScheduler::Ptr()->iLock; \
								__NK_ASSERT_DEBUG(nt->iHeldFastMutex==&m && m.iHoldingThread==nt); \
								}

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
@released
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
	STimerQ iTickQ[ENumTimerQueues];	/**< @internalComponent */	// NOTE: the order of member data is important
	TUint32 iPresent;					/**< @internalComponent */	// The assembler code relies on it
	TUint32 iMsCount;					/**< @internalComponent */
	SDblQue iHoldingQ;					/**< @internalComponent */
	SDblQue iOrderedQ;					/**< @internalComponent */
	SDblQue iCompletedQ;				/**< @internalComponent */
	TDfc iDfc;							/**< @internalComponent */
	TUint8 iTransferringCancelled;		/**< @internalComponent */
	TUint8 iCriticalCancelled;			/**< @internalComponent */
	TUint8 iPad1;						/**< @internalComponent */
	TUint8 iPad2;						/**< @internalComponent */
	TDebugFn iDebugFn;					/**< @internalComponent */
	TAny* iDebugPtr;					/**< @internalComponent */
	TInt iTickPeriod;					/**< @internalComponent */	// in microseconds
	/**
	This member is intended for use by ASSP/variant interrupt code as a convenient
	location to store rounding error information where hardware interrupts are not
	exactly one millisecond. The Symbian kernel does not make any use of this member.
	@publishedPartner
	@released
	*/
	TInt iRounding;
	};

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


#define	__ACQUIRE_BTRACE_LOCK()
#define	__RELEASE_BTRACE_LOCK()

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
#define	MASK_NO_FAST_MUTEX			0x1
#define	MASK_CRITICAL				0x2
#define	MASK_NO_CRITICAL			0x4
#define	MASK_KERNEL_LOCKED			0x8
#define	MASK_KERNEL_UNLOCKED		0x10
#define	MASK_KERNEL_LOCKED_ONCE		0x20
#define	MASK_INTERRUPTS_ENABLED		0x40
#define	MASK_INTERRUPTS_DISABLED	0x80
#define	MASK_SYSTEM_LOCKED			0x100
#define	MASK_NOT_ISR				0x400
#define	MASK_NOT_IDFC				0x800 
#define	MASK_NOT_THREAD				0x1000
#define	MASK_NO_CRITICAL_IF_USER	0x2000
#define	MASK_ALWAYS_FAIL			0x4000
#define	MASK_NO_RESCHED				0x8000
#define MASK_NO_KILL_OR_SUSPEND	0x10000

#define MASK_THREAD_STANDARD ( MASK_NO_FAST_MUTEX | MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC )
#define MASK_THREAD_CRITICAL ( MASK_THREAD_STANDARD | MASK_CRITICAL )

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
#define __ASSERT_WITH_MESSAGE_DEBUG(cond,message,function)

#endif//_DEBUG

#if (!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
#define __ASSERT_WITH_MESSAGE_ALWAYS(cond,message,function)
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
