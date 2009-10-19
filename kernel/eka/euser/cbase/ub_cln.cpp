// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\cbase\ub_cln.cpp
// 
//

#include "ub_std.h"
#include "us_data.h"

const TInt KCleanupGranularity=4;
const TInt KCleanupInitialSlots=8;

LOCAL_C void doDelete(CBase *aPtr)
//
// Delete the CBase pointer
//
	{

	delete aPtr;
	}

LOCAL_C CCleanup &cleanup()
//
// Return the CTrapHandler's cleanup list.
//
	{

	TCleanupTrapHandler *pH=(TCleanupTrapHandler *)GetTrapHandler();
	__ASSERT_ALWAYS(pH!=NULL,Panic(EClnNoTrapHandlerInstalled));
	return(pH->Cleanup());
	}




TCleanupTrapHandler::TCleanupTrapHandler()
	: iCleanup(NULL)
/**
Default constructor.
*/
	{}




void TCleanupTrapHandler::Trap()
/**
Deals with the invocation of a call to TRAP.
*/
	{

	iCleanup->NextLevel();
	}




void TCleanupTrapHandler::UnTrap()
/**
Deals with a function exiting a TRAP without leaving.
*/
	{

	iCleanup->PreviousLevel();
	}




void TCleanupTrapHandler::Leave(TInt /*aValue*/)
/**
Deals with a function within a TRAP leaving.
	
@param aValue The leave value.
*/
	{

	iCleanup->PopAndDestroyAll();
	}




class TCleanupStackItem
	{
public:
	void Set(const TCleanupItem &aItem);
	inline void Cleanup();
	inline TBool IsLevelMarker() const;
	inline void MarkLevel();
	inline void PushLevel();
	inline TInt PopLevel();
	inline TBool Check(TAny* aExpectedItem) const;
private:
	TCleanupOperation iOperation;
	union
		{
		TAny *iPtr;
		TInt iLevelCount;			// may stack >1 level on this entry
		};
	};
inline void TCleanupStackItem::MarkLevel()
	{ iOperation=NULL; iLevelCount=1; }
inline TBool TCleanupStackItem::IsLevelMarker() const
	{ return (iOperation==NULL); }
inline void TCleanupStackItem::Cleanup()
	{ (*iOperation)(iPtr); }
inline void TCleanupStackItem::PushLevel()
	{ ++iLevelCount; }
inline TInt TCleanupStackItem::PopLevel()
	{ return (--iLevelCount); }
inline TBool TCleanupStackItem::Check(TAny* aExpectedItem) const
	{ return (iOperation && iPtr==aExpectedItem); }

void TCleanupStackItem::Set(const TCleanupItem &anItem)
//
// Initialise an entry as a cleanup item.
//
	{

	__ASSERT_ALWAYS(anItem.iOperation!=NULL,Panic(EClnNoCleanupOperation));
	iOperation=anItem.iOperation;
	iPtr=anItem.iPtr;
	}




EXPORT_C CCleanup *CCleanup::New()
/**
Creates a new cleanup stack object.

The cleanup stack itself is allocated with enough space initially to hold 
a number of stack items.

@return A pointer to the new cleanup stack object. This is Null if there is 
        insufficient memory.
*/
	{

	CCleanup *pC=new CCleanup;
	if (pC!=NULL)
		{
		TCleanupStackItem *base=(TCleanupStackItem *)User::Alloc(KCleanupInitialSlots*sizeof(TCleanupStackItem));
		if (base!=NULL)
			{
			pC->iBase=base;
			pC->iNext=base;
			pC->iTop=base+KCleanupInitialSlots;
			}
		else
			{
			delete pC;
			pC=NULL;
			}
		}
	return(pC);
	}




EXPORT_C CCleanup *CCleanup::NewL()
/**
Creates a new cleanup stack object, and leaves if there is insufficient memory 
to create it.

The cleanup stack itself is allocated with enough space initially to hold 
a number of stack items.

@return A pointer to the new cleanup stack object. This is Null if there is 
        nsufficient memory.
*/
	{

	CCleanup *pC=New();
	User::LeaveIfNull(pC);
	return(pC);
	}




EXPORT_C CCleanup::CCleanup()
/**
Default constructor.
*/
	{

//	iBase=NULL;
//	iTop=NULL;
//	iNext=NULL;
	}




EXPORT_C CCleanup::~CCleanup()
/**
Destructor.

Pops and destroys all items from the cleanup stack and then destroys
the cleanup stack itself.
*/
	{

	while (iNext>iBase)
		PopAndDestroyAll();
	User::Free(iBase);
	}




EXPORT_C void CCleanup::NextLevel()
/**
Goes to the next cleanup level.
*/
	{

	if (iNext>iBase && (iNext-1)->IsLevelMarker())
		(iNext-1)->PushLevel();
	else
		{
		iNext->MarkLevel();
		++iNext;
		}
	}




EXPORT_C void CCleanup::PreviousLevel()
/**
Goes to the previous cleanup level.

@panic E32USER-CBase 71 If the previous stack item does not represent a cleanup 
       level.
*/
	{

	TCleanupStackItem *item=iNext;
	--item;
	// current level must be empty
	__ASSERT_ALWAYS(item->IsLevelMarker(), Panic(EClnLevelNotEmpty));
	if (item->PopLevel())
		++item;
	iNext=item;
	}




EXPORT_C void CCleanup::PushL(TAny *aPtr)
/**
Pushes a cleanup item onto the cleanup stack.

The cleanup item represents an operation that frees the specified heap cell.

@param aPtr A pointer to a heap cell that will be freed by
            the cleanup operation.
*/
	{

	PushL(TCleanupItem(User::Free,aPtr));
	}




EXPORT_C void CCleanup::PushL(CBase *anObject)
/**
Pushes a cleanup item onto the cleanup stack.

The cleanup item represents an operation that deletes the specified CBase 
derived object.

@param anObject A pointer to CBase derived object that will be deleted by 
                the cleanup operation.
*/
	{

	PushL(TCleanupItem(TCleanupOperation(doDelete),anObject));
	}




EXPORT_C void CCleanup::PushL(TCleanupItem anItem)
/**
Pushes a cleanup item onto the cleanup stack.

The cleanup item represents a call back operation that performs the required 
cleanup.

@param anItem Encapsulates a cleanup operation and an object on which the 
              cleanup operation is to be performed.
              
@see CleanupClosePushL
@see CleanupReleasePushL
@see CleanupDeletePushL
*/
	{

	TCleanupStackItem *item=iNext;
	__ASSERT_ALWAYS(item>iBase,Panic(EClnPushAtLevelZero));
	__ASSERT_ALWAYS(item<iTop,Panic(EClnNoFreeSlotItem));
	item->Set(anItem);
	iNext=++item;
//
// We always try and make sure that there are two free slots in the cleanup array.
// one for a level marker and one for an item to follow it
// If this fails its o.k. as we have already added the entry to the list, so
// it will be cleaned up o.k.
//
	if (item+1>=iTop)
		{
		TInt size=(TUint8 *)(iTop+KCleanupGranularity)-(TUint8 *)iBase;
		TCleanupStackItem *base=(TCleanupStackItem *)User::ReAllocL(iBase,size);
		iNext=PtrAdd(base,(TUint8 *)item-(TUint8 *)iBase);
		iBase=base;
		iTop=PtrAdd(base,size);
		}
	}




EXPORT_C void CCleanup::DoPop(TInt aCount,TBool aDestroy)
/**
Provides an implementation for Pop() and PopAndDestroy().

@param aCount   The number of cleanup items to be popped from
                the cleanup stack.
@param aDestroy ETrue, if cleanup is to be performed; EFalse, otherwise.
*/
	{

	__ASSERT_ALWAYS(aCount>=0,Panic(EClnPopCountNegative));
	__ASSERT_ALWAYS((iNext-aCount)>=iBase,Panic(EClnPopUnderflow));
	while (aCount--)
		{
		--iNext;
		__ASSERT_ALWAYS(!iNext->IsLevelMarker(),Panic(EClnPopAcrossLevels));
		if (aDestroy)
			{
			TInt offset = iNext - iBase;
			iNext->Cleanup();
			// Check that there are no extra items on the cleanup stack
			// (if there are, we will not be deleting the right aCount items)
			__ASSERT_ALWAYS((iNext - iBase) == offset,Panic(EClnStackModified));
			}
		}
	}




EXPORT_C void CCleanup::DoPopAll(TBool aDestroy)
/**
Provides an implementation for PopAll() and PopAndDestroyAll().

@param aDestroy ETrue, if cleanup is to be performed; EFalse, otherwise.
*/
	{

	__ASSERT_ALWAYS(iNext>iBase,Panic(EClnLevelUnderflow));
	while (!(--iNext)->IsLevelMarker())
		{
		if (aDestroy)
			iNext->Cleanup();
		}
	if (iNext->PopLevel())
		++iNext;				// still marks a level
	}




EXPORT_C void CCleanup::Pop()
/**
Pops a single cleanup item from the cleanup stack.

@panic E32USER-CBase 64 If there are no items on the cleanup stack.
@panic E32USER-CBase 63 If a cleanup level is crossed.
*/
	{

	DoPop(1,EFalse);
	}




EXPORT_C void CCleanup::Pop(TInt aCount)
/**
Pops the specified number of cleanup items from the cleanup stack.

@param aCount The number of cleanup items to be popped from the cleanup stack.

@panic E32USER-CBase 70 If the specified number of cleanup items is negative.
@panic E32USER-CBase 64 If the specifed number of items is greater than the 
       number of items on the cleanup stack.
@panic E32USER-CBase 63 If the specified number of items is such that it causes 
       a cleanup level to be crossed.
*/
	{

	DoPop(aCount,EFalse);
	}




EXPORT_C void CCleanup::PopAll()
/**
Pops all cleanup items at the current level, and then decrements the level.
*/
	{

	DoPopAll(EFalse);
	}




EXPORT_C void CCleanup::PopAndDestroy()
/**
Pops a single cleanup item from the cleanup stack, and invokes its cleanup 
operation.

@panic E32USER-CBase 64 If there are no items on the cleanup stack.
@panic E32USER-CBase 63 If a cleanup level is crossed.
*/
	{

	DoPop(1,ETrue);
	}




EXPORT_C void CCleanup::PopAndDestroy(TInt aCount)
/**
Pops the specified number of cleanup items from the cleanup stack, and invokes 
their cleanup operations.

@param aCount The number of cleanup items to be popped from the cleanup stack.

@panic E32USER-CBase 70 If the specified number of cleanup items is negative.
@panic E32USER-CBase 64 If the specifed number of items is greater than the 
       number of items on the cleanup stack.
@panic E32USER-CBase 63 If the specified number of items is such that it causes 
       a cleanup level to be crossed.
*/
	{

	DoPop(aCount,ETrue);
	}




EXPORT_C void CCleanup::PopAndDestroyAll()
/**
Pops all cleanup items at the current level, invokes their cleanup operations 
and then decrements the level.
*/
	{

	DoPopAll(ETrue);
	}




EXPORT_C void CCleanup::Check(TAny* aExpectedItem)
/**
Checks that the cleanup item at the top of the cleanup stack
represents a cleanup operation for the specified object.

@param aExpectedItem The object which is the subject of the test.
*/
	{

	TCleanupStackItem* last=iNext-1;
	__ASSERT_ALWAYS(last>=iBase && last->Check(aExpectedItem), Panic(EClnCheckFailed));
	}




EXPORT_C CTrapCleanup *CTrapCleanup::New()
/**
Allocates and constructs a cleanup stack.

If successfully constructed, this cleanup stack becomes
the current cleanup stack.

@return A pointer to the new cleanup stack. This pointer is NULL, if allocation 
        fails.
*/
	{

	CTrapCleanup *pT=new CTrapCleanup;
	if (pT!=NULL)
		{
		CCleanup *pC=CCleanup::New();
		if (pC!=NULL)
			{
			pT->iHandler.iCleanup=pC;
			pT->iOldHandler=User::SetTrapHandler(&pT->iHandler);
			}
		else
			{
			delete pT;
			pT=NULL;
			}
		}
	return(pT);
	}




EXPORT_C CTrapCleanup::CTrapCleanup()
/**
Default constructor.
*/
//	: iHandler()
	{
	}




EXPORT_C CTrapCleanup::~CTrapCleanup()
/**
Destructor.

Frees resources owned by the object, prior to its destruction. This cleanup 
stack ceases to be the current cleanup stack.

If there is a stack of cleanup stacks, then the next cleanup stack becomes 
the current cleanup stack.
*/
	{

	if (iHandler.iCleanup!=NULL)
		{
		User::SetTrapHandler(iOldHandler);
		delete iHandler.iCleanup;
		}
	}




EXPORT_C void CleanupStack::PushL(TAny *aPtr)
/**
Pushes a pointer to an object onto the cleanup stack.

If a leave occurs while an object is on the stack, it is cleaned
up automatically. Untyped objects are cleaned up with User::Free()
(a rather limited form of cleanup, not even the C++ destructor is called).

Typically, when an object has been fully constructed and it can be guaranteed
that a pointer to this new object is stored in some other object before a leave
occurs, issue CleanupStack::Pop() to pop it back off the stack.

If no cleanup stack has been allocated, a panic occurs.

It is guaranteed that the object is pushed onto the cleanup stack. However,
this function may leave if a stack frame for the next PushL() cannot be
allocated. In this case, the cleanup stack will be cleaned up as normal, and
no extra programmer intervention is needed.

@param aPtr Pointer to any object. If cleanup is necessary, the object will be
            freed by User::Free(), which does not invoke any destructor: it
            simply frees its memory

@panic E32USER-CBase 66 if a call to this function is made when no prior
       call to TRAP has been made.
*/
	{

	cleanup().PushL(aPtr);
	}




EXPORT_C void CleanupStack::PushL(CBase *aPtr)
/**
Pushes a pointer to an object onto the cleanup stack.

If a leave occurs while an object is on the stack, it is cleaned
up automatically. CBase derived objects are cleaned up with delete.

Typically, when an object has been fully constructed and it can be guaranteed
that a pointer to this new object is stored in some other object before a leave
occurs, issue CleanupStack::Pop() to pop it back off the stack. 

If no cleanup stack has been allocated, a panic occurs.

It is guaranteed that the object is pushed onto the cleanup stack. However,
this function may leave if a stack frame for the next PushL() cannot be
allocated. In this case, the cleanup stack will be cleaned up as normal,
and no extra programmer intervention is needed.

@param aPtr Pointer to a CBase-derived object. If cleanup is necessary, the
            object will be freed by delete, thus invoking its destructor,
            and freeing its memory.

@panic E32USER-CBase 66 if a call to this function is made when no prior
       call to TRAP has been made.
*/
	{

	cleanup().PushL(aPtr);
	}




EXPORT_C void CleanupStack::PushL(TCleanupItem anItem)
/**
Pushes a cleanup item onto the cleanup stack.

If a leave occurs while a cleanup item is on the stack, the cleanup operation
defined in the construction of the TCleanupItem, is invoked.

Typically, when an object has been fully constructed and it can be guaranteed
that a pointer to this new object is stored in some other object before a leave
occurs, issue CleanupStack::Pop() to pop it back off the stack. 

If no cleanup stack has been allocated, a panic occurs.

It is guaranteed that the object is pushed onto the cleanup stack. However,
this function may leave if a stack frame for the next PushL() cannot be
allocated. In this case, the cleanup stack will be cleaned up as normal,
and no extra programmer intervention is needed.

@param anItem A cleanup item. If cleanup is necessary, the cleanup operation
              defined in the construction of anItem is called.

@panic E32USER-CBase 66 if a call to this function is made when no prior
       call to TRAP has been made.
*/		
	{

	cleanup().PushL(anItem);
	}




EXPORT_C void CleanupStack::Pop()
/**
Pops an object previously pushed onto the cleanup stack
by CleanupStack::PushL(). 

After an object has been successfully constructed and stored within
another object, it cannot be orphaned and, therefore, the object
(i.e. a pointer or a cleanup item) can be popped from the cleanup stack.

If no cleanup stack has been allocated, or there is nothing on the stack,
a panic is raised.
*/
	{

	cleanup().Pop();
	}




EXPORT_C void CleanupStack::Pop(TInt aCount)
/**
Pops a specified number of objects previously pushed onto the
cleanup stack by CleanupStack::PushL().

After an object has been successfully constructed and stored within another
object, it cannot be orphaned and, therefore, the object(s), that is, pointers
and cleanup items can be popped from the cleanup stack.

If no cleanup stack has been allocated, or there is nothing on the stack,
a panic is raised.

@param aCount The number of objects to be popped off the cleanup stack.
*/
	{

	cleanup().Pop(aCount);
	}




EXPORT_C void CleanupStack::PopAndDestroy()
/**
Pops and cleans up an item pushed onto the stack.

If the item on the stack is a CBase* pointer, the pointer is removed from
the stack and the object is destroyed with delete.

If the item on the stack is a TAny* pointer, the pointer is removed from
the stack and the memory occupied by the object is freed with User::Free().

If the item on the stack is a cleanup item, i.e. an object of
type TCleanupItem, the item is removed from the stack and the cleanup
operation defined during construction of the TCleanupItem object is invoked.

If no cleanup stack has been allocated, or there is nothing on the stack,
a panic occurs.
*/
	{

	cleanup().PopAndDestroy();
	}




EXPORT_C void CleanupStack::PopAndDestroy(TInt aCount)
/**
Pops and cleans up the specified number of items pushed onto the stack.

If an item on the stack is a CBase* pointer, the pointer is removed from 
the stack and the object is destroyed with delete.

If an item on the stack is a TAny* pointer, the pointer is removed from the 
stack and the memory occupied by the object is freed with User::Free().

If an item on the stack is a cleanup item, i.e. an object of type TCleanupItem, 
the item is removed from the stack and the cleanup operation defined during 
construction of the TCleanupItem object is invoked.

If no cleanup stack has been allocated, or there is nothing on the stack, 
a panic occurs.

@param aCount The number of objects to be popped off the cleanup stack and 
destroyed.
*/
	{

	cleanup().PopAndDestroy(aCount);
	}




EXPORT_C void CleanupStack::Check(TAny* aExpectedItem)
/**
Checks that the specified object is at the top of the cleanup stack.

If the specified item is not at the top of the cleanup stack, then the function 
raises an E32USER-CBase 90 panic.

The function is part of Symbian OS in both debug and release builds, and is 
an aid to debugging.

@param aExpectedItem A pointer to the item expected to be at the top of the 
                     cleanup stack.
*/
	{

	cleanup().Check(aExpectedItem);
	}
