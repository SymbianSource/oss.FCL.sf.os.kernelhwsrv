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
// e32\kernel\object.cpp
// 
//


#include <kernel/kern_priv.h>
#include "dobject.h"

#ifdef _DEBUG
#define __DEBUG_SAVE(p)				TInt dbgNestLevel = ((RHeap::SDebugCell*)p)[-1].nestingLevel
#define __DEBUG_RESTORE(p)			((RHeap::SDebugCell*)p)[-1].nestingLevel = dbgNestLevel
#else
#define __DEBUG_SAVE(p)
#define __DEBUG_RESTORE(p)
#endif

_LIT(KLitLocal,"Local-");
_LIT8(KLitLocal8,"Local-");
_LIT(KColonColon,"::");
_LIT8(KColonColon8,"::");
_LIT(KLitDObjectCon,"DObjectCon");
_LIT(KLitHandleMutex,"HandleMutex");

#ifdef __SMP__
// SMP_FIXME: Investigate what the purpose of this is
TSpinLock TraceSpinLock(TSpinLock::EOrderNone);
#endif

inline TUint64 DObject::ObjectId() const
	{
	return iObjectId >> 18;
	}

/** Default constructor for DObject
	Sets access count to 1, no name, no container, no owner.

	@internalComponent
 */
EXPORT_C DObject::DObject()
	{
//	iContainer=NULL;
//	iOwner=NULL;
//	iName=NULL;
	iAccessCount=1;
	iObjectId = __e32_atomic_add_ord64(&NextObjectId, TUint64(1<<18));
	}


/** Base class destructor for DObject
	Removes the name, removes object from its container if any, and removes owner.

    @pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled
	
	@internalComponent
 */
EXPORT_C DObject::~DObject()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObject::~DObject");		
	__ASSERT_ALWAYS(AccessCount()==0,Panic(EObjObjectStillReferenced));
	__KTRACE_OPT(KOBJECT,Kern::Printf("DObject::~DObject %O owner %O",this,iOwner));
	NKern::FMWait(&Lock);
	HBuf* pN=iName;
	iName=NULL;
	DObjectCon* pC;
	if(iContainerID)
		{
		pC=K::Containers[iContainerID-1];
		iContainerID=0;
		}
	else
		pC=NULL;
	DObject* pO=iOwner;
	iOwner=NULL;
	NKern::FMSignal(&Lock);
	if (pC)
		pC->Remove(this);
	if (HasNotifierQ())	// this can't go from zero to nonzero now since no-one has a reference on this object
		{
		SDblQue dq;
		TMiscNotifierMgr& m = K::TheMiscNotifierMgr;
		m.Lock();
		SMiscNotifierQ* q = NotifierQ();
		SetNotifierQ(0);
		dq.MoveFrom(q);
		m.Unlock();
		if (q)
			{
			Kern::Free(q);
			m.CompleteNotifications(dq);
			}
		}
	if (pN)
		Kern::Free(pN);
	if (pO)
		pO->Close(NULL);
	}

void K::ObjDelete(DObject* aObj)
	{
	SDblQue dq;
	SMiscNotifierQ* q = 0;
	TMiscNotifierMgr& m = K::TheMiscNotifierMgr;
	if (aObj->HasNotifierQ())	// this can't go from zero to nonzero now since no-one has a reference on this object
		{
		m.Lock();
		q = aObj->NotifierQ();
		aObj->SetNotifierQ(0);
		dq.MoveFrom(q);
		m.Unlock();
		}
	TBool deferCodesegCleanup = aObj->iObjectFlags&DObject::EObjectDeferKernelCodesegCleanup;
	if(deferCodesegCleanup)
		DCodeSeg::DeferKernelCleanup();
	DBase::Delete(aObj);
	if (q)
		{
		Kern::Free(q);
		m.CompleteNotifications(dq);
		}
	if(deferCodesegCleanup)
		DCodeSeg::EndDeferKernelCleanup();
	}


/**	Closes a reference on a DObject-derived class.

	This is the default implementation of the virtual function.
	
	Decrements reference count and, if it becomes zero, deletes the object.
	Uses an atomic safe decrement so that a zero access count is unchanged.

	@param	aPtr	Points to relevant DProcess if a user handle is being closed
					NULL if any other reference is being closed

	@return	Bit mask of values from enum DObject::TCloseReturn
			EObjectDeleted	Set if object has been deleted
			EObjectUnmapped	Set if the last reference from a user process has been removed
							Only used for DLibrary
  
    @pre Calling thread must be in a critical section.
    @pre No fast mutex can be held.
	@pre Call in a thread context.
	@pre Kernel must be unlocked
	@pre interrupts enabled
 */
EXPORT_C TInt DObject::Close(TAny* /*aPtr*/)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObject::Close");		
	__KTRACE_OPT(KOBJECT,Kern::Printf("DObject::Close %d %O",AccessCount(),this));
	if (Dec()==1)
		{
		NKern::LockSystem();		// just in case someone is still using this object
		NKern::UnlockSystem();
		K::ObjDelete(this);
		return EObjectDeleted;
		}
	return 0;
	}


/**	Opens a reference on a DObject-derived class, checking that its reference
	count was not initially zero.

	@panic	Current thread KERN-EXEC 0 if initial reference count was 0
	
	@pre    Call in a thread context.
	@pre	System lock must be held.

	@internalComponent
 */
EXPORT_C void DObject::CheckedOpen()
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC|MASK_SYSTEM_LOCKED,"DObject::CheckedOpen");		
	if (Open())
		K::PanicCurrentThread(EBadHandle);
	}


/** Appends the object's short name to a descriptor.

	This is the default implementation of the virtual function.
	
	This is overridden by some
	DObject-derived classes to add extra information to the name.

	@param	aName	Writeable descriptor to which name should be appended.
	
	@pre    Call in a thread context.
	@pre	DObject::Lock fast mutex held.
 */
EXPORT_C void DObject::DoAppendName(TDes& aName)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC,"DObject::DoAppendName");		
	__ASSERT_WITH_MESSAGE_DEBUG(NKern::HeldFastMutex()==&DObject::Lock,"DObject::Lock fast mutex must be held","DObject::DoAppendName"); 	
  	if (iName)
		aName.Append(*iName);
	else
		{
		aName.Append(KLitLocal);
		aName.AppendNumFixedWidth((TInt)this,EHex,8);
		}
	}


/** Appends the object's full name to a descriptor.

	@param	aFullName	Writeable descriptor to which name should be appended.
	
	@pre    Call in a thread context.
	@pre	DObject::Lock fast mutex held.
 */
EXPORT_C void DObject::DoAppendFullName(TDes& aFullName)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC,"DObject::DoAppendFullName");		
	__ASSERT_WITH_MESSAGE_DEBUG(NKern::HeldFastMutex()==&DObject::Lock,"DObject::Lock fast mutex must be held","DObject::DoAppendFullName");
  	if (iOwner)
		{
		iOwner->DoAppendFullName(aFullName);
		aFullName.Append(KColonColon);
		}
	DoAppendName(aFullName);
	}


/** Gets the object's short name into a descriptor.

	@param	aName	Writeable descriptor to which name should be copied.
	
    @pre	No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void DObject::Name(TDes& aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DObject::Name");		
	aName.Zero();
	AppendName(aName);
	}


/** Appends the object's short name to a descriptor.

	@param	aName	Writeable descriptor to which name should be appended.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void DObject::AppendName(TDes& aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DObject::AppendName");		
	NKern::FMWait(&Lock);
	DoAppendName(aName);
	NKern::FMSignal(&Lock);
	}


/** Gets the object's full name into a descriptor.

	@param	aFullName	Writeable descriptor to which name should be copied.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void DObject::FullName(TDes& aFullName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DObject::FullName");		
	aFullName.Zero();
	AppendFullName(aFullName);
	}


/** Appends the object's full name to a descriptor.

	@param	aFullName	Writeable descriptor to which name should be appended.
	
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void DObject::AppendFullName(TDes& aFullName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DObject::AppendFullName");		
	NKern::FMWait(&Lock);
	DoAppendFullName(aFullName);
	NKern::FMSignal(&Lock);
	}


/** Appends the object's short name to a descriptor without waiting on any fast
	mutexes.
	
	Used in kernel tracing.

	@param	aName	Writeable descriptor to which name should be appended.
	@param	aLock	TRUE if kernel should be locked while copying out name
	
	@pre    Call either in a thread or an IDFC context, if aLock=TRUE.
	@pre    Call in any context, if aLock=FALSE.
*/
EXPORT_C void DObject::TraceAppendName(TDes8& aName, TBool aLock)
	{
	TInt c = NKern::CurrentContext();
#ifdef __SMP__
	TInt irq = 0;
	if (aLock)
		{
		if (c==NKern::EInterrupt)
			irq = TraceSpinLock.LockIrqSave();
		else
			{
			NKern::Lock();
			TraceSpinLock.LockOnly();
			}
		}
#else
	if (aLock && c!=NKern::EInterrupt)
		NKern::Lock();
#endif
	if (iName)
		aName.Append(*iName);
	else
		{
		aName.Append(KLitLocal8);
		aName.AppendNumFixedWidth((TUint)this,EHex,8);
		}
#ifdef __SMP__
	if (aLock)
		{
		if (c==NKern::EInterrupt)
			TraceSpinLock.UnlockIrqRestore(irq);
		else
			{
			TraceSpinLock.UnlockOnly();
			NKern::Unlock();
			}
		}
#else
	if (aLock && c!=NKern::EInterrupt)
		NKern::Unlock();
#endif
	}


/** Appends the object's full name to a descriptor without waiting on any fast
	mutexes.
	
	Used in kernel tracing.

	@param	aFullName Writeable descriptor to which name should be appended.
	@param	aLock	TRUE if kernel should be locked while copying out name.
	
	@pre    Call either in a thread or an IDFC context, if aLock=TRUE.
	@pre    Call in any context, if aLock=FALSE.
 */
EXPORT_C void DObject::TraceAppendFullName(TDes8& aFullName, TBool aLock)
	{
	TInt c = NKern::CurrentContext();
#ifdef __SMP__
	TInt irq = 0;
	if (aLock)
		{
		if (c==NKern::EInterrupt)
			irq = TraceSpinLock.LockIrqSave();
		else
			{
			NKern::Lock();
			TraceSpinLock.LockOnly();
			}
		}
#else
	if (aLock && c!=NKern::EInterrupt)
		NKern::Lock();
#endif
	if (iOwner)
		{
		iOwner->TraceAppendFullName(aFullName,EFalse);
		aFullName.Append(KColonColon8);
		}
	TraceAppendName(aFullName,EFalse);
#ifdef __SMP__
	if (aLock)
		{
		if (c==NKern::EInterrupt)
			TraceSpinLock.UnlockIrqRestore(irq);
		else
			{
			TraceSpinLock.UnlockOnly();
			NKern::Unlock();
			}
		}
#else
	if (aLock && c!=NKern::EInterrupt)
		NKern::Unlock();
#endif
	}


/** Sets the object's name, clearing any previous name.

	If the object's name had already been set the old name is replaced. If aName
	is NULL the object ends up with no name.

	@param	aName	Pointer to descriptor containing new name;
					NULL if there is no new name.
					
	@return	KErrNone, if name set successfully;
	        KErrNoMemory, if a heap buffer could not be allocated for the new name;
	        KErrBadName, if new name contained invalid characters.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled

	@internalComponent
 */
EXPORT_C TInt DObject::SetName(const TDesC* aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObject::SetName");		
	__KTRACE_OPT(KOBJECT,Kern::Printf("SetName %O (%S)",this,aName));
	TAny* pN=NULL;
	if (aName)
		{
		TInt r = Kern::ValidateName(*aName);
		if(r!=KErrNone)
			return r;
		pN=HBuf::New(*aName);
		if (!pN)
			return(KErrNoMemory);
		}
	NKern::FMWait(&Lock);
	pN = __e32_atomic_swp_ord_ptr(&iName, pN);
	if (iName)
		{
		__KTRACE_OPT(KOBJECT,Kern::Printf("Name is now %S",iName));
		}
	else
		{
		__KTRACE_OPT(KOBJECT,Kern::Printf("No name"));
		}
#ifdef _DEBUG
	if (pN && iName)
		{
		__DEBUG_SAVE(pN);
		__DEBUG_RESTORE(iName);
		}
#endif
	NKern::FMSignal(&Lock);
	if (pN)
		Kern::Free(pN);
	return KErrNone;
	}


/** Sets the object's owner, clearing any previous owner.

	If the object's owner had already been set the old owner is replaced. If aOwner
	is NULL the object ends up with no owner.
	An extra reference is opened on the new owner. The old owner, if any, has a
	reference closed to compensate.

	@param	aOwner	Pointer to DObject-derived class which is the new owner
					NULL if there is no new owner
					
	@return	KErrNone, if name set successfully;
	        KErrGeneral, if new owner had zero access count.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled

	@internalComponent
 */
EXPORT_C TInt DObject::SetOwner(DObject* aOwner)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObject::SetOwner");		
	__KTRACE_OPT(KOBJECT,Kern::Printf("SetOwner %O (%O)",this,aOwner));
	NKern::FMWait(&Lock);
	DObject* pO=iOwner;
	iOwner=NULL;
	TInt r=KErrNone;
	if (aOwner)
		{
		r=aOwner->Open();
		if (r==KErrNone)
			{
			iOwner=aOwner;
			}
		}
	NKern::FMSignal(&Lock);
	if (pO)
		pO->Close(NULL);
	return r;
	}


/** Notifies an object that a handle to it has been requested by a user thread.

	This is the default implementation of the virtual function.

	This function allows a DObject-derived class to control access to itself
	from user threads. For example it might wish to restrict its use to a single
	thread or to a single user process.

	@param	aThread	Pointer to thread which requests the handle
	@param	aType	Whether requested handle is thread or process relative
	
	@return	KErrNone, if it is acceptable for the handle to be created;
	        KErrAccessDenied, if it is not acceptable.

	@pre    Calling thread must be in a critical section.
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObject::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
	(void)aThread;
	(void)aType;
	return KErrNone;
	}

/** Notifies an object that a handle to it has been requested by a user thread.

	This is the default implementation of the virtual function.

	This function allows a DObject-derived class to control access to itself
	from user threads. For example it might wish to restrict its use to a single
	thread or to a single user process.

	@param	aThread	Pointer to thread which requests the handle
	@param	aType	Whether requested handle is thread or process relative
	@param	aAttr	Handle attributes

	@return	KErrNone, if it is acceptable for the handle to be created;
	        KErrAccessDenied, if it is not acceptable.

	@pre    Calling thread must be in a critical section.
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */

EXPORT_C TInt DObject::RequestUserHandle(DThread* aThread, TOwnerType aType, TUint /* aAttr*/)
	{
	return RequestUserHandle(aThread, aType);
	}

/** Notifies an object that a handle to it has been opened by a user thread.

	This is the default implementation of the virtual function.

	This function allows a DObject-derived class to track which user processes
	are using it. For example a library knows when it must be mapped into a
	user process address space and when it can be unmapped.

	@param	aProc	Pointer to process which has opened the handle.
	
	@return	KErrNone, if no error;
	        negative (<0) value, if error.
	
	@pre    Calling thread must be in a critical section.
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.

	@internalComponent
 */
EXPORT_C TInt DObject::AddToProcess(DProcess* /*aProc*/)
	{
	CHECK_PRECONDITIONS(MASK_CRITICAL|MASK_NO_FAST_MUTEX|MASK_NOT_ISR|MASK_NOT_IDFC,"DObject::AddToProcess");		
	return KErrNone;
	}

/** Notifies an object that a handle to it has been opened by a user thread.

	This is the default implementation of the virtual function.

	This function allows a DObject-derived class to track which user processes
	are using it. For example a library knows when it must be mapped into a
	user process address space and when it can be unmapped.

	@param	aProc	Pointer to process which has opened the handle.
	@param	aAttr	Handle attributes.

	@return	KErrNone, if no error;
	        negative (<0) value, if error.

	@pre    Calling thread must be in a critical section.
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.

	@internalComponent
 */
EXPORT_C TInt DObject::AddToProcess(DProcess* aProc, TUint /* aAttr */)
	{
	CHECK_PRECONDITIONS(MASK_CRITICAL|MASK_NO_FAST_MUTEX|MASK_NOT_ISR|MASK_NOT_IDFC,"DObject::AddToProcess");		
	return AddToProcess(aProc);
	}

/** Gets the object's 'base' name into a descriptor. This is its name without
    any adornments given to it (like process generation numbers or UID values).

    @param  aName   Writeable descriptor to which name should be copied.
    
    @pre    No fast mutex can be held.
    @pre    Call in a thread context.
    @pre    Kernel must be unlocked
    @pre    interrupts enabled
 */
void DObject::BaseName(TDes& aName)
    {
    aName.Zero();
    NKern::FMWait(&Lock);
    DObject::DoAppendName(aName);
    NKern::FMSignal(&Lock);
    }

DObjectCon::DObjectCon(TInt aUniqueID)
//
// Constructor
//
	: iUniqueID(aUniqueID)
	{
	}

DObjectCon::~DObjectCon()
//
// Destructor
//
	{
	// this should never occur
	Panic(EDObjectConDestroyed);
	}


/**	Adds an object to the container.

	The object's full name must be unique among objects in this container.

	@param	aObj	Pointer to the object to be added.
	
	@return	KErrNone, if successful;
	        KErrBadHandle, if object's name contains invalid characters;
	        KErrAlreadyExists, if another object's full name matches the one to be added;
	        KErrNoMemory, if there is insufficient memory to expand the array.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObjectCon::Add(DObject* aObj)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::Add");		
	Wait();
	TInt r = KErrNone;

	// If object is named, check the name doesn't clash with any object already in the container
	if (aObj->iName) 
		{
		r = CheckUniqueFullName(aObj);
		if (r!=KErrNone)
			{
			Signal();
			return r;
			}
		}

	if (iCount==iAllocated)
		{
#ifdef _DEBUG
		TInt newAlloc=iAllocated + 1;
#else
		TInt newAlloc;
		if (iAllocated==0)
			newAlloc = KObjectConMinSize;
		else
			{
			// increase in sequence 8, 12, 16, 24, ... , 2^n, 3*2^n-1, ...
			// can't get sign problems since iAllocated can't exceed 0x20000000
			newAlloc = iAllocated + (iAllocated>>1) - ((iAllocated&(iAllocated-1))>>2);
			}
#endif
		r=Kern::SafeReAlloc((TAny*&)iObjects,iCount*sizeof(DObject*),newAlloc*sizeof(DObject*));
		if (r!=KErrNone)
			{
			Signal();
			return r;
			}
		iAllocated=newAlloc;
		}
	iObjects[iCount++]=aObj;
	aObj->iContainerID=(TUint8)this->UniqueID();
	Signal();
	return KErrNone;
	}


/**	Removes an object from the container.

	@param	aObj	Pointer to object to be removed. The object must be present.
	
	@pre    Calling thread must be in a critical section.
	@pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C void DObjectCon::Remove(DObject* aObj)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::Remove");		
	Wait();
	
	// The object to be removed is often the last one, so search backwards
	DObject** pS=iObjects;
	DObject** pO=pS+iCount;
	DObject** pE=pO-1;
	while(--pO>=pS && *pO!=aObj) {}
	if (pO<pS)
		Panic(EObjRemoveObjectNotFound);

	if (pO<pE)
		wordmove((TAny*)pO,(TAny*)(pO+1),TInt(pE)-TInt(pO));
	--iCount;
#ifdef _DEBUG
	TInt newAlloc=iCount;
#else
	TInt newAlloc = 0;
	if (iCount)	// if now empty, free all memory
		{
		newAlloc = iAllocated;
		if (iAllocated > KObjectConMinSize)	// don't shrink below minimum size
			{
			// calculate next size down
			TInt na2 = iAllocated - (iAllocated>>2) - ((iAllocated&(iAllocated-1))>>3);

			// shrink if half full or 64 less than next size down, whichever comes first
			if (iCount <= Max(iAllocated>>1, na2-64))
				newAlloc = na2;
			}
		}
#endif
	if (newAlloc!=iAllocated)
		{
		Kern::SafeReAlloc((TAny*&)iObjects,iAllocated*sizeof(DObject*),newAlloc*sizeof(DObject*));
		iAllocated=newAlloc;
		}
	Signal();
	}


#ifndef __DOBJECT_MACHINE_CODED__
/**	Looks up the object at a specified array index within the container.

	@param	aIndex	Array index to look up.
	
	@return	Pointer to object at that position.

	@pre	Call in a thread context.
	@pre	Container mutex must be held.
 */
EXPORT_C DObject* DObjectCon::operator[](TInt aIndex)
	{
	CHECK_PRECONDITIONS(MASK_NOT_ISR|MASK_NOT_IDFC,"DObjectCon::operator[]");	
	
	__ASSERT_WITH_MESSAGE_MUTEX(iMutex,"Container mutex must be held","DObjectCon::operator[])");
	__ASSERT_ALWAYS(aIndex>=0 && aIndex<iCount, Panic(EArrayIndexOutOfRange));
	return iObjects[aIndex];
	}


/**	Looks up the object in the container by find handle.

	@param	aFindHandle Find handle to look up.
	
	@return	Pointer to object, NULL if find handle invalid.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Container mutex must be held.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C DObject *DObjectCon::At(const TFindHandle& aFindHandle)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::At(TInt aFindHandle)");		
	__ASSERT_WITH_MESSAGE_MUTEX(iMutex,"Container mutex must be held","DObjectCon::At");
	if (aFindHandle.UniqueID()!=iUniqueID || iCount==0)
		return NULL;
	TUint64 objectId=aFindHandle.ObjectID();
	DObject** pS=iObjects;
	DObject** pO=pS+Min(aFindHandle.Index(), iCount-1);
	while (pO>pS && (*pO)->ObjectId()>objectId)
		--pO;
	return (*pO)->ObjectId()==objectId ? *pO : NULL;
	}


#else
GLDEF_C void PanicDObjectConIndexOutOfRange(void)
	{
	Panic(EArrayIndexOutOfRange);
	}
#endif


/**	Checks whether an object would have a valid unique full name if it were renamed.

	Checks the object's full name assuming that the object owner is aOwner and the short
	name is aName.

	@param	aOwner	Pointer to owner of the object.
	@param	aName	Short name to check.
	
	@return	KErrNone, if OK;
	        KErrBadName, if the supplied short name is invalid;
	        KErrAlreadyExists, if the full name would not be unique.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Container mutex must be held.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObjectCon::CheckUniqueFullName(DObject* aOwner, const TDesC& aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::CheckUniqueFullName(DObject* aOwner, const TDesC& aName)");		
	__ASSERT_WITH_MESSAGE_MUTEX(iMutex,"Container mutex must be held","DObjectCon::CheckUniqueFullName(DObject* aOwner, const TDesC& aName)");
	TInt r=Kern::ValidateName(aName);
	if (r!=KErrNone)
		return r;
	if (!iCount)
		return KErrNone;

	DObject** pS=iObjects;
	DObject** pE=pS+iCount;
	do
		{
		DObject* pO=*pS;
		if (pO->iAccessCount>0 && NamesMatch(aOwner,aName,pO) && pO->iAccessCount>0)
			return KErrAlreadyExists;
		} while(++pS<pE);
	return KErrNone;
	}


TBool DObjectCon::NamesMatch(DObject* aOwner, const TDesC& aObjectName, DObject* aCurrentObject)
//
// Return TRUE if aCurrentObject has the same full name as aOwner::aObjectName
// Enter and leave with no fast mutexes held
//
	{
	TBool result=EFalse;
	NKern::FMWait(&DObject::Lock);

	// if current object has no name or owners are different, not the same
	if (aCurrentObject->iName && aCurrentObject->iOwner==aOwner)
		{
		// same owners, so compare short names
		TKName n;
		aCurrentObject->DoAppendName(n);
		if (aObjectName.MatchF(n)==0)
			result=ETrue;
		}
	NKern::FMSignal(&DObject::Lock);
	return result;
	}


/**	Checks whether the full name of an object would be valid in this container.

	@param	aObject	Pointer to object to check.
	
	@return	KErrNone, if OK;
	        KErrBadName, if the object's short name is invalid;
	        KErrAlreadyExists, if the full name would not be unique.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Container mutex must be held.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObjectCon::CheckUniqueFullName(DObject* aObject)
	{

	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::CheckUniqueFullName(DObject* aObject)");		
	__ASSERT_WITH_MESSAGE_MUTEX(iMutex,"Container mutex must be held","DObjectCon::CheckUniqueFullName(DObject* aObject)");

	__KTRACE_OPT(KOBJECT,Kern::Printf("DObjectCon::CheckUniqueFullName(%O)",aObject));
	__KTRACE_OPT(KOBJECT,Kern::Printf("DObjectCon::iCount=%d",iCount));

	TKName name;
	aObject->Name(name);
	TInt r=Kern::ValidateName(name);
	__KTRACE_OPT(KOBJECT,Kern::Printf("ValidateName returns %d",r));
	if (r!=KErrNone)
		return r;

	if (!iCount)
		return KErrNone;

	DObject** pS=iObjects;
	DObject** pE=pS+iCount;
		
	if (aObject->iName)
		{
		do
			{
			DObject* pO=*pS;
			if (pO->iAccessCount>0 && NamesMatch(aObject->iOwner,name,pO) && pO->iAccessCount>0)
				return KErrAlreadyExists;
			} while(++pS<pE);
		}			
	else
		{
		// object has no name, just compare pointers
		do
			{
			DObject* pO=*pS;
			if (pO==aObject)
				return KErrAlreadyExists;
			} while(++pS<pE);
		}

	return KErrNone;
	}


/**	Finds an object in the container by short name.

	Starts from a specified find handle and returns a find handle to the object.
	Unnamed objects will never be returned. Objects with zero access count will
	also never be returned.

	@param	aFindHandle	Find handle to start from and place to put returned find handle.
	@param	aMatch		Match string to compare object names against.
	@param	aName		Returns short name of object found.
	
	@return	KErrNone, if an object was found;
	        KErrBadHandle, if aFindHandle was invalid;
	        KErrNotFound, if no object with a matching name was found.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObjectCon::FindByName(TFindHandle& aFindHandle, const TDesC& aMatch, TKName& aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::FindByName");			

	if (aFindHandle.Handle()!=0 && aFindHandle.UniqueID()!=iUniqueID)
		return KErrBadHandle;
	Wait();
	DObject** pS=iObjects;
	DObject** pE=pS+iCount;
	DObject** pP=pS;
	if (aFindHandle.Handle()!=0)
		{
		// find start position - either after the last object found, or the
		// oldest object younger than it, if it no longer exists
		pP+=Min(aFindHandle.Index(), iCount-1);
		while (pP>=pS && (*pP)->ObjectId()>aFindHandle.ObjectID())
			--pP;
		++pP;
		}
	while(pP<pE)
		{
		DObject* pO=*pP;
		if (pO->iAccessCount>0 && pO->NameBuf())
			{
			pO->Name(aName);
			if (aName.MatchF(aMatch)!=KErrNotFound)
				{
				if (pO->iAccessCount>0)
					{
					aFindHandle.Set(pP-pS,iUniqueID,pO->ObjectId());
					Signal();
					return(KErrNone);
					}
				}
			}
		++pP;
		}
	Signal();
	aName.Zero();
	aFindHandle.Set(KObjectMaxIndex,iUniqueID,KMaxTInt64);
	return(KErrNotFound);
	}


/**	Finds an object in the container by full name.

	Starts from a specified find handle and returns a find handle to the object.
	Objects with zero access count will never be returned.
	Unnamed objects may be returned unless the calling thread has secure APIs
	enabled.

	@param	aFindHandle	Find handle to start from and place to put returned find handle.
	@param	aMatch		Match string to compare object full names against.
	@param	aFullName	Returns full name of object found.
	
	@return	KErrNone, if an object was found;
	        KErrBadHandle, if aFindHandle was invalid;
	        KErrNotFound, if no object with a matching name was found.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObjectCon::FindByFullName(TFindHandle& aFindHandle, const TDesC& aMatch, TFullName& aFullName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::FindByFullName");			
	TInt processIsolation = TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation;
	if (aFindHandle.Handle()!=0 && aFindHandle.UniqueID()!=iUniqueID)
		return KErrBadHandle;
	Wait();
	DObject** pS=iObjects;
	DObject** pE=pS+iCount;
	DObject** pP=pS;
	if (aFindHandle.Handle()!=0)
		{
		// find start position - either after the last object found, or the
		// oldest object younger than it, if it no longer exists
		pP+=Min(aFindHandle.Index(), iCount-1);
		while (pP>=pS && (*pP)->ObjectId()>aFindHandle.ObjectID())
			--pP;
		++pP;
		}
	while(pP<pE)
		{
		DObject* pO=*pP;
		if (pO->iAccessCount>0 && (pO->iName || !processIsolation))
			{
			pO->FullName(aFullName);
			if (aFullName.MatchF(aMatch)!=KErrNotFound)
				{
				if (pO->iAccessCount>0)
					{
					aFindHandle.Set(pP-pS,iUniqueID,pO->ObjectId());
					Signal();
					return(KErrNone);
					}
				}
			}
		++pP;
		}
	Signal();
	aFullName.Zero();
	aFindHandle.Set(KObjectMaxIndex,iUniqueID,KMaxTInt64);
	return(KErrNotFound);
	}


/**	Finds and opens an object in the container by full name.

	Starts from a specified find handle and returns a find handle to the object.
	The reference count of the object found is incremented.
	Objects with zero access count will never be returned.
	In addition if the calling thread has secure APIs enabled, unnamed objects
	and objects which are not global will never be returned.

	@param	aFindHandle	Find handle to start from and place to put returned find handle.
	@param	aMatch		Match string to compare object full names against.
	@param	aFullName	Returns full name of object found.
	
	@return	KErrNone, if an object was found;
	        KErrBadHandle, if aFindHandle was invalid;
	        KErrNotFound, if no object with a matching name was found.

    @pre    Calling thread must be in a critical section.
    @pre    No fast mutex can be held.
	@pre	Call in a thread context.
	@pre	Kernel must be unlocked
	@pre	interrupts enabled
 */
EXPORT_C TInt DObjectCon::OpenByFullName(DObject*& aObject, const TDesC& aName)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_CRITICAL,"DObjectCon::OpenByFullName");			
	TInt processIsolation = TheSuperPage().KernelConfigFlags() & EKernelConfigPlatSecProcessIsolation;

	aObject=NULL;
	Wait();
	TFullName fn;
	DObject** pS=iObjects;
	DObject** pE=pS+iCount;
	while(pS<pE)
		{
		DObject* pO=*pS++;
		if (pO->iAccessCount>0)
			{
			if(!pO->iName && processIsolation)
				{ continue; }

			pO->FullName(fn);
			if (fn.MatchF(aName)!=KErrNotFound)
				{
				if(pO->Protection()!=DObject::EGlobal && processIsolation)
					{
					Signal();
					return KErrPermissionDenied;
					}

				TInt r=pO->Open();
				if (r==KErrNone)
					{
					Signal();
					aObject=pO;
					return KErrNone;
					}
				}
			}
		}
	Signal();
	return KErrNotFound;
	}

DObjectCon* DObjectCon::New(TInt aUniqueID)
	{
	DObjectCon* pC=new DObjectCon(aUniqueID);
	if (pC)
		{
		TBuf<16> n(KLitDObjectCon());
		n.AppendNum(pC->UniqueID());
		TInt r=K::MutexCreate(pC->iMutex, n, NULL, EFalse, KMutexOrdObjectCon);
		if (r==KErrNone)
			{
			__KTRACE_OPT(KOBJECT,Kern::Printf("Container %d created OK",pC->UniqueID()));
			return pC;
			}
		__KTRACE_OPT(KOBJECT,Kern::Printf("Error %d creating mutex %S",r,&n));
		}
	return NULL;
	}

/** Returns the kernel object containers array.

	@return Pointer to containers array.  The array contains ENumObjectTypes 
			containers and is indexed by TObjectType enum values.
*/
EXPORT_C DObjectCon* const *Kern::Containers()
	{
	return K::Containers;
	}

DObjectCon* K::ContainerFromFindHandle(const TFindHandle& aFindHandle)
	{
	TInt u=aFindHandle.UniqueID()-1;
	if ((TUint)u<(TUint)ENumObjectTypes)
		return K::Containers[u];
	return NULL;
	}

TInt K::CreateObjectContainers()
	{
	DObjectCon** pC=K::Containers;
	TInt u;
	TInt r;
	for (u=1; u<=ENumObjectTypes; ++u)
		{
		*pC=DObjectCon::New(u);
		if (!*pC)
			return KErrNoMemory;
		++pC;
		}
	K::Containers[EServer]->Lock()->iOrder = KMutexOrdObjectCon2;
	r=K::MutexCreate(RObjectIx::HandleMutex, KLitHandleMutex, NULL, EFalse, KMutexOrdHandle);
	return r;
	}

void K::LockContainer(TInt aObjType)
	{
	K::Containers[aObjType]->Wait();
	}

void K::UnlockContainer(TInt aObjType)
	{
	K::Containers[aObjType]->Signal();
	}

TInt K::AddObject(DObject* aObj, TInt aType)
	{
	return K::Containers[aType]->Add(aObj);
	}

#ifndef __DES8_MACHINE_CODED__

#ifndef __KERNEL_MODE__
#error "TDesC is not 8-bit as __KERNEL_MODE__ is not defined (see e32cmn.h)"
#endif //__KERNEL_MODE
/** Checks whether the specified name is a valid DObject full name.

	A name is deemed to be invalid, if it contains an asterisk or a question
	mark.

	@param aName Name to check.
	
	@return KErrNone, if valid;
	        KErrBadName, if invalid.
*/
EXPORT_C TInt Kern::ValidateFullName(const TDesC& aName)
	{
	TUint8*	pName = const_cast<TUint8*>(aName.Ptr());
	TInt	pNameLen = aName.Length();
	for(;pNameLen;pName++,pNameLen--)
		if(*pName>0x7e || *pName<0x20 || *pName=='*' || *pName=='?')
			return(KErrBadName);
	return(KErrNone);
	}
#endif //#if !defined(__DES8_MACHINE_CODED__)



