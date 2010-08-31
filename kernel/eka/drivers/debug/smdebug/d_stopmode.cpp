// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Responsible for dealing with requests for the SM interface
//

/**
@file
@internalComponent
@prototype
*/

#include <sm_debug_api.h>

using namespace Debug;

_LIT(KLitLocal,"Local-");
_LIT(KColonColon,"::");

/**
 * This is the generic exit point for all stop mode routines
 * @param aReturnValue Symbian Error Code to return
 * @return One of the Symbian wide error codes
 */
EXPORT_C TInt StopModeDebug::ExitPoint(const TInt aReturnValue)
	{
	return aReturnValue;
	}

/**
 * This is the routine that shall return lists of information such as
 * process list or code segment list etc
 * @pre aItem->iBufferAddress must be word aligned
 * @param aItem Describes the list to retrieve
 * @param aCheckConsistent Should we check the mutex on the list
 * @return One of the Symbian wide error codes
 */
EXPORT_C TInt StopModeDebug::GetList(const TListItem* aItem, TBool aCheckConsistent)
	{
	//Check arguments
	if(!aItem ||
		((TUint32)aItem->iBufferAddress == 0)||
		((TUint32)aItem->iBufferAddress % 4) )
		{
		return StopModeDebug::ExitPoint(KErrArgument);
		}

	switch(aItem->iListId)
		{
		case ECodeSegs:
			{
			return StopModeDebug::ExitPoint(StopModeDebug::GetCodeSegList(aItem, aCheckConsistent));
			}
		case EProcesses:
			{
			return StopModeDebug::ExitPoint(StopModeDebug::GetProcessList(aItem, aCheckConsistent));
			}
		case EStaticInfo:
		    {
		    return StopModeDebug::ExitPoint(StopModeDebug::GetStaticInfo(aItem, aCheckConsistent));
		    }
		default:
			{
			return StopModeDebug::ExitPoint(KErrUnknown);
			}
		}
	}

TInt StopModeDebug::CopyAndExpandDes(const TDesC& aSrc, TDes& aDest)
	{
	//check bounds
	if(aSrc.Length() * 2 > aDest.MaxLength())
		{
		return KErrArgument;
		}

	//get a pointer to the start of the destination descriptor
	TUint16* destPtr = (TUint16*)aDest.Ptr();

	//get pointers to the start and end of the aSrc descriptor
	const TUint8* srcPtr = aSrc.Ptr();
	const TUint8* srcEnd = srcPtr + aSrc.Length();

	//copy the characters from aSrc into aDest, expanding to make them 16-bit characters
	while(srcPtr < srcEnd)
		{
		*destPtr = (TUint16)*srcPtr;
		destPtr++;
		srcPtr++;
		}

	//set aDest's length to reflect the new contents
	aDest.SetLength(2*aSrc.Length());
	return KErrNone;
	}

/** 
 * This is a function used to test communications with the Stop Mode API
 * We pass in aItem which is interpreted in a different way to normal to allow 
 * us to test different scenarios:
 *		
 *		1. Sending in aItem.iSize = 0xFFFFFFFF will result in the response buffer being
 *		   filled with 0xFFFFFFFF
 *
 * @param aItem Drives the test according to its parameters
 */
EXPORT_C TInt StopModeDebug::TestAPI(const TListItem* aItem)
	{
	//Check params are valid
	if(!aItem	||	((TUint32)aItem->iBufferAddress % 4)	||
		   ((TUint32)aItem->iBufferAddress == 0) ||
		   (aItem->iBufferSize % 4)	)
		{
		return StopModeDebug::ExitPoint(KErrArgument);
		}

	//Performs the test function
	if(aItem->iSize == 0xFFFFFFFF)
		{		
		//Write all 0xFFFFFFFF into the entire buffer
		TUint8* pos = (TUint8*)aItem->iBufferAddress;
		while(pos < (	(TUint8*)aItem->iBufferAddress + aItem->iBufferSize) )
			{
			*pos = 0xFF;
			++pos;
			}
		}
		
	return StopModeDebug::ExitPoint(KErrNone);
	}

/**
 * Reads the raw name for this object instead of using API in DObject,
 * as we can't meet the preconditions
 * @param DObject object whose name we want
 */
void StopModeDebug::GetObjectFullName(const DObject* aObj, TFullName& aName)
	{
	if(aObj->iOwner)
		{
		GetObjectFullName(aObj->iOwner, aName);
		aName.Append(KColonColon);
		}
	
    if (aObj->iName)
		{
        aName.Append(*aObj->iName);
		}
     else
        {
        aName.Append(KLitLocal);
        aName.AppendNumFixedWidth((TInt)aObj,EHex,8);
        }
	}

// End of file d_stopmode.cpp
