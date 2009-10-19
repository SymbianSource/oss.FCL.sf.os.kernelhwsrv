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
// e32\debug\crashMonitor\src\scmconfigitem.cpp
// 
//

/**
 @file
 @internalTechnology
*/
#include <e32def.h>
#include <e32def_private.h>

#include <scmconfigitem.h>
#include <scmdatatypes.h>
#include <scmtrace.h>

namespace Debug
	{	
	
/**
 * TConfigItem constructor
 */
TConfigItem::TConfigItem() 
: iDataType(ELast)
, iSizeToDump(0)
, iSpaceRequiredToDump(0)
, iPriority(0)
, iNext(NULL)
	{
	}

/**
 * TConfigItem constructor
 * @param aDataType - data type id
 * @param aSizeToDump - size of the config data 
 * @param aPriority - priority of this data type (if 0 then not used)
 */	
TConfigItem::TConfigItem(TSCMDataType aDataType,  TUint8 aPriority, TInt32 aSizeToDump)
: iDataType(aDataType)
, iSizeToDump(aSizeToDump)
, iPriority(aPriority)
, iNext(NULL)
	{	
	}
	
/**
 * Serialize - writes this object to the supplied byte stream
 * @param aItem -  aWriter - the TByteStreamWriter that will be written to
 * @return one of the OS wide codes
 */
TInt TConfigItem::Serialize(TByteStreamWriter& aWriter)
	{	
	TInt startPos = aWriter.CurrentPosition();
	
	aWriter.WriteByte((TUint8)iDataType);
	aWriter.WriteInt(iSizeToDump);
	aWriter.WriteByte(iPriority);				

	TInt sizeWritten = aWriter.CurrentPosition() - startPos;
	if(sizeWritten != GetSize())
		{
		// error between actual size & real size in data
		CLTRACE2("TConfigItem serialization size error sizeWritten %d GetSize() %d", sizeWritten, GetSize());
		return KErrCorrupt;
		}
	
	return KErrNone;
	}
	

/**
 * Deserialize - read this objects state from the supplied byte stream
 * @param aItem -  aReader - the TByteStreamReader that will be read from
 * @return One of the OS wide codes
 */
TInt TConfigItem::Deserialize(TByteStreamReader& aReader)
	{
	TInt startPos = aReader.CurrentPosition();

	iDataType = (TSCMDataType) aReader.ReadByte();	
	iSizeToDump = aReader.ReadInt();
	iPriority = aReader.ReadByte();	
	
	TInt sizeRead = aReader.CurrentPosition() - startPos;
	if(sizeRead != GetSize())
		{
		// error between actual size & real size in data
		CLTRACE("(TConfigItem::Deserialize) ERROR size error");
		return KErrCorrupt;
		}				
	
	return KErrNone;
	}


/**
 * GetDataType 
 * @return data type of this config item
 */
 TConfigItem::TSCMDataType TConfigItem::GetDataType() const
	{
	return iDataType;
	}

/**
 * GetPriority 
 * @return priority of this config item (0-255)
 */ 
TInt TConfigItem::GetPriority() const
	{
	return iPriority;
	}

/**
 * GetSize 
 * @return size to dump in bytes
 */
TInt TConfigItem::GetSizeToDump() const
	{
	return iSizeToDump;
	}

/**
 * GetSize 
 * @return size of this object when streamed in bytes
 */
TInt TConfigItem::GetSize() const
	{
	return 6;
	}



/**
 * Returns next item
 * @return Next item
 */
TConfigItem* TConfigItem::Next() const
	{
	return iNext;
	}

	/**
	 * Print - displays info about this TConfigItem 
	 * @return void
	 */
 	void TConfigItem::Print() const 
 		{
 		CLTRACE3( "(TConfigItem::Print) iDataType = %d iPriority = %d iSizeToDump = %d"
 				, iDataType, iPriority, iSizeToDump);
 		}
 	
 	TBool TConfigItem::operator == (const TConfigItem& aOther) const
 		{
 		return (iDataType == aOther.iDataType && iSizeToDump == aOther.iSizeToDump && iPriority == aOther.iPriority);
 		}

/**
 * Sets the space required parameter for this config item
 * @param aSpaceReq Space required
 */
void TConfigItem::SetSpaceRequired(TUint aSpaceReq)
	{
	iSpaceRequiredToDump = aSpaceReq;
	}

/**
 * Gets the space required to store this config item
 * @return
 */
TUint TConfigItem::GetSpaceRequired()
	{
	return iSpaceRequiredToDump;
	}

#ifndef __KERNEL_MODE__

// human readable strings for TSCMDataType
_LIT(KExceptionStacks, "Exception Stacks");
_LIT(KTraceData, "Trace data");
_LIT(KProcessCodeSegs, "ProcessCodeSegs");
_LIT(KThreadsUsrStack, "Threads UserStack");
_LIT(KThreadsSvrStack, "Threads Supervisor Stack");
_LIT(KKernelHeap, "Kernel Heap");
_LIT(KThreadsUsrRegisters, "Threads User Registers");
_LIT(KThreadsSvrRegisters, "Threads Supervisor Registers");
_LIT(KProcessMetaData, "Process Meta Data");
_LIT(KThreadsMetaData, "Threads Meta Data");
_LIT(KCrashedProcessCodeSegs, "Crashed Process Code Segs");
_LIT(KCrashedProcessUsrStacks, "Crashed Process' User Stack's");
_LIT(KCrashedProcessSvrStacks, "Crashed Process' Supervisor Stack's");
_LIT(KCrashedProcessMetaData, "Crashed Process Meta Data");
_LIT(KCrashedThreadMetaData, "Crashed Thread Meta Data");				
_LIT(KLocks, "SCM Locks");
_LIT(KVariantSpecific, "Variant Specific Data");
_LIT(KRomInfo, "ROM Info");
_LIT(KUnknown,  "Unknown");

/**
 * helper function for converting TSCMDataType enul to human readable form
 */
 const TDesC& TConfigItem::GetSCMConfigOptionText(TConfigItem::TSCMDataType aType)
 		{
 		switch (aType)
 			{
 			case TConfigItem::EExceptionStacks:
 				return KExceptionStacks;
 			case TConfigItem::ETraceData:
 				return KTraceData;
 			case TConfigItem::EProcessCodeSegs:
 				return KProcessCodeSegs();
 			case TConfigItem::EThreadsUsrStack:
 				return KThreadsUsrStack();
 			case TConfigItem::EThreadsSvrStack:
 				return KThreadsSvrStack;
 			case TConfigItem::EThreadsUsrRegisters:
 				return KThreadsUsrRegisters();
 			case TConfigItem::EThreadsSvrRegisters:
 				return KThreadsSvrRegisters();
 			case TConfigItem::EProcessMetaData:
 				return KProcessMetaData();
 			case TConfigItem::EThreadsMetaData:
 				return KThreadsMetaData();
 			case TConfigItem::ECrashedProcessCodeSegs:
 				return KCrashedProcessCodeSegs();
 			case TConfigItem::ECrashedProcessUsrStacks:
 				return KCrashedProcessUsrStacks();
 			case TConfigItem::ECrashedProcessSvrStacks:
 				return KCrashedProcessSvrStacks();
 			case TConfigItem::ECrashedProcessMetaData:
 				return KCrashedProcessMetaData();
 			case TConfigItem::ECrashedThreadMetaData:						
 				return KCrashedThreadMetaData();
 			case TConfigItem::ELocks:						
 				return KLocks();
 			case TConfigItem::EKernelHeap:
 				return KKernelHeap();
 			case TConfigItem::EVariantSpecificData:
 				return KVariantSpecific();
 			case TConfigItem::ERomInfo:
 				return KRomInfo();	
			case TConfigItem::ELast:
 			default:
 				return KUnknown();
 			}
 		}
#endif // ! __KERNEL_MODE__
}

//eof


