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

const TInt KNumPriClasses = 4;
extern const TUint8 KClassFromPriority[KNumPriorities];

#ifndef __LOAD_BALANCE_INFO_DEFINED__
/**
@internalComponent
*/
struct SLbInfo
	{
	TUint64		i__Dummy;
	};
#endif

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
	/**
	@internalComponent
	*/
	enum
		{
		EReadyGroup=1,
		EReadyCpuMask=0x1f,
		EReadyCpuSticky=0x40,
		EReadyOffset=0x80,
		};

	/**
	@internalComponent
	*/
	enum NReadyFlags
		{
		ENewTimeslice=1,
		EPreferSameCpu=2,
		EUnPause=4,
		};

	/**
	@internalComponent
	*/
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

	/**
	@internalComponent
	*/
	enum NLbState
		{
		ELbState_Inactive	= 0x00u,	// not currently involved in load balancing
		ELbState_Global		= 0x01u,	// flag indicating this is on global load balance list
		ELbState_Temp		= 0x02u,	// flag indicating this is on a temporary load balance list
		ELbState_CpuMask	= 0x1Fu,	// mask of bits indicating CPU if on per-CPU list
		ELbState_PerCpu		= 0x20u,	// flag indicating this is on per-CPU load balance list
		ELbState_ExtraRef	= 0x40u,	// flag indicating extra reference has been taken after thread/group died
		ELbState_Generation	= 0x80u,	// 1 bit generation number
		};

	/**
	@internalComponent
	*/
	enum NCpuStatsSelect
		{
		E_RunTime=0x01u,
		E_RunTimeDelta=0x02u,
		E_ActiveTime=0x04u,
		E_ActiveTimeDelta=0x08u,
		E_LastRunTime=0x10u,
		E_LastActiveTime=0x20u,

		E_AllStats = 0x3fu
		};

	/**
	@internalComponent
	*/
	struct SCpuStats
		{
		TUint64		iRunTime;			// total run time
		TUint64		iRunTimeDelta;		// run time since we last asked
		TUint64		iActiveTime;		// total active time
		TUint64		iActiveTimeDelta;	// active time since we last asked
		TUint64		iLastRunTime;		// how long ago this last ran
		TUint64		iLastActiveTime;	// how long ago this was last active
		};
public:
	NSchedulable();								/**< @internalComponent */
	void AcqSLock();							/**< @internalComponent */
	void RelSLock();							/**< @internalComponent */
	void LAcqSLock();							/**< @internalComponent */
	void RelSLockU();							/**< @internalComponent */
	void ReadyT(TUint aMode);					/**< @internalComponent */ // make ready, assumes lock held
	TInt BeginTiedEvent();						/**< @internalComponent */
	void EndTiedEvent();						/**< @internalComponent */
	TInt AddTiedEvent(NEventHandler* aEvent);	/**< @internalComponent */
	TBool TiedEventReadyInterlock(TInt aCpu);	/**< @internalComponent */
	void UnPauseT();							/**< @internalComponent */ // decrement pause count and make ready if necessary
	static void DeferredReadyIDfcFn(TAny*);		/**< @internalComponent */
	void DetachTiedEvents();					/**< @internalComponent */
	TBool TakeRef();							/**< @internalComponent */
	TBool DropRef();							/**< @internalComponent */
	void LbUnlink();							/**< @internalComponent */
	void LbTransfer(SDblQue& aDestQ);			/**< @internalComponent */
	void RemoveFromEnumerateList();				/**< @internalComponent */
	void GetCpuStats(TUint aMask, SCpuStats& aOut);		/**< @internalComponent */
	void GetCpuStatsT(TUint aMask, SCpuStats& aOut);	/**< @internalComponent */
	void GetLbStats(TUint64 aTime);				/**< @internalComponent */
	void LbDone(TUint aFlags);					/**< @internalComponent */
	TUint32 SetCpuAffinityT(TUint32 aAffinity);	/**< @internalComponent */
	TBool ShouldMigrate(TInt aCpu);				/**< @internalComponent */
	void InitLbInfo();							/**< @internalComponent */
	void NominalPriorityChanged();				/**< @internalComponent */
	void AddToEnumerateList();					/**< @internalComponent */
	void SetEventCpu();							/**< @internalComponent */
public:
	static TUint32 PreprocessCpuAffinity(TUint32 aAffinity);			/**< @internalComponent */
	inline TBool IsGroup()			{return !iParent;}					/**< @internalComponent */
	inline TBool IsLoneThread()		{return iParent==this;}				/**< @internalComponent */
	inline TBool IsGroupThread()	{return iParent && iParent!=this;}	/**< @internalComponent */
public:
//	TUint8				iReady;					/**< @internalComponent */	// flag indicating thread on ready list = cpu number | EReadyOffset
//	TUint8				iCurrent;				/**< @internalComponent */	// flag indicating thread is running
//	TUint8				iLastCpu;				/**< @internalComponent */	// CPU on which this thread last ran
	TUint8				iPauseCount;			/**< @internalComponent */	// count of externally requested pauses extending a voluntary wait
	TUint8				iSuspended;				/**< @internalComponent */	// flag indicating active external suspend (Not used for groups)
	TUint8				iACount;				/**< @internalComponent */	// access count
	TUint8				iPreferredCpu;			/**< @internalComponent */

	TInt				iActiveState;			/**< @internalComponent */
	TUint8				i_NSchedulable_Spare2;	/**< @internalComponent */
	TUint8				iForcedCpu;				/**< @internalComponent */
	TUint8				iTransientCpu;			/**< @internalComponent */
	TUint8				iLbState;				/**< @internalComponent */

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

	union {
		TUint64HL		iRunCount;				/**< @internalComponent */	// number of times this thread has run
		TUint64HL		iLastStartTime;			/**< @internalComponent */	// last start time for groups
		};
	TUint64HL			iLastRunTime;			/**< @internalComponent */	// time when this thread last ran
	TUint64HL			iTotalCpuTime;			/**< @internalComponent */	// total CPU time used by this thread
	TUint64HL			iLastActivationTime;	/**< @internalComponent */	// time when this thread last became active
	TUint64HL			iTotalActiveTime;		/**< @internalComponent */	// total time this thread has been active
	TUint64HL			iSavedCpuTime;			/**< @internalComponent */	// Total CPU time used at last check
	TUint64HL			iSavedActiveTime;		/**< @internalComponent */	// Total active time at last check
	SDblQueLink			iLbLink;				/**< @internalComponent */	// Link into queue of tasks requiring load balancing
	SIterDQLink			iEnumerateLink;			/**< @internalComponent */

	enum {EMaxLbInfoSize = 48};					/**< @internalComponent */
	union	{
		TUint64			i__Dummy[EMaxLbInfoSize/sizeof(TUint64)];	/**< @internalComponent */
		SLbInfo			iLbInfo;				/**< @internalComponent */
		};
	};

__ASSERT_COMPILE(!(_FOFF(NSchedulable,iSSpinLock)&7));
__ASSERT_COMPILE(!(_FOFF(NSchedulable,iRunCount)&7));
__ASSERT_COMPILE(!(_FOFF(NSchedulable,iTotalCpuTime)&7));
__ASSERT_COMPILE(!(_FOFF(NSchedulable,iLbInfo)&7));
__ASSERT_COMPILE(sizeof(SLbInfo) <= NSchedulable::EMaxLbInfoSize);
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
	TInt Create(SNThreadCreateInfo& anInfo,	TBool aInitial);	/**< @internalComponent */
	void UnReadyT();										/**< @internalComponent */
	TBool SuspendOrKill(TInt aCount);						/**< @internalComponent */
	TBool DoSuspendOrKillT(TInt aCount, TSubScheduler* aS);	/**< @internalComponent */
	TBool CancelTimerT();									/**< @internalComponent */
	void DoReleaseT(TInt aReturnCode, TUint aMode);			/**< @internalComponent */
	TBool CheckFastMutexDefer();							/**< @internalComponent */
	void DoCsFunctionT();									/**< @internalComponent */
	TBool Resume(TBool aForce);								/**< @internalComponent */
	IMPORT_C TBool Suspend(TInt aCount);					/**< @internalComponent */
	IMPORT_C TBool Resume();								/**< @internalComponent */
	IMPORT_C TBool ForceResume();							/**< @internalComponent */
	IMPORT_C void Release(TInt aReturnCode, TUint aMode);	/**< @internalComponent */
	IMPORT_C void RequestSignal();							/**< @internalComponent */
	IMPORT_C void SetPriority(TInt aPriority);				/**< @internalComponent */
	void SetNominalPriority(TInt aPriority);				/**< @internalComponent */
	void SetMutexPriority(NFastMutex* aMutex);				/**< @internalComponent */
	void LoseInheritedPriorityT();							/**< @internalComponent */
	void ChangeReadyThreadPriority();						/**< @internalComponent */
	TBool TiedEventLeaveInterlock();						/**< @internalComponent */
	TBool TiedEventJoinInterlock();							/**< @internalComponent */
	IMPORT_C void Kill();									/**< @internalComponent */
	void Exit();											/**< @internalComponent */
	// hooks for platform-specific code
	void OnKill();											/**< @internalComponent */
	void OnExit();											/**< @internalComponent */
public:
	static void TimerExpired(TAny* aPtr);					/**< @internalComponent */

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
//	TUint8				iNominalPri;			/**< @internalComponent */	// nominal priority of thread (excluding effect of higher level inheritance)
	TUint8				iLinkedObjType;
	TUint8				i_ThrdAttr;				/**< @internalComponent */
	TUint8				iInitial;				/**< @internalComponent */	// TRUE if this is an initial thread
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

	TUint8				iCoreCycling;			/**< @internalComponent */	// this thread is currently cycling through all active cores
	TUint8				iRebalanceAttr;			/**< @internalComponent */	// behaviour of load balancing wrt this thread
	TUint8				iNThreadBaseSpare4c;	/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint8				iNThreadBaseSpare4d;	/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare5;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare6;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare7;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare8;		/**< @internalComponent */	// spare to allow growth while preserving BC
	TUint32				iNThreadBaseSpare9;		/**< @internalComponent */	// spare to allow growth while preserving BC
	};

__ASSERT_COMPILE(!(_FOFF(NThreadBase,iWaitLink)&7));
__ASSERT_COMPILE(!(sizeof(NThreadBase)&7));

#ifdef __INCLUDE_NTHREADBASE_DEFINES__
#define	iReady				iSpare1				/**< @internalComponent */
#define	iCurrent			iSpare2				/**< @internalComponent */
#define	iLastCpu			iSpare3				/**< @internalComponent */

#define iBasePri			iWaitLink.iSpare1	/**< @internalComponent */
#define	iMutexPri			iWaitLink.iSpare2	/**< @internalComponent */
#define	iNominalPri			iWaitLink.iSpare3	/**< @internalComponent */
#define	i_NThread_Initial	iInitial			/**< @internalComponent */

#endif

/** @internalComponent */
#define	i_NThread_BasePri		iWaitLink.iSpare1
#define	i_NThread_NominalPri	iWaitLink.iSpare3

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
	TDfc* iDestructionDfc;									/**< @internalComponent */
	TPriList<NThreadBase, KNumPriorities> iNThreadList;		/**< @internalComponent */
	};

/********************************************
 * Scheduler
 ********************************************/

#include <nk_plat.h>

/**
@internalComponent
*/
enum
	{
	EQueueEvent_Kick=1,
	EQueueEvent_WakeUp=2,
	};

/**
@internalComponent
*/
class TScheduler;
class NThread;
class NIrqHandler;
struct SIdlePullThread;
class TSubScheduler
	{
public:
	TSubScheduler();
	void QueueDfcs();
	void RotateReadyList(TInt aPriority);
	NThread* SelectNextThread();
	TInt QueueEvent(NEventHandler* aEvent);
	void QueueEventAndKick(NEventHandler* aEvent);
	void SaveTimesliceTimer(NThreadBase* aThread);
	void UpdateThreadTimes(NThreadBase* aOld, NThreadBase* aNew);
	void SSAddEntry(NSchedulable* aEntry);
	void SSAddEntryHead(NSchedulable* aEntry);
	void SSRemoveEntry(NSchedulable* aEntry);
	void SSChgEntryP(NSchedulable* aEntry, TInt aNewPriority);
	void IdlePullSearch(SIdlePullThread& a, TSubScheduler* aDest);
	void GetLbThreads(SDblQue& aQ);
	TBool Detached();	// platform specific

	inline TInt HighestPriority()
		{ return iSSList.HighestPriority(); }
	inline NSchedulable* EntryAtPriority(TInt aPri)
		{ return (NSchedulable*)iSSList.iQueue[aPri]; }
private:
	TPriList<NSchedulable, KNumPriorities>	iSSList;
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
	TUint8			iCCSyncPending;
	TUint8			iLbCounter;
	TAny*			iAddressSpace;

	TUint32			iReschedIPIs;
	TScheduler*		iScheduler;

	TInt			iDeferShutdown;				// counts reasons why this CPU can't shut down
	TInt			iRdyThreadCount;			// number of ready threads excluding idle thread
	TUint16			iPriClassThreadCount[KNumPriClasses];

	TUint64HL		iLastTimestamp;				// timestamp at which last reschedule occurred
	TUint64HL		iReschedCount;

	TGenericIPI*	iNextIPI;					// next generic IPI to run on this CPU
	NThread*		iInitialThread;				// Initial (idle) thread on this CPU

	TSpinLock		iEventHandlerLock;			// lock to protect event handler queue

	SDblQue			iEventHandlers;				// queue of pending event handlers on this CPU

	TUint64			iSpinLockOrderCheck;		// bitmask showing which spinlock orders currently held

	TSubSchedulerX	iSSX;						// platform specific extras

	volatile TAny*	iUncached;					// points to platform specific uncached data structure
	TUint 			iMadeReadyCounter;			// Number of times this core made a thread ready.

	TUint 			iMadeUnReadyCounter;		// Number of times this core made a thread unready.
	TUint 			iTimeSliceExpireCounter;	// Number of times this core hass reschedualed due to time slice exireation.

	TUint32			iSubSchedulerPadding[70];
	SDblQue			iLbQ;						// threads to be considered by subsequent periodic load balance

	TAny*			iSubSchedScratch[16];		// For use by code outside NKern
	};

const TInt KSubSchedulerShift = 10;				// log2(sizeof(TSubScheduler))

__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iExIDfcLock)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iEventHandlerLock)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iReadyListLock)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iLastTimestamp)&7));
__ASSERT_COMPILE(!(_FOFF(TSubScheduler,iReschedCount)&7));
__ASSERT_COMPILE(sizeof(TSubSchedulerX)==256);
__ASSERT_COMPILE(sizeof(TSubScheduler)==(1<<KSubSchedulerShift));	// make it a nice power of 2 size for easy indexing

struct SCoreControlAction;
struct SVariantInterfaceBlock;

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
	void PeriodicBalance();
	TBool ReBalance(SDblQue& aQ, TBool aCC);
	void CCReactivate(TUint32 aMore);
	void CCIpiReactivate();
	void CCRequest();
	void GetLbThreads(SDblQue& aQ);
	void CCUnDefer();
	void ChangeThreadAcceptCpus(TUint32 aNewMask);
	TUint32 ReschedInactiveCpus(TUint32 aMask);
	void InitCCAction(SCoreControlAction& aA);
	TUint32 ModifyCCState(TUint32 aAnd, TUint32 aXor);
	TUint32 CpuShuttingDown(TSubScheduler& aSS);
	void AllCpusIdle();
	void FirstBackFromIdle();

	void InitLB();
	void StartRebalanceTimer(TBool aRestart);
	void StopRebalanceTimer(TBool aTemp);
	static void BalanceTimerExpired(TAny*);
	static void StartPeriodicBalancing();
	static void CCSyncDone(TAny*);
	static void CCReactivateDfcFn(TAny*);
	static void CCRequestDfcFn(TAny*);
	static void CCIpiReactivateFn(TAny*);
	static TDfcQue* RebalanceDfcQ();
	static NThread* LBThread();
	static TBool CoreControlSupported();
	static void CCInitiatePowerUp(TUint32 aCores);
	static void CCIndirectPowerDown(TAny*);
	static void DoFrequencyChanged(TAny*);
public:
	TLinAddr		iMonitorExceptionHandler;
	TLinAddr		iProcessHandler;

	volatile TUint32 iThreadAcceptCpus;			// bit n set if CPU n is accepting unlocked threads
	volatile TUint32 iIpiAcceptCpus;			// bit n set if CPU n is accepting generic IPIs
	volatile TUint32 iCpusComingUp;				// bit n set if CPU n is in the process of powering up
	volatile TUint32 iCpusGoingDown;			// bit n set if CPU n is in the process of powering down and is no longer accepting IPIs
	volatile TInt	 iCCDeferCount;				// >0 means CPUs on the way down will stop just before the 'point of no return'
	volatile TUint32 iCCSyncCpus;				// bit n set if CPU n has not yet observed a change to iThreadAcceptCpus
	volatile TUint32 iCCReactivateCpus;
	volatile TUint32 iCCState;

	TInt			iNumCpus;					// number of CPUs under the kernel's control
	TLinAddr		iRescheduleHook;

	SDblQue			iGenIPIList;				// list of active generic IPIs
	TSpinLock		iGenIPILock;				// spin lock protects iGenIPIList, also iIpiAcceptCpus, iCpusComingUp, iCpusGoingDown, iCCDeferCount

	TSubScheduler*	iSub[KMaxCpus];				// one subscheduler per CPU

	TAny*			iSchedScratch[16];			// for use by code outside NKern

	TSchedulerX		iSX;						// platform specific extras

	NFastMutex		iLock;						// the 'system lock' fast mutex

	TSpinLock		iIdleBalanceLock;

	TSpinLock		iIdleSpinLock;				// lock to protect list of DFCs to be run on idle

	SDblQue			iIdleDfcs;					// list of DFCs to run when all CPUs go idle

	TUint32			iCpusNotIdle;				// bitmask - Bit n set => CPU n is not idle
	TUint8			iIdleGeneration;			// Toggles between 0 and 1 each time iIdleDfcs list is spilled to a CPU IDFC queue
	TUint8			iIdleSpillCpu;				// Which CPU last spilled the iIdleDfcs list to its IDFC queue
	TUint8			iLbCounter;
	volatile TUint8	iNeedBal;

	TUint32			iIdleGenerationCount;		// Incremented each time iIdleDfcs list is spilled to a CPU IDFC queue
	TDfcQue*		iRebalanceDfcQ;

	TSpinLock		iEnumerateLock;				// lock to protect iAllThreads, iAllGroups
	SIterDQ			iAllThreads;				// list of all nanokernel threads in order of creation
	SIterDQ			iAllGroups;					// list of all thread groups in order of creation
	TSpinLock		iBalanceListLock;			// lock to protect iBalanceList
	TUint64			iLastBalanceTime;			// time at which last rebalance occurred
	SDblQue			iBalanceList;				// list of threads/groups for load balancing
	NTimer			iBalanceTimer;				// triggers periodic rebalancing
	TDfc			iCCSyncIDFC;				// runs when a change to iThreadAcceptCpus has been observed by all CPUs
	TDfc			iCCReactivateDfc;			// runs when a reschedule IPI is targeted to an inactive CPU

	TUint32			iCCRequestLevel;			// Number of active cores last requested
	volatile TUint32 iCCIpiReactivate;			// Cores to be woken up because of IPIs

	TDfc			iCCRequestDfc;				// runs when a request is made to change the number of active cores
	TDfc			iCCPowerDownDfc;			// runs when indirect power down of core(s) is required
	TDfc			iCCIpiReactIDFC;			// runs when an IPI needs to wake up a core
	TDfc			iFreqChgDfc;				// runs when frequency changes required

	TSubScheduler*	iPoweringOff;				// CPU last to power off
	TUint32			iDetachCount;				// detach count before power off

	SVariantInterfaceBlock* iVIB;
	TUint32			i_Scheduler_Padding[29];
	};

__ASSERT_COMPILE(!(_FOFF(TScheduler,iGenIPILock)&7));
__ASSERT_COMPILE(!(_FOFF(TScheduler,iIdleSpinLock)&7));
__ASSERT_COMPILE(!(_FOFF(TScheduler,iIdleBalanceLock)&7));
__ASSERT_COMPILE(!(_FOFF(TScheduler,iEnumerateLock)&7));
__ASSERT_COMPILE(!(_FOFF(TScheduler,iBalanceListLock)&7));
__ASSERT_COMPILE(sizeof(TSchedulerX)==32*4);
__ASSERT_COMPILE(sizeof(TScheduler)==1024);

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
extern "C" void send_resched_ipi(TInt aCpu);

/**
@internalComponent
*/
extern "C" void send_resched_ipi_and_wait(TInt aCpu);

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

/** @internalComponent */
inline TBool CheckCpuAgainstAffinity(TInt aCpu, TUint32 aAffinity, TUint32 aActive)
	{
	if (aAffinity & NTHREADBASE_CPU_AFFINITY_MASK)
		return aActive & aAffinity & (1<<aCpu);
	return (aAffinity==(TUint32)aCpu) && (aActive & (1<<aCpu));
	}

/** @internalComponent */
inline TUint32 AffinityToMask(TUint32 aAffinity)
	{
	return (aAffinity & NTHREADBASE_CPU_AFFINITY_MASK) ? (aAffinity & ~NTHREADBASE_CPU_AFFINITY_MASK) : (1u<<aAffinity);
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

	/**
	This member is intended for use by ASSP/variant interrupt code as a convenient
	location to store the value of a free running counter at the point where the
	system tick is started.
	@publishedPartner
	@prototype
	*/
	TUint32			iFRCOffset;

	union {
		TUint32			iMsCount;				/**< @internalComponent */
		TUint64			iMsCount64;				/**< @internalComponent */
		};
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
#define	MASK_NO_KILL_OR_SUSPEND		0x10000
#define	MASK_THREAD_STANDARD		( MASK_NO_FAST_MUTEX | MASK_KERNEL_UNLOCKED | MASK_INTERRUPTS_ENABLED | MASK_NOT_ISR | MASK_NOT_IDFC )
#define	MASK_THREAD_CRITICAL		( MASK_THREAD_STANDARD | MASK_CRITICAL )

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
