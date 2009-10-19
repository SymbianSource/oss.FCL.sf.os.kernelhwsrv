// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\concur\cfafshmem.cpp
// Definition of the THMem class member functions.
// 
//

#include "cfafshmem.h"


THMem::THMem(void * aNewMemPtr) :	iMemPtr(aNewMemPtr), 
	//iNoMess(ETrue),
	//threadHandle(NO_THREAD), 
	iBaseOffset(KNoOffset)
/**
 *  Constructor. Initialises the members of THMem.
 *
 *  @param aNewMemPtr  Pointer to the memory that this THMem will
 *                     represent
 *  @internalComponent
 */
 
	{ 
	iMessage=NULL;
	}


THMem::THMem(const void * aNewMemPtr,const RMessagePtr2& aMessage,int aNewOffset) 
	: iMemPtr((void*)aNewMemPtr), iBaseOffset(aNewOffset)
/**
 *  Constructor. Initialises the members of THMem.
 *
 *  @param aNewMemPtr  Pointer to the memory that this THMem will
 *                     represent
 *  @param aMessage
 *  @param aNewOffset
 *  @internalComponent
 */
 
	{ 
	iMessage=(RMessagePtr2*)&aMessage;
	}



TInt THMem::Read(void *aBuf, TUint32 aLength, TUint aOffset) const
/**
 *  Reads a chunk of memory from the THMem.
 *
 *  @param aBuf     goal buffer bointer
 *  @param aLength  how many bytes that will be read from
 *                  the THMem
 *  @param aOffset  offset into the THMem where the function
 *                  will start reading
 *  @return Error code              -   KErrNone on success
 *  @internalComponent
 */
	{
	if(iMessage==NULL)
		{
		memcpy(aBuf, ((TUint8*)iMemPtr + aOffset), (TInt)aLength);
		// Always return success
		return KErrNone;
		}
	else if ((iMessage == NULL) || (iMessage->Handle() == KLocalMessageHandle))
		{
		TUint8* ptr = (TUint8*) ((TDes8*) iMemPtr)->Ptr();
		memcpy(aBuf, ptr + iBaseOffset + aOffset, (TInt)aLength);
		// Always return success
		return KErrNone;
		}
	else
		{
#if defined(__SYMBIAN32__)
		// Create EPOC memory representation
		TPtr8 mem((TUint8*)aBuf,aLength);

		// Create EPOC memory representation
		TRAPD(res,iMessage->ReadL(0,mem, iBaseOffset + aOffset));

		// Return error code
		return (TInt)res;
#else
		LFFS_ASSERT(0);
		return E_GENERAL;
#endif
		}
	}



TInt THMem::Write(const void *aBuf, TUint32 aLength, TUint32 aOffset)
/**
 *  Writes a chunk of memory to the THMem.
 *
 *  @param aBuf     source buffer bointer
 *  @param aLength  how many bytes that will be written to
 *                  the THMem
 *  @param aOffset  offset into the THMem where the function
 *                  will start writing
 *  @return Error code              -   KErrNone on success
 *  @internalComponent
 */
	{
	if (iMessage == NULL)
		{
		memcpy(((TUint8*)iMemPtr + aOffset), aBuf, (TInt)aLength);
		// Always return success
		return KErrNone;
		}
	else if ((iMessage == NULL) || (iMessage->Handle() == KLocalMessageHandle))
		{
		TUint8* ptr = (TUint8*) ((TDes8*) iMemPtr)->Ptr();
		memcpy(ptr + iBaseOffset + aOffset, aBuf, (TInt)aLength);
		return KErrNone;
		}
	else
		{
#if defined(__SYMBIAN32__)
		// Create EPOC memory representation
		TPtrC8 mem((TUint8*)aBuf,aLength);

		// Use message to write back to client thread
		TRAPD( res,iMessage->WriteL(0,mem, iBaseOffset + aOffset) );

		// Return error code
		return (TInt)res;
#else
		LFFS_ASSERT(0);
		return E_GENERAL;
#endif
		}
	}



