// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_obj.cpp
// 
//

#include "sf_std.h"

const TInt KObjectIxGranularity=8;		
const TInt KObjectConGranularity=8;
const TInt KObjectConIxGranularity=4;
const TInt KObjectIndexMask=0x7fff;
const TInt KObjectMaxIndex=0x7fff;
const TInt KObjectInstanceShift=16;
const TInt KObjectInstanceMask=0x3fff;
const TInt KObjectUniqueIDShift=16;
const TInt KObjectIxMaxHandles=0x8000;

inline TInt index(TInt aHandle)
	{return(aHandle&KObjectIndexMask);}
inline TInt instance(TInt aHandle)
	{return((aHandle>>KObjectInstanceShift)&KObjectInstanceMask);}
inline TInt instanceLimit(TInt& aCount)
	{return ((aCount&KObjectInstanceMask)==0) ? ((++aCount)&KObjectInstanceMask) : aCount&KObjectInstanceMask;}
inline TInt makeHandle(TInt anIndex,TInt anInstance)
	{return((TInt)((anInstance<<KObjectInstanceShift)|anIndex));}

inline TInt uniqueID(TInt aHandle)
	{return((aHandle>>KObjectUniqueIDShift)&KObjectInstanceMask);}
inline TInt makeFindHandle(TInt anIndex,TInt anUniqueID)
	{return((TInt)((anUniqueID<<KObjectUniqueIDShift)|anIndex));}


CFsObjectConIx* CFsObjectConIx::NewL()
//
// Create an instance of this class.
//
	{

	return new(ELeave) CFsObjectConIx;
	}

CFsObjectConIx::CFsObjectConIx()
//
// Constructor
//
	: iNextUniqueID(1)
	{
	}

CFsObjectConIx::~CFsObjectConIx()
//
// Destructor
//
	{
	if (iCount)
		{
		TInt i=-1;
		while(++i<iCount)
			{
			CFsObjectCon* pS=iContainers[i];
			delete pS;
			}
		}
	delete iContainers;
	}

void CFsObjectConIx::CreateContainerL(CFsObjectCon*& aContainer)
//
// Actually create the container
//
	{

	aContainer=CFsObjectCon::NewL();
	aContainer->iUniqueID=iNextUniqueID;
	if (iCount==iAllocated)
		{
		TInt newAlloc=iAllocated+KObjectConIxGranularity;
		iContainers=(CFsObjectCon**)User::ReAllocL(iContainers, newAlloc*sizeof(CFsObjectCon*));
		iAllocated=newAlloc;
		}
	iContainers[iCount++]=aContainer;
	}

CFsObjectCon* CFsObjectConIx::CreateL()
//
// Create a new container.
//
	{

	CFsObjectCon* pC=NULL;
	TRAPD(r,CreateContainerL(pC))
	if (r!=KErrNone)
		{
		delete pC;
		User::Leave(r);
		}
	iNextUniqueID++;
	return pC;
	}

void CFsObjectConIx::Remove(CFsObjectCon* aCon)
//
// Remove a container from the index.
//
	{
	if (!aCon)
		return;
	CFsObjectCon** pS=iContainers;
	CFsObjectCon** pE=pS+iCount;
	while(pS<pE)
		{
		if (*pS==aCon)
			{
			Mem::Move((TAny*)pS,(TAny*)(pS+1),TInt(pE)-TInt(pS)-sizeof(CFsObjectCon*));
			TInt newAlloc=--iCount;
			newAlloc=(newAlloc+(KObjectConIxGranularity-1))&~(KObjectConIxGranularity-1);
			if (newAlloc!=iAllocated)
				{
				if (newAlloc)
				    {
					iContainers=(CFsObjectCon**)User::ReAlloc(iContainers,newAlloc*sizeof(CFsObjectCon*));
					if(!iContainers)
					    {
					    Fault(EContainerHeapCorruptionOnRemove);
					    }
				    }
				else
					{
					delete iContainers;
					iContainers=NULL;
					}
				iAllocated=newAlloc;
				}
			delete aCon;
			return;
			}
		pS++;
		}
	Fault(EObjRemoveContainerNotFound);
	}


/**
Constructs the object and initializes the reference count to one. 
 
Once constructed, a reference counting object cannot be deleted until its 
reference count is reduced to zero. 
 
@see CFsObject::Close
*/
EXPORT_C CFsObject::CFsObject()
	{
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	__e32_atomic_add_ord32(&ObjectCount, 1);
#endif
//	iContainer=NULL;
//	iName=NULL;
	iAccessCount=1;
	}


/**
Destructor.
 
Deallocates memory associated with this objects name, if a name 
has been set.
 
@panic FSERV 104 if the reference count is not zero when
       the destructor is called.
*/
EXPORT_C  CFsObject::~CFsObject()
	{
	__PRINT1(_L("CFsObject::~CFsObject() 0x%x"),this);
	__ASSERT_ALWAYS(Dec()==0,Fault(EObjDestructorAccessCount));	
	__ASSERT_ALWAYS(!iContainer,Fault(EObjDestructorContainer));
	if(iName)
		User::Free(iName);
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
	__e32_atomic_add_ord32(&ObjectCount, (TUint32) -1);
#endif
	}


/**
Opens this reference counting object.

The default behaviour increments the reference count by one. 

Where a derived class implements its own version of this function, it must 
either use the protected member function Inc() to increment the reference 
count or make a base call to this function.

@return KErrNone, or another system-wide error code.
*/

EXPORT_C TInt CFsObject::Open()
	{
	TInt count=Inc();
	(void)count;
	__THRD_PRINT2(_L("CFsObject::Open() object=0x%x count=%d"),this,count);
	__ASSERT_DEBUG(count>=1,Fault(EFsObjectOpen));
	return KErrNone;
	}


/**
Removes an assigned container object then deletes this reference
counting object.
*/
void CFsObject::DoClose()
	{
	__THRD_PRINT(_L("CFsObject::DoClose()"));
	if(iContainer)
		{
		iContainer->Remove(this,ETrue);
		iContainer=NULL;
		}
	delete(this);
	}


/**
Closes this reference counting object.

The default behaviour decrements the reference count by one. If this becomes 
zero, then the function calls DoClose on this reference counting object.
 
Where a derived class implements its own version of this function, it can 
use the protected member functions Dec() & DoClose() or make a base call to 
this function.

@see CFsObject::DoClose
*/
EXPORT_C void CFsObject::Close()
	{
	__THRD_PRINT(_L("CFsObject::Close()"));
	if(Dec()==1)
		DoClose();
	}


/**
Determine if this object is within the correct drive thread context 
for file based operations.

The default behaviour is to return True.

A derived class implementation is required.

@return ETrue
*/
EXPORT_C TBool CFsObject::IsCorrectThread()
	{
	return(ETrue);
	}



/**
Sets or clears this reference counting object's name.

To set the name, the specified descriptor must contain the name to be set. 
Once the name has been successfully set, then the specified source descriptor 
can be discarded.

To clear an existing name, specify a NULL argument.

@param aName A pointer to the descriptor containing the name to be set, or 
             NULL if an existing name is to be cleared.

@return KErrNone if the function is successful;
        KErrNoMemory if there is insufficient memory available.

@panic USER 11 if the length of aName is greater than KMaxName
       for a 16-bit descriptor.
@panic USER 23 if the length of aName is greater than KMaxName
       for an 8-bit descriptor.
*/
EXPORT_C TInt CFsObject::SetName(const TDesC *aName)
	{
	User::Free(iName);
	iName=NULL;
	if (aName!=NULL)
		{
		iName=aName->Alloc();
		if (iName==NULL)
			return(KErrNoMemory);
		}
	return(KErrNone);
	}


/**
Gets the name of this reference counting object.

The default behaviour provided by this function depends on whether a name 
has been explicitly set into the object:

If a name has previously been set, then the function returns that name.

If a name has not been set, then the function returns NULL.

@return A modifiable buffer descriptor with a defined maximum length containing 
        the name of this reference counting object or a TName with no contents.
*/
EXPORT_C TName CFsObject::Name() const
	{
	if (iName)
		return(*iName);
	
	TName empty;
	return empty;
	}


/**
Gets the Unique ID of the assigned object container to this object.

@return A unique number for file system container objects.
*/
TInt CFsObject::UniqueID() const {return(iContainer->UniqueID());}


CFsObjectIx* CFsObjectIx::NewL()
//
// Create an instance of this class.
//
	{
	CFsObjectIx* pO=new(ELeave) CFsObjectIx;
	TInt r=pO->iLock.CreateLocal();
	if(r!=KErrNone)
		{
		delete(pO);
		User::Leave(r);
		}
	return(pO);
	}

CFsObjectIx::CFsObjectIx()
//
// Constructor
//
	: iNextInstance(1)
	{
//	iAllocated=0;
//	iNumEntries=0;
//	iHighWaterMark=0;
//	iObjects=NULL;
	}


/**
    Close all objects that were created in the main file server thread.
    For sync. drives all objects must be closed in the main file server thread, because
    they are created in this thread, as soon as all synch. requests are processed there.
*/
void CFsObjectIx::CloseMainThreadObjects()
	{
	__ASSERT_DEBUG(FsThreadManager::IsMainThread(),Fault(EObjectIxMainThread));
	__PRINT(_L("CFsObjectIx::CloseThreadObjects()"));
    
	Lock();
	// We have to be very careful here. Calling Close() on the objects in the array
	// may result in other entries being removed from the array before we delete
	// them here, and may result in the array being ReAlloc()ed, corrupting the removed
	// entries, hence we must check the iHighWaterMark value each time round the loop.
	TInt i=-1;
	while(++i<iHighWaterMark)
		{
		SFsObjectIxRec* pS=iObjects+i;
		CFsObject *pO=pS->obj;
		if (pO && pO->IsCorrectThread())
			{
			// invalidate entry before closing it
			pS->obj=NULL;
			pO->Close();	
			}
		}
	Unlock();
	}


CFsObjectIx::~CFsObjectIx()
//
// Destructor
// Assumes that no need to lock
//
	{
	__PRINT1(_L("CFsObjectIx::~CFsObjectIx() 0x%x"),this);
    
	// We have to be very careful here. Calling Close() on the objects in the array
	// may result in other entries being removed from the array before we delete
	// them here, and may result in the array being ReAlloc()ed, corrupting the removed
	// entries, hence we must check the iHighWaterMark value each time round the loop.
	TInt i=-1;
	while(++i<iHighWaterMark)
		{
		SFsObjectIxRec* pS=iObjects+i;
		CFsObject *pO=pS->obj;
		if (pO)
			{
			// invalidate entry before closing it
			pS->obj=NULL;
			pO->Close();	
			}
		}
	delete iObjects;
	iLock.Close();
	}

TInt CFsObjectIx::AddL(CFsObject* anObj,TBool aLock)
//
// Add a new object to the index.
//
	{
	if(aLock)
		Lock();
	SFsObjectIxRec *pS=iObjects;
	SFsObjectIxRec *pE=pS+iHighWaterMark;
	TInt i=0;
	TInt inc=0;
	while(pS<pE && pS->obj)
		pS++, i++;
	if (pS==pE)
		inc=1;
	if (pS==pE && iAllocated==iHighWaterMark)
		{
		// no slots free, so reallocate array
		if (iHighWaterMark==KObjectIxMaxHandles)
			{
			if(aLock)
				Unlock();
			User::LeaveNoMemory();
			}
		TInt newAlloc=iAllocated + KObjectIxGranularity;
		SFsObjectIxRec* pA=(SFsObjectIxRec*)User::ReAlloc(iObjects, newAlloc*sizeof(SFsObjectIxRec));
		if(!pA)
			{
			if(aLock)
				Unlock();
			User::Leave(KErrNoMemory);
			}
		iObjects=pA;
		iAllocated=newAlloc;
		i=iHighWaterMark;
		pS=pA+i;
		}
	pS->obj=anObj;
	pS->uniqueID=(TUint16)anObj->UniqueID();
	pS->instance=(TUint16)instanceLimit(iNextInstance);
	iNextInstance++;
	iHighWaterMark+=inc;
	++iNumEntries;
	TInt h=makeHandle(i,pS->instance);
	if(aLock)
		Unlock();
	return(h);
	}

void CFsObjectIx::Remove(TInt aHandle,TBool aLock)
//
// Remove an object from the index.
//
	{
	if(aLock)
		Lock();
	TInt i=index(aHandle);
	__ASSERT_ALWAYS(i<iHighWaterMark,Fault(EObjRemoveBadHandle));
	SFsObjectIxRec* pR=iObjects+i;
	CFsObject *pO=pR->obj;
	if (!pO || pR->instance!=instance(aHandle) || pR->uniqueID!=pO->UniqueID())
		Fault(EObjRemoveBadHandle);
	pR->obj=NULL;
	--iNumEntries;
	if (i==iHighWaterMark-1)
		{
		do
			{
			i--;
			pR--;
			} while(i>=0 && !pR->obj);
		TInt newAlloc=(i+KObjectIxGranularity)&~(KObjectIxGranularity-1);
		if (newAlloc!=iAllocated)
			{
			if (newAlloc)
			    {
				iObjects=(SFsObjectIxRec*)User::ReAlloc(iObjects,newAlloc*sizeof(SFsObjectIxRec));
				if(!iObjects)
				    {
                    Fault(EContainerHeapCorruptionOnRemove);
				    }
			    }
			else
				{
				delete iObjects;
				iObjects=NULL;
				}
			iAllocated=newAlloc;
			}
		iHighWaterMark=i+1;
		}
	if(aLock)
		Unlock();
	pO->Close();
	}

CFsObject *CFsObjectIx::At(TInt aHandle,TInt aUniqueID,TBool aLock)
//
// Return the object from its handle.
//
	{
	if(aLock)
		Lock();
	TInt i=index(aHandle);
	if (i>=iHighWaterMark)
		{
		if(aLock)
			Unlock();
		return NULL;
		}
	SFsObjectIxRec *pS=iObjects+i;
	if (pS->instance!=instance(aHandle) || pS->uniqueID!=aUniqueID)
		{
		if(aLock)
			Unlock();
		return NULL;
		}
	if(aLock)
		Unlock();
	return pS->obj;	
	}

CFsObject *CFsObjectIx::At(TInt aHandle,TBool aLock)
//
// Return the object from its handle.
//
	{
	if(aLock)
		Lock();
	TInt i=index(aHandle);
	if (i>=iHighWaterMark)
		{
		if(aLock)
			Unlock();
		return NULL;
		}
	SFsObjectIxRec *pS=iObjects+i;
	if (pS->instance!=instance(aHandle))
		{
		if(aLock)
			Unlock();
		return NULL;
		}
	if(aLock)
		Unlock();
	return pS->obj;
	}

TInt CFsObjectIx::At(const CFsObject* anObj,TBool aLock)
//
// Return the handle from an object.
//
	{
	if(aLock)
		Lock();
	if (iHighWaterMark)
		{
		SFsObjectIxRec* pS=iObjects;
		SFsObjectIxRec* pE=pS+iHighWaterMark;
		TInt i=0;
		while(pS<pE && pS->obj!=anObj)
			pS++, i++;
		if (pS<pE)
			{
			TInt h=makeHandle(i,pS->instance);
			if(aLock)
				Unlock();	
			return(h);	
			}
		}
	if(aLock)
		Unlock();
	return KErrNotFound;
	}


CFsObject* CFsObjectIx::operator[](TInt anIndex)
//
// Return the object at anIndex
//
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iHighWaterMark,Fault(EArrayIndexOutOfRange));
	return iObjects[anIndex].obj;
	}

CFsObjectCon* CFsObjectCon::NewL()
//
// Create a new instance of this class.
//
	{
	CFsObjectCon* pO=new(ELeave) CFsObjectCon(ENotOwnerID);
	TInt r=pO->iLock.CreateLocal();
	if(r!=KErrNone)
		{
		delete(pO);
		User::Leave(r);
		}
	return(pO);
	}

CFsObjectCon::CFsObjectCon(TInt aUniqueID)
//
// Constructor
//
	: iUniqueID(aUniqueID)
	{
//	iAllocated=0;
//	iCount=0;
//	iObjects=NULL;
	}

CFsObjectCon::~CFsObjectCon()
//
// Destructor
//
	{
	__ASSERT_ALWAYS(iCount==0,Fault(EObjectConDestructor));
	iLock.Close();
	delete iObjects;
	}

void CFsObjectCon::AddL(CFsObject* anObj,TBool aLock)
//
// Install a new object to the container. The full name must be unique.
//
	{
	if(anObj->iName)
		User::LeaveIfError(CheckUniqueName(anObj));
	if(aLock)
		Lock();
	if (iCount==iAllocated)
		{
		TInt newAlloc=iAllocated+KObjectConGranularity;
		CFsObject** pO=(CFsObject**)User::ReAlloc(iObjects, newAlloc*sizeof(CFsObject*));
		if(!pO)
			{
			if(aLock)
				Unlock();
			User::Leave(KErrNoMemory);
			}
		iObjects=pO;
		iAllocated=newAlloc;
		}
	iObjects[iCount++]=anObj;
	if (iUniqueID!=ENotOwnerID)
		anObj->iContainer=this;
	if(aLock)
		Unlock();
	}

void CFsObjectCon::Remove(CFsObject *anObj,TBool aLock)	
//
// Remove an object from the container.
// This assumes that close is called by the calling function
// 
	{
	if(aLock)
		Lock();
	CFsObject** pS=iObjects;
	CFsObject** pE=pS+iCount;
	while(pS<pE)
		{
		if (*pS==anObj)
			{
			Mem::Move((TAny*)pS,(TAny*)(pS+1),TInt(pE)-TInt(pS)-sizeof(CFsObject*));
			TInt newAlloc=--iCount;
			newAlloc=(newAlloc+(KObjectConGranularity-1))&~(KObjectConGranularity-1);
			if (newAlloc!=iAllocated)
				{
				if (newAlloc)
				    {
					iObjects=(CFsObject**)User::ReAlloc(iObjects,newAlloc*sizeof(CFsObject*));
					if(!iObjects)
					    {
					    Fault(EContainerHeapCorruptionOnRemove);
					    }
				    }
				else
					{
					delete iObjects;
					iObjects=NULL;
					}
				iAllocated=newAlloc;
				}
			if(aLock)
				Unlock();
			anObj->iContainer = NULL;
			return;
			}
		pS++;
		}
	Fault(EObjRemoveObjectNotFound);
	}

CFsObject *CFsObjectCon::operator[](TInt anIndex)
//
// Return the object at anIndex.
//
	{
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount, Fault(EArrayIndexOutOfRange));
	return iObjects[anIndex];
	}

CFsObject *CFsObjectCon::At(TInt aFindHandle) const
//
// Return the object at aFindHandle.
// Should only be used there is no other access to the CFsObject
//
	{

	__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,Fault(EObjFindBadHandle));
	TInt ix=index(aFindHandle);
	__ASSERT_ALWAYS(ix<iCount,Fault(EObjFindIndexOutOfRange));
	return iObjects[ix];
	}

CFsObject *CFsObjectCon::AtL(TInt aFindHandle) const
//
// Return the object at aFindHandle.
// Should only be used if no other access to the CFsObject
//
	{

	__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,User::Leave(KErrBadHandle));
	TInt ix=index(aFindHandle);
	__ASSERT_ALWAYS(ix<iCount,User::Leave(KErrArgument));
	return iObjects[ix];
	}

LOCAL_C TInt validateName(const TDesC &aName)
//
// Return KErrBadName if the name is invalid.
//
	{

	if (aName.Locate('*')!=KErrNotFound || aName.Locate('?')!=KErrNotFound || aName.Locate(':')!=KErrNotFound)
		return(KErrBadName);
	return(KErrNone);
	}


TBool CFsObjectCon::NamesMatch(const TName& anObjectName, const CFsObject* aCurrentObject) const
//
// 
//
	{

	if (aCurrentObject->iName==NULL) // current object has no name, therefore not the same
		return(EFalse);
	return(anObjectName.Compare(*aCurrentObject->iName)==0); // short names are different, therefore not the same
	}

TInt CFsObjectCon::CheckUniqueName(const CFsObject* anObject) const
//
// Returns KErrBadName if the name is invalid or
// returns KErrAlreadyExists if the full name is not unique.
//
	{

	TName name(*(anObject->iName));
	TInt r=validateName(name);
	if (r!=KErrNone)
		return r;

	if (!iCount)
		return KErrNone;

	CFsObject** pS=iObjects;
	CFsObject** pE=pS+iCount;

	// if it's name is null, just need to check it's not already there
	if (!anObject->iName)
		{
		do
			{
			if (*pS==anObject)
				return KErrAlreadyExists;
			} while(++pS<pE);
		return KErrNone;
		}

	do
		{
		if (NamesMatch(name,*pS))
			return KErrAlreadyExists;
		} while(++pS<pE);
	return KErrNone;
	}

TInt CFsObjectCon::FindByName(TInt &aFindHandle,const TDesC &aMatch) const
//
// Find an object starting at aFindHandle which matches aMatch
// just using the objects name.
//
	{

	if (!iCount)
		return KErrNotFound;
	TInt ix=0;
	if (aFindHandle!=0)
		{
		__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,Fault(EObjFindBadHandle));
		ix=index(aFindHandle)+1;
		};
	CFsObject** pS=iObjects;
	CFsObject** pE=pS+iCount;
	pS+=ix;
	while(pS<pE)
		{
		TName name=*((*pS++)->iName);
		if (name.MatchF(aMatch)!=KErrNotFound)
			{
			aFindHandle=makeFindHandle(ix,iUniqueID);
			return KErrNone;
			}
		ix++;
		}
	aFindHandle=makeFindHandle(KObjectMaxIndex,iUniqueID);
	return KErrNotFound;
	}

/**
Constructs the object and initializes the Drive number as an invalid drive. 
 
Once constructed, a dispatch object cannot be deleted until its reference 
count is reduced to zero. 
 
@see CFsDispatchObject::Close
*/
CFsDispatchObject::CFsDispatchObject()
	: iDriveNumber(-1) 
	{}

/**
Initialises dispatch object

Creates an internal request and sets it as a file resource close operation.
Initialises drive number as specified.

@param aDrvNumber Drive for which the request is intended.
*/
void CFsDispatchObject::DoInitL(TInt aDrvNumber)
	{
	CFsInternalRequest* PR= new(ELeave) CFsInternalRequest;
	__THRD_PRINT1(_L("internal request = 0x%x"),PR);
	PR->Set(DispatchObjectCloseOp,NULL);
	iRequest = PR;
	iRequest->SetDriveNumber(aDrvNumber);
	iRequest->SetScratchValue((TUint)this);
	iDriveNumber=aDrvNumber;
	}

/**
Closes this dispatch object.

Decrements the reference count by one. If this becomes zero then the request 
will either call Dispatch() if currently not within the correct thread 
context otherwise the function calls DoClose() on this dispatch object.

@see CFsDispatchObject::IsCorrectThread
     CFsDispatchObject::Dispatch
     CFsObject::DoClose
*/
EXPORT_C void CFsDispatchObject::Close()
	{
	__THRD_PRINT1(_L("CFsDispatchObject::Close() 0x%x"),this);
	if(Dec()!=1)
		return;
	if(!IsCorrectThread())
		Dispatch();
	else
		DoClose();
	}

/**
Destructor.
 
Deletes assigned CFsRequest object.
*/
CFsDispatchObject::~CFsDispatchObject()
	{
	__THRD_PRINT(_L("CFsDispatchObject::~CFsDispatchObject()"));
	if(iRequest)
		delete(iRequest);
	}

/**
Dispatches an assigned CFsRequest object.

@see CFsRequest::Dispatch()
*/
void CFsDispatchObject::Dispatch()
	{
	__ASSERT_DEBUG(iRequest,Fault(EDispatchObjectDispatch));
	iRequest->Dispatch();
	}


/**
Determine if this object is within the correct drive thread context 
for file resource request.
For example subsession close request.

@return ETrue if within the correct drive thread context.
*/
EXPORT_C TBool CFsDispatchObject::IsCorrectThread()
	{
	if(!iRequest)
		return(ETrue);
	FsThreadManager::LockDrive(iRequest->DriveNumber());
	TBool b;
	__ASSERT_ALWAYS(FsThreadManager::IsDriveAvailable(iRequest->DriveNumber(),EFalse) || FsThreadManager::IsMainThread(), Fault(EDispatchObjectThread));
	if(!FsThreadManager::IsDriveAvailable(iRequest->DriveNumber(),EFalse) && FsThreadManager::IsMainThread())
		b=ETrue;
	else if(FsThreadManager::IsDriveSync(iRequest->DriveNumber(),EFalse) && FsThreadManager::IsMainThread())
		b=ETrue;
	else
		b=FsThreadManager::IsDriveThread(iRequest->DriveNumber(),EFalse);
	FsThreadManager::UnlockDrive(iRequest->DriveNumber());
	return(b);
	}

TInt TFsCloseObject::Initialise(CFsRequest* /*aRequest*/)
//
//
//
	{
	return(KErrNone);
	}

TInt TFsCloseObject::DoRequestL(CFsRequest* /*aRequest*/)
//
//
//
	{
	__PRINT(_L("TFsCloseObject::DoRequestL()"));
	return(KErrNone);
	}

TInt TFsCloseObject::Complete(CFsRequest* aRequest)
//
//
//
	{
	__PRINT(_L("TFsCloseObject::Complete()"));
	CFsDispatchObject* pO=(CFsDispatchObject*)aRequest->ScratchValue();
	// set CFsDispatchObject::iRequest to NULL since request will be deleted in Free()
	pO->iRequest=NULL;
	pO->DoClose();
	return(KErrNone);
	}


