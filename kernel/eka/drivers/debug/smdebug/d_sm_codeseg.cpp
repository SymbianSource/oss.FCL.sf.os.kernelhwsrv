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
// Stop-mode debug interface implementation as defined in sm_debug_api.h
//

/**
 * @file
 * @internalComponent
 * @prototype
*/

#include <sm_debug_api.h>

using namespace Debug;

/**
 * Stop Mode routine to retrieve the code seg list and place it in the response buffer
 * @param aItem List item describing the list
 * @return One of the system wide error codes
 */
TInt StopModeDebug::GetCodeSegList(const TListItem* aItem, bool aCheckConsistent)
	{
	if(aCheckConsistent)
		{
		DMutex* codeMutex = Kern::CodeSegLock();
		if(!codeMutex || codeMutex->iHoldCount)
			{
			return ExitPoint(KErrNotReady);
			}
		}

	TUint8* bufferInitial = (TUint8*)aItem->iBufferAddress;
	TUint8* buffer = (TUint8*)aItem->iBufferAddress + 12;
	TUint32 bufferSize = aItem->iBufferSize;

	TUint32* buffer32 = (TUint32*)aItem->iBufferAddress;
	*buffer32 = (TUint32)aItem;
	TUint32* size = buffer32+1;
	*size = 12;
	TUint32* nextElementSize = buffer32+2;
	*nextElementSize = 0;
	TUint32 start = aItem->iStartElement;

	while(1)
		{
		DEpocCodeSeg* codeSeg = GetNextCodeSeg(start, aItem->iListScope, aItem->iScopeData);
		if(!codeSeg)
			{
			*size = buffer-bufferInitial;
			return KErrNone; // we're done
			}
		start = (TUint32)codeSeg + 1;

		TInt ret = ProcessCodeSeg(buffer, bufferSize, codeSeg);
		if(KErrNone != ret)
			{
			if(ret > 0)
				{
				Kern::Printf("Next element is %d bytes big\n", ret);
				*nextElementSize = ret;
				ret = KErrTooBig;
				}
			*size = buffer-bufferInitial;
			return ret;
			}
		}
	}

DEpocCodeSeg* StopModeDebug::GetNextCodeSeg(const TUint32 aStart, const TListScope aListScope, const TUint64 aScopeData)
	{
	switch(aListScope)
		{
		case EScopeGlobal:
			return GetNextGlobalCodeSeg(aStart);
		case EScopeThreadSpecific:
			return GetNextThreadSpecificCodeSeg(aStart, aScopeData);
		default:
			return NULL;
		}
	}

DEpocCodeSeg* StopModeDebug::GetNextThreadSpecificCodeSeg(const TUint32 aStart, const TUint64 aThreadId)
	{
	return NULL;
	}

DEpocCodeSeg* StopModeDebug::GetNextGlobalCodeSeg(const TUint32 aStart)
	{
	//get global code seg list
	SDblQue* codeSegList = Kern::CodeSegList();

	DEpocCodeSeg* codeSeg = NULL;
	for (SDblQueLink* codeSegPtr= codeSegList->First(); codeSegPtr!=(SDblQueLink*) (codeSegList); codeSegPtr=codeSegPtr->iNext)
		{
		DEpocCodeSeg* tempCodeSeg = (DEpocCodeSeg*)_LOFF(codeSegPtr,DCodeSeg, iLink);
		if((TUint32)tempCodeSeg >= aStart)
			{
			if(!codeSeg || tempCodeSeg < codeSeg)
				{
				codeSeg = tempCodeSeg;
				}
			}
		}
	return codeSeg;
	}

TInt StopModeDebug::ProcessCodeSeg(TUint8*& aBuffer, TUint32& aBufferSize, DEpocCodeSeg* aCodeSeg)
	{
	//create a memory info object for use in the loop
	TModuleMemoryInfo memoryInfo;

	//get the memory info
	TInt err = aCodeSeg->GetMemoryInfo(memoryInfo, NULL);
	if(err != KErrNone)
		{
		//there's been an error so return it
		return err;
		}
	//calculate data values
	TFileName fileName(aCodeSeg->iFileName->Ptr());
	TBool isXip = (aCodeSeg->iXIP) ? ETrue : EFalse;

	//get the code seg type
	TCodeSegType type =
		aCodeSeg->IsExe() ? EExeCodeSegType :
		aCodeSeg->IsDll() ? EDllCodeSegType :
		EUnknownCodeSegType;

	//append data to buffer
	return AppendCodeSegData(aBuffer, aBufferSize, memoryInfo, isXip, type, fileName, aCodeSeg);
	}

TInt StopModeDebug::AppendCodeSegData(TUint8*& aBuffer, TUint32& aBufferSize, const TModuleMemoryInfo& aMemoryInfo, const TBool aIsXip, const TCodeSegType aCodeSegType, const TDesC8& aFileName, DEpocCodeSeg* aCodeSeg)
	{
	//get some data elements to put in buffer
	TUint16 fileNameLength = aFileName.Length();

	//calculate the resultant size
	TUint dataSize = Align4(sizeof(TCodeSegListEntry) + (2*fileNameLength) - sizeof(TUint16));
	if(dataSize > aBufferSize)
		{
		// data won't fit in the buffer so quit
		return dataSize;
		}
	//Create a TCodeSegListEntry which references the buffer.
	TCodeSegListEntry& entry = *(TCodeSegListEntry*)aBuffer;
	entry.iCodeBase = aMemoryInfo.iCodeBase;
	entry.iCodeSize = aMemoryInfo.iCodeSize;
	entry.iConstDataSize = aMemoryInfo.iConstDataSize;
	entry.iInitialisedDataBase = aMemoryInfo.iInitialisedDataBase;
	entry.iInitialisedDataSize = aMemoryInfo.iInitialisedDataSize;
	entry.iUninitialisedDataSize = aMemoryInfo.iUninitialisedDataSize;
	entry.iIsXip = aIsXip;
	entry.iCodeSegType = aCodeSegType;
	entry.iNameLength = fileNameLength;
	//entry.iSpare1 = (TUint32)aCodeSeg;

	//have to convert the stored name to 16 bit Unicode
	TPtr name = TPtr((TUint8*)&(entry.iName[0]), fileNameLength*2, fileNameLength*2);
	TInt err = CopyAndExpandDes(aFileName, name);
	if(err != KErrNone)
		{
		return KErrGeneral;
		}

	//increase length         
	aBufferSize-=dataSize;
	aBuffer+=dataSize;
	return KErrNone;
	}

