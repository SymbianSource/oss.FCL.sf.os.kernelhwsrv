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
// e32\euser\cbase\ub_circ.cpp
// 
//

#include "ub_std.h"

EXPORT_C CCirBufBase::CCirBufBase(TInt aSize)
/**
Constructor taking the size of an object within the buffer.

@param aSize The size of an object in the buffer.

@panic E32USER-CBase 72, if aSize is zero or negative. 
*/
	: iSize(aSize)
	{

	__ASSERT_ALWAYS(iSize>0,Panic(ECircItemSizeNegativeOrZero));
//	iCount=0;
//	iLength=0;
//	iPtr=NULL;
//	iPtrE=NULL;
//	iHead=NULL;
//	iTail=NULL;
	}

EXPORT_C CCirBufBase::~CCirBufBase()
/**
Destructor.

This frees the memory allocated to the buffer.
*/
	{

	User::Free(iPtr);
	}

EXPORT_C void CCirBufBase::SetLengthL(TInt aLength)
/**
Sets the maximum capacity of this circular buffer, and resets all
of the buffer pointers.

The capacity is the maximum number of elements that the buffer can hold.

The buffer itself is allocated as a result of a call to this function. If 
the function has previously been called, then any existing buffer is freed and 
any information in it is lost.

Notes:

1. This function must be called before attempting to add any objects to
   the buffer.

2. The function can leave if there is insufficient memory available to
   allocate the buffer.

@param aLength The maximum capacity of the circular buffer.

@panic E32USER-CBase 73, if aLength is zero or negative. 
*/
	{

	__ASSERT_ALWAYS(aLength>0,Panic(ECircSetLengthNegativeOrZero));
	iPtr=(TUint8 *)User::ReAllocL(iPtr,aLength*iSize);
	iPtrE=iPtr+(aLength*iSize);
	iHead=iTail=iPtr;
	iLength=aLength;
	iCount=0;
	}

EXPORT_C void CCirBufBase::Reset()
/**
Empties the buffer.
*/
	{

	iHead=iTail=iPtr;
	iCount=0;
#if defined(_DEBUG)
	Mem::FillZ(iPtr,iLength*iSize);
#endif
	}

EXPORT_C TInt CCirBufBase::DoAdd(const TUint8 *aPtr)
/**
Implementation function for CCirBuf::Add(const T*)

Adds a single object to the circular buffer, but only if there is
space available.

@param aPtr A pointer to the object to be added.

@return 1 if the object is successfully added. 0 if the object cannot be added 
        because the circular buffer is full.

@panic E32USER-CBase 74, if a call to CCirBufBase::SetLengthL() has not been
                         made before calling this function.

@see CCirBuf::Add
@see CCirBufBase::SetLengthL
*/
	{

	__ASSERT_ALWAYS(iPtr!=NULL,Panic(ECircNoBufferAllocated));
	if (iCount>=iLength)
		return(KErrNone);
	Mem::Copy(iHead,aPtr,iSize);
	iCount++;
	iHead+=iSize;
	if (iHead>=iPtrE)
		iHead=iPtr;
	return(1);
	}

EXPORT_C TInt CCirBufBase::DoAdd(const TUint8 *aPtr,TInt aCount)
/**
Implementation function for CCirBuf::Add(const T*,TInt)

Adds multiple objects to the circular buffer, but only if there is
space available.

@param aPtr   A pointer to a set of contiguous objects to be added.

@param aCount The number of objects to be added.

@return The number of objects successfully added to the buffer. This value 
        may be less than the number requested and can range from 0 to aCount. 

@panic E32USER-CBase 74, if a call to CCirBufBase::SetLengthL() has not been
                         made before calling this function.
@panic E32USER-CBase 75, if aCount is not a positive value. 

@see CCirBuf::Add
@see CCirBufBase::SetLengthL
*/
	{

	__ASSERT_ALWAYS(iPtr!=NULL,Panic(ECircNoBufferAllocated));
	__ASSERT_ALWAYS(aCount>0,Panic(ECircAddCountNegative));
	TInt rem=iLength-iCount;
	if (rem==0)
		return(0);
	aCount=Min(aCount,rem);
	rem=(iPtrE-iHead)/iSize;
	if (aCount<=rem)
		iHead=Mem::Copy(iHead,aPtr,aCount*iSize);
	else
		{
		TInt len=(rem*iSize);
		Mem::Copy(iHead,aPtr,len);
		iHead=Mem::Copy(iPtr,aPtr+len,(aCount*iSize)-len);
		}
	if (iHead>=iPtrE)
		iHead=iPtr;
	iCount+=aCount;
	return(aCount);
	}

EXPORT_C TInt CCirBufBase::DoRemove(TUint8 *aPtr)
/**
Implementation function for CCirBuf::Remove(T*)

Removes a single object from the circular buffer, but only if there are
objects in the buffer.

A binary copy of the object is made to aPtr.

@param aPtr A pointer to a location supplied by the caller.

@return 1 if an object is successfully removed. 0 if an object cannot be removed 
        because the circular buffer is empty.

@see CCirBuf::Remove
*/
	{

	if (iCount==0)
		return(0);
	Mem::Copy(aPtr,iTail,iSize);
	iTail+=iSize;
	if (iTail>=iPtrE)
		iTail=iPtr;
	iCount--;
	return(1);
	}

EXPORT_C TInt CCirBufBase::DoRemove(TUint8 *aPtr,TInt aCount)
/**
Implementation function for CCirBuf::Remove(T*,TInt)

Attempts to remove aCount objects from the circular buffer, but only if there
are objects in the buffer.

A binary copy of the objects is made to aPtr.

@param aPtr   A pointer to a location supplied by the caller capable of
              holding aCount objects.

@param aCount The number of objects to be removed from the circular buffer.

@return The number of objects successfully removed from the buffer. This value
        may be less than the number requested, and can range from 0 to aCount.

@panic E32USER-CBase 76, if aCount is not a positive value.

@see CCirBuf::Remove
*/
	{

	if (iCount==0)
		return(0);
	__ASSERT_ALWAYS(aCount>0,Panic(ECircRemoveCountNegative));
	aCount=Min(aCount,iCount);
	TInt rem=(iPtrE-iTail)/iSize;
	TInt len=rem*iSize;
	if (aCount<=rem)
		{
		Mem::Copy(aPtr,iTail,aCount*iSize);
		iTail+=aCount*iSize;
		}
	else
		{
		Mem::Copy(aPtr,iTail,len);
		rem=(aCount*iSize)-len;
		Mem::Copy(aPtr+len,iPtr,rem);
		iTail=iPtr+rem;
		}
	if (iTail>=iPtrE)
		iTail=iPtr;
	iCount-=aCount;
	return(aCount);
	}

EXPORT_C CCirBuffer::CCirBuffer()
	: CCirBuf<TUint8>()
/**
Default C++ constructor.
*/
	{}

EXPORT_C CCirBuffer::~CCirBuffer()
/**
Destructor
*/
	{
	}

EXPORT_C TInt CCirBuffer::Get()
/**
Removes an unsigned 8-bit integer value from the circular buffer and returns
its value. 

The returned TUint8 is promoted to a TInt to allow for negative error codes, 
e.g. KErrGeneral.

@return The unsigned 8-bit integer value removed from the circular buffer.
        KErrGeneral, if the circular buffer is empty.
*/
	{

	if (iCount==0)
		return(KErrGeneral);
	TUint8 *p=iTail++;
	if (iTail>=iPtrE)
		iTail=iPtr;
	iCount--;
	return(*p);
	}

EXPORT_C TInt CCirBuffer::Put(TInt aVal)
/**
Adds an unsigned 8-bit integer value in the range 0 to 255 to the circular buffer.

If the specified integer is outside the range 0 to 255,
this method discards all but the lowest 8 bits and treats those
as the unsigned integer to store.
For example, specifying -2 (or 510, or -258, etc) will result in 254 being stored,
and therefore in 254 being returned by the Get() method
(and not the number passed to Put()).

@param aVal The unsigned 8-bit integer value to be added.
@return KErrNone, if the unsigned integer is successfully added.
        KErrGeneral, if the unsigned integer cannnot be added because
        the circular buffer is full.

@see CCirBuffer::Get()
*/
	{

	__ASSERT_ALWAYS(iPtr!=NULL,Panic(ECircNoBufferAllocated));
	if (iCount>=iLength)
		return(KErrGeneral);
	*iHead++=(TUint8)aVal;
	if (iHead>=iPtrE)
		iHead=iPtr;
	iCount++;
	return(KErrNone);
	}

