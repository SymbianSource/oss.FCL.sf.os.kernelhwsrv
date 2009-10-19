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
// e32\euser\cbase\ub_obj.cpp
// 
//

#include "ub_std.h"

const TInt KObjectIxGranularity=8;
const TInt KObjectConMinSize=8;		// must be power of 2 or 3*power of 2
const TInt KObjectConIxGranularity=4;
const TInt KObjectIndexMask=0x7fff;
const TInt KObjectMaxIndex=0x7fff;
const TInt KObjectInstanceShift=16;
const TInt KObjectInstanceMask=0x3fff; 
const TInt KObjectUniqueIDShift=16;
const TInt KObjectUniqueIDMask=0xffff;
const TInt KObjectIxMaxHandles=0x8000;
const TInt KObjectConIxMaxSize=0x10000-1;

inline TInt index(TInt aHandle)
	{return(aHandle&KObjectIndexMask);}
inline TInt instance(TInt aHandle)
	{return((aHandle>>KObjectInstanceShift)&KObjectInstanceMask);}
inline TInt instanceLimit(TInt& aCount)
	{return ((aCount&KObjectInstanceMask)==0) ? ((++aCount)&KObjectInstanceMask) : aCount&KObjectInstanceMask;}
inline TInt makeHandle(TInt anIndex,TInt anInstance)
	{return((TInt)((anInstance<<KObjectInstanceShift)|anIndex));}

inline TInt uniqueID(TInt aHandle)
	{return((aHandle>>KObjectUniqueIDShift)&KObjectUniqueIDMask);}
inline TInt makeFindHandle(TInt anIndex,TInt anUniqueID)
	{return((TInt)((anUniqueID<<KObjectUniqueIDShift)|anIndex));}

/**
@internalComponent
*/
enum {ENotOwnerID};

LOCAL_C void makeFullName(TFullName &aFullName,const CObject *anOwner,const TDesC &aName)
//
// Make a name from its owner name and aName.
//
	{

	aFullName.Zero();
	if (anOwner)
		{
        makeFullName(aFullName,anOwner->Owner(),anOwner->Name());
		aFullName+=_L("::");
		}
	aFullName+=aName;
	}




/**
Constructs the object and initializes the reference count to one. 

Once constructed, a reference counting object cannot be deleted until its 
reference count is reduced to zero. 

@see CObject::Close
*/
EXPORT_C CObject::CObject()
	{

//	iContainer=NULL;
//	iOwner=NULL;
//	iName=NULL;
	iAccessCount=1;
	}




/**
Destructor.

It removes this reference counting object from its object container,
a CObjectCon type.

@panic E32USER-CBase 33 if the reference count is not zero when
       the destructor is called.

@see CObjectCon
*/
EXPORT_C CObject::~CObject()
	{

	__ASSERT_ALWAYS(AccessCount()==0,Panic(EObjObjectStillReferenced));
	User::Free(iName);
	if (iContainer)
		{
		CObjectCon *p=iContainer;
		iContainer=NULL;
		p->Remove(this);
		}
	}




/**
Opens this reference counting object.

The default behaviour increments the reference count by one and
returns KErrNone. 
Where a derived class implements its own version of this function, it must 
either use the protected member function Inc() to increment the reference 
count or make a base call to this function.

@return KErrNone.
*/
EXPORT_C TInt CObject::Open()
	{

	Inc();
	return(KErrNone);
	}




/**
Closes this reference counting object.

The default behaviour decrements the reference count by one. If this becomes 
zero, then the function deletes this reference counting object.

Where a derived class implements its own version of this function, it can 
use the protected member function Dec() to decrement the reference count or 
make a base call to this function.

@panic E32USER-CBase 34 if the reference count is negative when this
       function is called.
*/
EXPORT_C void CObject::Close()
	{

	Dec();
	__ASSERT_ALWAYS(AccessCount()>=0,Panic(EObjNegativeAccessCount));
	if (AccessCount()==0)
		delete this;
	}




LOCAL_C TName GetLocalObjectName(const CObject *anObj)
	{
	TName n=_L("Local-");
	n.AppendNumFixedWidth((TInt)anObj,EHex,8);
	return n;
	}




/**
Gets the name of this reference counting object.

The default behaviour provided by this function depends on whether a name 
has been explicitly set into the object:

if a name has previously been set, then the function returns that name.

if a name has not been set, then the function builds a default name. This 
is fourteen characters and has the format: LOCAL-nnnnnnnn where nnnnnnnn is 
the hexadecimal character representation of this reference counting object's 
address. This default name is, therefore, guaranteed to be unique within the 
current process.

@return A modifiable buffer descriptor with a defined maximum length containing 
        the name of this reference counting object.
*/
EXPORT_C TName CObject::Name() const
	{
	if (iName)
		return(*iName);
	return GetLocalObjectName(this);
	}




/**
Gets the full name of this reference counting object.

By default, the full name is a concatenation of this reference counting
object's name with the full name of its owning reference counting object.

@return A modifiable buffer descriptor with a defined maximum length containing 
        the full name of this reference counting object.
*/
EXPORT_C TFullName CObject::FullName() const
	{

	TFullName n;
	makeFullName(n,Owner(),Name());
	return(n);
	}




/**
Sets or clears this reference counting object's name.

To set the name, the specified descriptor must contain the name to be set. 
Once the name has been successfully set, then the specified source descriptor 
can be discarded.

To clear an existing name, specify a NULL argument.

@param aName A pointer to the descriptor containing the name to be set, or 
             NULL if an existing name is to be cleared.

@return KErrNone, if the function is successful;
        KerrNoMemory, if there is insufficient memory available.

@panic USER 11 if the length of aName is greater than KMaxName
       for a 16-bit descriptor.
@panic USER 23 if the length of aName is greater than KMaxName
       for an 8-bit descriptor.
*/
EXPORT_C TInt CObject::SetName(const TDesC *aName)
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
Sets or clears this reference counting object's name, and leaves on error.

To set the name, the specified descriptor must contain the name to be set. 
Once the name has been successfully set, then the specified source descriptor 
can be discarded.

To clear an existing name, specify a NULL argument.

The function leaves if there is insufficient memory.

@param aName A pointer to the descriptor containing the name to be set, or 
             NULL if an existing name is to be cleared.

@panic USER 11 if the length of aName is greater than KMaxName
       for a 16-bit descriptor.
@panic USER 23 if the length of aName is greater than KMaxName
       for an 8-bit descriptor.
*/
EXPORT_C void CObject::SetNameL(const TDesC *aName)
	{

	User::Free(iName);
	iName=NULL;
	if (aName!=NULL)
		iName=aName->AllocL();
	}




/**
Extension function


*/
EXPORT_C TInt CObject::Extension_(TUint aExtensionId, TAny*& a0, TAny* a1)
	{
	return CBase::Extension_(aExtensionId, a0, a1);
	}




/**
Creates a new object index.

@return A pointer to the newly created object index.
*/
EXPORT_C CObjectIx* CObjectIx::NewL()
	{

	return new(ELeave) CObjectIx;
	}




/**
Default constructor.
*/
EXPORT_C CObjectIx::CObjectIx()
	: iNextInstance(1), iFree(-1)
	{
	}




/**
Destructor.

Frees all resources owned by the object index, prior to its destruction.
In particular, it calls Close() on all reference counting objects in the index.

@see CObject::Close
*/
EXPORT_C CObjectIx::~CObjectIx()
	{
	// We have to be very careful here. Calling Close() on the objects in the array
	// may result in other entries being removed from the array before we delete
	// them here, and may result in the array being ReAlloc()ed, corrupting the removed
	// entries, hence we must check the iHighWaterMark value each time round the loop.
	TInt i=-1;
	while(++i<iHighWaterMark)
		{
		SObjectIxRec* pS=iObjects+i;
		CObject *pO=pS->obj;
		if (pO)
			{
			pO->Close();
			pS->obj=NULL;	// invalidate entry after closing it
			}
		}
	delete iObjects;
	}



/**
Adds the specified reference counting object into this object index and
returns the handle number that represents it.

@param anObj The reference counting object to be added to this object index.

@return The handle number.
*/
EXPORT_C TInt CObjectIx::AddL(CObject* anObj)
	{
	TInt index=iFree; //iFree contains the index of the first empty slot or -1 if there is no such.
	if (index<0) //Check if the free list is empty
		{
		// The free list is empty, so more slots must be allocated.
		if (iHighWaterMark==KObjectIxMaxHandles)
 			User::LeaveNoMemory();
		
		//Those are internal checking of the object consistency
		__ASSERT_DEBUG(iAllocated==iHighWaterMark,Panic(EObjInconsistent));
		__ASSERT_DEBUG(iAllocated==iNumEntries,Panic(EObjInconsistent));
		
		//Double allocated memory
		TInt newAlloc=iAllocated ? 2*iAllocated : KObjectIxGranularity;
		if(newAlloc>KObjectIxMaxHandles)
 			newAlloc=KObjectIxMaxHandles;
		SObjectIxRec* pA=(SObjectIxRec*)User::ReAllocL(iObjects, newAlloc*sizeof(SObjectIxRec));
		iObjects=pA;

		//NOTE: There is no need to initialize newly allocated memory (e.g. to zero iObjects) as it all goes
		//beyond HWM and will be not considered when search in At(...) or operator[]() methods. 
		//As the free list is initially ordered, each slot must go to the states as follows:
		//-Created as the part of the free list beyond HWM. - uninitialized and not searched in any method.
		//-In use  - initialized.
		//-The part of the free list within HWM - initialized to zero.
		//Also, UpdateState() does not reorder free list beyond HWM but keep it preserverd.

		iAllocated=newAlloc;					//Update the number of allocated slots
		iUpdateDisabled = iAllocated/2;//The number of Remove() calls before the object update (free list, HWM,...)

		//Connect all newly allocated slots into the list and set 'index' to point to the first one.
		index=newAlloc-1;
		pA[index].nextEmpty = -1;
		while (iHighWaterMark <= --index) 
			pA[index].nextEmpty=index+1;
		index++;
		}

	//At this point, 'index' variable points to the slot that will be used for the new entry.
	//It also represents the first element in the list of empty slots.

	SObjectIxRec *pS=iObjects+index; // pS points to the object that will be used for the new entry.
	iFree=pS->nextEmpty;			 // Update iFree to point to the next empty slot. 
	
	//Initialize data of the new element of the array.
	pS->obj=anObj;
	pS->str.uniqueID=(TUint16)anObj->UniqueID();
	pS->str.instance=(TUint16)instanceLimit(iNextInstance);
	
	iNextInstance++;
	
	if (index>=iHighWaterMark)	//Update HWM to points to the slot after the last in use.
		iHighWaterMark=index+1;
	
	++iNumEntries;

	__ASSERT_DEBUG( (iFree==-1 && iAllocated==iHighWaterMark && iAllocated==iNumEntries)
				  ||(iFree!=-1 && iAllocated>iNumEntries),Panic(EObjInconsistent));
	
	return(makeHandle(index,pS->str.instance)); //Create and return the handle
	}




/**
Removes the reference counting object identified by handle number from this 
object index and closes it.

If the reference counting object cannot be closed, because CObjectIx::ENoClose 
is ORed into the handle number, then it is neither removed from the object 
index nor closed.

@param aHandle The handle number of the reference counting object to be removed 
               and closed.
               
@panic E32USER-CBase 37 if aHandle does not represent an object known to this
                        object index.
*/
EXPORT_C void CObjectIx::Remove(TInt aHandle)
	{
	if (aHandle&ENoClose)
		return;
	TInt i=index(aHandle);
	__ASSERT_ALWAYS(i<iHighWaterMark,Panic(EObjRemoveBadHandle));
	SObjectIxRec* pR=iObjects+i;
	CObject *pO=pR->obj;
	if (!pO || pR->str.instance!=instance(aHandle) || pR->str.uniqueID!=pO->UniqueID())
		Panic(EObjRemoveBadHandle);
	pO->Close();
	pR->obj=NULL;

	if(--iNumEntries)
		{	
		// Add the entry onto the free list
		pR->nextEmpty=iFree;
		iFree=i;

		if(iUpdateDisabled) 
			iUpdateDisabled--; //Count down till state update is enabled again.

		if (									 //Update the states(HWM, resort free list & memory shrink) if:
			(!iUpdateDisabled) &&				 //There were a number of Remove() calls since the last ReAlloc
			(iAllocated>=2*KObjectIxGranularity))//Allocated memory is above the limit.
			{
			UpdateState();
			iUpdateDisabled = iAllocated/2;//The number of Remove() calls before the next update comes.
			}
		}
	else
		{
		//There is no any CObject left. Reset the object to initial state (except iNextInstance)
		delete iObjects;
		iObjects=NULL;
		iAllocated=0;
		iHighWaterMark=0;
		iFree=-1;		  //Empty free list
		}

	//This is internal checking of the object consistency
	__ASSERT_DEBUG( (iFree==-1 && iAllocated==iHighWaterMark && iAllocated==iNumEntries)
				  ||(iFree!=-1 && iAllocated>iNumEntries),Panic(EObjInconsistent));
	}


//1. Reorder free list.
//2. Update iHighWaterMark. This is the only place where HWM is decreased, while it can be increased during AddL().
//3. Shrinks the heap memory (pointed by iObjects) if it can be at least halved.
//The function is entered with at least one occupied slot in iObjects array.
//The array is searched from its end. Special care is given to the  case where
//HWM is less then KObjectIxGranularity as the size of the arrey does not go below it.
void CObjectIx::UpdateState()
	{
	TBool toShrink = EFalse;
	TBool foundFreeBelowLimit = EFalse;//Used to handle spacial case when HWM is below the minimum alloc. limit
	TInt newHWM = 0;

	//Start from the HWM as all slots beyond are free and sorted already.
	TInt current = iHighWaterMark;
	TInt prevFreeSlot = iHighWaterMark == iAllocated ? -1 : iHighWaterMark;
	while (--current>=0)
		{
		if (iObjects[current].obj)
			{
			//This is the slot with the valid entry. Check if this is the last in the array.
			if(!newHWM)
				{
				//This is the first occupied slot we found => It is new HWM.
				newHWM=current+1;
				if (current < iAllocated/2)
					{
					//At this point we decide to shrink memory.
					toShrink = ETrue;
					//Once we find HWM and decide to shrink, all slots after that point should be removed
					//from the free list as that memory will be freed. The exception is the case when HWM is below
					//the minimum of allocated memory (8 slots as the moment).
					if((current >= KObjectIxGranularity) || (!foundFreeBelowLimit))
						prevFreeSlot = -1; //The next free slot to find will be the last one in the list.
					}
				}
			}
		else
			{
			//This is the free slot.
			if ((!newHWM) && (!foundFreeBelowLimit) &&(current<KObjectIxGranularity))
				{
				//The special case. 
				//We just reached the first free slot below minimum alloc. size and still we found no occupied slots.
				iObjects[current].nextEmpty = -1; //This will be the end of free list.
				foundFreeBelowLimit = ETrue; //Mark that we found the special case
				}
			else
				{
				iObjects[current].nextEmpty = prevFreeSlot;//Link the empty slot in the free list.
				}
			prevFreeSlot = current;
			}
		}

	iHighWaterMark = newHWM;
	iFree = prevFreeSlot;

	if (toShrink)
		{
		//Do not reallocate less then the initial value.
		iAllocated = Max(newHWM,KObjectIxGranularity);
		//Update member data and re-allocate memory. ReAlloc cannot return NULL as we are asking for less memory.
		iObjects=(SObjectIxRec*)User::ReAlloc(iObjects,iAllocated*sizeof(SObjectIxRec));
		}
	}



/**
Gets the number of occurrences of the specified reference counting object
within this object index.

Note that the same reference counting object can be added to an object index 
more than once.

@param anObject The reference counting object.

@return The number of occurrences.
*/
EXPORT_C TInt CObjectIx::Count(CObject* anObject) const
	{

	TInt n=0;
	if (iHighWaterMark)
		{
		SObjectIxRec* pS=iObjects;
		SObjectIxRec* pE=pS+iHighWaterMark;
		do
			{
			if (pS->obj==anObject)
				n++;
			} while (++pS<pE);
		}
	return n;
	}




#ifndef __COBJECT_MACHINE_CODED__
/**
Gets a pointer to the reference counting object with the specified handle
number and matching unique ID.

@param aHandle   The handle number of the reference counting object.
@param aUniqueID The unique ID.

@return A pointer to the reference counting object. If there is no matching 
        object, then this is NULL.
*/
EXPORT_C CObject *CObjectIx::At(TInt aHandle,TInt aUniqueID)
	{
	TInt i=index(aHandle);
	if (i>=iHighWaterMark)
		return NULL;
	SObjectIxRec *pS=iObjects+i;
	if (pS->str.instance!=instance(aHandle) || pS->str.uniqueID!=aUniqueID)
		return NULL;
	return pS->obj;
	}




/**
Gets a pointer to the reference counting object with the specified
handle number.

@param aHandle The handle number of the reference counting object.

@return A pointer to the reference counting object. If there is no matching 
        object, then this is NULL.
*/
EXPORT_C CObject *CObjectIx::At(TInt aHandle)
	{
	TInt i=index(aHandle);
	if (i>=iHighWaterMark)
		return NULL;
	SObjectIxRec *pS=iObjects+i;
	if (pS->str.instance!=instance(aHandle))
		return NULL;
	return pS->obj;
	}
#endif





/**
Constructs and returns the handle number representing the specified reference 
counting object within this object index.
	
@param anObj The reference counting object.

@return The handle number representing the reference counting object;
        KErrNotFound, if the reference counting object could not be found
        within the object index.
*/
EXPORT_C TInt CObjectIx::At(const CObject* anObj) const
	{

	if (iHighWaterMark)
		{
		SObjectIxRec* pS=iObjects;
		SObjectIxRec* pE=pS+iHighWaterMark;
		TInt i=0;
		while(pS<pE && pS->obj!=anObj)
			pS++, i++;
		if (pS<pE)
			return(makeHandle(i,pS->str.instance));
		}
	return KErrNotFound;
	}




/**
Gets a pointer to the reference counting object with the specified handle
number and matching unique ID.

@param aHandle   The handle number of the reference counting object.
@param aUniqueID The unique ID.

@return A pointer to the reference counting object.

@leave KErrBadHandle if there is no matching object.
*/
EXPORT_C CObject *CObjectIx::AtL(TInt aHandle,TInt aUniqueID)
	{

	CObject* pO=At(aHandle,aUniqueID);
	if (!pO)
		User::Leave(KErrBadHandle);
	return pO;
	}




/**
Gets a pointer to the reference counting object with the specified handle
number.

@param aHandle The handle number of the reference counting object.

@return A pointer to the reference counting object.

@leave KErrBadHandle if there is no matching object.
*/
EXPORT_C CObject *CObjectIx::AtL(TInt aHandle)
	{

	CObject* pO=At(aHandle);
	if (!pO)
		User::Leave(KErrBadHandle);
	return pO;
	}




#ifndef __COBJECT_MACHINE_CODED__
/**
Gets a pointer to a reference counting object located at the specified offset 
within the object index.

@param anIndex The offset of the reference counting object within the object 
               index. Offset is relative to zero. 
               
@return A pointer to the reference counting object.

@panic E32USER-CBase 21 if the value of anIndex is negative or is greater than
                        or equal to the total number of objects held by
                        the index.
*/
EXPORT_C CObject* CObjectIx::operator[](TInt anIndex)
	{

	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iHighWaterMark,Panic(EArrayIndexOutOfRange));
	return iObjects[anIndex].obj;
	}
#else
GLDEF_C void PanicCObjectIxIndexOutOfRange(void)
	{
	Panic(EArrayIndexOutOfRange);
	}
#endif




/**
Creates an object container.

Open code rarely, if ever, explicitly calls this function. Instead, call the 
CreateL() member function of the container index, CObjectConIx, which uses 
this function in its implementation.

@return A pointer to the new object container.

@see CObjectConIx::CreateL
*/
EXPORT_C CObjectCon* CObjectCon::NewL()
	{

	return new(ELeave) CObjectCon(ENotOwnerID);
	}




/**
Constructor taking a unique Id. 

@param aUniqueID The unique Id value.
*/
EXPORT_C CObjectCon::CObjectCon(TInt aUniqueID)
	: iUniqueID(aUniqueID)
	{
//	iAllocated=0;
//	iCount=0;
//	iObjects=NULL;
	}




/**
Destructor.

Frees all resources owned by the object container, prior to its destruction.

In particular, it destroys all contained reference counting objects.

@see CObject
*/
EXPORT_C CObjectCon::~CObjectCon()
	{

	if (iUniqueID!=ENotOwnerID && iCount)
		{
		// Careful here in case deleting one object causes other objects in the array
		// to be removed and Count to change.
		TInt i=-1;
		while(++i<iCount)
			{
			CObject* pS=iObjects[i];
			delete pS;
			}
		}
	delete iObjects;
	}




/**
Adds a reference counting object to this object container.

If the specified reference counting object has a name, it must be valid,
otherwise the function leaves with KErrBadName; in addition, the reference
counting object's full name must be unique to this object container, otherwise
the function leaves with KErrAlreadyExists.

If the specified reference counting object has no name, then the object itself 
must be unique to the object container, i.e. the object container should not 
already contain the same reference counting object, otherwise the function 
leaves with KErrAlreadyExists.

@param anObj A pointer to the reference counting object to be added.
*/
EXPORT_C void CObjectCon::AddL(CObject* anObj)
	{

	User::LeaveIfError(CheckUniqueFullName(anObj));
	if (iCount==iAllocated)
		{
		TInt newAlloc;
		if (iAllocated==0)
			newAlloc = KObjectConMinSize;
		else
			{
			// increase in sequence 8, 12, 16, 24, ... , 2^n, 3*2^n-1, ...
			// can't get sign problems since iAllocated can't exceed 0x20000000
			newAlloc = iAllocated + (iAllocated>>1) - ((iAllocated&(iAllocated-1))>>2);
			}
		iObjects=(CObject**)User::ReAllocL(iObjects, newAlloc*sizeof(CObject*));
		iAllocated=newAlloc;
		}
	iObjects[iCount++]=anObj;
	if (iUniqueID!=ENotOwnerID)
		anObj->iContainer=this;
	}




/**
Removes a reference counting object from this object container.

The specified reference counting object is destroyed on removal.

@param anObj A pointer to the reference counting object to be removed.

@panic E32USER-CBase 35 if the reference counting object is not held by this
                        object container.
*/
EXPORT_C void CObjectCon::Remove(CObject *anObj)
	{
	CObject** pS=iObjects;
	CObject** pE=pS+iCount;
	while(pS<pE)
		{
		if (*pS==anObj)
			{
			Mem::Move((TAny*)pS,(TAny*)(pS+1),TInt(pE)-TInt(pS)-sizeof(CObject*));
			TInt used = --iCount;
			if (used)	// if now empty, free all memory
				{
				if (iAllocated > KObjectConMinSize)	// don't shrink below minimum size
					{
					// calculate next size down
					TInt newAlloc = iAllocated - (iAllocated>>2) - ((iAllocated&(iAllocated-1))>>3);

					// shrink if half full or 64 less than next size down, whichever comes first
					if (used <= Max(iAllocated>>1, newAlloc-64))
						{
						iObjects=(CObject**)User::ReAlloc(iObjects,newAlloc*sizeof(CObject*));
						iAllocated=newAlloc;
						}
					}
				}
			else
				{
				delete iObjects;
				iObjects=NULL;
				iAllocated=0;
				}
			if (iUniqueID!=ENotOwnerID && anObj->iContainer)
				{
//
// An object's destructor can scan the container so its best
// to remove the object from the container before destroying it.
//
				anObj->iContainer=NULL;
				delete anObj;
				}
			return;
			}
		pS++;
		}
	Panic(EObjRemoveObjectNotFound);
	}




#ifndef __COBJECT_MACHINE_CODED__
/**
Gets a pointer to the reference counting object located at the specified offset 
within the object container.

@param anIndex The offset of the reference counting object within the object 
               container. Offset is relative to zero.

@return A pointer to the owning reference counting object.

@panic E32USER-CBase 21 if anIndex is negative or is greater than or equal to
                        the total number of objects held by the container.
*/
EXPORT_C CObject *CObjectCon::operator[](TInt anIndex)
	{
	__ASSERT_ALWAYS(anIndex>=0 && anIndex<iCount, Panic(EArrayIndexOutOfRange));
	return iObjects[anIndex];
	}




/**
Gets a pointer to the reference counting object with the specified find-handle 
number.

A find-handle number is an integer which uniquely identifies a reference
counting object with respect to its object container.

@param aFindHandle The find-handle number of the reference counting object. 
                   The unique Id part of this number must be the same as the
                   unique Id of this container.
                   The index part of the find-handle number must be
                   a valid index. 

@return A pointer to the reference counting object.

@panic E32User-CBase 38 if the unique Id part of aFindHandle is not the same as
                        the unique Id of this container.
@panic E32User-CBase 39 if the index part of aFindHandle is negative or greater
                        than or equal to the total number of reference counting
                        objects held by this object container.
*/
EXPORT_C CObject *CObjectCon::At(TInt aFindHandle) const
	{

	__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,Panic(EObjFindBadHandle));
	TInt ix=index(aFindHandle);
	__ASSERT_ALWAYS(ix<iCount,Panic(EObjFindIndexOutOfRange));
	return iObjects[ix];
	}




/**
Gets a pointer to the reference counting object with the specified find-handle 
number, and leaves on error..

A find-handle number is an integer which uniquely identifies a reference
counting object with respect to its object container.

@param aFindHandle The find-handle number of the reference counting object. 
                   The unique Id part of this number must be the same as
                   the unique Id of this container.
                   The index part of the find-handle number must be
                   a valid index. 

@return A pointer to the reference counting object.

@leave KErrBadHandle if the unique Id part of aFindHandle is not the same as
                     the unique Id of this container.
@leave KErrArgument if the index part of aFindHandle is negative or greater
                    than or equal to the total number of reference counting
                    objects held by this object container.
*/
EXPORT_C CObject *CObjectCon::AtL(TInt aFindHandle) const
	{

	__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,User::Leave(KErrBadHandle));
	TInt ix=index(aFindHandle);
	__ASSERT_ALWAYS(ix<iCount,User::Leave(KErrArgument));
	return iObjects[ix];
	}
#else
GLDEF_C void PanicCObjectConFindBadHandle(void)
	{
	Panic(EObjFindBadHandle);
	}





GLDEF_C void PanicCObjectConFindIndexOutOfRange(void)
	{
	Panic(EObjFindIndexOutOfRange);
	}

GLDEF_C void PanicCObjectConIndexOutOfRange(void)
	{
	Panic(EArrayIndexOutOfRange);
	}
#endif




/**
Checks whether a specified name is a valid CObject name.

A name is deemed to be invalid, if it contains any of the characters:
"*", "?", ":" i.e. the characters: asterisk, question mark and single colon.

@param aName A reference to the descriptor containing the name to be checked.

@return KErrBadName, if the name is invalid. KErrNone, otherwise.

@see CObject
*/
EXPORT_C TInt User::ValidateName(const TDesC &aName)
	{
#ifdef _UNICODE
	TUint16* pName = const_cast<TUint16*>(aName.Ptr());
#else
	TUint8*	pName = const_cast<TUint8*>(aName.Ptr());
#endif
	TInt pNameLen = aName.Length();
	for(;pNameLen;pName++,pNameLen--)
		if(*pName>0x7e || *pName<0x20 || *pName=='*' || *pName=='?' || *pName==':')
			return(KErrBadName);
	return(KErrNone);
	}




/**
Checks that a name will be unique.

The function checks that no reference counting object exists in this object
container with the same full name as that generated from the specified name
and the specified owning reference counting object.

This is a useful test to ensure that the name for a potential new reference
counting object will result in a unique full name.

@param anOwner A pointer to a potential owning reference counting object.
@param aName   The name for a potential new reference counting object.

@return KErrNone, if the full name does not already exist in this
        object container;
        KErrBadName, if the specified name is invalid;
        KErrAlreadyExists, if a reference counting object with the same
        fullname as the generated one already exists in this object container.
*/
EXPORT_C TInt CObjectCon::CheckUniqueFullName(const CObject *anOwner,const TDesC &aName) const
	{

	TInt r=User::ValidateName(aName);
	if (r==KErrNone)
		{
		TFullName n;
		makeFullName(n,anOwner,aName);
		TFullName res;
		TInt h=0;
		if (FindByFullName(h,n,res)==KErrNone)
			r=KErrAlreadyExists;
		}
	return(r);
	}




/**
@internalComponent

protected recursive function for use by CheckUniqueFullName
*/
TBool CObjectCon::NamesMatch(const CObject* anObject, const CObject* aCurrentObject) const
	{

	if (aCurrentObject->iName==NULL) // current object has no name, therefore not the same
		return(EFalse);
	if ((anObject->Name()).Compare(aCurrentObject->Name())!=0) // short names are different, therefore not the same
		return(EFalse);
	if ((aCurrentObject->Owner()==NULL)&&(anObject->Owner()==NULL)) // same short name, no owners = same
		return (ETrue);
	if ((aCurrentObject->Owner()==NULL)||(anObject->Owner()==NULL)) // one has no owner, therefore not the same
		return(EFalse);
	return(NamesMatch(anObject->Owner(),aCurrentObject->Owner())); // go recursive
	}

/**
@internalComponent

protected recursive function for use by CheckUniqueFullName
*/
TBool CObjectCon::NamesMatch(const CObject* anObject, const TName& anObjectName, const CObject* aCurrentObject) const
	{

	if (aCurrentObject->iName==NULL) // current object has no name, therefore not the same
		return(EFalse);
	if (anObjectName.Compare(aCurrentObject->Name())!=0) // short names are different, therefore not the same
		return(EFalse);
	if ((aCurrentObject->Owner()==NULL)&&(anObject->Owner()==NULL)) // same short name, no owners = same
		return (ETrue);
	if ((aCurrentObject->Owner()==NULL)||(anObject->Owner()==NULL)) // one has no owner, therefore not the same
		return(EFalse);
	return(NamesMatch(anObject->Owner(),aCurrentObject->Owner())); // go recursive
	}




/**
Checks that the specified reference counting object does not already exist in 
this object container.

Uniqueness is decided by name, if the object has a name, otherwise by pointer.

If the reference counting object has a name, then it is unique only if there 
is no other reference counting object in the container with the same full 
name.

If the reference counting object has no name, then it is unique only if there 
is no other reference counting object in the container with the same pointer.

@param anObject A pointer to the reference counting object to be checked.

@return KErrNone, if the reference counting object does not already exist in 
        this object container;
        KErrBadName, if the name of the reference counting 
        object is invalid;
        KErrAlreadyExists, if the reference counting object already exists.
*/
EXPORT_C TInt CObjectCon::CheckUniqueFullName(const CObject* anObject) const
	{

	TName name(anObject->Name());
	TInt r=User::ValidateName(name);
	if (r!=KErrNone)
		return r;

	if (!iCount)
		return KErrNone;

	CObject** pS=iObjects;
	CObject** pE=pS+iCount;

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
		if (NamesMatch(anObject,name,*pS))
			return KErrAlreadyExists;
		} while(++pS<pE);
	return KErrNone;
	}




/**
Searches for the reference counting object whose name matches the specified 
match pattern.

The search starts at the reference counting object following the one associated 
with the specified find-handle number. If the specified find-handle number 
is zero, then searching starts at the beginning of the object container.

Notes:

1. names are folded for the purpose of pattern matching

2. if the specified find-handle number is non-zero, then the unique Id part of 
   the number must be the same as the unique Id of this container.

@param aFindHandle On entry, contains the find-handle number of a reference 
                   counting object from where searching is to start, or zero.
                   On return, if an object is found, then this is set to the
                   find-handle number of that object; 
                   if no object is found, then this is set to a generated
                   number, the index part of which has the value 0x7fff.
                   If the object container is empty, then this 
                   reference is not changed.
@param aMatch      The match pattern.
@param aName       A modifiable buffer descriptor with a defined maximum 
                   length. On return, if an object is found, then this
                   contains the name of that object; if no object is found,
                   then the length of this descriptor is set to zero. 
                   If the object container is empty, then this reference is
                   not changed.
                   
@return KErrNone, if a matching reference counting object is found;
        KErrNotFound, if no matching reference counting object can be found or
        the object container is empty.

@panic E32User-CBase 38 if aFindHandle is non-zero and the unique Id part of 
                        it is not the same as the unique Id of this container.
*/
EXPORT_C TInt CObjectCon::FindByName(TInt &aFindHandle,const TDesC &aMatch,TName &aName) const
	{

	if (!iCount)
		return KErrNotFound;
	TInt ix=0;
	if (aFindHandle!=0)
		{
		__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,Panic(EObjFindBadHandle));
		ix=index(aFindHandle)+1;
		};
	CObject** pS=iObjects;
	CObject** pE=pS+iCount;
	pS+=ix;
	while(pS<pE)
		{
		aName=(*pS++)->Name();
		if (aName.MatchF(aMatch)!=KErrNotFound)
			{
			aFindHandle=makeFindHandle(ix,iUniqueID);
			return KErrNone;
			}
		ix++;
		}
	aName.Zero();
	aFindHandle=makeFindHandle(KObjectMaxIndex,iUniqueID);
	return KErrNotFound;
	}




/**
Searches for the reference counting object whose full name matches the
specified match pattern.

The search starts at the reference counting object following the one associated 
with the specified find-handle number. If the specified find-handle number 
is zero, then searching starts at the beginning of the object container.

Notes:

1. names are folded for the purpose of pattern matching

2. if the specified find-handle number is non-zero, then the unique Id part of 
   the number must be the same as the unique Id of this container.

@param aFindHandle On entry, contains the find-handle number of a reference 
                   counting object from where searching is to start or zero.
                   On return, if an object is found, then this is set to the
                   find-handle number of that object; 
                   if no object is found, then this is set to a generated
                   number, the index part of which has the value 0x7fff.
                   If the object container is empty, then this reference is
                   not changed.
@param aMatch      The match pattern.
@param aFullName   A modifiable buffer descriptor with a defined maximum length. 
                   On return, if an object is found, then this contains the
                   full name of that object;
                   if no object is found, then the length of this descriptor
                   is set to zero.
                   If the object container is empty, then this reference is not
                   changed.
                   
@return KErrNone, if a matching reference counting object is found;
        KErrNotFound, if no matching reference counting object can be found or
        the object container is empty.

@panic E32User-CBase 38 if aFindHandle is non-zero and the unique Id part of 
                        it is not the same as the unique Id of this container.
*/
EXPORT_C TInt CObjectCon::FindByFullName(TInt &aFindHandle,const TDesC &aMatch,TFullName &aFullName) const
	{

	if (!iCount)
		return KErrNotFound;
	TInt ix=0;
	if (aFindHandle!=0)
		{
		__ASSERT_ALWAYS(uniqueID(aFindHandle)==iUniqueID,Panic(EObjFindBadHandle));
		ix=index(aFindHandle)+1;
		};
	CObject** pS=iObjects;
	CObject** pE=pS+iCount;
	pS+=ix;
	while(pS<pE)
		{
		aFullName=(*pS++)->FullName();
		if (aFullName.MatchF(aMatch)!=KErrNotFound)
			{
			aFindHandle=makeFindHandle(ix,iUniqueID);
			return KErrNone;
			}
		ix++;
		}
	aFullName.Zero();
	aFindHandle=makeFindHandle(KObjectMaxIndex,UniqueID());
	return KErrNotFound;
	}




/**
Creates a new container index.

@return A pointer to the newly created container index.
*/
EXPORT_C CObjectConIx* CObjectConIx::NewL()
	{

	return new(ELeave) CObjectConIx;
	}




/**
Default constructor.
*/
EXPORT_C CObjectConIx::CObjectConIx()
	: iNextUniqueID(1)
	{
	}




/**
Destructor.

Frees all resources owned by the container index, prior to its destruction.

In particular, it destroys all of its contained object containers.
*/
EXPORT_C CObjectConIx::~CObjectConIx()
	{
	if (iCount)
		{
		TInt i=-1;
		while(++i<iCount)
			{
			CObjectCon* pS=iContainers[i];
			delete pS;
			}
		}
	delete iContainers;
	}




/**
@internalComponent

Actually create the container
*/
EXPORT_C void CObjectConIx::CreateContainerL(CObjectCon*& aContainer)
	{

	aContainer=CObjectCon::NewL();
	if (iUniqueIDHasWrapped)
		{
		// Must search for next free id
		while (LookupByUniqueId(iNextUniqueID) != NULL)
			{
			++iNextUniqueID;
			if (iNextUniqueID == 0)
				iNextUniqueID = 1;
			}
		}
	aContainer->iUniqueID=iNextUniqueID;
	if (iCount==iAllocated)
		{
		TInt newAlloc=iAllocated+KObjectConIxGranularity;
		iContainers=(CObjectCon**)User::ReAllocL(iContainers, newAlloc*sizeof(CObjectCon*));
		iAllocated=newAlloc;
		}
	iContainers[iCount++]=aContainer;
	}




/**
Creates a new object container and adds it into this container
index's collection.

In addition to creating the object container, the function assigns
the next available unique ID to it.

@return A pointer to the new object container.
*/
EXPORT_C CObjectCon* CObjectConIx::CreateL()
	{

	if (iCount == KObjectConIxMaxSize)
		User::Leave(KErrOverflow);
	CObjectCon* pC=NULL;
	TRAPD(r,CreateContainerL(pC))
	if (r!=KErrNone)
		{
		delete pC;
		User::Leave(r);
		}
	++iNextUniqueID;
	if (iNextUniqueID == 0)
		{
		iNextUniqueID = 1;
		iUniqueIDHasWrapped = 1;
		}
	return pC;
	}




/**
Removes the specified object container from this container index and
deletes it.

@param aCon A pointer to the object container to be removed. If the pointer is NULL,
            the function just returns.

@panic E32USER-CBASE 36 if the object container cannnot be found.
*/
EXPORT_C void CObjectConIx::Remove(CObjectCon* aCon)
	{
	if (!aCon)
		return;
	CObjectCon** pS=iContainers;
	CObjectCon** pE=pS+iCount;
	while(pS<pE)
		{
		if (*pS==aCon)
			{
			Mem::Move((TAny*)pS,(TAny*)(pS+1),TInt(pE)-TInt(pS)-sizeof(CObjectCon*));
			TInt newAlloc=--iCount;
			if (iCount == 0)
				iUniqueIDHasWrapped = 0;
			newAlloc=(newAlloc+(KObjectConIxGranularity-1))&~(KObjectConIxGranularity-1);
			if (newAlloc!=iAllocated)
				{
				if (newAlloc)
					iContainers=(CObjectCon**)User::ReAlloc(iContainers,newAlloc*sizeof(CObjectCon*));
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
	Panic(EObjRemoveContainerNotFound);
	}




/**
Gets a pointer to the object container with the specified unique ID, or NULL.
*/
CObjectCon* CObjectConIx::LookupByUniqueId(TInt aUniqueId) const
	{
	
	CObjectCon** pS=iContainers;
	CObjectCon** pE=pS+iCount;
	while(pS<pE)
		{
		CObjectCon *pO=*pS++;
		if (pO->iUniqueID==aUniqueId)
			return pO;
		}
	return NULL;
	}




/**
Gets a pointer to the object container with the unique ID from the specified
find handle.

@param aFindHandle The find handle.

@return A pointer to the object container with a matching unique ID. If no 
        matching object container can be found, then this is NULL.
*/
EXPORT_C CObjectCon* CObjectConIx::Lookup(TInt aFindHandle) const
	{

	return LookupByUniqueId(uniqueID(aFindHandle));
	}

