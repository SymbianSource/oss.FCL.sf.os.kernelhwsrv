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
// e32\euser\us_ref.cpp
// 
//

#include "us_std.h"



/**
Frees the memory holding the contained object.
*/
EXPORT_C void RRefBase::Free()
	{

	if (iPtr)
		{
		if (--iPtr[-1]==0)
			User::Free(iPtr-1);
		iPtr=NULL;
		}
	}




/**
Creates a copy of an object, which is to be contained by
this reference object.

@param aPtr  A pointer to memory holding a copy of the object.
@param aSize The size of the memory required to hold the object.
*/
EXPORT_C void RRefBase::DoAlloc(const TAny *aPtr,TInt aSize)
	{

	__ASSERT_ALWAYS(aSize>=0,Panic(ERefAllocSizeNegative));
	Free();
	iPtr=(TInt *)User::Alloc(aSize+sizeof(TInt));
	if (iPtr!=NULL)
		{
		iPtr++;
		iPtr[-1]=1;
		Mem::Copy(iPtr,aPtr,aSize);
		}
	}




/**
Creates a copy of an object, which is to be contained by
this reference object, and leaves on error.

@param aPtr  A pointer to memory holding a copy of the object.
@param aSize The size of the memory required to hold the object.
*/
EXPORT_C void RRefBase::DoAllocL(const TAny *aPtr,TInt aSize)
	{

	DoAlloc(aPtr,aSize);
	User::LeaveIfNull(iPtr);
	}




/**
Provides an implementation for the RRef class copy
constructor and assignment operator.

@param aRef The reference object to be copied.

@see RRef
*/
EXPORT_C void RRefBase::Copy(const RRefBase &aRef)
	{

	if (this!=(&aRef))
		{
		Free();
		iPtr=aRef.iPtr;
		iPtr[-1]++;
		}
	}

