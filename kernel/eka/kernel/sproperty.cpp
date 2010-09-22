// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// \e32\kernel\sproperty.cpp
// 
//

#include <kernel/kern_priv.h>
#include "execs.h"

#define __PS_ASSERT(aCond) \
	__ASSERT_DEBUG( (aCond), ( \
						Kern::Printf("Assertion '" #aCond "' failed;\nFile: '" \
						__FILE__ "' Line: %d\n", __LINE__), \
						Kern::Fault("Pub&Sub", 0)) )

inline DProcess* CurProcess()
	{
	return TheCurrentThread->iOwningProcess;
	}

// Used by a thread to schedule cancelation of Subscribe request in Supervisor thread.
class TCancelQ: public SDblQueLink
{
public:
    TPropertySubsRequest* iPropSubRequest;//The request to be cancelled, can be NULL
    NFastSemaphore iFSemaphore;//Semafore to be signalled by Supervisor thread after the request is cancelled.
};

class TProperty 
	{
public:
	static TInt Init();

	static TInt Attach(TUint aCategory, TUint aKey, TProperty** aProp);
	static TInt Open(TUint aCategory, TUint aKey, TProperty** aProp);
	void Close();

	TInt Define(const TPropertyInfo*, DProcess*);
	TInt Delete(DProcess*);

	TInt Subscribe(TPropertySubsRequest* aSubs, DProcess*);
	void Cancel(TPropertySubsRequest* aSubs);

	TInt GetI(TInt* aValue, DProcess*);
	TInt GetB(TUint8* aBuf, TInt* aSize, DProcess*, TBool aUser); 
	TInt SetI(TInt aValue, DProcess*);
	TInt SetB(const TUint8*, TInt aSize, DProcess*, TBool aUser);

	static TInt FindGetI(TUint aCategory, TUint aKey, TInt* aValue, DProcess*);
	static TInt FindSetI(TUint aCategory, TUint aKey, TInt aValue, DProcess*);

	// Called with system or feature locked
	TBool IsDefined()
		{
		return iType != RProperty::ETypeLimit;
		}

	// Called with system or feature locked
	RProperty::TType Type()
		{
		return (RProperty::TType) iType;
		}

	// Called with system or feature locked
	TInt BufSize()
		{
		__PS_ASSERT(iType == RProperty::EByteArray || iType == RProperty::ELargeByteArray);
		return iBuf ? iBuf->iBufSize : 0;
		}


	// The property attributes.
	// Meaningful for defined properties only (ie. iType != RProperty::ETypeLimit)
	// Constant while the property is defined
	TUint32	Owner()
		{ return iOwner; }

	/// Ensure pages in the source buffer are paged in and lock them
	static TInt LockSourcePages(const TUint8* aBuf, TInt aSize);

	/// Unlock source pages again
	static void UnlockSourcePages();

#ifdef _DEBUG
	static TBool IsLocked()
		// used in assertions only
		{ return FeatureLock->iCleanup.iThread == TheCurrentThread; }
#endif

public:
	const TUint		iCategory;
	const TUint		iKey;

private:
	// Acquire the feature lock
	// Called in CS
	static void Lock()
		{
		Kern::MutexWait(*FeatureLock);
		}

	// Release the feature lock
	// Called in CS
	static void Unlock()
		{
		Kern::MutexSignal(*FeatureLock);
		}

	static void CompleteRequest(TPropertySubsRequest*, TInt aReason);	
	static void CompleteQue(SDblQue* aQue, TInt aReason);	
	static void CompleteDfc(TAny* aQue);
	static void CompleteDfcByKErrPermissionDenied(TAny* aQue);
	static void CompleteDfcByKErrNotFound(TAny* aQue);
	static void CompleteCancellationQDfc(TAny* aQue);
	
	static TUint Hash(TUint aCategory, TUint aKey);
	static TProperty** Lookup(TUint aCategory, TUint aKey);
	static TInt LookupOrCreate(TUint aCategory, TUint aKey, TProperty**);

	TAny* operator new(TUint aSize) __NO_THROW
		{ return Kern::AllocZ(aSize); }

	TProperty(TUint aCategory, TUint aKey);

	void SetNotDefined()
		{ iType = RProperty::ETypeLimit; }

	inline void Use();
	void Release();

	void CompleteByDfc();

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	TBool DoCheckDeleteRights(DProcess* aProcess, const char* aDiagnostic);
	TBool DoCheckDefineRights(DProcess* aProcess, const char* aDiagnostic);
	TBool DoCheckGetRights(DProcess* aProcess, const char* aDiagnostic);
	TBool DoCheckSetRights(DProcess* aProcess, const char* aDiagnostic);
#endif //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	TBool DoCheckDeleteRights(DProcess* aProcess);
	TBool DoCheckDefineRights(DProcess* aProcess);
	TBool DoCheckGetRights(DProcess* aProcess);
	TBool DoCheckSetRights(DProcess* aProcess);

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	inline TBool CheckDeleteRights(DProcess* aProcess, const char* aDiagnostic=0)
		{
		return DoCheckDeleteRights(aProcess, aDiagnostic);
		}

	inline TBool CheckDefineRights(DProcess* aProcess, const char* aDiagnostic=0)
		{
		return DoCheckDefineRights(aProcess, aDiagnostic);
		}

	inline TBool CheckGetRights(DProcess* aProcess, const char* aDiagnostic=0)
		{
		return DoCheckGetRights(aProcess, aDiagnostic);
		}

	inline TBool CheckSetRights(DProcess* aProcess, const char* aDiagnostic=0)
		{
		return DoCheckSetRights(aProcess, aDiagnostic);
		}

#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	// Only available to NULL arguments
	inline TBool CheckDeleteRights(DProcess* aProcess, OnlyCreateWithNull /*aDiagnostic*/=NULL)
		{
		return DoCheckDeleteRights(aProcess);
		}

	inline TBool CheckDefineRights(DProcess* aProcess, OnlyCreateWithNull /*aDiagnostic*/=NULL)
		{
		return DoCheckDefineRights(aProcess);
		}

	inline TBool CheckGetRights(DProcess* aProcess, OnlyCreateWithNull /*aDiagnostic*/=NULL)
		{
		return DoCheckGetRights(aProcess);
		}

	inline TBool CheckSetRights(DProcess* aProcess, OnlyCreateWithNull /*aDiagnostic*/=NULL)
		{
		return DoCheckSetRights(aProcess);
		}

#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

	enum { KCancellationDfcPriority = 1, KCompletionDfcPriority = 2  };
	static TDfc		CompletionDfc;
	static TDfc		CompletionDfcPermissionDenied;
	static TDfc		CompletionDfcNotFound;
	static TDfc		CancellationDfc;
	
	// subscriptions to be completed by the DFCs, protected by system lock
	static SDblQue	CompletionQue; // to be completed by KerrNone
	static SDblQue	CompletionQuePermissionDenied;
	static SDblQue	CompletionQueNotFound;
	static SDblQue	CancellationQue;		

	static DMutex*	FeatureLock;			///< Order KMutexOrdPubSub

	// hash table collision lists
	enum { KHashTableLimit = 32 };	// must be power of 2
	static TProperty*	Table[KHashTableLimit];	// protected by the feature lock
	
#ifdef __DEMAND_PAGING__
	static DMutex*	PagingLockMutex;		///< Mutex used to protect demand paging lock, order KMutexOrdPubSub2
	static DDemandPagingLock* PagingLock; 	///< Locks user memory when copying large properties
#endif
	
	// iType == RProperty::ETypeLimit means not defined
	TUint8	iType;	// updates require the system lock AND the feature lock 
					// reads require only one of them
	// The property attributes.
	// Meaningful for defined properties only (ie. iType != RProperty::ETypeLimit)
	// Constant while the property is defined
	TUint8	iAttr;
	TCompiledSecurityPolicy iReadPolicy;
	TCompiledSecurityPolicy iWritePolicy;
	TUint32	iOwner;

	TUint		iRefCount;	// protected by the feature lock
	TProperty*	iNext;		// hash table collision list link -
							//		protected by the feature lock

	class TBuf
		{ // Viraiable-size buffer for  byte array property values
	public:
		static TBuf* New(TInt aSize);

		TUint16	iBufSize;		// buffer size - constant
		TUint16	iSize;			// actual property size - protected by the system lock
		TUint8	iBytes[1];		// byte array - protected by the system lock
		};

	// The property value
	// Meaningful for defined properties only (ie. iType != RProperty::ETypeLimit)
	union	// the value (ie. iValue or iBuf->iBytes) is protected by the system lock
		{ 
		TBuf*	iBuf;   // pointer updates of a defined property (eg. buffer 
						//   reallocation) require the system AND the feature locks;
						// pointer reads (eg to get/set byte values) require any of them
		TInt	iValue;
		};

	// Called with system or feature locked
	TInt Size()
		{
		__PS_ASSERT(iType == RProperty::EByteArray || iType == RProperty::ELargeByteArray);
		return iBuf ? iBuf->iSize : 0;
		}

	// Called with system or feature locked
	TUint8* Buf()
		{
		__PS_ASSERT(iType == RProperty::EByteArray || iType == RProperty::ELargeByteArray);
		return iBuf ? iBuf->iBytes : NULL;
		}

	// Called with system or feature locked
	void SetSize(TInt aSize)
		{
		__PS_ASSERT(TUint(aSize) <= TUint(BufSize()));
		if (iBuf)
			iBuf->iSize = TUint16(aSize);
		}

	TInt _GetB(TUint8* aBuf, TInt* aSize, DProcess* aProcess, TBool aUser);
	TInt _SetB(const TUint8*, TInt aSize, DProcess*, TBool aUser, TBuf** aNewBufHdr);

	SDblQue	iPendingQue;	// pending subscriptions - protected by the system lock
	};


// Completion/Cancelation DFCs and their corresponding queues.
// All subscribe requests are completed in Supervisor thread.
TDfc		TProperty::CompletionDfc(TProperty::CompleteDfc, &TProperty::CompletionQue, KCompletionDfcPriority);
SDblQue		TProperty::CompletionQue;

TDfc		TProperty::CompletionDfcPermissionDenied(TProperty::CompleteDfcByKErrPermissionDenied, &TProperty::CompletionQuePermissionDenied, KCompletionDfcPriority);
SDblQue		TProperty::CompletionQuePermissionDenied;

TDfc		TProperty::CompletionDfcNotFound(TProperty::CompleteDfcByKErrNotFound, &TProperty::CompletionQueNotFound, KCompletionDfcPriority);
SDblQue		TProperty::CompletionQueNotFound;

TDfc		TProperty::CancellationDfc(TProperty::CompleteCancellationQDfc, &TProperty::CancellationQue, KCancellationDfcPriority);
SDblQue		TProperty::CancellationQue;

DMutex*		TProperty::FeatureLock;	
TProperty*	TProperty::Table[KHashTableLimit];

#ifdef __DEMAND_PAGING__
DMutex*		TProperty::PagingLockMutex;
DDemandPagingLock* TProperty::PagingLock;
_LIT(KPubSubMutexName2, "PropertyPagingLockMutex");
#endif

TInt PubSubPropertyInit()
	{ return TProperty::Init(); }

_LIT(KPubSubMutexName, "PropertyLock");

TInt TProperty::Init()
	{ // static
	TInt r = Kern::MutexCreate(FeatureLock, KPubSubMutexName, KMutexOrdPubSub);
	if (r != KErrNone)
		return r;
	CompletionDfc.SetDfcQ(K::SvMsgQ);
	CompletionDfcPermissionDenied.SetDfcQ(K::SvMsgQ);
	CompletionDfcNotFound.SetDfcQ(K::SvMsgQ);
	CancellationDfc.SetDfcQ(K::SvMsgQ);
	
#ifdef __DEMAND_PAGING__	
	r = Kern::MutexCreate(PagingLockMutex, KPubSubMutexName2, KMutexOrdPubSub2);
	if (r != KErrNone)
		return r;
	PagingLock = new DDemandPagingLock();
	if (!PagingLock)
		return KErrNoMemory;
	r = PagingLock->Alloc(RProperty::KMaxLargePropertySize);
	if (r != KErrNone)
		return r;
#endif
	
	return KErrNone;
	}

TProperty::TProperty(TUint aCategory, TUint aKey) : iCategory(aCategory), iKey(aKey)
	{ SetNotDefined(); }

TUint TProperty::Hash(TUint aCategory, TUint aKey)
	{ // a naive hash function 
	TUint code = (aCategory ^ aKey) & (KHashTableLimit - 1);
	__PS_ASSERT(code < KHashTableLimit);	// KHashTableLimit must be a power of 2
	return code;
	}

// Called feature locked.
TProperty** TProperty::Lookup(TUint aCategory, TUint aKey)
	{ // static
	TProperty** propP = &Table[Hash(aCategory, aKey)];
	for (;;)
		{
		TProperty* prop = *propP;
		if (!prop) break;
		if ((prop->iCategory == aCategory) && (prop->iKey == aKey)) break;
		propP = &prop->iNext;
		}
	return propP;
	}

// Called feature locked.
TInt TProperty::LookupOrCreate(TUint aCategory, TUint aKey, TProperty** aProp)
	{ // static
	TProperty** propP = Lookup(aCategory, aKey);
	TProperty* prop = *propP;
	if (!prop)
		{
		prop = new TProperty(aCategory, aKey);
		if (!prop)
			{
			return KErrNoMemory;
			}
		*propP = prop;			
		}
	*aProp = prop;
	return KErrNone;
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
TBool TProperty::DoCheckDefineRights(DProcess* aProcess, const char* aDiagnostic)
	{
	TUint32 cat=iCategory;
	if(cat==aProcess->iS.iSecureId)
		return ETrue;
	if(cat<(TUint32)KUidSecurityThresholdCategoryValue)
		return aProcess->HasCapability(ECapabilityWriteDeviceData, aDiagnostic);
	PlatSec::SecureIdCheckFail(aProcess, cat, aDiagnostic);
	return EFalse;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

TBool TProperty::DoCheckDefineRights(DProcess* aProcess)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return DoCheckDefineRights(aProcess, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	TUint32 cat=iCategory;
	if(cat==aProcess->iS.iSecureId)
		return ETrue;
	if(cat<(TUint32)KUidSecurityThresholdCategoryValue)
		return aProcess->HasCapability(ECapabilityWriteDeviceData);
	return EFalse;
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
// Called feature locked
TBool TProperty::DoCheckDeleteRights(DProcess* aProcess, const char* aDiagnostic)
	{
	__ASSERT_MUTEX(FeatureLock);
	if(aProcess->iS.iSecureId == Owner())
		{
		return ETrue;
		}
	PlatSec::SecureIdCheckFail(aProcess, Owner(), aDiagnostic);
	return EFalse;
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

TBool TProperty::DoCheckDeleteRights(DProcess* aProcess)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return DoCheckDeleteRights(aProcess, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	__ASSERT_MUTEX(FeatureLock);
	if(aProcess->iS.iSecureId == Owner())
		{
		return ETrue;
		}
	return EFalse;
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
// Called system locked
TBool TProperty::DoCheckGetRights(DProcess* aProcess, const char* aDiagnostic)
	{
	return iReadPolicy.CheckPolicy(aProcess, aDiagnostic);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

TBool TProperty::DoCheckGetRights(DProcess* aProcess)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return DoCheckGetRights(aProcess, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return iReadPolicy.CheckPolicy(aProcess);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	}

#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
// Called system locked
TBool TProperty::DoCheckSetRights(DProcess* aProcess, const char* aDiagnostic)
	{
	return iWritePolicy.CheckPolicy(aProcess, aDiagnostic);
	}
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__

TBool TProperty::DoCheckSetRights(DProcess* aProcess)
	{
#ifndef __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return DoCheckSetRights(aProcess, NULL);
#else //__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	return iWritePolicy.CheckPolicy(aProcess);
#endif // !__REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
	}

// Called in CS
TProperty::TBuf* TProperty::TBuf::New(TInt aSize)
	{ // static
	__ASSERT_CRITICAL;
	TBuf* buf = (TBuf*) Kern::Alloc(_FOFF(TBuf, iBytes) + aSize);
	if (!buf)
		{
		return NULL;
		}
	buf->iBufSize = TUint16(aSize);
	buf->iSize = 0;
	return buf;
	}

// Called in CS
TInt TProperty::Define(const TPropertyInfo* aInfo, DProcess* aProcess)
	{
	__ASSERT_CRITICAL;
	if (aProcess && !CheckDefineRights(aProcess,__PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Define a Publish and Subscribe Property")))
		{
		return KErrPermissionDenied;
		}
	TCompiledSecurityPolicy readPolicy;
	TInt r = readPolicy.Set((aInfo->iReadPolicy));
	if(r != KErrNone)
		{
		return r;
		}
	TCompiledSecurityPolicy writePolicy;
	r = writePolicy.Set((aInfo->iWritePolicy));
	if(r != KErrNone)
		{
		return r;
		}
	if ((aInfo->iType < 0) || (RProperty::ETypeLimit <= aInfo->iType) ||
		((aInfo->iType == RProperty::EInt) && aInfo->iSize) ||
		(aInfo->iAttr != 0))
		{
		return KErrArgument;
		}
	if ((aInfo->iType == RProperty::EByteArray && aInfo->iSize > RProperty::KMaxPropertySize))
		{
		return KErrTooBig;
		}
	Lock();
	if (IsDefined())
		{
		Unlock();
		return KErrAlreadyExists;
		}
	if (aInfo->iSize)
		{
		__PS_ASSERT((aInfo->iType == RProperty::EByteArray) || (aInfo->iType == RProperty::ELargeByteArray));
		__PS_ASSERT(iBuf == NULL);
		iBuf = TBuf::New(aInfo->iSize);
		}

	iReadPolicy = readPolicy;
	iWritePolicy = writePolicy;
	iOwner = CurProcess()->iS.iSecureId;
	iAttr = 0;

	// We need to check all pending requests for read access rights and 
	// complete those that havn't.
	// We have the following constraints:
	//	- we don't want to hold the system lock iterating through 'iPendingQue' list.
	//	- as soon as the system lock is released new entries can be added by 
	//    'Subscribe()' and existing entries can be removed by 'Cancel()'.
	//	- we don't want to complete a request holding the feature lock
	// To deal with these issues we move all  'iPendingQue' entries to one of 
	// two temporary queues:
	//	'localPendingQue' for entries that will remain pending
	//	'localCompletionQue' for entries that will be completed
	// We move entries one by one dropping and reacquiring the system lock on each 
	// iteration; we get (move) always the first 'iPendingQue' entry until the 
	// queue becomes empty.
	//

	SDblQue localPendingQue;    // Will hold requests with sufficient capabilities. 
	TInt accessDeniedCounter = 0;// Will count requests with no sufficient capabilities.
	NKern::LockSystem();
	while (!iPendingQue.IsEmpty())
		{
		TPropertySubsRequest* subs = (TPropertySubsRequest*) iPendingQue.GetFirst();
		DProcess* process = subs->iProcess;
		subs->iProcess = NULL;
		if (process && !CheckGetRights(process, __PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Subscribe to a Publish and Subscribe Property")))
			{ // Check fails - will complete the subscription with an error
			CompletionQuePermissionDenied.Add(subs);
			accessDeniedCounter++;
			}
		else
			{ // Check OK - will leave in the pending queue 
			localPendingQue.Add(subs);
			}
		NKern::FlashSystem();	// preemption point
		}
	iPendingQue.MoveFrom(&localPendingQue);
	iType = (TUint8) aInfo->iType;
	NKern::UnlockSystem();
	// Now the property can be accessed by other threads.
	Use();
	Unlock();

	// Schedule DFC to complete requests of those with insufficient capabilities.
	if (accessDeniedCounter)
		CompletionDfcPermissionDenied.Enque();
	return KErrNone;
	}

// Called in CS
TInt TProperty::Attach(TUint aCategory, TUint aKey, TProperty** aProp)
	{ //static 
	Lock();
	TProperty* prop;
	// Attach can create a non defined property.
	TInt r = LookupOrCreate(aCategory, aKey, &prop);
	if (r != KErrNone) 
		{
		Unlock();
		return r;
		}
	prop->Use();
	*aProp = prop;
	Unlock();
	return KErrNone;
	}

// Called in CS
TInt TProperty::Open(TUint aCategory, TUint aKey, TProperty** aProp)
	{ //static 
	Lock();
	TProperty* prop = *Lookup(aCategory, aKey);
	if (!prop) 
		{
		Unlock();
		return KErrNotFound;
		}
	prop->Use();
	*aProp = prop;
	Unlock();
	return KErrNone;
	}

// Called in CS
void TProperty::Close()
	{
	Lock();
	Release();
	// '*this' may do not exist any more
	Unlock();
	}

// Called in CS
TInt TProperty::Delete(DProcess* aProcess)
	{
	Lock();
	if (!IsDefined())
		{
		Unlock();
		return KErrNotFound;
		}
	if (aProcess && !CheckDeleteRights(aProcess,__PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Delete a Publish and Subscribe Property")))
		{
		Unlock();
		return KErrPermissionDenied;
		}
	NKern::LockSystem();
	// Remember that iType is the tag of iBuf and iValue union
	TBuf* buf = (iType != RProperty::EInt) ? iBuf : NULL;
	SetNotDefined();
	// Down from here nobody can access the property value

	// Move all pending requests to completion queue (to be completed by KErrNotFound).
	TBool pendingQueEmpty = iPendingQue.IsEmpty();
	if(!pendingQueEmpty)
		CompletionQueNotFound.MoveFrom(&iPendingQue);

	NKern::UnlockSystem();

	iBuf = NULL;
	iValue = 0;
	delete buf;
	Release();
	// '*this' may do not exist any more
	Unlock();

	 // Schedule Svc Dfc to complete all requests from CompletionQueNotFound.
	if (!pendingQueEmpty)
		CompletionDfcNotFound.Enque();
	return KErrNone;
	}

// Enter feature locked.
// Return feature locked.
inline void TProperty::Use()
	{
	__ASSERT_MUTEX(FeatureLock);
	++iRefCount;
	}

// Enter feature locked.
// Return feature locked.
void TProperty::Release()
	{
	__ASSERT_MUTEX(FeatureLock);
	__PS_ASSERT(iRefCount);
	if (--iRefCount == 0)
		{
		__PS_ASSERT(!IsDefined()); // property must not be defined.
		// Lookup to find the previous element in the simply linked collision list.
		TProperty** propP = Lookup(iCategory, iKey);
		*propP = iNext;
		delete this;
		}
	}

// Enter system locked.
// Return system unlocked.
TInt TProperty::Subscribe(TPropertySubsRequest* aSubs, DProcess* aProcess)
	{
	__ASSERT_SYSTEM_LOCK;
	if (aSubs->iNext)
		{ // already pending - will Panic
		NKern::UnlockSystem();
		return KErrInUse;
		}
	TBool defined = IsDefined();
	if (defined)
		{ // property is defined - check access now
		aSubs->iProcess = NULL;	
		if (aProcess && !CheckGetRights(aProcess,__PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Subscribe to a Publish and Subscribe Property")))
			{
			CompletionQuePermissionDenied.Add(aSubs);
			CompletionDfcPermissionDenied.Enque(SYSTEM_LOCK);
			return KErrNone;
			}
		}	
	else
		{ // will check when defined
		aSubs->iProcess = aProcess;
		}
	iPendingQue.Add(aSubs);
	NKern::UnlockSystem();
	return KErrNone;
	}

// Enter system locked.
// Return system unlocked.
void TProperty::Cancel(TPropertySubsRequest* aSubs)
	{
	__ASSERT_SYSTEM_LOCK;

	// This is set if the request is about to be completed (in SVC thread). In that case, a 'dummy' CancelationDFC
	// will be scheduled here. This will just ensure that we don't return from Cancel before ongoing completion has finished.  
	TBool scheduledForCompletition = (TInt)(aSubs->iProcess) & TPropertySubsRequest::KScheduledForCompletion;

	if (!aSubs->iNext && !scheduledForCompletition)
		{ // Neither in any queue nor scheduled for completion.
		  // The request is not active - silently return.
		NKern::UnlockSystem();
		return;
		}

	if (aSubs->iNext)
		{
		// Take it out from the current queue. It is usually pending queue of a property but could
		// also be one of the completion queues.
		aSubs->Deque();
		aSubs->iNext = NULL;
		}
	
	// Set process to NULL (leave KScheduledForCompletion bit as it is)
	aSubs->iProcess = (DProcess*)((TInt)aSubs->iProcess & ~TPropertySubsRequest::KProcessPtrMask);
	
	if (&Kern::CurrentThread() == K::SvThread)
		{   // Complete the request immediatelly if already running in supervisor thread...
		if (!scheduledForCompletition)
			CompleteRequest(aSubs, KErrCancel); //This will also release system lock.
		else
			NKern::UnlockSystem();              // Nothing to be done here. Just release system lock.
		}
	else
		{   //... or schedule DFC in supervisor thread to complete the request.
		TCancelQ linkQ;
		if (!scheduledForCompletition)
			linkQ.iPropSubRequest = aSubs;      // CancelationDFC will complete this request with KErrCancel.
		else
			linkQ.iPropSubRequest = NULL;       // Schedule 'dummy' CancelationDFC (no request will be completed).
		linkQ.iFSemaphore.iOwningThread = NKern::CurrentThread();
		CancellationQue.Add(&linkQ);
		CancellationDfc.Enque(SYSTEM_LOCK);     // This will also release system lock.

		NKern::FSWait(&linkQ.iFSemaphore);      // Wait for CancellationDfc to finish.
		}
	}

// Enter system locked.
// Return system unlocked.
// Executed in supervisor thread.
void TProperty::CompleteRequest(TPropertySubsRequest* aSubs, TInt aResult)
	{ // static
	__ASSERT_SYSTEM_LOCK;
	__PS_ASSERT(&Kern::CurrentThread() == K::SvThread);
	TPropertyCompleteFn	fn = aSubs->iCompleteFn;
	TAny* ptr = aSubs->iPtr;
	// Mark that this request is about to be completed.
	aSubs->iProcess = (DProcess*)((TInt)aSubs->iProcess | TPropertySubsRequest::KScheduledForCompletion);
	NKern::UnlockSystem();
	(*fn)(ptr, aResult);
	}

// Executed in supervisor thread.
void TProperty::CompleteQue(SDblQue* aQue, TInt aReason)
	{ // static
	__PS_ASSERT(&Kern::CurrentThread() == K::SvThread);
	NKern::LockSystem();
	while (!aQue->IsEmpty())
		{ 
		TPropertySubsRequest* subs = (TPropertySubsRequest*) aQue->First();
		subs->Deque();
		subs->iNext = NULL;
		CompleteRequest(subs, aReason);
		NKern::LockSystem();
		}	 
	NKern::UnlockSystem();
	}

// Executed in supervisor thread. Completes requests with KErrNone.
void TProperty::CompleteDfc(TAny* aQue)
	{ // static
	CompleteQue((SDblQue*) aQue, KErrNone);
	}

// Executed in supervisor thread.
void TProperty::CompleteDfcByKErrPermissionDenied(TAny* aQue)
	{ // static
	CompleteQue((SDblQue*) aQue, KErrPermissionDenied);
	}

// Executed in supervisor thread.
void TProperty::CompleteDfcByKErrNotFound(TAny* aQue)
	{ // static
	CompleteQue((SDblQue*) aQue, KErrNotFound);
	}

// Executed in supervisor thread.
void TProperty::CompleteCancellationQDfc(TAny* aAny)
	{ // static
	SDblQue* aQue = (SDblQue*)aAny;
	NKern::LockSystem();
	while (!aQue->IsEmpty() )
		{
        TCancelQ* first = static_cast<TCancelQ*>(aQue->First());
        first->Deque();
        first->iNext = NULL;

        if( first->iPropSubRequest)
        	{
            CompleteRequest(first->iPropSubRequest, KErrCancel); // This will also release system lock.
            NKern::LockSystem();
        	}
        else
        	{
        	// Do not complete the request.
        	// It was already just about to be completed when Cancel request was issued.
			// As all complitions (this method included) run is Svc thread, we can here be sure that
			// the request is now completed.
			NKern::FlashSystem();	// Preemption point
        	}
        NKern::FSSignal( &first->iFSemaphore ); // Issue the signal that the request is now completed.
        }
    NKern::UnlockSystem();
	}

// Enter system locked.
// Return system unlocked.
void TProperty::CompleteByDfc()
	{
	__ASSERT_SYSTEM_LOCK;
	// Append elements from the pending queue to the completion queue
	if (!iPendingQue.IsEmpty())
		{
		CompletionQue.MoveFrom(&iPendingQue);
		CompletionDfc.Enque(SYSTEM_LOCK);
		}
	else 
		{
		NKern::UnlockSystem();
		}
	}
	
// Enter system locked.
// Return system unlocked.
TInt TProperty::GetI(TInt* aValue, DProcess* aProcess)
	{
	__ASSERT_SYSTEM_LOCK;
	if (!IsDefined())
		{
		NKern::UnlockSystem();
		return KErrNotFound;
		}
	if (aProcess && !CheckGetRights(aProcess,__PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Get a Publish and Subscribe Property")))
		{
		NKern::UnlockSystem();
		return KErrPermissionDenied;
		}
	if (iType != RProperty::EInt)
		{
		NKern::UnlockSystem();
		return KErrArgument;
		}
	*aValue = iValue;
	NKern::UnlockSystem();
	return KErrNone;
	}

TInt TProperty::FindGetI(TUint aCategory, TUint aKey, TInt* aValue, DProcess* aProcess)
	{
	Lock();
	TProperty* prop = *TProperty::Lookup(aCategory, aKey);
	TInt r;
	if (!prop) 
		r = KErrNotFound;
	else
		{
		NKern::LockSystem();
		r = prop->GetI(aValue, aProcess);
		}
	Unlock();
	return r;
	}

TInt TProperty::FindSetI(TUint aCategory, TUint aKey, TInt aValue, DProcess* aProcess)
	{
	Lock();
	TProperty* prop = *TProperty::Lookup(aCategory, aKey);
	TInt r;
	if (!prop) 
		r = KErrNotFound;
	else
		{
		NKern::LockSystem();
		r = prop->SetI(aValue, aProcess);
		}
	Unlock();
	return r;
	}

#ifdef __EPOC32__
extern "C" { extern void kumemput_no_paging_assert(TAny* /*aAddr*/, const TAny* /*aKernAddr*/, TInt /*aLength*/); }
#else
inline void kumemput_no_paging_assert(TAny* aAddr, const TAny* aKernAddr, TInt aLength)
	{kumemput(aAddr,aKernAddr,aLength);}
#endif

// Enter system locked.
// Return system unlocked.
TInt TProperty::_GetB(TUint8* aBuf, TInt* aSize, DProcess* aProcess, TBool aUser)
	{
	if (!IsDefined())
		{
		NKern::UnlockSystem();
		return KErrNotFound;
		}
	if (aProcess && !CheckGetRights(aProcess,__PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Get a Publish and Subscribe Property")))
		{
		NKern::UnlockSystem();
		return KErrPermissionDenied;
		}
	if (iType == RProperty::EInt)
		{
		NKern::UnlockSystem();
		return KErrArgument;
		}
	if (iType == RProperty::ELargeByteArray)
		{
		// copy with system lock released
		__ASSERT_MUTEX(FeatureLock);
		NKern::UnlockSystem();
		}
	TInt bufSize = *aSize;
	TInt size = Size();
	TInt r = KErrNone;
	if (bufSize < size) 
		{
		size = bufSize;
		r = KErrOverflow;
		}
	TUint8* buf = Buf();
	if (aUser)
		{ // Don't use 'r' to preserve possible KErrOverflow condition
		XTRAPD(res, XT_DEFAULT, kumemput_no_paging_assert(aBuf, buf, size));
		if (res != KErrNone)
			r = KErrBadDescriptor;
		}
	else
		{ 
		memcpy(aBuf, buf, size); 
		}
	*aSize = size;
	if (iType != RProperty::ELargeByteArray)
		{
		NKern::UnlockSystem();
		}
	return r;
	}

// Enter system locked.
// Return system unlocked.
TInt TProperty::GetB(TUint8* aBuf, TInt* aSize, DProcess* aProcess, TBool aUser)
	{
	__ASSERT_SYSTEM_LOCK;
	if (iType != RProperty::ELargeByteArray)
		return _GetB(aBuf,aSize,aProcess,aUser);

	// Acquire feature lock for accessing large properties
	NKern::ThreadEnterCS();
	FeatureLock->Wait();	// returns with system lock still held
	TInt r = _GetB(aBuf,aSize,aProcess,aUser);
	Unlock();
	NKern::ThreadLeaveCS();
	return r;
	}

// Enter system locked.
// Return system unlocked.
TInt TProperty::SetI(TInt aValue, DProcess* aProcess)
	{
	__ASSERT_SYSTEM_LOCK;
	if (!IsDefined())
		{
		NKern::UnlockSystem();
		return KErrNotFound;
		}
	if (aProcess && !CheckSetRights(aProcess, __PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Set a Publish and Subscribe Property")))
		{
		NKern::UnlockSystem();
		return KErrPermissionDenied;
		}
	if (iType != RProperty::EInt)
		{
		NKern::UnlockSystem();
		return KErrArgument;
		}
	iValue = aValue;
	CompleteByDfc(); //This will also release system lock.
	return KErrNone;
	}


#ifdef __EPOC32__
extern "C" { extern void kumemget_no_paging_assert(TAny* /*aKernAddr*/, const TAny* /*aAddr*/, TInt /*aLength*/); }
#else
inline void kumemget_no_paging_assert(TAny* aKernAddr, const TAny* aAddr, TInt aLength)
	{kumemget(aKernAddr,aAddr,aLength);}
#endif

// Enter with system locked.
// Return system unlocked.
TInt TProperty::_SetB(const TUint8* aBuf, TInt aSize, DProcess* aProcess, TBool aUser, TBuf** aNewBuf)
	{
	if (!IsDefined())
		{
		NKern::UnlockSystem();
		return KErrNotFound;
		}
	if (aProcess && !CheckSetRights(aProcess, __PLATSEC_DIAGNOSTIC_STRING("Checked whilst trying to Set a Publish and Subscribe Property")))
		{
		NKern::UnlockSystem();
		return KErrPermissionDenied;
		}
	if (iType == RProperty::EInt)
		{
		NKern::UnlockSystem();
		return KErrArgument;
		}
	else if (iType == RProperty::EByteArray)
		{
		if (aSize > RProperty::KMaxPropertySize)
			{
			NKern::UnlockSystem();
			return KErrTooBig;
			}
		}
	else if (iType == RProperty::ELargeByteArray)
		{
		__ASSERT_MUTEX(FeatureLock);
		NKern::UnlockSystem();
		if (aSize > RProperty::KMaxLargePropertySize)
			{
			return KErrTooBig;
			}
		}
	if (aSize > BufSize())
		{
		if (aNewBuf == NULL || *aNewBuf == NULL)
			{
			if (iType != RProperty::ELargeByteArray)
				{
				NKern::UnlockSystem();
				}
			return KErrOverflow;
			}
		// New buffer is provided by the caller - switch on it
		TBuf* oldBuf = iBuf;
		iBuf = *aNewBuf;
		// Caller will deallocate the old buffer out of the system lock context
		*aNewBuf = oldBuf;
		}
	if (aUser)
		{
		__NK_ASSERT_DEBUG(iType==RProperty::ELargeByteArray);
		XTRAPD(r, XT_DEFAULT, kumemget_no_paging_assert(Buf(), aBuf, aSize));
		if (r != KErrNone)
			return KErrBadDescriptor;
		}
	else
		{
		memcpy(Buf(), aBuf, aSize); 
		}
	SetSize(aSize);
	if (iType == RProperty::ELargeByteArray)
		{
		NKern::LockSystem();
		}
	CompleteByDfc();
	return KErrNone;
	}

// Enter system locked.
// Return system unlocked.
TInt TProperty::SetB(const TUint8* aBuf, TInt aSize, DProcess* aProcess, TBool aUser)
	{ // static
	__ASSERT_SYSTEM_LOCK;
	TBuf* nBuf = 0;
	if (iType != RProperty::ELargeByteArray)
		{
		// Try to set without buffer reallocation (ie. "RT") first
		TInt r = _SetB(aBuf, aSize, aProcess, aUser, NULL);
		if (r != KErrOverflow)
			return r;

		// Needs buffer reallocation
		NKern::ThreadEnterCS();
		// Allocate a new buffer
		nBuf = TBuf::New(aSize);
		if (!nBuf)
			{
			NKern::ThreadLeaveCS();
			return KErrNoMemory;
			}
		NKern::LockSystem();
		
		r = _SetB(aBuf, aSize, aProcess, aUser, &nBuf);
		// May be the old buffer (or NULL), may be the new one (if wasn't used)
		delete nBuf;
		NKern::ThreadLeaveCS();
		return r;
		}
	
	NKern::ThreadEnterCS();

	// Large property - need feature lock
	FeatureLock->Wait();
	TInt r = _SetB(aBuf, aSize, aProcess, aUser, &nBuf);
	if (r == KErrOverflow)
		{
		// Allocate a new buffer
		nBuf = TBuf::New(aSize);
		if (!nBuf)
			r = KErrNoMemory;
		else
			{
			NKern::LockSystem();
			r = _SetB(aBuf, aSize, aProcess, aUser, &nBuf);
			}
		}
	Unlock();
	// May be the old buffer (or NULL), may be the new one (if wasn't used)
	delete nBuf;
	NKern::ThreadLeaveCS();
	return r;
	}

#ifdef __DEMAND_PAGING__

TInt TProperty::LockSourcePages(const TUint8* aBuf, TInt aSize)
	{
	// When copying from user RAM we must lock the source data in case it is demand paged, otherwise
	// we could get a mutex ordering voliation when waiting on the demand paging mutex
	__ASSERT_CRITICAL;
	Kern::MutexWait(*PagingLockMutex);
	TInt r = PagingLock->Lock(TheCurrentThread, (TLinAddr)aBuf, aSize);
	if(r>=0)
		return KErrNone;
	Kern::MutexSignal(*PagingLockMutex);
	return r;
	}

void TProperty::UnlockSourcePages()
	{
	__ASSERT_CRITICAL;
	PagingLock->Unlock();
	Kern::MutexSignal(*PagingLockMutex);
	}

#else

TInt TProperty::LockSourcePages(const TUint8*, TInt)
	{
	return KErrNone;
	}

void TProperty::UnlockSourcePages()
	{
	}

#endif

//
// User Interface
//
	
class DPropertyRef : public DObject
	{
public:

	static TInt Define(TUint aCategory, TUint aKey, const TPropertyInfo*);
	static TInt Delete(TUint aCategory, TUint aKey);
	static TInt Attach(TUint aCategory, TUint aKey, TOwnerType aType);

	static TInt FindGetI(TUint aCategory, TUint aKey, TInt* aValue);
	static TInt FindSetI(TUint aCategory, TUint aKey, TInt aValue);

	DPropertyRef();
	~DPropertyRef();

	TInt Subscribe(TRequestStatus* aStatus);
	inline void Cancel();

	TInt Close(TAny* /*aPtr*/);

	inline TInt GetI(TInt* aValue);
	inline TInt GetB(TUint8* aBuf, TInt* aSize, TBool aUser);
	inline TInt SetI(TInt aValue);
	TInt SetB(const TUint8* aBuf, TInt aSize, TBool aUser);

	inline RProperty::TType Type()
		{ return iProp->Type(); }
private:

	static void CompleteFn(TAny* aPtr, TInt aReason);

	TProperty*				iProp;
	TClientRequest*			iRequest;
	DThread*				iClient;
	TPropertySubsRequest	iSubs;
	};

DPropertyRef::DPropertyRef() : iSubs(CompleteFn, this)
	{
	}

// Called in CS
TInt DPropertyRef::Define(TUint aCategory, TUint aKey, const TPropertyInfo* aInfo)
	{ // static
	__ASSERT_CRITICAL;
	TProperty* prop;
	TInt r = TProperty::Attach(aCategory, aKey, &prop);
	if (r != KErrNone)
		{
		return r;
		}
	r = prop->Define(aInfo, CurProcess());
	prop->Close();
	return r;
	}

// Called in CS
TInt DPropertyRef::Delete(TUint aCategory, TUint aKey)
	{ // static
	__ASSERT_CRITICAL;
	TProperty* prop;
	TInt r = TProperty::Open(aCategory, aKey, &prop);
	if (r != KErrNone)
		{
		return r;
		}
	r = prop->Delete(CurProcess());
	prop->Close();
	return r;
	}

// Called in CS
TInt DPropertyRef::Attach(TUint aCategory, TUint aKey, TOwnerType aType)
	{ // static
	__ASSERT_CRITICAL;
	DPropertyRef* ref = new DPropertyRef();
	if (ref == NULL)
		{
		return KErrNoMemory;
		}
	TInt r = Kern::CreateClientRequest(ref->iRequest);
	if (r != KErrNone)
		{
		ref->Close(NULL);
		return r;
		}
	r = TProperty::Attach(aCategory, aKey, &ref->iProp);
	if (r != KErrNone)
		{
		ref->Close(NULL);
		return r;
		}
	r = K::AddObject(ref, EPropertyRef);
	if (r != KErrNone)
		{
		ref->Close(NULL);
		return r;
		}
	r = K::MakeHandle(aType, ref);
	if (r < 0)
		{ // error
		ref->Close(NULL);
		}
	return r;
	}

// Enter system locked.
// Return system unlocked.
TInt DPropertyRef::Subscribe(TRequestStatus* aStatus)
	{
	__ASSERT_SYSTEM_LOCK;
	if (iRequest->SetStatus(aStatus) != KErrNone)
		{ // already pending - will Panic
		NKern::UnlockSystem();
		return KErrInUse;
		}
	iClient = TheCurrentThread;
	iClient->Open();
	return iProp->Subscribe(&iSubs, CurProcess());
	}

// Called in CS or a DFC context
void DPropertyRef::CompleteFn(TAny* ptr, TInt aReason)
	{ // static
	__ASSERT_CRITICAL;
	DPropertyRef* self = (DPropertyRef*) ptr;
	// Local 'client' variable is necessary because after we call Kern::QueueRequestComplete the
	// structure may be reused for another subscription
	DThread* client = self->iClient;
	__PS_ASSERT(client);
	self->iClient = NULL;
	Kern::QueueRequestComplete(client, self->iRequest, aReason);
	client->Close(NULL);
	}


TInt DPropertyRef::Close(TAny* /*aPtr*/)
	{
	TInt error = KErrNone;
	if (Dec()==1)
		{
		NKern::LockSystem();
		iProp->Cancel(&iSubs); //Releases System Lock
		K::ObjDelete(this);
		return EObjectDeleted;
		}

	return error;
	}


// Enter system locked.
// Return system unlocked.
inline void DPropertyRef::Cancel()
	{
	__ASSERT_SYSTEM_LOCK;
	
	if (Open() != KErrNone) 
		{ // Too late - destruction in progress ....
		NKern::UnlockSystem();
		return;
		}
	
	iProp->Cancel(&iSubs);

	Close(NULL);
	}

// Enter system locked.
// Return system unlocked.
inline TInt DPropertyRef::GetI(TInt* aValue)
	{
	return iProp->GetI(aValue, CurProcess());
	}

// Enter system locked.
// Return system unlocked.
inline TInt DPropertyRef::GetB(TUint8* aBuf, TInt* aSize, TBool aUser)
	{
	return iProp->GetB(aBuf, aSize, CurProcess(), aUser);
	}

// Enter system locked.
// Return system unlocked.
inline TInt DPropertyRef::SetI(TInt aValue)
	{
	return iProp->SetI(aValue, CurProcess());
	}

// Called in CS
inline TInt DPropertyRef::FindGetI(TUint aCategory, TUint aKey, TInt* aValue)
	{
	return TProperty::FindGetI(aCategory, aKey, aValue, CurProcess());
	}

// Called in CS
inline TInt DPropertyRef::FindSetI(TUint aCategory, TUint aKey, TInt aValue)
	{
	return TProperty::FindSetI(aCategory, aKey, aValue, CurProcess());
	}


// Enter system locked.
// Return system unlocked.
TInt DPropertyRef::SetB(const TUint8* aBuf, TInt aSize, TBool aUser)
	{
	TBool open = 
		(iProp->Type() == RProperty::EByteArray) && (iProp->BufSize() < aSize);
	if (open)
		{
		// The buffer extension will release the system lock. 
		// We need to increment the DObject count to protect our-self against 
		// a concurrent DPropertyRef deletion by DObject::Close().
		if (Open() != KErrNone)
			{ // Too late - destruction in progress ....
			NKern::UnlockSystem();
			return KErrNotFound;
			}
		}
	TInt r = iProp->SetB(aBuf, aSize, CurProcess(), aUser);
	if (open)
		{
		Close(NULL);
		}
	return r;
	}

//
// User interface entry points
//

// Called in CS
DPropertyRef::~DPropertyRef()
	{
	__ASSERT_CRITICAL;
	if (iProp)
		{
		iProp->Close();
		iProp = NULL;
		}
	Kern::DestroyClientRequest(iRequest);
	}

TInt ExecHandler::PropertyDefine(TUint aCategory, TUint aKey, TPropertyInfo* aInfo)
	{
	TPropertyInfo info;
	kumemget(&info, aInfo, sizeof(info));
	if (aCategory == KMaxTUint)
		aCategory = TUint(Kern::CurrentProcess().iS.iSecureId);
	NKern::ThreadEnterCS();
	TInt r = DPropertyRef::Define(aCategory, aKey, &info);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::PropertyDelete(TUint aCategory, TUint aKey)
	{
	if (aCategory == KMaxTUint)
		aCategory = TUint(Kern::CurrentProcess().iS.iSecureId);
	NKern::ThreadEnterCS();
	TInt r = DPropertyRef::Delete(aCategory, aKey);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::PropertyAttach(TUint aCategory, TUint aKey, TOwnerType aType)
	{
	NKern::ThreadEnterCS();
	TInt r = DPropertyRef::Attach(aCategory, aKey, aType);
	NKern::ThreadLeaveCS();
	return r;
	}

// Enter system locked.
// Return system unlocked.
void ExecHandler::PropertySubscribe(DPropertyRef* aRef, TRequestStatus* aStatus)
	{
	TInt r = aRef->Subscribe(aStatus);
	if (r != KErrNone)
		{
		__PS_ASSERT(r == KErrInUse);
		K::PanicKernExec(ERequestAlreadyPending);
		}
	}

// Enter system locked.
// Return system unlocked.
void ExecHandler::PropertyCancel(DPropertyRef* aRef)
	{
	NKern::ThreadEnterCS();
	aRef->Cancel();
	NKern::ThreadLeaveCS();
	}

// Enter system locked.
// Return system unlocked.
TInt ExecHandler::PropertyGetI(DPropertyRef* aRef, TInt* aValue)
	{
	TInt value;
	TInt r = aRef->GetI(&value);
	if (r != KErrNone)
		{
		return r;
		}
	kumemput32(aValue, &value, sizeof(value));
	return KErrNone;
	}


// Enter system locked.
// Return system unlocked.
TInt ExecHandler::PropertyGetB(DPropertyRef* aRef, TUint8* aBuf, TInt aSize)
	{
	TInt r;

	if(aRef->Type()!=RProperty::ELargeByteArray)
		{
		// Use intermediate kernel buffer for small properties
		TUint8 sbuf[RProperty::KMaxPropertySize];
		if(aSize > (TInt)sizeof(sbuf))
			aSize = sizeof(sbuf);
		r = aRef->GetB(sbuf, &aSize, EFalse);
		if(r==KErrNone || r==KErrOverflow)
			kumemput(aBuf,sbuf,aSize);
		}
	else
		{
		// For large properties we must pin user memory to prevent demand paging.
		aRef->CheckedOpen();
		NKern::ThreadEnterCS();
		NKern::UnlockSystem();

		if(aSize>RProperty::KMaxLargePropertySize)
			aSize = RProperty::KMaxLargePropertySize;
		r = TProperty::LockSourcePages(aBuf, aSize);
		if(r>=0)
			{
			NKern::LockSystem();
			r = aRef->GetB(aBuf, &aSize, ETrue);
			TProperty::UnlockSourcePages();
			}

		aRef->Close(0);
		NKern::ThreadLeaveCS();
		}

	if (r == KErrBadDescriptor)
		{
		K::PanicKernExec(ECausedException);
		}
	else if (r != KErrNone)
		{
		return r;
		}
	return aSize;
	}

// Enter system locked.
// Return system unlocked.
TInt ExecHandler::PropertySetI(DPropertyRef* aRef, TInt aValue)
	{
	return aRef->SetI(aValue);
	}

// Enter and return system unlocked.
TInt ExecHandler::PropertySetB(TInt aPropertyRefHandle, const TUint8* aBuf, TInt aSize)
	{
	// If property small enough then copy it to local buffer
	TBool user = ETrue;
	TUint8 sbuf[RProperty::KMaxPropertySize];
	if(aSize<=RProperty::KMaxPropertySize)
		{
		kumemget(sbuf,aBuf,aSize);
		aBuf = sbuf;
		user = EFalse;
		}
	
	NKern::ThreadEnterCS();

	TInt r = KErrNone;
	if (user)
		r = TProperty::LockSourcePages(aBuf, aSize);
	
	// Set the property
	if(r>=0)
		{
		NKern::LockSystem();
		DPropertyRef* propertyRef = (DPropertyRef*)TheCurrentThread->ObjectFromHandle(aPropertyRefHandle,EPropertyRef);
		if (propertyRef)
			{
			r = propertyRef->SetB(aBuf, aSize, user);
			// unlocks system lock
			}
		else
			{
			r = KErrBadHandle;
			NKern::UnlockSystem();
			}

		if (user)
			TProperty::UnlockSourcePages();
		}
	
	NKern::ThreadLeaveCS();
	
	if (r == KErrBadDescriptor)
		K::PanicKernExec(ECausedException);
	if (r == KErrBadHandle)
		K::PanicKernExec(EBadHandle);
	return r;
	}

TInt ExecHandler::PropertyFindGetI(TUint aCategory, TUint aKey, TInt* aValue)
	{
	TInt value;
	NKern::ThreadEnterCS();
	TInt r = DPropertyRef::FindGetI(aCategory, aKey, &value);
	NKern::ThreadLeaveCS();
	if (r != KErrNone)
		{
		return r;
		}
	kumemput32(aValue, &value, sizeof(value));
	return KErrNone;
	}

TInt ExecHandler::PropertyFindGetB(TUint aCategory, TUint aKey, TUint8* aBuf, 
								   TInt aSize)
	{
	NKern::ThreadEnterCS();

	TProperty* prop;
	TInt r = TProperty::Open(aCategory, aKey, &prop);
	if (r == KErrNone)
		{
		if(prop->Type()!=RProperty::ELargeByteArray)
			{
			// Use intermediate kernel buffer for small properties
			TUint8 sbuf[RProperty::KMaxPropertySize];
			if(aSize > (TInt)sizeof(sbuf))
				aSize = sizeof(sbuf);
			NKern::LockSystem();
			r = prop->GetB(sbuf, &aSize, CurProcess(), EFalse);
			if(r==KErrNone || r==KErrOverflow)
				kumemput(aBuf,sbuf,aSize);
			}
		else
			{
			// For large properties we must pin the use memory to prevent demand paging.
			if(aSize>RProperty::KMaxLargePropertySize)
				aSize = RProperty::KMaxLargePropertySize;
			r = TProperty::LockSourcePages(aBuf, aSize);
			if(r>=0)
				{
				NKern::LockSystem();
				r = prop->GetB(aBuf, &aSize, CurProcess(), ETrue);
				TProperty::UnlockSourcePages();
				}
			}
		prop->Close();
		}

	NKern::ThreadLeaveCS();

	if (r == KErrBadDescriptor)
		{
		K::PanicKernExec(ECausedException);
		}
	else if (r != KErrNone)
		{
		return r;
		}
	return aSize;
	}

TInt ExecHandler::PropertyFindSetI(TUint aCategory, TUint aKey, TInt aValue)
	{
	NKern::ThreadEnterCS();
	TInt r = DPropertyRef::FindSetI(aCategory, aKey, aValue);
	NKern::ThreadLeaveCS();
	return r;
	}

TInt ExecHandler::PropertyFindSetB(TUint aCategory, TUint aKey, TUint8* aBuf, 
								   TInt aSize)
	{
	// If property small enough then copy it to local buffer
	TBool user = ETrue;
	TUint8 sbuf[RProperty::KMaxPropertySize];
	if(aSize<=RProperty::KMaxPropertySize)
		{
		kumemget(sbuf,aBuf,aSize);
		aBuf = sbuf;
		user = EFalse;
		}

	NKern::ThreadEnterCS();

	TInt r = KErrNone;
	if (user)
		r = TProperty::LockSourcePages(aBuf, aSize);
	
	// Find and open property
	if(r>=0)
		{
		TProperty* prop;
		r = TProperty::Open(aCategory, aKey, &prop);
		if (r == KErrNone)
			{
			// Set the property
			NKern::LockSystem();
			r = prop->SetB(aBuf, aSize, CurProcess(), user);
			prop->Close();
			}
		
		if (user)
			TProperty::UnlockSourcePages();
		}

	NKern::ThreadLeaveCS();
	if (r == KErrBadDescriptor)
		K::PanicKernExec(ECausedException);
	return r;
	}

/** Attaches to a property.

	This performs the same action as RPropertyRef::Open(). However, if the property does
	not exist then it is created first.
	
  	@param aCategory The property category.
	@param aKey The property sub-key.
	
	@return KErrNone, if successful;
	        KErrNoMemory, if insufficient memory.
	
	@pre Calling thread must be in a critical section.
	@pre Property has not been opened.
	@pre Call in a thread context.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
	@pre No fast mutex can be held

	@post Calling thread is in a critical section.

	@see RPropertyRef::Open()
*/
EXPORT_C TInt RPropertyRef::Attach(TUid aCategory, TInt aKey)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPropertyRef::Attach");			
	__PS_ASSERT(iProp == NULL);
	return TProperty::Attach(TUint(aCategory.iUid), aKey, &iProp);
	}
	
/** Opens a property.

 	Once opened:
 	 - the property can be defined, deleted, and  subscribed to
 	 - the property value can be retrieved (read) and published (written)
	using the appropriate member-functions in the appropriate order.
	
	Note that the property must already exist. If it does not exist,
	the function will not create it - it will return an error.
	Use RPropertyRef::Attach() if you need to create the property.

	@param aCategory The property category.
	@param aKey The property sub-key.
	
	@return	KErrNotFound, if the property does not exist;
			KErrNone, if successful;
			Otherwise, one of the other system-wide error codes.

	@pre Calling thread must be in a critical section.
	@pre Property has not been opened.
	@pre Call in a thread context.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
	@pre No fast mutex can be held

	@post Calling thread is in a critical section.
*/
EXPORT_C TInt RPropertyRef::Open(TUid aCategory, TInt aKey)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPropertyRef::Open");			
	__PS_ASSERT(iProp == NULL);
	return TProperty::Open(TUint(aCategory.iUid), aKey, &iProp);
	}

/** Releases the property.

	@pre Calling thread must be in a critical section.
	@pre Call in a thread context.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
	@pre No fast mutex can be held

	@post Calling thread is in a critical section.
*/
EXPORT_C void RPropertyRef::Close()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPropertyRef::Close");			
	if (iProp)
		{
		iProp->Close();
		iProp = NULL;
		}
	}

/** Defines the attributes and access control for a property.

    This can only be done once for each property.

	Following defintion, the property has a default value:
	- 0 for integer properties
	- zero-length data for byte-array and text properties.
	
	Pending subscriptions for this property will not be completed
	until a new value is published.

	@param aAttr		Bits 0-7 define the property type.
						Upper bits contain additional property attributes.
	@param aReadPolicy  Read policy. Defines the set capabilities that a process 
	                    will require before being allowed to retrieve (read) the
	                    property value.
	@param aWritePolicy Write policy. Defines the set capabilities that a process 
	                    will require before being allowed to publish (write) the
	                    property value.
	@param aPreallocate Pre-allocated buffer size for EByteArray or ELargeByteArray 
	                    property types.
	@param aProcess		If defining a property within the 'system services' category, 
	                    as defined by the KUidSystemCategoryValue UId, then this
	                    is the process whose security attributes will be checked
	                    to see whether it has sufficient authority to perform
	                    this define operation.
						If NULL, these security checks will NOT be performed.

	@return	KErrPermissionDenied, if aProcess doesn't have rights to define this property;
			KErrArgument, if aPreallocate is negative,
			              OR the property type passed to the aAttr argument
			              is greater than or equal to RProperty::ETypeLimit,
			              OR aPreallocate has a non-zero value when the property
			              type is an integer;
			KErrTooBig, if aPreallocate is greater than RProperty::KMaxPropertySize
			            when the property type is RProperty::EByteArray,
			            OR if aPreallocate is greater than RProperty::KMaxLargePropertySize
			            when the property type is RProperty::ELargeByteArray;
			KErrAlreadyExists, if the property has already been defined.

	@pre Calling thread must be in a critical section.
	@pre Property has been opened.
	@pre Call in a thread context.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
	@pre No fast mutex can be held

	@post Calling thread is in a critical section.
	
	@see  RProperty::TType
*/
EXPORT_C TInt RPropertyRef::Define(TInt aAttr, const TSecurityPolicy& aReadPolicy, const TSecurityPolicy& aWritePolicy, TInt aPreallocate, DProcess* aProcess)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPropertyRef::Define");			
	__PS_ASSERT(iProp);
	
	if(aPreallocate < 0)
		return KErrArgument;
	if(aPreallocate > RProperty::KMaxLargePropertySize)
		return KErrTooBig;

	TPropertyInfo info;
	info.iType = (RProperty::TType)(aAttr & RProperty::ETypeMask);
	info.iAttr = (aAttr & ~RProperty::ETypeMask);
	info.iSize = (TUint16) aPreallocate;
	info.iReadPolicy = aReadPolicy;
	info.iWritePolicy = aWritePolicy;
	return iProp->Define(&info, aProcess);
	}

/** Deletes the property.

	Any pending subscriptions for this property will be completed with KErrNotFound.
	Any new request will not complete until the property is defined and published again.

	@param aProcess The process whoes security attributes will be used for security checks.
					If NULL the security checks will not be performed.

	@return KErrNone, if the delete succeeds;
            KErrNotFound, if the property has not been defined;
			KErrPermissionDenied, if aProcess is specified and is not the owner
			of the process.
			
	@pre Calling thread must be in a critical section.
	@pre Property has been opened.
	@pre Call in a thread context.
	@pre Kernel must be unlocked.
	@pre interrupts enabled
	@pre No fast mutex can be held

	@post Calling thread is in a critical section.
*/
	
EXPORT_C TInt RPropertyRef::Delete(DProcess* aProcess)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"RPropertyRef::Delete");			
	__PS_ASSERT(iProp);
	return iProp->Delete(aProcess);
	}

/** Requests notification when the property is changed.

	If the property has not been defined, the request will not complete until the property
	is defined and published.

	Only one subscription per TPropertySubsRequest object is allowed.

	@param aReq Specifies this request.
	@param aProcess The process whoes security attributes will be used for security checks.
					If NULL the security checks will not be performed.

	@return KErrNone, if the subcribe action succeeds.

	@pre Property has been opened.
*/
EXPORT_C TInt RPropertyRef::Subscribe(TPropertySubsRequest& aReq, DProcess* aProcess)
	{
	__PS_ASSERT(iProp);
	NKern::LockSystem();
	TInt r = iProp->Subscribe(&aReq, aProcess);
	__PS_ASSERT(r != KErrInUse);
	return r;
	}

/** Cancels a subscription request originally made with RPropertyRef::Subscribe().

	@param aReq The TPropertySubsRequest object used in the original subscribe request.

	@pre Property has been opened.

	@see RPropertyRef::Subscribe()
*/
EXPORT_C void RPropertyRef::Cancel(TPropertySubsRequest& aReq)
	{
	__PS_ASSERT(iProp);
	NKern::LockSystem();
	iProp->Cancel(&aReq);
	}

/** Retrieves (reads) the value of an integer property.

	@param aValue An integer to hold the returned property value
	@param aProcess The process whose security attributes will be used for security checks.
					If NULL the security checks will not be performed.

	@return KErrNotFound, if the property has not been defined;
			KErrPermissionDenied, if aProcess does not have the read capabilities;
			KErrArgument, if the property is not of type EInt;
			KErrNone, if the property was read succesfully.

	@pre Property has been opened.

	@realtime
*/
EXPORT_C TInt RPropertyRef::Get(TInt& aValue, DProcess* aProcess)
	{
	__PS_ASSERT(iProp);
	NKern::LockSystem();
	return iProp->GetI(&aValue, aProcess);
	}

/** Publishes (writes) the value of an integer property.

	@param aValue The new value for the property.
	@param aProcess The process whose security attributes will be used for security checks.
					If NULL the security checks will not be performed.

	@return KErrNotFound, if the property has not been defined;
			KErrPermissionDenied, if aProcess does not have the write capabilities;
			KErrArgument, if the property is not of type EInt;
			KErrNone, if the property was set succesfully.

	@pre Property has been opened.

	@realtime
*/
EXPORT_C TInt RPropertyRef::Set(TInt aValue, DProcess* aProcess)
	{
	__PS_ASSERT(iProp);
	NKern::LockSystem();
	return iProp->SetI(aValue, aProcess);
	}
	
/** Retrieves (reads) the value of a binary property.

	@param aDes A descriptor to hold the returned property value.
	@param aProcess The process whose security attributes will be used for security checks.
					If NULL the security checks will not be performed.

	@return KErrNotFound, if the property has not been defined;
			KErrPermissionDenied, if aProcess does not have the read capabilities;
			KErrArgument, if the property is not of type EByteArray;
			KErrOverflow, if the property is larger than the maximum size of aDes,
            in which case aDes is filled with as much of the property value that will fit;
			KErrNone, if the property was read succesfully.

	@pre Property has been opened.

	@realtime There are real time guarantees if the property is not large.
*/
EXPORT_C TInt RPropertyRef::Get(TDes8& aDes, DProcess* aProcess)
	{
	__PS_ASSERT(iProp);
	TInt size = aDes.MaxSize();
	NKern::LockSystem();
	TInt r = iProp->GetB((TUint8*) aDes.Ptr(), &size, aProcess, EFalse);
	if ((r == KErrNone) || (r == KErrOverflow))
		{
		aDes.SetLength(size);
		}
	return r;
	}

/** Publishes (writes) the value of a binary property.

	@param aDes The new value for the property.
	@param aProcess The process whoes security attributes will be used for security checks.
					If NULL the security checks will not be performed.

	@return KErrNotFound, if the property has not been defined;
			KErrPermissionDenied, if aProcess does not have the write capabilities;
			KErrArgument, if the property is not of type EByteArray;
			KErrTooBig, if the property is larger than KMaxPropertySize;
			KErrNone, if the property was set succesfully.

	@pre Property has been opened.

	@realtime There are realtime guarantees if the property is not large
	          and the size is not greater than the previously published value.
*/
EXPORT_C TInt RPropertyRef::Set(const TDesC8& aDes, DProcess* aProcess)
	{
	__PS_ASSERT(iProp);
	NKern::LockSystem();
	return iProp->SetB((TUint8*) aDes.Ptr(), aDes.Size(), aProcess, EFalse);
	}

/** Gets property status information.
	
	@param aStatus property status output.

	@return ETrue if the property is defined; otherwise, EFalse.

	@pre System must be locked.
	@pre Property has been opened.

	@post System is locked.
*/
EXPORT_C TBool RPropertyRef::GetStatus(TPropertyStatus& aStatus)
	{
	CHECK_PRECONDITIONS(MASK_SYSTEM_LOCKED,"RPropertyRef::GetStatus");			
	__PS_ASSERT(iProp);
	aStatus.iType			= iProp->Type();
	aStatus.iAttr			= 0;
	aStatus.iOwner			= iProp->Owner();
	if (iProp->Type() == RProperty::EByteArray || iProp->Type() == RProperty::ELargeByteArray)
		{
		aStatus.iSize		= TUint16(iProp->BufSize());
		}
	else
		{
		aStatus.iSize		= 0;
		}
	return iProp->IsDefined();
	}
