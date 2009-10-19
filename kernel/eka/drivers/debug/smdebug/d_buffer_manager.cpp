// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <kernel/kernel.h>
#include "d_buffer_manager.h"

using namespace Debug;

// Global buffer manager
DBufferManager TheDBufferManager;

DBufferManager::DBufferManager()
	{
	}

DBufferManager::~DBufferManager()
	{
	for(TInt i=0; i<iBuffers.Count(); i++)
		{
		if(iBuffers[i].iSize>0 && iBuffers[i].iValue)
			{
			Kern::Free((TAny*)iBuffers[i].iValue);
			}
		}
	
	iBuffers.Close();
	}

/**
 * Creates a buffer and adds it to the buffer manager
 * @param aBufferDetails Contains the size and tag ID of the buffer to be created and the location
 *						in which the resulting buffer is created
 * @return One of the System wide error codes
 */
TInt DBufferManager::CreateBuffer(TTag& aBufferDetails)
	{
	if(aBufferDetails.iType != ETagTypePointer)
		{
		return KErrArgument;
		}
	if(aBufferDetails.iSize == 0)
		{
		return KErrArgument;
		}

	for(TInt i=0; i<iBuffers.Count(); i++)
		{
		if(iBuffers[i].iTagId == aBufferDetails.iTagId)
			{
			return KErrAlreadyExists;
			}
		}
	
	TAny* buffer = Kern::AllocZ(aBufferDetails.iSize);
	if(!buffer)
		{
		return KErrNoMemory;
		}
	
	aBufferDetails.iValue = (TUint32)buffer;
	if(KErrNone != iBuffers.Append(aBufferDetails))
		{
		Kern::Free(buffer);
		return KErrNoMemory;
		}
	
	return KErrNone;
	}

/**
 * Retrieves buffer details for a given tag
 * @param aTag The tag for which to get buffer details
 * @return One of the System wide error codes
 */
TInt DBufferManager::GetBufferDetails(const TBufferType aBufferType, TTag& aTag) const
	{
	for(TInt i=0; i<iBuffers.Count(); i++)
		{
		if(iBuffers[i].iTagId == aBufferType)
			{
			aTag = iBuffers[i];
			return KErrNone;
			}
		}
	
	aTag.iTagId = aBufferType;
	aTag.iType = ETagTypePointer;
	aTag.iSize = 0;
	aTag.iValue = NULL;

	return KErrNotFound;
	}

