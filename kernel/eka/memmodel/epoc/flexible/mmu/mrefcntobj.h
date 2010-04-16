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
//

/**
 @file
 @internalComponent
*/

#ifndef MREFCNTOBJ_H
#define MREFCNTOBJ_H

#include <klib.h>

/**
Base class for creating reference counted objects.

This provide basic reference counting and asynchronous cleanup
for derived classes. An object whose reference count reaches zero
will be destroyed.
*/
class DReferenceCountedObject : public DBase
	{
public:
	/**
	Constructs this object with an initial reference count of one.
	*/
	DReferenceCountedObject();

	/**
	Atomically increment this object's reference count.
	A fault is raised if the count was initially <= 0.

	@pre Calling thread must be in a critical section
	@pre #iReferenceCount>0
	*/
	void Open();

	/**
	Atomically increment this object's reference count if it 
	was initially > 0.

	This is for use in situations where a reference counted object 
	is found by looking in a list, and the list may contain
	objects which have had their reference counts decremented
	to zero and are in the process of being cleaned up. A failure
	to open a reference on these objects should be taken as an
	indication that the object should be ignored, and treated as
	though it were never found.
	
	@return True if the count was incremented.
			False if it wasn't: because it was initially <= 0.

	@pre Calling thread must be in a critical section
	*/
	TBool TryOpen();

	/**
	Decrement this object's reference count and if it reaches zero,
	delete this object.

	@pre Calling thread must be in a critical section.
	@pre No fast mutex can be held.
	@pre No mutexes with order greater less than KMutexOrdKernelHeap can be held.
	*/
	void Close();

	/**
	Decrement this object's reference count and if this reaches zero,
	queue this object for asynchronous deletion.

	@pre Calling thread must be in a critical section.
	@pre No fast mutex can be held.
	*/
	void AsyncClose();

protected:
	/**
	Destructor which asserts in debug builds that the object's reference count is zero.
	*/
	virtual ~DReferenceCountedObject();

	/**
	Return true if the preconditions for #Close are met.
	This is for use by derived classes which overload the #Close method.
	*/
	TBool CheckCloseIsSafe();

protected:

	/**
	This object's reference count. Must always be >= 0.
	*/
	TInt iReferenceCount;
	};


FORCE_INLINE DReferenceCountedObject::DReferenceCountedObject()
	: iReferenceCount(1)
	{
	}

#endif
