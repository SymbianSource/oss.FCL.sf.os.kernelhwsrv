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
// e32\include\kernel\kern_priv.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __K32KERN_H__
#define __K32KERN_H__
#include <kernel/kernel.h>
#include <nk_priv.h>
#include <kernel/kpower.h>
#include <platform.h>
#include <e32uid.h>
#include <kernel/kernboot.h>
#include <e32def_private.h>
#include <e32const_private.h>
#include <e32des8_private.h>
#include <e32event_private.h>
#include <kernel/heap_hybrid.h>

#ifndef __MINIMUM_MACHINE_CODE__
#ifdef __MARM__
#ifndef __BIG_ENDIAN__

#ifdef __SMP__
	#define __FASTEXEC_MACHINE_CODED__
 	#define __TBMA_MACHINE_CODED__
#else
// These macros are intended for Symbian use only.
// It may not be possible to build the kernel if particular combinations 
// of these macros are undefined
	#define __FASTEXEC_MACHINE_CODED__
	#define __MMU_MACHINE_CODED__
	#define __PROCESS_MACHINE_CODED__
 	#define __TBMA_MACHINE_CODED__
	#define __MESSAGE_MACHINE_CODED_2__
	#define	__HANDLES_MACHINE_CODED__
	#define __REQUEST_COMPLETE_MACHINE_CODED__
	#define __MESSAGE_MACHINE_CODED__
#endif

#endif
#endif
#endif

// size of each buffer used to store entropy data before it is passed to the RNG
const TUint KEntropyBufferSizeWords = 1024;// maximum size of 1024 words (32Kbit);

const TInt KKernelServerDefaultPriority=16;
const TInt KDefaultExitPriority=KKernelServerDefaultPriority;

// counter to ensure that priority inheritance has bounded execution time
const TInt KMaxPriorityInheritanceNesting=10;


const TInt KArgIndex = 16;	//number of slots in DProcess for passing info to child process on creation

const TInt KDefaultMaxMemCopyInOneGo = 512;

// Forward declare Shareable data buffer classes
class DShPool;
class TShPoolCreateInfo;

//Forward declare Debug namespace for StopMode API
namespace Debug { class StopModeDebug; }

/***************************************************
 * Kernel-side trap harness for catching exceptions
 * Not nestable, one per thread only
 ***************************************************/
class TExcTrap;
typedef void (*TExcTrapHandler)(TExcTrap*,DThread*,TAny*);

class TExcTrap
	{
public:
	IMPORT_C TInt Trap();	// use default exception handler
	IMPORT_C TInt Trap(TExcTrapHandler aHandler);
	inline void UnTrap();
	IMPORT_C void Exception(TInt aResult);
public:
	TUint32 iState[EXC_TRAP_CTX_SZ];
	TExcTrapHandler iHandler;
	DThread* iThread;
	};

class TPagingExcTrap
	{
public:
	TInt Trap();
	inline void UnTrap();
	void Exception(TInt aResult);
public:
	TUint32 iState[EXC_TRAP_CTX_SZ];
	DThread* iThread;
	};

#define XT_DEFAULT
#define XTRAP(_r,_h,_s)	{							\
						TExcTrap __t;				\
						if (((_r)=__t.Trap(_h))==0)	\
							{						\
							_s;						\
							__t.UnTrap();			\
							}						\
						}

#define XTRAPD(_r,_h,_s) TInt _r;					\
						{							\
						TExcTrap __t;				\
						if (((_r)=__t.Trap(_h))==0)	\
							{						\
							_s;						\
							__t.UnTrap();			\
							}						\
						}

#ifdef __DEMAND_PAGING__

#define CHECK_PAGING_SAFE				__NK_ASSERT_DEBUG(M::CheckPagingSafe(EFalse))

#define CHECK_PAGING_SAFE_RANGE(s, l)	__NK_ASSERT_DEBUG(M::CheckPagingSafe(EFalse, (s), (l)))

#define CHECK_DATA_PAGING_SAFE				__NK_ASSERT_DEBUG(M::CheckPagingSafe(ETrue))

#define CHECK_DATA_PAGING_SAFE_RANGE(s, l)	__NK_ASSERT_DEBUG(M::CheckPagingSafe(ETrue, (s), (l)))

//
// Important - Do not use XTRAP inside an XTRAP_PAGING
// I.e.
//		XTRAP(XTRAP_PAGING(foo))
// is OK, but
//		XTRAP_PAGING(XTRAP(foo))
// will put the system into a bad state when a paging exception happens
// because TExcTrap::UnTrap won't get called.
//

#define XTRAP_PAGING(_r,_s)	{								\
							TPagingExcTrap __t;				\
							if (((_r)=__t.Trap())==0)		\
								{							\
								_s;							\
								__t.UnTrap();				\
								}							\
							}

#define XTRAP_PAGING_START(_r)	{								\
								TPagingExcTrap __t;				\
								if (((_r)=__t.Trap())==0)		\
									{

#define XTRAP_PAGING_END			__t.UnTrap();				\
									}							\
								}

#define XTRAP_PAGING_RETRY(_s)	{											\
								_retry:										\
									TInt _exc;								\
									XTRAP_PAGING_START(_exc);				\
									_s;										\
									XTRAP_PAGING_END;						\
									if(_exc) goto _retry;					\
								}

#else

#define XTRAP_PAGING(_r,_s)		{ _s }
#define XTRAP_PAGING_START(_r)	{_r=0;{ 
#define XTRAP_PAGING_END			  }} 
#define CHECK_PAGING_SAFE
#define CHECK_PAGING_SAFE_RANGE(s, l)
#define CHECK_DATA_PAGING_SAFE
#define CHECK_DATA_PAGING_SAFE_RANGE(s, l)
#define XTRAP_PAGING_RETRY(_s)  { _s }

#endif

class TIpcExcTrap : public TExcTrap
	{
public:
	TInt Trap(DThread* aClientThread); 	// use default ipc exception handler
	inline void UnTrap();
	enum TExcLocation { EExcUnknown, EExcLocal, EExcRemote };
	TExcLocation ExcLocation(DThread* aThread, TAny* aContext);
	TBool IsTIpcExcTrap();
public:
	TLinAddr iLocalBase;
	TLinAddr iRemoteBase;
	TUint32 iSize;
	TInt iDir;		// 0=read, 1=write
	};



/**************************************************
 * Structure used for miscellaneous notifications
 **************************************************/
class TMiscNotifier : public TClientRequest
	{
public:
	SDblQueLink		iThreadLink;
	SDblQueLink		iObjLink;
	DThread*		iThread;		// doesn't need to be reference counted since all notifiers are removed when thread terminates
private:
	~TMiscNotifier();				// call Close(), don't delete
	};

struct SMiscNotifierQ : public SDblQue
	{
	};

class TMiscNotifierMgr
	{
public:
	TMiscNotifierMgr();
	void Init2();
	void CompleteNotifications(SDblQue& aQ);
	static void CompletionDfcFn(TAny* aMgr);
	static void IdleDfcFn(TAny* aMgr);
	inline void Lock()
		{NKern::FMWait(&iLock);}
	inline void Unlock()
		{NKern::FMSignal(&iLock);}
	TInt NewMiscNotifier(TRequestStatus* aStatus, TBool aObj, TAny* aPtr, TMiscNotifier*& aN, SMiscNotifierQ*& aQ);
public:
	SDblQue			iCompleted;
	TDfc			iCompletionDfc;
	SMiscNotifierQ*	iIdleNotifierQ;
	TDfc			iIdleDfc;
	NFastMutex		iLock;
	};



/********************************************
 * Kernel object index (handles array)
 ********************************************/
class DMutex;

/** Handle analysis and synthesis
@internalComponent
*/
inline TBool HandleIsSpecial(TInt aHandle)
	{ return aHandle<0; }
inline TBool HandleIsThreadLocal(TInt aHandle)
	{ return aHandle&0x40000000; }
inline TBool HandleNoClose(TInt aHandle)
	{ return aHandle&0x00008000; }
inline TInt HandleIndex(TInt aHandle)
	{ return aHandle&0x00007FFF; }
inline TInt HandleInstance(TInt aHandle)
	{ return (aHandle&0x3FFF0000)>>16; }
inline TInt MakeHandle(TInt aIndex, TInt aInstance)
	{ return ((aInstance&0x3FFF)<<16)|(aIndex&0x7FFF); }


/**
@internalComponent
*/
class RObjectIx
	{
public:
	enum {ENoClose=KHandleNoClose,ELocalHandle=0x40000000};
	enum {EReserved=0x80000000u, EAttributeMask=0xfffu};
	enum {EMinSlots=8, EMaxSlots=32768};
public:
	enum {ENumFreeQ=6, EModCount=4, EBitMapSize=128, EMaxLockedIter=8};
private:
	// queue numbers
	enum {EQFree=-6, EQAltFree=-5, EQTempFree=-4, EQRsvd=-3, EQAltRsvd=-2, EQTempRsvd=-1};
	// iObjR, iRsvd fields
	enum {EObjROccupied=4u, EObjRObjMask=0xfffffffcu, EObjRRsvd=1u};
	// states
	enum {ENormal=0u, ETidying=1u, EFindingLast=2u, ECounting=3u, ESearching=4u, ETerminated=5u};
	struct	SSlotQLink
		{
		TInt16		iNext;	// pointer to next free slot, -n if no next
		TInt16		iPrev;	// pointer to previous free slot, -n if no previous
		};
	struct	SFreeSlot : public SSlotQLink
		{
		TUint32		iRsvd;	// 0 for normal, 1 for reserved slot
		};
	struct	SUsedSlot
		{
		TUint32		iAttr;	// bits 0-13 = instance (nonzero), bits 14-19 = object type, bits 20-31 = handle attributes
		TUint32		iObjR;	// pointer to object (nonzero), bit 0=1 if reserved slot
		};
	union SSlot
		{
		SFreeSlot	iFree;
		SUsedSlot	iUsed;
		};
	struct SMonitorObj
		{
		DObject*	iObj;
		TInt		iBoundary;
		TInt		iResult;
		};
	union SModList
		{
		TInt16		iIndex[EModCount];
		TUint32		iBitMap[EBitMapSize/32];
		SMonitorObj	iMonitor;
		};
private:
#ifdef __HANDLES_USE_RW_SPIN_LOCK__
// Beginning of support for spinlock-only protection (i.e. not requiring the system lock)
// for access to handles.  Requires changes everywhere objects returned from handles are
// used, and some additional work in the handle lookup code in cutils.cia.
#error "spinlocks for handle lookup not supported"
	inline void AcquireReadLock()
		{ __SPIN_LOCK_IRQ_R(iRWL); }
	inline void ReleaseReadLock()
		{ __SPIN_UNLOCK_IRQ_R(iRWL); }
	inline void AcquireWriteLock()
		{ __SPIN_LOCK_IRQ_W(iRWL); }
	inline void ReleaseWriteLock()
		{ __SPIN_UNLOCK_IRQ_W(iRWL); }
#else
	/* Places which use a read lock would already have the system lock held */
	inline void AcquireReadLock()
		{ __ASSERT_SYSTEM_LOCK; }
	inline void ReleaseReadLock()
		{  }
	inline void AcquireWriteLock()
		{ NKern::LockSystem(); }
	inline void ReleaseWriteLock()
		{ NKern::UnlockSystem(); }
#endif
private:
	static inline DObject* Occupant(SSlot* aS)
		{ return (DObject*)(aS->iUsed.iObjR & EObjRObjMask); }
	static inline TBool IsReserved(SSlot* aS)
		{ return aS->iUsed.iObjR & EObjRRsvd; }
	static inline TBool IsFreeReserved(SSlot* aS)
		{ return (aS->iUsed.iObjR & EObjRRsvd) && (aS->iUsed.iObjR<EObjROccupied); }
	void Empty(TInt aQueue);
	SSlot* Dequeue(TInt aSlotIndex);
	inline void AddHead(TInt aQueue, TInt aSlotIndex);
	inline void AddTail(TInt aQueue, TInt aSlotIndex);
	void AddBefore(TInt aBase, TInt aSlotIndex);
	void AddAfter(TInt aBase, TInt aSlotIndex);
	void AppendList(TInt aSrcQ, TInt aDestQ);
	void PrependList(TInt aSrcQ, TInt aDestQ);
	TInt DoAdd(DObject* aObj, TUint32 aAttr, SSlot* aSlot);	// add aObj using an existing slot (don't grow)
	TInt DoRemove(TInt aHandle, DObject*& aObject, TUint32& aAttr);	// remove a handle (don't shrink)
	void MarkModified(TInt aSlotIndex);
	static TUint32 GetNextInstanceValue();
	TInt UnReserveSlots(TInt aCount, TBool aAmortize);
	TInt ReserveSlots(TInt aCount);
	TInt Grow(TInt aReserve, SSlot* aSlotData);
	void TidyAndCompact();
	inline SSlotQLink* Link(TInt aIndex)
		{ return (aIndex<0) ? (iFreeQ+ENumFreeQ+aIndex) : &(iSlots+aIndex)->iFree; }
public:
	// common operations
	RObjectIx();
	TInt Close(TAny* aPtr);

	TInt Add(DObject* aObj, TUint32 aAttr);
	TInt Remove(TInt aHandle, DObject*& aObject, TUint32& aAttr);
	TInt Reserve(TInt aCount);
	DObject* At(TInt aHandle, TInt aUniqueID, TUint32 aRequiredAttr);
	DObject* At(TInt aHandle, TInt aUniqueID, TUint32* aAttr);
	DObject* At(TInt aHandle, TInt aUniqueID);
	DObject* At(TInt aHandle);
	static void Wait();
	static void Signal();
	inline TInt Count()
		{ return iCount; }
	inline TInt ActiveCount()
		{ return iActiveCount; }
public:
	// uncommon operations
	DObject* operator[](TInt aIndex);
	TInt At(DObject* aObject);
	TInt Count(DObject* aObject);
	TInt LastHandle();
#ifdef DOBJECT_TEST_CODE
	typedef void (*TValidateEntry)(TInt, TInt, TInt, TInt, TUint, DObject*);
	void Check(TValidateEntry aV);
#endif
private:
	TRWSpinLock		iRWL;
	TInt			iAllocated;			// Max entries before realloc needed
	volatile TInt	iCount;				// Points to at least 1 above the highest occupied slot or unoccupied reserved slot
	volatile TInt	iActiveCount;		// Number of occupied entries in the index (reserved or normal)
	volatile TInt	iReservedFree;		// Number of unoccupied reserved slots
	volatile TInt	iReservedTotal;		// Number of reserved slots (occupied or unoccupied)
	volatile TInt	iReservedFreeHWM;	// Points to at least 1 above the last unoccupied reserved slot
	SSlotQLink		iFreeQ[ENumFreeQ];	// queues of free slots
	SSlot*			iSlots;				// array of handle slots
	TInt			iAmortize;			// Number of handle removals before we see if we can shrink
	TUint8			iState;
	TUint8			iModCount;			// 255=not in use, 0...EModCount->use iModList.iIndex[], EModCount+1->use iModList.iBitMap
	TUint8			iModListShift;
	TUint8			iSpare1;
	SModList		iModList;			// Entries modified while array moving
public:
	static volatile TUint32 NextInstance;
	static DMutex* HandleMutex;
public:
	friend void PreprocessHandler();
	friend class DThread;
	friend class K;
	friend class Monitor;
	};
//
// These are the same as their e32base.h versions
/*inline TBool IsLocalHandle(TInt aHandle)
	{return(aHandle&RObjectIx::ELocalHandle);}
inline void SetLocalHandle(TInt &aHandle)
	{aHandle|=RObjectIx::ELocalHandle;}
inline void UnSetLocalHandle(TInt &aHandle)
	{aHandle&=(~RObjectIx::ELocalHandle);}
*/
/********************************************
 * Kernel object container
 ********************************************/
class DObjectCon : public DBase
	{
protected:
	enum {ENotOwnerID};
public:
	~DObjectCon();
	static DObjectCon* New(TInt aUniqueID);
	IMPORT_C void Remove(DObject* aObj);
	IMPORT_C TInt Add(DObject* aObj);
	IMPORT_C DObject* operator[](TInt aIndex);
	IMPORT_C DObject* At(const TFindHandle& aFindHandle);
	IMPORT_C TInt CheckUniqueFullName(DObject* aOwner, const TDesC& aName);
	IMPORT_C TInt CheckUniqueFullName(DObject* aObject);
	IMPORT_C TInt FindByName(TFindHandle& aFindHandle, const TDesC& aMatch, TKName& aName);
	IMPORT_C TInt FindByFullName(TFindHandle& aFindHandle, const TDesC& aMatch, TFullName& aFullName);
	IMPORT_C TInt OpenByFullName(DObject*& aObject, const TDesC& aMatch);
	inline TInt UniqueID() {return iUniqueID;}
	inline TInt Count() {return iCount;}
	inline void Wait() {Kern::MutexWait(*iMutex);}
	inline void Signal() {Kern::MutexSignal(*iMutex);}
	inline DMutex* Lock() {return iMutex;}
protected:
	DObjectCon(TInt aUniqueID);
	TBool NamesMatch(DObject* aObject, const TDesC& aObjectName, DObject* aCurrentObject);
public:
	TInt iUniqueID;
private:
	TInt iAllocated;
	TInt iCount;
	DObject** iObjects;
	DMutex* iMutex;
public:
	friend class Monitor;
	friend class Debugger;
	friend class SCMonitor;
	friend class Debug::StopModeDebug;
	};

/********************************************
 * Process and process-relative thread priorities
 ********************************************/
enum TThrdPriority
	{
	EThrdPriorityIdle=-8,
	EThrdPriorityMuchLess=-7,
	EThrdPriorityLess=-6,
	EThrdPriorityNormal=-5,
	EThrdPriorityMore=-4,
	EThrdPriorityMuchMore=-3,
	EThrdPriorityRealTime=-2,

	EThrdPriorityNull=0,
	EThrdPriorityAbsoluteVeryLow=1,
	EThrdPriorityAbsoluteLowNormal=3,
	EThrdPriorityAbsoluteLow=5,
	EThrdPriorityAbsoluteBackgroundNormal=7,
	EThrdPriorityAbsoluteBackground=10,
	EThrdPriorityAbsoluteForegroundNormal=12,
	EThrdPriorityAbsoluteForeground=15,
	EThrdPriorityAbsoluteHighNormal=19,
	EThrdPriorityAbsoluteHigh=23,
	EThrdPriorityAbsoluteRealTime1=24,
	EThrdPriorityAbsoluteRealTime2=25,
	EThrdPriorityAbsoluteRealTime3=26,
	EThrdPriorityAbsoluteRealTime4=27,
	EThrdPriorityAbsoluteRealTime5=28,
	EThrdPriorityAbsoluteRealTime6=29,
	EThrdPriorityAbsoluteRealTime7=30,
	EThrdPriorityAbsoluteRealTime8=31
	};

enum TProcPriority
	{
	EProcPriorityLow=0,
	EProcPriorityBackground=1,
	EProcPriorityForeground=2,
	EProcPriorityHigh=3,
	EProcPrioritySystemServer1=4,
	EProcPrioritySystemServer2=5,
	EProcPrioritySystemServer3=6,
	EProcPriorityRealTimeServer=7,
	};

/********************************************
 * Multipurpose timer
 ********************************************/
#ifndef MAX
#define MAX(a,b)		((a)<(b)?(b):(a))
#endif
#ifndef MAX3
#define MAX3(a,b,c)		MAX(MAX(a,b),c)
#endif
#ifndef MAX4
#define MAX4(a,b,c,d)	MAX(MAX(a,b),MAX(c,d))
#endif
#define TIMER_SIZE		MAX4(sizeof(TTickLink),sizeof(TSecondLink),sizeof(NTimer),sizeof(TInactivityLink))

class TTimer
	{
public:
	enum TTimerType {ERelative=1,EAbsolute=2,ELocked=4,EHighRes=8,EInactivity=16};
	enum TTimerState {EIdle,EWaiting,EWaitHighRes};
public:
	TInt Create();
	inline TTimer();
	~TTimer();
	TInt After(TInt aInterval, TTickCallBack aFunction, TRequestStatus& aStatus);
	TInt At(const TTimeK& aTime, TSecondCallBack aFunction, TRequestStatus& aStatus);
	TInt AfterHighRes(TInt aInterval, NTimerFn aFunction, TRequestStatus& aStatus);
	TInt AgainHighRes(TInt aInterval, NTimerFn aFunction, TRequestStatus& aStatus);
	TInt Inactivity(TInt aSeconds, TInactivityCallBack aFunction, TRequestStatus& aStatus);
	void Cancel(DThread* aThread);
	void Abort(DThread* aThread, TInt aTypeMask);
	void SetType(TTimerType aType);
public:
	inline TTickLink& TickLink()
		{ return *(TTickLink*)iUnion; }
	inline TSecondLink& SecondLink()
		{ return *(TSecondLink*)iUnion; }
	inline NTimer& Ms()
		{ return *(NTimer*)iUnion; }
	inline TInactivityLink& Inact()
		{ return *(TInactivityLink*)iUnion; }
	inline TTimerType Type()
		{ return (TTimerType)iType; }
public:
	TUint32 iUnion[TIMER_SIZE/sizeof(TUint32)];	// this field must always be 8-byte aligned
	TClientRequest* iRequest;
	TUint8 iState;
	TUint8 iType;
	};

inline TTimer::TTimer()
	: iState(EIdle)
	{}

/********************************************
 * Timer control block
 ********************************************/
class DTimer : public DObject
	{
public:
	TInt Create(DThread* aThread);
public:
	DTimer();
	~DTimer();
public:
	void Cancel();
	TInt After(TRequestStatus& aStatus, TInt aInterval);
	TInt At(TRequestStatus& aStatus, const TTimeK& aTime);
	TInt Lock(TRequestStatus& aStatus, TTimerLockSpec aLock);
	void HighRes(TRequestStatus& aStatus, TInt aInterval);
	void AgainHighRes(TRequestStatus& aStatus, TInt aInterval);
	TInt Inactivity(TRequestStatus& aStatus, TInt aSeconds);
	void Abort(TBool aAbortAbsolute);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
private:
	inline DThread* Owner();
	static void TimerComplete(TAny* aPtr);
	static void SecondComplete(TAny* aPtr);
	static void LockComplete(TAny* aPtr);
	static void LockSynchronize(TAny* aPtr);
	static void MsComplete(TAny* aTimer);
	inline static DTimer* FromPtr(TAny* aPtr);
public:
//	TAny* __padding;	// for alignment
	TTimer iTimer;
public:
	friend class Monitor;
	};

__ASSERT_COMPILE((_FOFF(DTimer,iTimer.iUnion) & 7)==0);

inline DTimer* DTimer::FromPtr(TAny* aPtr)
	{ return _LOFF(aPtr,DTimer,iTimer); }
inline DThread* DTimer::Owner()
	{return (DThread*)DObject::Owner();}


/********************************************
 * Thread cleanup entries
 ********************************************/
class DThread;
class TThreadCleanup : public TPriListLink
	{
public:
	IMPORT_C TThreadCleanup();
	void ChangePriority(TInt aNewPriority);
	IMPORT_C void Remove();
	virtual void Cleanup()=0;
public:
	DThread* iThread;
public:
	friend class Monitor;
	};

class TThreadMutexCleanup : public TThreadCleanup
	{
public:
	inline TThreadMutexCleanup(DMutex* aMutex);
	virtual void Cleanup();
	DMutex *iMutex;
public:
	friend class Monitor;
	};

/********************************************
 * Deterministic thread wait queue
 ********************************************/
class TThreadWaitList
	{
private:
	enum { EEmpty=0u, EFlagList=1u, EInitValue=3u, ENonEmptyMask=0xfffffffcu };
private:
	class TList : public TPriListBase
		{
	private:
		TList() : TPriListBase(KNumPriorities) {}
		inline TList*& Next()
			{ return *(TList**)this; }
	private:
		SDblQueLink* iExtraQueues[KNumPriorities-1];

		friend class TThreadWaitList;
		friend class Monitor;
		};
public:
	inline TThreadWaitList()
		{ iWaitPtr=EInitValue; }
	~TThreadWaitList();
	TInt Construct();
	inline TBool NonEmpty() const
		{ return (iWaitPtr & ENonEmptyMask); }
	DThread* First() const;
	TInt HighestPriority() const;
	void Add(DThread* aThread);
	void Remove(DThread* aThread);
	void ChangePriority(DThread* aThread, TInt aNewPriority);
	static TInt ThreadCreated();
	static void ThreadDestroyed();
private:
	static TInt Up(TBool aThread);
	static void Down(TBool aThread);
#ifdef _DEBUG
	void Check() const;
#endif
private:
	TLinAddr	iWaitPtr;

	static TList* FirstFree;	// first free list
	static TInt NLists;			// number of TLists in existence
	static TInt NWaitObj;		// number of wait objects in existence which use TThreadWaitList
	static TInt NThrd;			// number of DThreads in existence
	};


/********************************************
 * Semaphore control block
 ********************************************/
class DSemaphore : public DObject
	{
public:
	TInt Create(DObject* aOwner, const TDesC* aName, TInt aInitialCount, TBool aVisible=ETrue);
public:
	~DSemaphore();
	void WaitCancel(DThread* aThread);
	void WaitCancelSuspended(DThread* aThread);
	void SuspendWaitingThread(DThread* aThread);
	void ResumeWaitingThread(DThread* aThread);
	void ChangeWaitingThreadPriority(DThread* aThread, TInt aNewPriority);
public:
	TInt Wait(TInt aNTicks);
	void Signal();
	void SignalN(TInt aCount);
	void Reset();
public:
	TInt iCount;
	TUint8 iResetting;
	TUint8 iPad1;
	TUint8 iPad2;
	TUint8 iPad3;
	SDblQue iSuspendedQ;
	TThreadWaitList iWaitQ;
public:
	friend class Monitor;
	};

/********************************************
 * Mutex control block
 ********************************************/
class DMutex : public DObject
	{
public:
	TInt Create(DObject* aOwner, const TDesC* aName, TBool aVisible, TUint aOrder);
public:
	DMutex();
	~DMutex();
	TInt HighestWaitingPriority();
	void WaitCancel(DThread* aThread);
	void WaitCancelSuspended(DThread* aThread);
	void SuspendWaitingThread(DThread* aThread);
	void ResumeWaitingThread(DThread* aThread);
	void ChangeWaitingThreadPriority(DThread* aThread, TInt aNewPriority);
	void SuspendPendingThread(DThread* aThread);
	void RemovePendingThread(DThread* aThread);
	void ChangePendingThreadPriority(DThread* aThread, TInt aNewPriority);
	void WakeUpNextThread();
public:
	TInt Wait(TInt aTimeout=0);	// 0 means wait forever, -1 means poll, n>0 means n nanokernel ticks
	void Signal();
	void Reset();
public:
	TInt iHoldCount;			// number of times held by current thread
	TInt iWaitCount;			// number of waiting threads
	TUint8 iResetting;
	TUint8 iOrder;
	TUint8 iPad1;
	TUint8 iPad2;
	TThreadMutexCleanup iCleanup;
	SDblQue iSuspendedQ;
	SDblQue iPendingQ;
	TThreadWaitList iWaitQ;
#ifdef _DEBUG
	SDblQueLink iOrderLink;		// used for acquisition order checking
#endif
public:
	friend class Monitor;
	};

/********************************************
 * Condition variable control block
 ********************************************/
class DCondVar : public DObject
	{
public:
	TInt Create(DObject* aOwner, const TDesC* aName, TBool aVisible);
public:
	DCondVar();
	~DCondVar();
	void WaitCancel(DThread* aThread);
	void WaitCancelSuspended(DThread* aThread);
	void SuspendWaitingThread(DThread* aThread);
	void ResumeWaitingThread(DThread* aThread);
	void ChangeWaitingThreadPriority(DThread* aThread, TInt aNewPriority);
public:
	TInt Wait(DMutex* aMutex, TInt aTimeout);
	void Signal();
	void Broadcast(DMutex* aMutex);
	void Reset();
	void UnBlockThread(DThread* aThread, TBool aUnlock);
public:
	TUint8 iResetting;
	TUint8 iPad1;
	TUint8 iPad2;
	TUint8 iPad3;
	DMutex* iMutex;
	TInt iWaitCount;
	SDblQue iSuspendedQ;
	TThreadWaitList iWaitQ;
public:
	friend class Monitor;
	};

/********************************************
 * Kernel representation of session message
 ********************************************/

class DSession;
class RMessageK : public TClientRequest
	{
public:
	inline TInt Arg(TInt aParam) const;
	inline TAny* Ptr(TInt aParam) const;
	inline TInt ArgType(TInt aParam) const;
	inline TUint IsDescriptor(TInt aParam) const;
	inline const TDesHeader& Descriptor(TInt aParam) const;
	inline TDesHeader& Descriptor(TInt aParam);
	inline TInt DesLength(TInt aParam) const;
	inline TInt DesMaxLength(TInt aParam) const;
	IMPORT_C DThread* Thread() const;
	IMPORT_C static RMessageK* MessageK(TInt aHandle);
	static RMessageK* MessageK(TInt aHandle, DThread* aThread);
	void OpenRef();
	void CloseRef();

public:
	// message is always in one of the following message states,
	// a change of state requires the system lock.
	inline TBool IsFree() const;
	inline TBool IsInitialising() const;
	inline TBool IsDelivered() const;
	inline TBool IsAccepted() const;
	inline TBool IsCompleting() const;
	inline void SetFree();
	inline void SetInitialising();
	inline void SetDelivered(SDblQue& aDeliveredQ);
	inline void SetAccepted(DProcess* aServerProcess);
	inline void SetCompleting();

	// WARNING: Some ARM assembler code makes assumptions about the *ordering* of these enum values!
	enum TMsgType {EDisc=32, ESync, ESession, EGlobal};
	static RMessageK* GetNextFreeMessage(DSession* aSession);
	static RMessageK* ClaimMessagePool(enum TMsgType aType, TInt aCount, DSession* aSession);
	void ReleaseMessagePool(enum TMsgType aType, TInt aMax);

private:
	friend class DSession;
	friend class ExecHandler;
	class TMsgArgs
		{
		friend class RMessageK;
		friend class DSession;
		friend class ExecHandler;
		friend class TServerMessage;
		void ReadDesHeaders(const TInt aArgsPtr[KMaxMessageArguments+1]);
		inline TInt ArgType(TInt aParam) const;
		inline TUint IsDescriptor(TInt aParam) const;
		inline TUint AllDescriptorFlags() const;
		inline TUint AllPinFlags() const;
		inline TUint AllDesWritten() const;
		inline void SetArgUndefined(TInt aParam);
		inline void SetDesWritten(TInt aParam);

		inline TBool IsDesWritten(TInt aParam) const;

		TUint		iArgFlags;						// describes which arguments are descriptors/handles etc.
		TInt		iArgs[KMaxMessageArguments];	// the arguments themselves
		TDesHeader	iDesInfo[KMaxMessageArguments];	// info for any descriptor args

		// The iArgFlags words has a strange layout. The bottom 16 bits have to match those
		// TIpcArgs::iFlags, which -- from LSB upwards -- is KMaxMessageArguments (4) fields
		// of KBitsPerType (3) each, describing arguments 0-3, followed by 4 bits requesting
		// pinning of the corresponding (descriptor) argument.  We then use the next 4 bits
		// to record whether each (descriptor) argument has been written.

		enum
			{
			KArgFlagMask		= (1 << TIpcArgs::KBitsPerType) - 1,
			KAllArgTypesMask	= (1 << (TIpcArgs::KBitsPerType*KMaxMessageArguments)) - 1,
			KAllIpcFlagsMask	= (KAllArgTypesMask | TIpcArgs::KPinMask),
			KAllDesArgMask		= (TIpcArgs::EFlagDes << (0 * TIpcArgs::KBitsPerType)) |
								  (TIpcArgs::EFlagDes << (1 * TIpcArgs::KBitsPerType)) |
								  (TIpcArgs::EFlagDes << (2 * TIpcArgs::KBitsPerType)) |
								  (TIpcArgs::EFlagDes << (3 * TIpcArgs::KBitsPerType)),

			KDesWrittenShift	= (TIpcArgs::KBitsPerType*KMaxMessageArguments + TIpcArgs::KPinArgShift),
			KDesWrittenMask		= ((1 << KMaxMessageArguments) - 1) << KDesWrittenShift
			};
		};

private:
	RMessageK();						// call GetNextFreeMessage() or ClaimMessagePool() instead
	~RMessageK();						// Don't delete, call CloseRef()
	void Free();						// only called from last CloseRef()
	static TInt ExpandMessagePool();
	static void CallbackFunc(TAny* aData, TUserModeCallbackReason aReason);
	TInt PinDescriptors(DSession* aSession, TBool aPinningServer);

public:
	TUint8			iMsgType;			// where this message belongs - one of the TMsgType values above
	TUint8			iAccessCount;
	DSession*		iSession;			// pointer to session
	SDblQueLink		iSessionLink;		// attaches message to session or freelist
	// WARNING: Some ARM assembler code makes assumptions about the layout of this structure!
	SDblQueLink		iServerLink;		// Also represents message state!
	TInt			iFunction;	
	TMsgArgs		iMsgArgs;
	DThread*		iClient;			// pointer to client thread (not reference counted, see below)

	// An encapsulated array of pointers to pin objects associated with any descriptor arguments.
	struct TPinArray
		{
		TVirtualPinObject* iPinPtrs[KMaxMessageArguments];
		}			*iPinArray;			// Pointer to (heap-allocated) array of pointers to pin objects

public:
	enum { KMessageSize = 128 };		// sizeof(RMessageK) rounded up to a power of 2
	};

inline TInt RMessageK::TMsgArgs::ArgType(TInt aParam) const
	{
	return (iArgFlags >> (aParam * TIpcArgs::KBitsPerType)) & KArgFlagMask;
	}
inline TUint RMessageK::TMsgArgs::IsDescriptor(TInt aParam) const
	{
	return (iArgFlags >> (aParam * TIpcArgs::KBitsPerType)) & TIpcArgs::EFlagDes;
	}
inline TUint RMessageK::TMsgArgs::AllDescriptorFlags() const
	{
	return iArgFlags & KAllDesArgMask;
	}
inline TUint RMessageK::TMsgArgs::AllPinFlags() const
	{
	return (iArgFlags & TIpcArgs::KPinMask) >> TIpcArgs::KPinArgShift;
	}
inline TUint RMessageK::TMsgArgs::AllDesWritten() const
	{
	return (iArgFlags & KDesWrittenMask) >> KDesWrittenShift;
	}
inline void RMessageK::TMsgArgs::SetArgUndefined(TInt aParam)
	{
	iArgFlags &= ~(KArgFlagMask << (aParam * TIpcArgs::KBitsPerType));
	iArgFlags |= TIpcArgs::EUnspecified << (aParam * TIpcArgs::KBitsPerType);
	}
inline void RMessageK::TMsgArgs::SetDesWritten(TInt aParam)
	{
	iArgFlags |= 1 << (aParam + KDesWrittenShift);
	}

inline TInt RMessageK::Arg(TInt aParam) const
	{
	return iMsgArgs.iArgs[aParam];
	}
inline TAny* RMessageK::Ptr(TInt aParam) const
	{
	return (TAny*)Arg(aParam);
	}
inline TInt RMessageK::ArgType(TInt aParam) const
	{
	return iMsgArgs.ArgType(aParam);
	}
inline TUint RMessageK::IsDescriptor(TInt aParam) const
	{
	return iMsgArgs.IsDescriptor(aParam);
	}
inline const TDesHeader& RMessageK::Descriptor(TInt aParam) const
	{
	return iMsgArgs.iDesInfo[aParam];
	}
inline TDesHeader& RMessageK::Descriptor(TInt aParam)
	{
	return iMsgArgs.iDesInfo[aParam];
	}
inline TInt RMessageK::DesLength(TInt aParam) const
	{
	return Descriptor(aParam).Length();
	}
inline TInt RMessageK::DesMaxLength(TInt aParam) const
	{
	return Descriptor(aParam).MaxLength();
	}
inline TBool RMessageK::IsFree() const
	{
	__ASSERT_SYSTEM_LOCK;
	return !iServerLink.iNext;
	}
inline TBool RMessageK::IsInitialising() const
	{
	__ASSERT_SYSTEM_LOCK;
	return iServerLink.iNext == (TAny*)2;
	}
inline TBool RMessageK::IsDelivered() const
	{
	__ASSERT_SYSTEM_LOCK;
	return iServerLink.iNext != 0 && (TLinAddr(iServerLink.iNext) & 3) == 0;
	}
inline TBool RMessageK::IsAccepted() const
	{
	__ASSERT_SYSTEM_LOCK;
	return ((TLinAddr)iServerLink.iNext & 3) == 3;
	}
inline TBool RMessageK::IsCompleting() const
	{
	__ASSERT_SYSTEM_LOCK;
	return iServerLink.iNext == (TAny*)1;
	}
inline void RMessageK::SetFree()
	{
	__ASSERT_SYSTEM_LOCK;
	iServerLink.iNext = 0;
	}
inline void RMessageK::SetInitialising()
	{
	__ASSERT_SYSTEM_LOCK;
	iServerLink.iNext = (SDblQueLink*)2;
	}
inline void RMessageK::SetDelivered(SDblQue& aDeliveredQ)
	{
	__ASSERT_SYSTEM_LOCK;
	aDeliveredQ.Add(&iServerLink);
	}
inline void RMessageK::SetAccepted(DProcess* aServerProcess)
	{
	__ASSERT_SYSTEM_LOCK;
	iServerLink.iNext = (SDblQueLink*)~TUint32(this);
	iServerLink.iPrev = (SDblQueLink*)~TUint32(aServerProcess);
	}
inline void RMessageK::SetCompleting()
	{
	__ASSERT_SYSTEM_LOCK;
	iServerLink.iNext = (SDblQueLink*)1;
	}

/********************************************
 * Thread control block
 ********************************************/
class DChunk;
class DProcess;
class TTrap;

const TInt KMaxThreadContext=256;
typedef void (*TUnknownStateHandler)(DThread*,TInt,TInt);

class DThread : public DObject
	{
public:
	enum {EDefaultUserTimeSliceMs = 20};

	enum TThreadState
		{
		ECreated,
		EDead,
		EReady,
		EWaitSemaphore,
		EWaitSemaphoreSuspended,
		EWaitMutex,
		EWaitMutexSuspended,
		EHoldMutexPending,
		EWaitCondVar,
		EWaitCondVarSuspended,
		};

	enum TOperation
		{
		ESuspend=0,
		EResume=1,
		EForceResume=2,
		EReleaseWait=3,
		EChangePriority=4,
		};

	enum TUserThreadState
		{
		EUserThreadNone,
		EUserThreadCreated,
		EUserThreadRunning,
		EUserThreadExiting,
		};
	   
public:
	DThread();
	void Destruct();
	TInt Create(SThreadCreateInfo& aInfo);
	TInt SetPriority(TThreadPriority aPriority);
	TInt MakeHandle(TOwnerType aType, DObject* aObject, TInt& aHandle);
	TInt MakeHandle(TOwnerType aType, DObject* aObject, TInt& aHandle, TUint aAttr);
	TInt MakeHandleAndOpen(TOwnerType aType, DObject* aObject, TInt& aHandle);
	TInt MakeHandleAndOpen(TOwnerType aType, DObject* aObject, TInt& aHandle, TUint aAttr);
	TInt HandleClose(TInt aHandle);
	TInt OpenFindHandle(TOwnerType aType, const TFindHandle& aFindHandle, TInt& aHandle);
	TInt OpenObject(TOwnerType aType, const TDesC& aName, TInt& aHandle, DObject*& aObj, TInt aObjType);
	IMPORT_C DObject* ObjectFromHandle(TInt aHandle);
	IMPORT_C DObject* ObjectFromHandle(TInt aHandle, TInt aObjType);
	IMPORT_C DObject* ObjectFromHandle(TInt aHandle, TInt aObjType, TUint& aAttr);
	void RequestComplete(TRequestStatus*& aStatus, TInt aReason);
	TInt SetTls(TInt aHandle, TInt aDllUid, TAny* aPtr);
	TAny* Tls(TInt aHandle, TInt aDllUid);
	void FreeTls(TInt aHandle);
	TInt Rename(const TDesC& aName);
	TBool IsExceptionHandled(TExcType aType);
	TInt RaiseException(TExcType aType);
	void SetExitInfo(TExitType aType, TInt aReason, const TDesC& aCategory);
	void CloseCreatedHeap();
	void RemoveClosingLibs();
	inline TBool HasCapability(/*TInt aCapability*/);
	TBool IsRealtime();
	void SetRealtimeState(TThreadRealtimeState aNewState);
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool HasCapability(TCapability aCapability, const char* aDiagnosticText=0);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool HasCapability(TCapability aCapability, OnlyCreateWithNull aDiagnostic=NULL);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
private:
	IMPORT_C TBool DoHasCapability(TCapability aCapability, const char* aDiagnosticText);
	IMPORT_C TBool DoHasCapability(TCapability aCapability);
public:
	// not memory model dependent
	virtual TInt Context(TDes8& aDes)=0;
public:
	inline void Stillborn();
	void Release();
	TInt CalcDefaultThreadPriority();
	IMPORT_C void AddCleanup(TThreadCleanup* aCleanup);
	void SetRequiredPriority();
	void SetActualPriority(TInt aActualPriority);
	void RevertPriority();
	void SvKill();
	IMPORT_C TInt ReleaseWait(TInt aReturnCode);
	IMPORT_C void CancelTimer();
	void ExtractMiscNotifiers(SDblQue& aQ, TRequestStatus* aStatus);
	void KillMiscNotifiers();
public:
	// not memory model dependent
	TInt DoCreate(SThreadCreateInfo& aInfo);
	IMPORT_C void SetThreadPriority(TInt aThreadPriority);
	void SetDefaultPriority(TInt aDefaultPriority);
	void AbortTimer(TBool aAbortAbsolute);
	void Suspend(TInt aCount);
	void Resume();
	void ForceResume();
	void Exit();
	void Die(TExitType aType, TInt aReason, const TDesC& aCategory);
	TInt Logon(TRequestStatus* aStatus, TBool aRendezvous);
	void Rendezvous(TInt aReason);
	void CleanupLeave(TInt aDepth = 1);
#ifdef __SMP__
	static void SMPSafeCallback(TAny* aThisPtr, TUserModeCallbackReason aReasonCode);
#endif
public:
	// memory model dependent
	TInt AllocateSupervisorStack();
	void FreeSupervisorStack();
	void FreeUserStack();
	TInt AllocateUserStack(TInt aSize, TBool aPaged);
	DChunk* OpenSharedChunk(const TAny* aAddress, TBool aWrite, TInt& aOffset);
	DChunk* OpenSharedChunk(TInt aHandle,TBool aWrite);
	TInt PrepareMemoryForDMA(const TAny* aAddress, TInt aSize, TPhysAddr* aPageList);
	TInt ReleaseMemoryFromDMA(const TAny* aAddress, TInt aSize, TPhysAddr* aPageList);

	static void IpcExcHandler(TExcTrap* aTrap, DThread* aThread, TAny* aContext);
	virtual void DoExit1();
public:
	// pure virtual
	virtual TInt SetupContext(SThreadCreateInfo& aInfo)=0;
	virtual void DoExit2()=0;
protected:
	virtual void SetPaging(TUint& aCreateFlags);
public:
	inline void UnknownState(TInt aOperation, TInt aParameter);
	inline static DThread* FromTimer(TAny* aPtr);
	static void ExitCurrentThread();
	static void DefaultUnknownStateHandler(DThread* aThread, TInt aOperation, TInt aParameter);
	static void EpocThreadFunction(TAny* aPtr);
	static TDfc* EpocThreadExitHandler(NThread* aThread);
	static void EpocThreadTimeoutHandler(NThread* aThread, TInt aOp);
public:
 	virtual void BTracePrime(TInt aCategory);

private:
	// Functions that access user-side memory
	TInt GetDesInfo(const TAny* aDes, TInt& aLength, TInt& aMaxLength, TUint8*& aPtr, TBool aWriteable);
	TInt GetDesLength(const TAny* aPtr);
	TInt GetDesMaxLength(const TAny* aPtr);
	TInt DesRead(const TAny* aPtr, TUint8* aDes, TInt aMax, TInt aOffset, TInt aMode);
	TInt DesWrite(const TAny* aPtr, const TUint8* aDes, TInt aLength, TInt aOffset, TInt aMode, DThread* aOriginatingThread);
	TInt DoDesRead(const TDesHeader& aDesInfo, TUint8* aDes, TInt aMax, TInt aOffset, TInt aMode);
	TInt DoDesWrite(const TAny* aPtr, TDesHeader& aDesInfo, const TUint8* aDes, TInt aLength, TInt aOffset, TInt aMode, DThread* aOriginatingThread);
	// (the following are memory model dependent)
	TInt RawRead(const TAny* aSrc, TAny* aDest, TInt aLength, TInt aFlags, TIpcExcTrap* aExcTrap);
	TInt RawWrite(const TAny* aDest, const TAny* aSrc, TInt aLength, TInt aFlags, DThread* aOriginatingThread, TIpcExcTrap* aExcTrap);
	TInt ReadAndParseDesHeader(const TAny* aSrc, TDesHeader& aDest);

public:
	TUint32 iIpcCount;
	TLinAddr iUserStackRunAddress;
	TInt iUserStackSize;
	TUint32 iFlags;
	DProcess* iOwningProcess;
	SDblQueLink iProcessLink;
	TInt iThreadPriority;
	RObjectIx iHandles;
	TUint iId;
	RAllocator* iAllocator;
	RAllocator* iCreatedAllocator;
	TTrap* iFrame;
	TTrapHandler* iTrapHandler;
	RArray<STls> iTls;
	CActiveScheduler* iScheduler;
	TExceptionHandler iExceptionHandler;
	TUint iExceptionMask;
	TExcTrap* iExcTrap;
	TInt iDebugMask;
	TThreadMessage iKernMsg;
	DObject* iTempObj;
	DObject* iExtTempObj;
	TAny* iTempAlloc;
	SDblQue iOwnedLogons;
	SDblQue iTargetLogons;
	RMessageK* iSyncMsgPtr;
	TDfc iKillDfc;
	SDblQue iClosingLibs;
	TPriListLink iWaitLink;
//	TUint8 iMState;							// use iSpare1 for iMState
//	TUint8 iExiting;						// use iSpare2 for iExiting
//	TUint8 iWaitListReserved;				// use iSpare3 for iWaitListReserved
	TInt iDefaultPriority;					// default scheduling priority
	TAny* iWaitObj;							// object on which this thread is waiting
	TUnknownStateHandler iUnknownStateHandler;	// handler for extra thread states - used by RTOS personality layers
	TAny* iExtras;							// pointer to extra data used by RTOS personality layers
	TAny* iSupervisorStack;					// thread's supervisor mode stack
	TInt iSupervisorStackSize;
	TUint8 iSupervisorStackAllocated;
	TUint8 iThreadType;
	TUint8 iExitType;
	TUint8 iEmiFlags;
	TInt iExitReason;
	TBufC<KMaxExitCategoryName> iExitCategory;
	SDblQue iMutexList;						// list of held mutexes, used only for acquisition order checking
	TPriList<TThreadCleanup,KNumPriorities> iCleanupQ;	// things to clean up when we die
	TTimer iTimer;
	TInt iLeaveDepth;						// recursive depth of User::Leave() calls
	TAny* iEmiStartMonitor;
	NThread iNThread;
	TPagingExcTrap* iPagingExcTrap;
	TUserThreadState iUserThreadState;
	SDblQue iMiscNotifiers;					// miscellaneous notifiers owned by this thread
	DThread* iIpcClient;					// client thread when doing IPC, used if realtime therad takes fault
#ifdef __SMP__
	TUserModeCallback iSMPSafeCallback;
#endif
	RMessageK* iTempMsg;

public:
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
#ifdef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// This exported Kern:: function takes diagnostic strings that
	// should still work by calling the DoHasCapability function here even
	// though we have removed diagnostic strings from ekern proper
	friend TBool Kern::DoCurrentThreadHasCapability(TCapability aCapability, const char* aContextText);
#endif //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

	friend class Monitor;
	friend class Kern;
	friend class ExecHandler;
	friend class TClientBuffer;
	};

__ASSERT_COMPILE((_FOFF(DThread,iTimer.iUnion) & 7)==0);

inline DThread* DThread::FromTimer(TAny* aPtr)
	{ return _LOFF(aPtr,DThread,iTimer); }
inline void DThread::UnknownState(TInt aOperation, TInt aParameter)
	{ (*iUnknownStateHandler)(this,aOperation,aParameter); }
inline void DThread::Stillborn()
	{iNThread.Stillborn();Release();}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
inline TBool DThread::HasCapability(TCapability aCapability, const char* aDiagnosticText)
	{
	return DoHasCapability(aCapability, aDiagnosticText);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
// Only available to NULL arguments
inline TBool DThread::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/)
	{
	return DoHasCapability(aCapability);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

#define TheCurrentThread	_LOFF(NCurrentThread(),DThread,iNThread)

class TLogon : public TClientRequest
	{
public:
	enum TType
		{
		ETargetThread=0,
		ETargetProcess=1,
		ERendezvous=0x80,
		};
	TInt Attach(SDblQue& aList, DThread* aOwner, DObject* aTarget, TRequestStatus* aStatus, TUint32 aType);
	static TInt Cancel(SDblQue& aList, DObject* aTarget, TRequestStatus* aStatus, TUint32 aType);

	enum TComplete { ETargetRendezvous, ETarget, EOwned };
	static void CompleteAll(SDblQue& aList, TComplete aAction, TInt aReason);

	static NFastMutex LogonLock;

private:
	inline static void Lock()
		{ NKern::FMWait(&LogonLock); }
	inline static void Unlock()
		{ NKern::FMSignal(&LogonLock); }

	TUint32 iType;
	inline TBool IsRendezvous()
		{ return (iType & ERendezvous); }

	DThread* iOwningThread;
	DObject* iTarget;
	SDblQueLink iOwningThreadLink;
	SDblQueLink iTargetLink;
private:
	~TLogon();					// call Close(), don't delete
	};

/********************************************
 * Platform-independent chunk type abstraction
 ********************************************/
enum TChunkType
	{
	// these never change or move or anything
	EKernelData = 0,	// Supervisor,Rw,Cacheable
	EKernelStack = 1,	// Supervisor,Rw,Cacheable
	EKernelCode = 2,	// Supervisor,Rw,Cacheable
	EDll = 3,			// User,Ro,Cacheable
	EUserCode = 4,

	// This changes on a PDE basis when the file server runs
	ERamDrive = 5,		// Supervisor,Rw,Cacheable

	// these change on PDE basis when run
	EUserData = 6,
	EDllData = 7,
	EUserSelfModCode = 8,

	ESharedKernelSingle = 9,		// Must be same value as TChunkCreateInfo::ESharedKernelSingle
	ESharedKernelMultiple = 10,		// Must be same value as TChunkCreateInfo::ESharedKernelMultiple

	ESharedIo = 11,
	ESharedKernelMirror = 12,

	EKernelMessage = 13,			// Dedicated chunk for RMessageK objects

	ENumChunkTypes	// Keep as last value
	};

/********************************************
 * Chunk creation info block
 ********************************************/
struct SChunkCreateInfo
	{
	enum {EAdjust=1, EAdd=2};	// iOperations

	TBool iGlobal;
	TInt iAtt;				// Set with values from TChunkCreate::TChunkCreateAtt NOT DChunk::TChunkAttributes
	TBool iForceFixed;
	TUint iOperations;
	TLinAddr iRunAddress;
	TChunkType iType;
	TInt iMaxSize;
	TInt iInitialBottom;
	TInt iInitialTop;
	TInt iPreallocated;
	TPtrC iName;
	DObject* iOwner;
	TUint32 iMapAttr;
	TDfc* iDestroyedDfc;
	TUint8 iClearByte;
public:
	SChunkCreateInfo();
	};

/** The different types of pages that the allocator should consider
*/
enum TZonePageType
	{
	EPageUnknown = 0, /**<Pages that are at physical addresses the kernel has no knowledge of*/ 
	EPageFixed,		/**<Pages that shouldn't be physically moved*/
	EPageMovable,	/**<Pages that may be physically moved*/
	EPageDiscard,	/**<Pages that may be removed*/
	EPageTypes		/**<The no of different page types*/
	};

/********************************************
 * Chunk control block
 ********************************************/
class DChunk : public DObject
	{
public:
	/**
	@see DMemModelChunk::TMemModelChunkAttributes for memory model specific attribute flags
	*/	
	enum TChunkAttributes
		{
		ENormal			=0x00,
		EDoubleEnded	=0x01,
		EDisconnected	=0x02,
		EConstructed	=0x04,
		EMemoryNotOwned	=0x08,
		ETrustedChunk	=0x10,
		EDataPaged		=0x20,		// Set when the chunk is data paged.
		ECache			=0x40,
		EReadOnly		=0x80,
		EChunkAttributesMask = 0xff,
		};

	enum TCommitType
		{
		ECommitDiscontiguous			= 0,
		ECommitContiguous				= 1,
		ECommitPhysicalMask				= 2,
		ECommitDiscontiguousPhysical	= ECommitDiscontiguous|ECommitPhysicalMask,
		ECommitContiguousPhysical		= ECommitContiguous|ECommitPhysicalMask,
		ECommitVirtual					= 4,  // only supported by moving and multiple memory models on arm
		};

	enum TDecommitType
		{
		EDecommitNormal					= 0,
		EDecommitVirtual				= 1,
		};
	
	DChunk();
	~DChunk();
	TInt Create(SChunkCreateInfo& aInfo);
	inline TInt Size() const {return iSize;}
	inline TInt MaxSize() const {return iMaxSize;}
#ifndef __MEMMODEL_FLEXIBLE__
	inline TUint8 *Base() const {return iBase;}
#endif
	inline TInt Bottom() const {return iStartPos;}
	inline TInt Top() const {return iStartPos+iSize;}
	inline DProcess* OwningProcess() const {return iOwningProcess;}
protected:
	virtual void SetPaging(TUint aCreateAtt);
public:
	virtual TInt AddToProcess(DProcess* aProcess);
	virtual TInt DoCreate(SChunkCreateInfo& aInfo)=0;
	virtual TInt Adjust(TInt aNewSize)=0;
	virtual TInt AdjustDoubleEnded(TInt aBottom, TInt aTop)=0;
	virtual TInt CheckAccess()=0;
	virtual TInt Commit(TInt aOffset, TInt aSize, TCommitType aCommitType=DChunk::ECommitDiscontiguous, TUint32* aExtraArg=0)=0;
	virtual TInt Allocate(TInt aSize, TInt aGuard=0, TInt aAlign=0)=0;
	virtual TInt Decommit(TInt aOffset, TInt aSize)=0;
	virtual TInt Lock(TInt aOffset, TInt aSize)=0;
	virtual TInt Unlock(TInt aOffset, TInt aSize)=0;
	virtual TInt Address(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress)=0;
	virtual TInt PhysicalAddress(TInt aOffset, TInt aSize, TLinAddr& aKernelAddress, TUint32& aPhysicalAddress, TUint32* aPhysicalPageList=NULL)=0;
 	virtual void BTracePrime(TInt aCategory);
	virtual void Substitute(TInt aOffset, TPhysAddr aOldAddr, TPhysAddr aNewAddr)=0;
	virtual TUint8* Base(DProcess* aProcess)=0;
public:
	DProcess* iOwningProcess;	// NULL for global chunks
	TInt iSize;					// always the committed size (i.e. gaps not included)
	TInt iMaxSize;
#ifndef __MEMMODEL_FLEXIBLE__
	TUint8* iBase;
#else
	TLinAddr iFixedBase;
#endif
	TInt iAttributes;
	TInt iStartPos;				// start of committed area for double-ended chunks
	TUint iControllingOwner;	// ProcessID of process which set protection
	TUint iRestrictions;
	TUint iMapAttr;				// only for shared kernel chunks
	TDfc* iDestroyedDfc;
	TChunkType iChunkType;
	TUint8 iClearByte;			// The byte value to clear all memory committed to the chunk to.
public:
	friend class Monitor;
	};

/********************************************
 * Platform security
 ********************************************/

/**
@deprecated
*/
#define __SECURE_KERNEL(p) { p; };

/**
@deprecated
*/
#define __KERNEL_CAPABILITY_CHECK(p) { p; };

/** TCompiledSecurityPolicy uses the upper two bits in iCaps to store a reduced
version of TSecurityPolicy's iType.  This constant is the number of free bits.
@internalTechnology
*/
const TUint32 KCSPBitsFree = 30;

/** TCompiledSecurityPolicy can only represent capabilities less than
KCSPBitsFree-1 
*/
__ASSERT_COMPILE(ECapability_Limit <= KCSPBitsFree);

/** A precompiled security policy useful for fast checking of policies.
@internalComponent
*/
class TCompiledSecurityPolicy
	{
public:
	enum TType
		{
		ETypeFail = 0,
		ETypeCapsOnly = 1,
		ETypeSecureId= 2,
		ETypeVendorId= 3,
		};
	enum TMask
		{
		EMaskFail = TUint32 (ETypeFail) << KCSPBitsFree, 
		EMaskCapsOnly = TUint32 (ETypeCapsOnly) << KCSPBitsFree,
		EMaskSecureId = TUint32 (ETypeSecureId) << KCSPBitsFree,
		EMaskVendorId = TUint32 (ETypeVendorId) << KCSPBitsFree,
		};
	TInt Set(const TSecurityPolicy& aPolicy);
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool CheckPolicy(DProcess* aProcess, const char* aDiagnostic=0) const
		{
		return DoCheckPolicy(aProcess, aDiagnostic);
		}

	inline TBool CheckPolicy(DThread* aThread, const char* aDiagnostic=0) const
		{
		return DoCheckPolicy(aThread, aDiagnostic);
		}
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool CheckPolicy(DProcess* aProcess, OnlyCreateWithNull /*aDiagnostic*/=NULL) const
		{
		return DoCheckPolicy(aProcess);
		}

	inline TBool CheckPolicy(DThread* aThread, OnlyCreateWithNull /*aDiagnostic*/=NULL) const
		{
		return DoCheckPolicy(aThread);
		}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

private:
#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
	TBool DoCheckPolicy(DProcess* aProcess, const char* aDiagnostic) const;
	TBool DoCheckPolicy(DThread* aThread, const char* aDiagnostic) const;
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__
	TBool DoCheckPolicy(DProcess* aProcess) const;
	TBool DoCheckPolicy(DThread* aThread) const;

	TBool CheckPolicy(const SSecurityInfo& aSecInfo, SSecurityInfo& aMissing) const;
	void AddCapability(TCapability aCap);

	inline void SetETypeFail() { /* Yes, it's a NOP */iCaps |= EMaskFail; }
	inline void SetETypePass() { iCaps |= EMaskCapsOnly; }
	inline void SetETypeCapsOnly() { iCaps |= EMaskCapsOnly; }
	inline void SetETypeSecureId() { iCaps |= (TUint32)EMaskSecureId; }
	inline void SetETypeVendorId() { iCaps |= (TUint32)EMaskVendorId; }

	inline TUint32 Caps() const { return iCaps & ~EMaskVendorId; }
	inline TUint32 Type() const { return iCaps >> KCSPBitsFree; }
	union
		{
		TUint32 iSecureId;
		TUint32 iVendorId;
		};
	TUint32 iCaps;
	};

 /********************************************
 * Process control block
 ********************************************/
struct SCodeSegEntry
	{
	DCodeSeg* iSeg;		// code segment in use
	DLibrary* iLib;		// client representation object (DLibrary or DxxDevice)
	};

class DProcess : public DObject
	{
public:
	/** Possible values for bitmask specifying how to iterate dependency graph.
		@see TraverseCodeSegs
		@publishedPartner
		@released
	 */
	enum TTraverseFlags 
		{
		/** Add code segment to queue if set, remove it if cleared */
		ETraverseFlagAdd=1,  	
		/** Restrict iteration to dynamic dependencies in ELoaded state if set, 
		    iterate everything if cleared */
		ETraverseFlagRestrict=2	
		};
public:
	DProcess();
	~DProcess();
	TInt Create(TBool aKernelProcess, TProcessCreateInfo& aInfo, HBuf* aCommandLine);
	TInt SetPriority(TProcessPriority aPriority);
	TInt ConvertPriority(TProcessPriority aPriority);
	void Destruct();
	TInt Rename(const TDesC& aName);
	TInt Logon(TRequestStatus* aStatus, TBool aRendezvous);
	void Rendezvous(TInt aReason);
	IMPORT_C TInt TraverseCodeSegs(SDblQue* aQ, DCodeSeg* aExclude, TUint32 aMark, TUint32 aFlags);
	TInt OpenDeps();
	TInt AddCodeSeg(DCodeSeg* aSeg, DLibrary* aLib, SDblQue& aQ);
	void RemoveCodeSeg(DCodeSeg* aCodeSeg, SDblQue* aQ);
	TInt CallList(SDblQue& aQ, DCodeSeg* aSeg, TUint32 aFlag);
	DCodeSeg* CodeSeg();
	TBool HasCapabilityNoDiagnostic(TCapability aCapability);
	inline TBool HasCapability(/*TInt aCapability*/);
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool HasCapability(TCapability aCapability, const char* aDiagnosticText=0);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool HasCapability(TCapability aCapability, OnlyCreateWithNull aDiagnostic=NULL);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
private:
	IMPORT_C TBool DoHasCapability(TCapability aCapability, const char* aDiagnostic);
	IMPORT_C TBool DoHasCapability(TCapability aCapability);
protected:
	virtual TInt SetPaging(const TProcessCreateInfo& aInfo);
public:
	virtual void DoAppendName(TDes& aName);
	virtual TInt NewChunk(DChunk*& aChunk, SChunkCreateInfo& aInfo, TLinAddr& aRunAddr)=0;
	virtual TInt AddChunk(DChunk* aChunk,TBool isReadOnly)=0;
	virtual TInt NewShPool(DShPool*& aPool, TShPoolCreateInfo& aInfo)=0;
	virtual TInt DoCreate(TBool aKernelProcess, TProcessCreateInfo& aInfo)=0;
	virtual TInt AttachExistingCodeSeg(TProcessCreateInfo& aInfo)=0;
	virtual TInt Loaded(TProcessCreateInfo& aInfo);
	virtual void FinalRelease()=0;
public:
	TInt NewThread(DThread*& aThread, SThreadCreateInfo& aInfo, TInt* aHandle, TOwnerType aType);
	virtual void Resume();
	void Die(TExitType aType,TInt aReason,const TDesC &aCategory);
	virtual void Release();
	TBool KillAllThreads(TExitType aType, TInt aReason, const TDesC &aCategory);

public:
	virtual TInt GetNewThread(DThread*& aThread, SThreadCreateInfo& aInfo)=0;
	virtual TInt CreateDataBssStackArea(TProcessCreateInfo& aInfo)=0;
	virtual TInt MapCodeSeg(DCodeSeg* aCodeSeg)=0;
	virtual void UnmapCodeSeg(DCodeSeg* aCodeSeg)=0;
	virtual void RemoveDllData()=0;
	virtual void BTracePrime(TInt aCategory);
public:
	TInt WaitProcessLock();
	TInt SignalProcessLock();
	TInt WaitDllLock();
	void SignalDllLock();
	void AddThread(DThread &aThread);
#ifdef __SMP__
	TInt UpdateSMPSafe();
#endif
	inline DThread* FirstThread()
		{ return _LOFF(iThreadQ.First(),DThread,iProcessLink); }
public:
	TInt iPriority;
	SDblQue iThreadQ;
	TUint8 iExitType;
	TUint8 iFinalReleaseFlag;
	TUint8 iPad2;
	TUint8 iDebugAttributes;
	TInt iExitReason;
	TBufC<KMaxExitCategoryName> iExitCategory;
	RObjectIx iHandles;
	TUidType iUids;
	TInt iGeneration;
	TUint iId;
	TUint32 iFlags;
	HBuf* iCommandLine;
	DProcess* iOwningProcess;
	SDblQue iTargetLogons;
	RArray<SCodeSegEntry> iDynamicCode;
//	RPointerArray<DLibrary> iDynamicCode;
	SSecurityInfo iS;
	SSecurityInfo iCreatorInfo;
	TUint iCreatorId;
	TUint iSecurityZone;
	TInt iEnvironmentData[KArgIndex];

public:
	enum TProcessAttributes	{
							EPrivate		= 0x00000002,
							ESupervisor		= (TInt)0x80000000,
							EBeingLoaded	= 0x08000000,
							EResumed		= 0x00010000,
							EDataPaged 		= 0x00000004,	// Set when the process is data paged
							};
	TInt iAttributes;
	TLinAddr iDataBssRunAddress;
	DChunk* iDataBssStackChunk;
	DCodeSeg* iCodeSeg;
	DCodeSeg* iTempCodeSeg;
	DMutex* iProcessLock;
	DMutex* iDllLock; // can be held while in user mode 
	TLinAddr iReentryPoint;	// user address to jump to for new threads, exceptions
	SDblQue iGarbageList;	// list of DLibrary containing code which will be unmapped later
	TInt iThreadsLeaving; // count of threads currently leaving [only w/C++ exceptions]
	TInt iUserThreadsRunning; // count of running user threads that have not yet called User::Exit
	volatile TInt iSMPUnsafeCount;   // count of top level codesegs that are SMP unsafe
							         // i.e. the exe itself or things RLibrary::Load'ed - not deps
#ifdef __SMP__
	NThreadGroup* iSMPUnsafeGroup;	// group used to keep unsafe threads scheduled together
#endif
public:
	friend class Monitor;

	};

inline TBool DProcess::HasCapabilityNoDiagnostic(TCapability aCapability)
	{
	if(aCapability==ECapability_None)
		return ETrue;
	if((TUint32)aCapability<(TUint32)ECapability_Limit)
		return (iS.iCaps[aCapability>>5]>>(aCapability&31))&1;
	return EFalse;
	}

inline TBool DProcess::HasCapability(/*TInt aCapability*/)
	{ return iS.iCaps[0]!=~1u; } // Always return true for now

inline TBool DThread::HasCapability(/*TInt aCapability*/)
	{ return iOwningProcess->HasCapability(/*aCapability*/); }
 
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
inline TBool DProcess::HasCapability(TCapability aCapability, const char* aDiagnosticText)
	{
	return DoHasCapability(aCapability, aDiagnosticText);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
// Only available to NULL arguments
inline TBool DProcess::HasCapability(TCapability aCapability, OnlyCreateWithNull /*aDiagnostic*/)
	{
	return DoHasCapability(aCapability);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__


/********************************************
 * Session control block
 ********************************************/
class DServer;
class DSession : public DObject
	{
public:
	DSession();
	virtual ~DSession();
	TInt Add(DServer* aSvr, const TSecurityPolicy* aSecurityPolicy);
	void Transfer(DServer* aServer, RMessageK* aMsg);
	TInt MakeHandle();
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	virtual TInt Close(TAny*);
	void Detach(TInt aReason);
	void CloseFromDisconnect();
	inline TBool IsClosing();
	inline void TotalAccessInc()
		{__ASSERT_SYSTEM_LOCK; iTotalAccessCount++;};
	TInt TotalAccessDec();
	TInt TotalAccessDecRel();
	void BTracePrime(TInt aCategory);

public:
	// Static methods called from exec handlers
	static TInt New(DSession*& aS, TInt aMsgSlots, TInt aMode);
	static TInt Send(TInt aHandle, TInt aFunction, const TInt* aPtr, TRequestStatus* aStatus);
	static TInt SendSync(TInt aHandle, TInt aFunction, const TInt* aPtr, TRequestStatus* aStatus);

private:
	TInt Send(RMessageK* aMsg, TInt aFunction, const RMessageK::TMsgArgs* aArgs, TRequestStatus* aStatus);
	RMessageK* GetNextFreeMessage();

public:
	DServer* 		iServer;				// pointer to kernel-side server object
	SDblQueLink 	iServerLink;			// link to attach session to server
	const TAny* 	iSessionCookie;			// pointer to server-side CSession2 (user cookie)
	TUint8 			iSessionType;			// TIpcSessionType
	TUint8 			iSvrSessionType;		// TIpcSessionType
	TUint8 			iHasSessionPool;		// TIpcSessionType
	TUint8	 		iTotalAccessCount;		// all client accesses together contribute 1, modify with system locked
	TInt 			iPoolSize;				// number of messages in private pool
	TInt 			iMsgCount;				// total number of outstanding messages on this session
	TInt 			iMsgLimit;				// maximum number of outstanding messages on this session
	SDblQue 		iMsgQ;					// queue of all outstanding messages on this session (by iSessionLink)
	RMessageK*		iDisconnectMsgPtr;		// preallocated disconnect message
	RMessageK* volatile	iConnectMsgPtr;		// pointer to connect message, if any
	RMessageK* volatile	iNextFreeMessage;	// pointer to next free message in per-session message pool, if any
	};

inline TBool DSession::IsClosing()
	{ return !AccessCount(); }

/********************************************
 * Server control block
 ********************************************/

class TServerMessage : public TClientRequest
	{
public:
	TServerMessage();
private:
	static void CallbackFunc(TAny* aData, TUserModeCallbackReason aReason);
private:
	TAny* iMessagePtr;					// where to deliver messages
	RMessageK* iMessageData;			// message to deliver
	friend class DServer;
private:
	~TServerMessage();					// call Close(), don't delete
	};

class DServer : public DObject
	{
public:
	DServer();
	TInt Create();
	virtual ~DServer();
	virtual TInt Close(TAny*);
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	void Receive(TRequestStatus& aStatus, TAny* aMessage);	// aMessage bit 0 = 0 -> RMessage, bit 0 = 1 -> RMessage2
	void Cancel();
	void Accept(RMessageK* aMsg);
	void Deliver(RMessageK* aMsg);
	void BTracePrime(TInt aCategory);
public:
	inline TBool IsClosing();
public:
	DThread*		iOwningThread;		// thread which receives messages
	TServerMessage*	iMessage;
	SDblQue			iSessionQ;			// list of sessions
	SDblQue			iDeliveredQ;		// list of messages delivered but not yet accepted
	TUint8			iSessionType;		// TIpcSessionType
	TUint8 			iPinClientDescriptors;	// Set to 1 to make the server pin client descriptor 
										// arguments it receives, 0 otherwise.
	TUint8 			iServerRole;		// TIpcServerRole: Standalone, Master, or Slave?
	TUint8 			iPadding;
	};

inline TBool DServer::IsClosing()
	{ return !AccessCount(); }


struct TCodeSegLoaderCookieList
	{
	TCodeSegLoaderCookieList* iNext;
	TCodeSegLoaderCookie iCookie;
	};

/********************************************
 * Code segment
 ********************************************/

/** Code segment

	Low-level abstraction describing an executable or DLL used by one or more
	processes.  Higher-level abstraction (DLibrary, DPhysicalDevice,
	DLogicalDevice and DProcess) use code segments internally.

	Each code segment maintains a table of all code segments it directly
	depends on, thus forming a dependency graph.

	Note that dependency information about ROM-based DLLs without static 
	data is lost during the ROM building process.  Therefore These DLLs are not
	included in the dependency graph.

	@internalTechnology
 */

class DCodeSeg : public DBase
	{
public:
	/** Possible values for DCodeSeg::iMark bitmask.
		All values except explicitly specified are reserved for internal 
		kernel use.
	 */
	enum TMark
		{
		EMarkListDeps=0x01,
		EMarkUnListDeps=0x02,
		EMarkLdr=0x04,
		EMarkProfilerTAG=0x08,			// used by profiler tool to mark processed non-XIP objects.
		EMarkLoaded=0x10,				// this code segment is reloadable
		EMarkRecursiveFlagsValid=0x20,	// the recursively computed bits are valid
		EMarkRecursiveFlagsCheck=0x40,	// the recursively computed bits are being checked or are valid
		EMarkData=0x04000000u,			// image has data and is not extension or variant (=KRomImageFlagData)
		EMarkDataInit=0x02000000u,		// graph below and including image requires user init (=KRomImageFlagDataInit)
		EMarkDataPresent=0x01000000u,	// graph below and including image has data (=KRomImageFlagDataPresent)
		EMarkDebug=0x08000000u,			// reserved for debug agents
		};
public:
	DCodeSeg();
	void Destruct();
	inline void Open() {++iAccessCount;}
	void Close();
	void CheckedClose();
	void CheckedOpen();
	void WaitCheckedOpen();
	TInt Create(TCodeSegCreateInfo& aInfo, DProcess* aProcess);
	TInt ListDeps(SDblQue* aQ, TUint32 aMark);
	TInt UnListDeps(TUint32 aMark);
	TInt MarkAndOpenDeps(TUint32 aMark);
	void FinaliseRecursiveFlags();
	void CalcRecursiveFlags();
	void SetAttachProcess(DProcess* aProcess);
public:
// virtual
	virtual void Info(TCodeSegCreateInfo& aInfo);
	virtual TInt Loaded(TCodeSegCreateInfo& aInfo);
	virtual TInt AddDependency(DCodeSeg* aExporter);
	virtual void BTracePrime(TInt aCategory);
// Pure virtual
	virtual TLibraryFunction Lookup(TInt aOrdinal) =0;
	virtual TInt GetMemoryInfo(TModuleMemoryInfo& aInfo, DProcess* aProcess) =0;
	virtual TInt DoCreate(TCodeSegCreateInfo& aInfo, DProcess* aProcess)=0;
	virtual void InitData()=0;
	virtual void ReadExportDir(TUint32* aDest)=0;
	virtual TBool FindCheck(DProcess* aProcess)=0;
	virtual TBool OpenCheck(DProcess* aProcess)=0;
public:
	static void Wait();
	static void Signal();
	IMPORT_C static DCodeSeg* VerifyHandle(TAny* aHandle);
	static DCodeSeg* VerifyHandleP(TAny* aHandle);
	static DCodeSeg* VerifyCallerAndHandle(TAny* aHandle);
	static DCodeSeg* FindCodeSeg(const TFindCodeSeg& aFind);
	static DCodeSeg* FindRomCode(const TAny* aRomImgHdr);
	IMPORT_C static void UnmarkAll(TUint32 aMask);
	static void UnmarkAndCloseAll(TUint32 aMask);
	static TInt ListMarked(SDblQue& aQ, TUint32 aMask);
	IMPORT_C static void EmptyQueue(SDblQue& aQ, TUint32 aMask);
	static void EmptyGarbageList();
	static void DeferDeletes();
	static void EndDeferDeletes();
	static TInt WriteCallList(SDblQue& aList, TLinAddr* aEpList, TBool aInitData);
	IMPORT_C static DCodeSeg* CodeSegFromEntryPoint(TInt aEntryPoint);
	static void DoKernelCleanup(TAny*);
	void ScheduleKernelCleanup(TBool aImmed);
	static void QueueKernelCleanup();
	static void DeferKernelCleanup();
	static void EndDeferKernelCleanup();
	void AppendFullRootName(TDes& aDes);
	void AppendFullFileName(TDes& aDes);
	void AppendVerExt(TDes& aDes);
	void TraceAppendFullName(TDes& aDes);
	TLinAddr ExceptionDescriptor();
public:
	inline TBool IsDll() const
		{return iUids.iUid[0].iUid==KDynamicLibraryUidValue;}
	inline TBool IsExe() const
		{return iUids.iUid[0].iUid==KExecutableImageUidValue;}
public:
	SDblQueLink iLink;		// link to global list
	SDblQueLink iTempLink;	// temporary link for computing lists
	SDblQueLink iGbgLink;	// link to garbage list
	TUint32 iGbgIdleGenerationCount;
	TInt iAccessCount;
	TLinAddr iEntryPtVeneer;	// address of first instruction to be called
	TLinAddr iFileEntryPoint;	// address of entry point within this code segment
	HBuf* iFileName;		// Full path name
	TPtrC iRootName;		// Filename.Ext - points into iFileName
	TInt iExtOffset;		// Offset of extension in iFileName
	TUidType iUids;
	DCodeSeg** iDeps;		// code segments on which this one depends (not reference counted)
	TInt iDepCount;			// number of dependencies
	TInt iNextDep;
	TUint32 iMark;
	TUint32 iAttr;
	DCodeSeg* iExeCodeSeg;		// EXE code segment with which this can be used
	DProcess* iAttachProcess;	// Process with which this can be used (NULL if several)
	TUint32 iModuleVersion;
	SSecurityInfo iS;
	TLinAddr iRunAddress;
	TUint32 iSize;
	
public:
	static SDblQue GlobalList;			// list of all existing DCodeSeg
	static SDblQue GarbageList;			// list of DCodeSeg whose deletion is pending due to DeferDeletes()
	static SDblQue KernelGarbageList;   // list of DCodeSeg containing kernel code which will be unmapped when the system goes idle
	static SDblQue DeferredKernelGarbageList;
	static TDfc KernelCleanupDfc;
	static TInt KernelCleanupLock;
	static DMutex* CodeSegLock;
	static TInt DeleteLock;
	static RPointerArray<DCodeSeg> CodeSegsByName;		// list of code segments in name order

	class RCodeSegsByAddress
		{
	public:
		RCodeSegsByAddress(TInt aMinGrowBy, TInt aFactor);
		TInt Add(DCodeSeg* aCodeSeg);
		TInt Remove(DCodeSeg* aCodeseg);
		DCodeSeg* Find(TLinAddr aAddress);
	private:
		static TInt Compare(const DCodeSeg& aA, const DCodeSeg& aB);
	private:
		TInt iActive;
		RPointerArray<DCodeSeg> iArray[2];
		};
	static RCodeSegsByAddress CodeSegsByAddress;	// list of code segments in run address order
	static TClientRequest* DestructNotifyRequest;	// Used to complete NotifyIfCodeSegDestroyed
	static DThread* DestructNotifyThread;			// Used to complete NotifyIfCodeSegDestroyed
	static TCodeSegLoaderCookieList* DestructNotifyList; //Used to hold the list of cookie for destroyed codsegs.
	};

/********************************************
 * Library control block
 ********************************************/
class DLibrary : public DObject
	{
public:
	enum TState
		{
		ECreated=0,			// initial state
		ELoaded=1,			// code segment loaded
		EAttaching=2,		// calling constructors
		EAttached=3,		// all constructors done
		EDetachPending=4,	// about to call destructors
		EDetaching=5,		// calling destructors
		};
public:
	static TInt New(DLibrary*& aLib, DProcess* aProcess, DCodeSeg* aSeg);
	DLibrary();
	void RemoveFromProcess();
	void ReallyRemoveFromProcess();
    virtual ~DLibrary();
	virtual TInt Close(TAny* aPtr);
	virtual TInt AddToProcess(DProcess* aProcess);
	virtual void DoAppendName(TDes& aName);
public:
	TInt iMapCount;
	TUint8 iState;
	SDblQueLink iThreadLink;	// attaches to opening or closing thread
	DCodeSeg* iCodeSeg;
	SDblQueLink iGbgLink;
	};

/********************************************
 * Change notifier control block
 ********************************************/
class DChangeNotifier : public DObject
	{
public:
	DChangeNotifier();
	~DChangeNotifier();
	TInt Create();
	void Notify(TUint aChanges);
	TInt Logon(TRequestStatus& aStatus, DThread* aThread);
	TInt LogonCancel(DThread* aThread);
	void Complete(TInt aResult);
public:
	TUint iChanges;
	TClientRequest* iRequest;	
	DThread* iThread;
public:
	friend class Monitor;
	};

/********************************************
 * Undertaker control block
 ********************************************/

class DUndertaker : public DObject
	{
public:
	TInt Create();
	~DUndertaker();
	void Notify(DThread* aDeadThread);
	TInt Logon(TRequestStatus *aStatus, TInt* aDeadThreadHandle);
	TInt LogonCancel();
	void Complete(TInt aValue, DThread* aDeadThread);
public:
	TClientDataRequest<TInt>* iRequest;
	DThread* iOwningThread;
public:
	friend class Monitor;
	};

/********************************************
 * Shared Io Buffers
 ********************************************/

/**
@publishedPartner
@released

	A kernel object for sharing memory between kernel code and user space code.
	
	It is used to avoid an extra copy when handling large amounts of data between
	kernel and user space code. Used in device drivers that will handle instances
	of this class and pass pointers to the buffer to user space clients.
	Each buffer can be mapped into a single process.
	The lifetime of this object is maintained by the device driver.
	These objects are intended for use where speed is essential, such as for
	streaming media data.
*/
class DSharedIoBuffer : public DBase
	{
public:
    /**
    @internalComponent
    */
	enum TPanic
		{
		EAlreadyCreated=1,
		EAlreadyMapped
		};
	IMPORT_C ~DSharedIoBuffer();
	IMPORT_C static TInt New(DSharedIoBuffer*& aBuffer, TUint32 aSize, TUint aAttribs);
#if defined(__EPOC32__)
	IMPORT_C static TInt New(DSharedIoBuffer*& aBuffer, TPhysAddr aPhysAddr, TUint32 aSize, TUint aAttribs);
#endif
	IMPORT_C TInt UserMap(DProcess* aUserProcess);
	IMPORT_C TInt UserUnmap();
	IMPORT_C TLinAddr UserToKernel(TLinAddr aUserAddress, TUint32 aSize);
	IMPORT_C TLinAddr KernelToUser(TLinAddr aKernelAddress);
public:
  	/**
	The address of the shared buffer that code running on the kernel side uses
	to access the buffer.
	
	@see DSharedIoBuffer::UserToKernel()
    @see DSharedIoBuffer::KernelToUser()
	*/
	TLinAddr iAddress;
	
	/**
    The size of the shared buffer.
    
    After mapping the buffer into the user-side address space via a call
    to DSharedIoBuffer::UserMap(), this value must be returned to
    the user-side via normal logical channel operations so that user
    code can start using the buffer. 
    
    @see DSharedIoBuffer::UserMap()
	*/
	TUint32 iSize;
	
	/**
	The address of the shared buffer that code running in the user process
	will use to access that buffer.
    
    After mapping the buffer into the user-side address space via a call
    to DSharedIoBuffer::UserMap(), this value must be returned to
    the user-side via normal logical channel operations so that user
    code can start using the buffer.
    
    @see DSharedIoBuffer::UserMap() 
    @see DSharedIoBuffer::UserToKernel()
    @see DSharedIoBuffer::KernelToUser()
	*/
	TLinAddr iUserAddress;

    /**
    @internalComponent
    */
	DProcess* iUserProcess;
private:
    /**
    @internalComponent
    */
	DChunk* iChunk;
private:
	TInt DoCreate(TUint32 aSize, TUint aAttribs);	/**< @internalComponent */
	TInt DoCreate(TUint32 aPhysAddr, TUint32 aSize, TUint aAttribs);	/**< @internalComponent */
private:
	DSharedIoBuffer();	/**< @internalComponent */
	};

/********************************************
 * System tick-based timer queues
 ********************************************/
/********************************************
 * Timers based on the system tick
 * Platform layer code
 ********************************************/
class TTickQ : public SDeltaQue
	{
public:
	TTickQ();
	void Tick();
	static TInt Init();
	static void Wait();
	static void Signal();
public:
	// In platform layer
	void Add(TTickLink* aLink, TInt aPeriod);
	void StartTimer();
	void Update();
	void Synchronise();				// update everything as if a tick has just occurred
	void Check();
public:
	TInt iLastDelta;				// delta ticks at next ms timer expiry
	TUint32 iLastTicks;				// tick count at last expiry
	Int64 iRtc;						// tick count acting as RTC (ticks from 00:00:00 01-01-0000 UTC)
	TInt iTickPeriod;				// tick period in microseconds
	TInt iTicksPerSecond;			// ticks per second, rounded up
	TInt iNominalTickPeriod;		// nominal tick period in microseconds
	TBool iInTick;
	static DMutex* Mutex;
public:
	TInt iRounding;					// rounding applied to current ms timer in us
	TInt iPrevRounding;				// rounding applied to last ms timer in us
	TUint32 iLastMs;				// ms count at last expiry
	TInt iMsTickPeriod;				// in microseconds
	TDfc iTickDfc;
	NTimer iMsTimer;
	};

class TSecondQ : public SDblQue
	{
public:
	TSecondQ();
	void Tick();
	void WakeUp();
	static void TickCallBack(TAny* aPtr);
	static void WakeUpDfc(TAny* aPtr);
public:
	void Add(TSecondLink* aLink);
	void StartTimer();
	TInt FirstDelta();
	TTimeK WakeupTime();
public:
	TBool iInTick;
	Int64 iNextTrigger;				// next trigger time in ticks from 00:00:00 01-01-0000 UTC
	Int64 iMidnight;				// next midnight (home time) in ticks from 00:00:00 01-01-0000 UTC 
	TInt iTicksPerDay;
	TTickLink iTimer;
	TDfc iWakeUpDfc;
	};

class TInactivityQ : public SDblQue
	{
public:
	TInactivityQ();
	void Reset();
	TInt InactiveTime();
public:
	void Expired(TBool aTicksUpdated);
	void EventDfc();
	static void TimerCallBack(TAny* aPtr);
	static void EventDfcFn(TAny* aPtr);
public:
	SDblQue iPending;
	TUint32 iLastEventTime;
	TBool iInTick;
	TTickLink iTimer;
	TDfc iEventDfc;
	};


/********************************************
 * Kernel heap
 ********************************************/
class RHeapK : public RHybridHeap
	{
public:
	static RHeapK* FixedHeap(TAny* aBase, TInt aMaxLength);
	TInt CreateMutex();
	void Mutate(TInt aOffset, TInt aMaxLength);
public:
	RHeapK(TInt aInitialSize);
	virtual TInt Compress();
	virtual void Reset();
	virtual TInt AllocSize(TInt& aTotalAllocSize) const;
	virtual TInt Available(TInt& aBiggestBlock) const;
public:
	inline TInt TotalAllocSize() const
		{ return iTotalAllocSize; }
	static void CheckThreadState();
	static void Fault(TInt aFault);
	inline TBool CheckForSimulatedAllocFail()
	    { return RHybridHeap::CheckForSimulatedAllocFail(); }	
	inline DMutex* Mutex() const; /**< @internalComponent */
public:
	friend class Monitor;
	};

/**
@internalComponent
*/
inline DMutex* RHeapK::Mutex() const
	{ return *(DMutex**)&iLock;	}

enum TSecureClockStatusFlags 
	{
	ESecureClockPresent = 1,		// signals a trusted time source has been found 
	ESecureClockOffsetPresent = 2,	// signals nonsecure offset was read from HAL
	ESecureClockOk = ESecureClockPresent | ESecureClockOffsetPresent
	};

/**
An unparsed descriptor header.

@see TDesHeader.

@prototype
@internalTechnology
*/
class TRawDesHeader
	{
public:
	const TUint32& operator[](TInt aIndex) const { return iData[aIndex]; }
	TUint32& operator[](TInt aIndex) { return iData[aIndex]; }
private:
	TUint32 iData[3];
	};

/********************************************
 * Functions defined in kernel layer 1
 ********************************************/

extern "C" {
extern TLinAddr SuperPageAddress;
}

class DObjectCon;
struct SExtInit1EntryPoint;
class K
	{
public:
	enum TFault
		{
		EBadPriListPriority=0,
		EKillWhileCSLocked=1,
		ESvrStraySignal=2,
		ESvrBadMessage=3,
		ESystemThreadPanic=4,
		EThreadDiedInCS=5,
		ECodeDependenciesInconsistent=6,
		EUnTrapWithoutTrap=7,
		EThrdEventHookDied=8,
		ECreateEventQueueFailed=9,
		EInvalidDfcPriority=10,
		EMessageBadWriteAddress=11,
		EMessageAlreadyPending=12,
		EKernelMsgNotFree=13,
		EKernelMsgNotAccepted=14,
		EPanicWhileKernelLocked=15,
		EPoweredChannelNoDfcQ=16,
		EKernelHeapCorrupted=17,
		EBadObjectType=18,
		ESessionDestruct=19,
		EMessageNotFree=20,
		ESessionCreateBadServerTAC=21,
		ESessionDestructMsgCount=22,
		EServerDestructMsgCount=23,
		EServerDestructSessionsRemain=24,
		EServerDestructMessagesRemain=25,
		EServerCloseLeftoverMsg=26,
		EInvalidSessionAccessCount=27,
		EMsgFreeBadPool=28,
		ESessionDestructMsgQ=29,
		EKHeapBadCellAddress=30,
		EKHeapBadAllocatedCellSize=31,
		EKHeapBadAllocatedCellAddress=32,
		EKHeapBadFreeCellAddress=33,
		EKHeapFreeBadNextCell=34,
		EKHeapFreeBadPrevCell=35,
		EKHeapReAllocBadNextCell=36,
		EKHeapAllocSizeNegative=37,
		EKHeapReAllocSizeNegative=38,
		EKHeapDebugUnmatchedCallToCheckHeap=39,
		EKHeapAllocCheckFailed=40,
		ETickQNotLocked=41,
		ELogicalChannelMsgUncompleted=42,
		ESessionDestructStillRef=43,
		ETHeapMaxLengthNegative=44,
		ETHeapMutateNotSame=45,
		EKernLeaveNoTrap=46,
		EProcResumeNotLoaded=47,
		EMachineConfigMutexCreateFailed=48,
		ERamDriveChunkCreateFailed=49,
		EInitMicrokernelFailed=50,
		EInit3Failed=51,
		EStartExtensionsFailed=52,
		ETBmaFreeNotAllocated=53,
		ETBmaBlockAllocNotFree=54,
		ETBmaBlockFreeNotAllocated=55,
		ETBmaAllocConsecBadLength=56,
		ERamDriveInitFailed=57,
		EKernelHeapOperationWithKernelLocked=58,
		EKernelHeapOperationNotCritical=59,
		EWaitMsTimerNotIdle=60,
		EThreadDieWhileExit=61,
		EThreadSvKillNotDead=62,
		EDeadThreadRunning=63,
		EBadTimerType=64,
		ESynchroniseMsDeltaTooBig=65,
		EBadThreadPriority=66,
		EMutexSignalWrongThread=67,
		EMutexSignalBadState=68,
		EMutexWaitBadState=69,
		EMutexWaitBadWaitObj=70,
		ESemSignalBadState=71,
		ESemWaitBadState=72,
		ESemWaitBadWaitObj=73,
		EInvalidKernHeapCPtr=74,
		ELibDestructBadMapCount=75,
		ECodeStillMapped=76,
		EProcessDataAddressInvalid=77,
		ECodeSegBadExeCodeSeg=78,
		ECodeSegAttachProcess=79,
		EPermanentThreadExit=80,
		EJitCrashHandlerCreation=81,
		EMsqQueueLengthInvalid=82,
		EProcessFromIdNoLock=83,
		EThreadFromIdNoLock=84,
		EMutexWaitNotInCs=85,
		EMutexSignalNotInCs=86,
		EDebugEventHandlerBadCallBack=87, 
		EMutexOrderingViolation=88,
		ECondVarWaitBadState1=89,
		ECondVarWaitBadState2=90,
		ECondVarWaitBadState3=91,
		ECondVarUnBlockBadState=92,
		ESharedIoBufferBadInternalState=93,
		ENonUserThreadKilled=94,
		EOutOfIds=95,
		EBadLogSize=96,
		EAlreadyCalled=97,
		EMutexWaitNotDThread=98,
		EBadKernelHookType=99,
		EKernelHookAlreadySet=100,
		EMsgCompleteDiscNotSent=101,
		ESystemProcessPanic=102,
		EPermanentProcessExit=103,
		ECodeSegBadFixupAddress=104,
		ECodeSegBadFixupTables=105,
		EAPInitialThreadCreateFailed=106,
		EInit2APTimeout=107,
		EChunkCommitBadType = 108,
		EBadSetRamZoneConfig = 109, /**< SetRamZoneConfig can only be invoked during boot*/
		ETooManyExtensions = 110,
		EExtensionArrayAllocationFailed = 111,
		EInsertExtensionFailed = 112,
		EExtensionArrayOverflowed = 113,
		EThreadWaitListDestroy = 114,
		EThreadWaitListCheck = 115,
		EThreadWaitListUp = 116,
		EThreadWaitListChangePriority = 117,
		EThreadWaitListRemove = 118,
		EThreadWaitListRemove2 = 119,
		EClientRequestDeletedNotClosed=120,
		EClientRequestSetStatusInWrongState=121,
		EClientRequestCompleteInWrongState=122,
		EClientRequestResetInWrongState=123,
		EClientRequestCallbackInWrongState=124,
		EClientRequestCloseInWrongState=126,
		EMessageInUse=127,
		EVirtualPinObjectBad=128,
		EIpcClientNotNull=129,
		EBufferRequestAddInWrongState=130,
		EBufferRequestEndSetupInWrongState=131,
		EBufferRequestResetInWrongState=132,
		EThreadBufReadWithNullPointer=133,
		EThreadBufWriteWithNullPointer=134,
		ECodeSegRemoveAbsent=135,
		EPhysicalPinObjectBad=136,
		EShBufVirtualNotDefined=137,	//< A required virtual method is not present in a shared buffer derived class (internal error)
		ESecureRNGInitializationFailed = 138,
		ESecureRNGInternalStateNotSecure = 139,
		ESecureRNGOutputsInBadState = 140,
		
		ESystemException=0x10000000,
		ESoftwareWarmReset=0x10000001
		};

	static void Fault(TFault aFault);
public:
	static TMachineConfig* MachineConfig;
	static RAllocator* Allocator;
	static struct SHeapInfo
		{
		DChunk* iChunk;
		TUint8* iBase;
		TUint iMaxSize;
		} HeapInfo;
	static struct SMsgInfo
		{
		DChunk* iChunk;
		TUint8* iBase;
		TUint iMaxSize;
		TUint iCurrSize;
		DMutex* iMsgChunkLock;
		RMessageK* iNextMessage;
		TInt iFreeMessageCount;
		} MsgInfo;
	static DThread* TheKernelThread;
	static DProcess* TheKernelProcess;
	static DThread* TheNullThread;
	static DThread* SvThread;
	static TDfcQue* SvMsgQ;
	static TMessageQue SvBarrierQ;
	static NFastMutex EventQueueMutex;
	static TRawEvent *EventHeadPtr;
	static TRawEvent *EventTailPtr;
	static TRawEvent *EventBufferStart;
	static TRawEvent *EventBufferEnd;
	static TClientDataRequest<TRawEvent>* EventRequest;
	static DThread *EventThread;
	static TDfcQue* DfcQ0;
	static TDfcQue* DfcQ1;
	static TDfcQue* TimerDfcQ;
	static TTickQ* TickQ;
	static TSecondQ* SecondQ;
	static TInactivityQ* InactivityQ;
	static TInt HomeTimeOffsetSeconds;
	static TInt NonSecureOffsetSeconds;
	static TInt SecureClockStatus;
	static TInt64 Year2000InSeconds;
	static DPowerModel* PowerModel;
	static TBool PowerGood;
	static TInt MaxMemCopyInOneGo;
	static TInt MaxFreeRam;
public:
	static DObjectCon* Containers[ENumObjectTypes];
public:
	static TInt NextId;
	static TInt DfcQId;
	static volatile TUint DynamicDfcQId;
	static SHalEntry2* HalEntryArray;
	static DMutex* MachineConfigMutex;
	static TAny* volatile AsyncFreeHead;
	static DBase* volatile AsyncDeleteHead;
	static TDfc AsyncFreeDfc;
	static TInt MemoryLowThreshold;
	static TInt MemoryGoodThreshold;
	static TUint AsyncChanges;
	static TDfc AsyncChangeNotifierDfc;
public:
	static TBool Initialising;
	static TBool ColdStart;
	static TInt ExtensionCount;
	static RArray<SExtInit1EntryPoint>* ExtensionArray;
	static DProcess* TheFileServerProcess;
	static DProcess* TheWindowServerProcess;
	static TInt PINestLevel;
	static TInt SupervisorThreadStackSize;
	static TUint32 MemModelAttributes;
	static TKernelHookFn KernelHooks[ENumKernelHooks];
	static TMiscNotifierMgr TheMiscNotifierMgr;
	static TAny* VariantData[31];
	static TUint32 EntropyBufferStatus[KMaxCpus];
	static TUint32* EntropyBuffer[KMaxCpus];
    static TUint32 TempEntropyBuffer[KEntropyBufferSizeWords];
    static TDfc EntropyBufferDfc;
public:
	static TInt InitialiseMicrokernel();
#ifdef __SMP__
	static void InitAP(TInt aCpu, volatile SAPBootInfo* aInfo, TInt aTimeout);
#endif
	static TInt Init3();
	static void InitNvRam();
	static void InitLocaleData();
	static void	SetDefaultLocaleData1();
	static void	SetDefaultLocaleData2();
	static void InitHalEntryArray();
	static TInt CreateObjectContainers();
	static DObjectCon* ContainerFromFindHandle(const TFindHandle& aFindHandle);
	static void LockContainer(TInt aObjType);
	static void UnlockContainer(TInt aObjType);
	static TInt AddObject(DObject* aObj, TInt aObjType);
	static TInt StartTickQueue();
	static void PanicKernExec(TInt aReason);
	static void PanicCurrentThread(TInt aReason);
	static void UnmarkAllLibraries();
	static void CreateEventQueue(TInt aSize);
	static void TryDeliverEvent();
	static DObject* ObjectFromHandle(TInt aHandle);
	static DObject* ObjectFromHandle(TInt aHandle, TInt aObjType);
	static DObject* ObjectFromHandle(TInt aHandle, TInt aObjType, TUint& aAttr);
	static TInt OpenObjectFromHandle(TInt aHandle, DObject*& aObject);
	static TInt MakeHandle(TOwnerType aType, DObject* aObject);
	static TInt MakeHandle(TOwnerType aType, DObject* aObject, TUint aAttr);
	static TInt MakeHandleAndOpen(TOwnerType aType, DObject* aObject, TInt& aHandle);
	static TInt MakeHandleAndOpen(TOwnerType aType, DObject* aObject, TInt& aHandle, TUint aAttr);
	static TInt HandleClose(TInt aHandle);
	static void ObjDelete(DObject* aObj);
	static TInt SecondsFrom2000(const TTimeK& aTime, TInt& aSeconds);
	static TInt SetSystemTime(TInt aSecondsFrom2000, TInt aUTCOffset, TUint& aChanges, TUint aMode);
	static TInt SetSystemTimeAndOffset(const TTimeK& aTime, TInt aOffset, TUint aTimeSetMode, TUint& aChanges, TUint aMode);
	static void StartKernelServer();
	static TInt MutexCreate(DMutex*& aMutex, const TDesC& aName, DObject* aOwner, TBool aVisible, TUint aOrder);
	static void Randomize();
	static void DoAsyncFree(TAny*);
	static void DoAsyncNotify(TAny*);
	static TUint CheckFreeMemoryLevel(TInt aInitial, TInt aFinal, TBool aFailed);
	static TBool CheckForSimulatedAllocFail();
	static void DoSvBarrier(TAny*);

//
	static void CheckKernelUnlocked();
	static TInt KernelHal(TInt aFunction, TAny* a1, TAny* a2);
	static void SetMachineConfiguration(const TDesC8& aConfig);

	static DThread* ThreadEnterCS();
	static DThread* ThreadLeaveCS();
	static DObject* ThreadEnterCS(TInt aHandle, TInt aObjType);
	static void CheckFileServerAccess();

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline static void ProcessIsolationFailure(const char* aContextText);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline static void ProcessIsolationFailure(OnlyCreateWithNull aContextText);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	static void LockedPlatformSecurityPanic();
	static void UnlockedPlatformSecurityPanic();
	static TBool IsInKernelHeap(const TAny* aPtr, TInt aSize);
	static TUint32 CompressKHeapPtr(const TAny* aPtr);
	static const TAny* RestoreKHeapPtr(TUint32 aCPtr);
	static TBool CheckUids(const TUidType& aUids, const TUidType& aRequestedUids);
	static TUint NewId();
	inline static TTraceHandler TraceHandler();
	inline static TNanoWaitHandler NanoWaitHandler();
	static TInt FloatingPointTypes(TUint32& aTypes);
	static TInt FloatingPointSystemId(TUint32& aSysId);
	inline static TInitialTimeHandler InitialTimeHandler();

	static void TextTrace(const TDesC8& aText, TTraceSource aTraceSource, TBool aNewLine=ETrue);
	static TUint TextTraceMode;

	static void CheckThreadNotRealtime(const char* aTraceMessage=NULL);
	static TBool IllegalFunctionForRealtimeThread(DThread* aThread,const char* aTraceMessage=NULL);
	static TAny* USafeRead(const TAny* aSrc, TAny* aDest, TInt aSize);
	static TAny* USafeWrite(TAny* aDest, const TAny* aSrc, TInt aSize);
	static TInt ParseDesHeader(const TAny* aDesPtr, const TRawDesHeader& aIn, TDesHeader& aOut);
	static TInt USafeReadAndParseDesHeader(TAny* aDesPtr, TDesHeader& aOut);
	static TUint32 KernelConfigFlags();

	static void DoFault(const TAny* a0, TInt a1);

	static TInt ShPoolCreate(DShPool*& aPool, TShPoolCreateInfo& aInfo);

private:
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	static void DoProcessIsolationFailure(const char* aContextText);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	static void DoProcessIsolationFailure();
	static void DoNanoWait(TUint32 aInterval);
	};

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
inline void K::ProcessIsolationFailure(const char* aContextText)
	{
	DoProcessIsolationFailure(aContextText);
	}
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
// Only available to NULL arguments
inline void K::ProcessIsolationFailure(OnlyCreateWithNull /*aContextText*/)
	{
	DoProcessIsolationFailure();
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

inline TTraceHandler K::TraceHandler()
	{
	return (TTraceHandler)KernelHooks[EHookTrace];
	}

inline TNanoWaitHandler K::NanoWaitHandler()
	{
	return (TNanoWaitHandler)KernelHooks[EHookNanoWait];
	}

inline TInitialTimeHandler K::InitialTimeHandler()
	{
	return (TInitialTimeHandler)KernelHooks[EHookInitialTime];
	}

#ifdef __DEBUGGER_SUPPORT__
#define __DEBUG_EVENT(aEvent, a1) DKernelEventHandler::Dispatch((aEvent),(a1),NULL)
#define __DEBUG_EVENT2(aEvent, a1, a2) DKernelEventHandler::Dispatch((aEvent),(a1),(a2))
#define __COND_DEBUG_EVENT(aCond, aEvent, a1) {if (aCond) DKernelEventHandler::Dispatch((aEvent),(a1),NULL);}

#ifdef __EPOC32__ 
//Setting breakpoints is only available on target and if kernel is built with __DEBUGGER_SUPPORT__ option
#define __REMOVE_CODESEG_FROM_CODEMODIFIER(segment, process) CodeModifier::CodeSegRemoved((segment),(process))
#else
#define __REMOVE_CODESEG_FROM_CODEMODIFIER(segment, process)
#endif

#else
#define __DEBUG_EVENT(aEvent, a1)
#define __DEBUG_EVENT2(aEvent, a1, a2)
#define __COND_DEBUG_EVENT(aCond, aEvent, a1)
#define __REMOVE_CODESEG_FROM_CODEMODIFIER(segment, process)
#endif

/********************************************
 * Functions defined in layer 1M or below of
 * the kernel and available to layer 1.
 ********************************************/
class P
	{
public:
	static TInt InitSystemTime();
	static TInt DefaultInitialTime();
	static void CreateVariant();
	static void StartExtensions();
	static void KernelInfo(TProcessCreateInfo& aInfo, TAny*& aStack, TAny*& aHeap);
	static void NormalizeExecutableFileName(TDes& aFileName);
	static void SetSuperPageSignature();
	static TBool CheckSuperPageSignature(); 
//
	static DProcess* NewProcess();
	};


	struct SCacheInfo;
 
	/*
 	Symbian OS currently supports 5 memory models one for the WIN32 platform (the emulator model)
 	and three for the EPOC platform (moving, multiple, direct and flexible).
 
 	Model layer is the third layer in EKA2 software layer which consist of 6 layers, 
 	There are two static interfaces to the memory model, the first to be in Class Epoc and the second in Class M.
   
	
 	Class M consists of functions provided by the memory model to the independent layer which is
 	the first layer in EKA2 software layer, M denotes the API exposed by the model layer
	*/
 

struct SCacheInfo;
struct SPageInfo;
struct SZone;
class M
	{
public:
	/**
	Flags to control the moving or discarding of a page via the methods M::DiscardPage(),
	M::MovePage()and M::MoveAndAllocPage().
	*/
	enum TMoveDiscardFlags
		{
		/** Set this flag so dirty discardable page are moved rather than 
			written to swap.
		*/
		EMoveDisMoveDirty = 0x1,
		/** Set this so the attempt to allocate the location to move the page to 
			will fail if the blocked RAM zone is reached.
		*/
		EMoveDisBlockRest = 0x2,
		};

	static void Init1();
	static void Init2();
#ifdef __SMP__
	static void GetAPBootInfo(TInt aCpu, volatile SAPBootInfo* aInfo);
	static void Init2AP();
#endif
	static void Init3();
	static void Init4();
	static TInt InitSvHeapChunk(DChunk* aChunk, TInt aSize);
	static TInt InitSvStackChunk();
	static TBool IsRomAddress(const TAny* aPtr);
	static TInt PageSizeInBytes();
	static TInt PageShift();
	static void SetupCacheFlushPtr(TInt aCache, SCacheInfo& c);
	static void FsRegisterThread();
	static DCodeSeg* NewCodeSeg(TCodeSegCreateInfo& aInfo);
	static void DemandPagingInit();
	static TInt DemandPagingFault(TAny* aExceptionInfo);
	static TBool CheckPagingSafe(TBool aDataPaging, TLinAddr aStartAddr=0, TUint aLength=KMaxTUint);
	static TInt LockRegion(TLinAddr aStart,TInt aSize);
	static TInt UnlockRegion(TLinAddr aStart,TInt aSize);
	static void BTracePrime(TUint aCategory);
	static void LockUserMemory();
	static void UnlockUserMemory();
	static TInt CreateVirtualPinObject(TVirtualPinObject*& aPinObject);
	static TInt PinVirtualMemory(TVirtualPinObject* aPinObject, TLinAddr aStart, TUint aSize, DThread* DThread);
	static TInt CreateAndPinVirtualMemory(TVirtualPinObject*& aPinObject, TLinAddr aStart, TUint aSize);
	static void UnpinVirtualMemory(TVirtualPinObject* aPinObject);	
	static void DestroyVirtualPinObject(TVirtualPinObject*& aPinObject);

	static TInt CreatePhysicalPinObject(TPhysicalPinObject*& aPinObject);
	static TInt PinPhysicalMemory(TPhysicalPinObject* aPinObject, TLinAddr aStart, TUint aSize, TBool aReadOnly, TPhysAddr& aAddress, TPhysAddr* aPages, TUint32& aMapAttr, TUint& aColour, DThread* aThread);
	static void UnpinPhysicalMemory(TPhysicalPinObject* aPinObject);
	static void DestroyPhysicalPinObject(TPhysicalPinObject*& aPinObject);

	static TInt CreateKernelMapObject(TKernelMapObject*& aMapObject, TUint aMaxReserveSize);
	static TInt MapAndPinMemory(TKernelMapObject* aMapObject, DThread* aThread, TLinAddr aStart, TUint aSize, TUint aMapAttributes, TLinAddr& aKernelAddr, TPhysAddr* aPages);
	static void UnmapAndUnpinMemory(TKernelMapObject* aMapObject);
	static void DestroyKernelMapObject(TKernelMapObject*& aMapObject);

	// RAM allocator and defrag interfaces.
	static void RamAllocLock();
	static void RamAllocUnlock();
	static void RamAllocIsLocked();
	static TUint NumberOfFreeDpPages();
	static TUint NumberOfDirtyDpPages();
	static TInt MovePage(TPhysAddr aOld, TPhysAddr& aNew, TUint aBlockZoneId, TUint aMoveDisFlags);
	static TInt MoveAndAllocPage(TPhysAddr aAddr, TZonePageType aPageType);
	static TInt DiscardPage(TPhysAddr aAddr, TUint aBlockZoneId, TUint aMoveDisFlags);
	static TBool GetFreePages(TUint aNumPages);
	static void RamZoneClaimed(SZone* aZone);
	static TInt RamDefragFault(TAny* aExceptionInfo);
	};

#ifdef __USER_MEMORY_GUARDS_ENABLED__
#define	LOCK_USER_MEMORY()			M::LockUserMemory()
#define	UNLOCK_USER_MEMORY()		M::UnlockUserMemory()
#define	COND_LOCK_USER_MEMORY(c)	((void)((c)&&(M::LockUserMemory(),0)))
#define	COND_UNLOCK_USER_MEMORY(c)	((void)((c)&&(M::UnlockUserMemory(),0)))
#else
#define	LOCK_USER_MEMORY()
#define	UNLOCK_USER_MEMORY()
#define	COND_LOCK_USER_MEMORY(c)
#define	COND_UNLOCK_USER_MEMORY(c)
#endif

/********************************************
 * Functions defined in layer 3 or below and
 * available to layer 2
 ********************************************/
class DPlatChunkHw;
class A
	{
public:
	static void Init1();
	static void Init2();
#ifdef __SMP__
	static void InitAPs();
	static void Init2AP();
#endif
	static void Init3();
	static void DebugPrint(const TText* aPtr, TInt aLen, TBool aNewLine=ETrue);
	static TInt CreateVariant(const TAny* aFile, TInt aMode);
	static DPlatChunkHw* NewHwChunk();
	static TPtr8 MachineConfiguration();
	static void StartCrashDebugger(const TAny* a0, TInt a1);
	static TInt CallSupervisorFunction(TSupervisorFunction aFunction, TAny* aParameter);
	static TInt VariantHal(TInt aFunction, TAny* a1, TAny* a2);
	static TInt SystemTimeInSecondsFrom2000(TInt& aTime);
	static TInt SetSystemTimeInSecondsFrom2000(TInt aTime);
	};

class Exc
	{
public:
	static void Dispatch(TAny* aPtr, NThread*);
	IMPORT_C static void Fault(TAny* aPtr);
	static TBool IsMagic(TLinAddr aAddress);
#ifdef __ATOMIC64_USE_SLOW_EXEC__
	static TBool IsMagicAtomic64(TLinAddr aAddress);
#endif //__ATOMIC64_USE_SLOW_EXEC__
	};

/********************************************
 * Super page definition
 ********************************************/
class DDebuggerInfo;
class TSuperPage : public SSuperPageBase
	{
public:
	TInt iDebugMask[KNumTraceMaskWords];	// kernel trace mask
	TInt iKernelExcId;
	TExcInfo iKernelExcInfo;
	TUint32 iSignature[2];
	TMachineStartupType iStartupReason;
	DDebuggerInfo* iDebuggerInfo;
	TUint32 iDisabledCapabilities[(((TInt)ECapability_HardLimit + 7)>>3) / sizeof(TUint32)];
	TUint32 iInitialBTraceFilter[8];
	TInt iInitialBTraceBuffer;
	TInt iInitialBTraceMode;

private:
	TUint32 iKernelConfigFlags;

public:
	/*
	 * Unless __PLATSEC_UNLOCKED__ is defined, __PLATSEC_FORCED_FLAGS__ is set to a
	 * bitmask of platsec flags which must always be considered to be set, even if
	 * they are not really set in iKernelConfigFlags.  Therefore, use this function
	 * to access iKernelConfigFlags.
	 *
	 * __PLATSEC_UNLOCKED__, if set, is set by the base port.
	 *
	 * __PLATSEC_FORCED_FLAGS__ is defined in u32std.h near the TKernelConfigFlags enumeration.
	 */
	inline TUint32 KernelConfigFlags() 
		{ 
#ifdef __PLATSEC_UNLOCKED__
		return (iKernelConfigFlags | __PLATSEC_FORCED_FLAGS__) & ~EKernelConfigPlatSecLocked;
#else
		return (iKernelConfigFlags | __PLATSEC_FORCED_FLAGS__ | EKernelConfigPlatSecLocked);
#endif
		} 

	inline void SetKernelConfigFlags(TUint32 aKernelConfigFlags)
		{
		iKernelConfigFlags = aKernelConfigFlags;
		}
	};

inline TSuperPage& TheSuperPage() {return *(TSuperPage*)SuperPageAddress;}
inline TMachineConfig& TheMachineConfig() {return *(TMachineConfig*)K::MachineConfig; }

#define TEST_DEBUG_MASK_BIT(n) ( TheSuperPage().iDebugMask[(n)>>5] & (1<<((n)&31)) )

/********************************************
 * Miscellaneous stuff
 ********************************************/

inline void TExcTrap::UnTrap()
	{iThread->iExcTrap=NULL;}
inline void TIpcExcTrap::UnTrap()
	{iThread->iExcTrap=NULL;iThread->iIpcClient=NULL;}
inline void TPagingExcTrap::UnTrap()
	{iThread->iPagingExcTrap=NULL;}

GLREF_D const TUint32 EpocFastExecTable[];
GLREF_D const TUint32 EpocSlowExecTable[];

inline SMiscNotifierQ* DObject::NotifierQ() const
	{
	TUint32 cptr = ((TUint32(iObjectId)<<14)>>6) | iNotQLow;
	return cptr ? (SMiscNotifierQ*)K::RestoreKHeapPtr(cptr) : (SMiscNotifierQ*)0;
	}

inline void DObject::SetNotifierQ(SMiscNotifierQ* aQ)
	{
	TUint32 cptr = aQ ? K::CompressKHeapPtr(aQ) : 0;
	iNotQLow = (TUint8)cptr;
	volatile TUint32& x = (volatile TUint32&)iObjectId;
	x = ((x>>18)<<18) | (cptr>>8);
	}

inline TBool DObject::HasNotifierQ() const
	{
	return (TUint32(iObjectId)<<14) | iNotQLow;
	}

#ifdef _DEBUG

/**
@publishedPartner
@released
*/
#define __ASSERT_CRITICAL	{	\
							DThread& t=Kern::CurrentThread(); \
							__NK_ASSERT_DEBUG(t.iThreadType!=EThreadUser || t.iNThread.iCsCount>0);	\
							}

/**
@publishedPartner
@released
*/
#define __ASSERT_MUTEX(m)		{	\
								DThread& t=Kern::CurrentThread(); \
								__NK_ASSERT_DEBUG((m)->iCleanup.iThread==&t);	\
								}
#else
/**
@publishedPartner
@released
*/
#define __ASSERT_CRITICAL

/**
@publishedPartner
@released
*/
#define __ASSERT_MUTEX(m)
#endif


#define LOGICAL_XOR(a,b) (((a)==0)^((b)==0))


#if defined(__GCC32__)
#define __RETURN_ADDRESS() __builtin_return_address(0)
#elif defined (__ARMCC__)
#define __RETURN_ADDRESS() ((TAny*)__return_address())
#else
#define __RETURN_ADDRESS() 0	// not supported
#endif

#ifdef _DEBUG
#if defined(__STANDALONE_NANOKERNEL__) || (!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))
#define __ASSERT_WITH_MESSAGE_MUTEX(m,message,function ) 

#else
/**
@internalComponent
*/
#define __ASSERT_WITH_MESSAGE_MUTEX(m,message,function ) \
			{	\
			DThread& t=Kern::CurrentThread(); \
			__ASSERT_WITH_MESSAGE_DEBUG((NKern::Crashed() || ((m)->iCleanup.iThread==&t)),message,function);	\
			}			

#endif//(!defined (__KERNEL_APIS_CONTEXT_CHECKS_WARNING__)&&!defined (__KERNEL_APIS_CONTEXT_CHECKS_FAULT__))

#else//if !DEBUG

#define __ASSERT_WITH_MESSAGE_MUTEX(m,message,function )

#endif//_DEBUG

// Implementation constants for TClientRequest
const T_UintPtr KClientRequestFlagClosing = 1 << 0;
const T_UintPtr KClientRequestFlagInUse   = 1 << 1;
const T_UintPtr KClientRequestFlagMask    = KClientRequestFlagClosing | KClientRequestFlagInUse;
const T_UintPtr KClientRequestNullStatus = 0x00000004;  // must be non-zero but never a valid TRequestStatus pointer

#endif
