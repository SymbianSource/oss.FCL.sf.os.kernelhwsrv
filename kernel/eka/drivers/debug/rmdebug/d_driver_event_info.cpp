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

#include "d_driver_event_info.h"
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

using namespace Debug;

TDriverEventInfo::TDriverEventInfo()
	{
	Reset();
	}

void TDriverEventInfo::Reset()
	{
	iProcessId = 0;
	iThreadId = 0;
	iCurrentPC = 0;
	iExceptionNumber = 0;
	iFileName.FillZ();
	iPanicCategory.FillZ();
	iCodeAddress = 0;
	iDataAddress = 0;
	iThreadIdValid = (TUint8)EFalse;
	iProcessIdValid = (TUint8)EFalse;
	iEventType = EEventsUnknown;
	iUidsValid = (TUint8)EFalse;
	};

/**
  Copy the data from this object into the object pointed to by aEventInfo in
  the client thread aClientThread. It is assumed that the write is performed
  on behalf of aClientThread.

  @param aClientThread client thread to write the data to
  @param aEventInfo TEventInfo object in the client thread to populate with data
  @param aAsyncGetValueRequest TClientDataRequest object used for pinning user memory

  @return KErrNone on success, or one of the other system wide error codes
  */
TInt TDriverEventInfo::WriteEventToClientThread(TClientDataRequest<TEventInfo>* aAsyncGetValueRequest, DThread* aClientThread) const
	{
	// create a temporary TEventInfo to populate with the relevant data
	TEventInfo eventInfo;
	TInt err = KErrNone;	
	
	// populate the data that is common to all events
	err = PopulateCommonEventInfo(eventInfo);

	if(KErrNone != err)
		{
		return err;
		}
	
	// populate the event specific data (means filling in the correct union member)
	err = PopulateEventSpecificInfo(eventInfo);

	// write the data to the client and return any error
	if(KErrNone == err)
		{
		aAsyncGetValueRequest->Data() = eventInfo;
		}
	
	return err;
	}
	
/**
  Write the common event values into aEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateCommonEventInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iEventType = iEventType;
	aEventInfo.iProcessId = iProcessId;
	aEventInfo.iProcessIdValid = iProcessIdValid;
	aEventInfo.iThreadId = iThreadId;
	aEventInfo.iThreadIdValid = iThreadIdValid;
	
	return KErrNone;
	}

/**
  Write the event specific values into aEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateEventSpecificInfo(TEventInfo& aEventInfo) const
	{
	TInt ret = KErrNone;
	
	switch(aEventInfo.iEventType)
		{
		case EEventsBreakPoint:
			ret = PopulateThreadBreakPointInfo(aEventInfo);
			return ret;
		case EEventsProcessBreakPoint:
			ret = PopulateThreadBreakPointInfo(aEventInfo);
			return ret;
		case EEventsSwExc:
			ret = PopulateThreadSwExceptionInfo(aEventInfo);
			return ret;
		case EEventsHwExc:
			ret = PopulateThreadHwExceptionInfo(aEventInfo);
			return ret;
		case EEventsKillThread:
			ret = PopulateThreadKillInfo(aEventInfo);
			return ret;
		case EEventsAddLibrary:
			ret = PopulateLibraryLoadedInfo(aEventInfo);
			return ret;
		case EEventsRemoveLibrary:
			ret = PopulateLibraryUnloadedInfo(aEventInfo);
			return ret;
		case EEventsUserTrace:
			ret = PopulateUserTraceInfo(aEventInfo);
			return ret;
		case EEventsStartThread:
			ret = PopulateStartThreadInfo(aEventInfo);
			return ret;
		case EEventsUserTracesLost:
			//no event specific data to be filled here
			return KErrNone;
		case EEventsAddProcess:
			ret = PopulateAddProcessInfo(aEventInfo);
			return ret;
		case EEventsRemoveProcess:
			ret = PopulateRemoveProcessInfo(aEventInfo);
			return ret;
		}
	
	return KErrArgument;
	}

/**
  Write the event specific values for a break point event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateThreadBreakPointInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iThreadBreakPointInfo.iExceptionNumber = (TExcType)iExceptionNumber;
	TInt ret = PopulateRmdArmExcInfo(aEventInfo);
	
	return ret;
	}

/**
  Write the event specific values for a thread exception event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateThreadSwExceptionInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iThreadSwExceptionInfo.iCurrentPC = iCurrentPC;
	aEventInfo.iThreadSwExceptionInfo.iExceptionNumber = (TExcType)iExceptionNumber;
	
	return KErrNone;
	}

/**
  Write the event specific values for a thread exception event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateThreadHwExceptionInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iThreadHwExceptionInfo.iExceptionNumber = (TExcType)iExceptionNumber;
	TInt ret = PopulateRmdArmExcInfo(aEventInfo);
	return ret;
	}

/**
  Write the event specific values for a thread panic event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateThreadKillInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iThreadKillInfo.iCurrentPC = iCurrentPC;
	aEventInfo.iThreadKillInfo.iExitReason = iExceptionNumber;
	aEventInfo.iThreadKillInfo.iExitType = iExitType;
	aEventInfo.iThreadKillInfo.iPanicCategoryLength = iPanicCategory.Length();
	TPtr8 panicCategoryPtr(&(aEventInfo.iThreadKillInfo.iPanicCategory[0]), iPanicCategory.Length());
	panicCategoryPtr = iPanicCategory;
	
	return KErrNone;
	}

/**
  Write the event specific values for a library loaded event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateStartThreadInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iStartThreadInfo.iFileNameLength = iFileName.Length();
	TPtr8 fileNamePtr(&(aEventInfo.iStartThreadInfo.iFileName[0]), iFileName.Length());
	fileNamePtr = iFileName;
	
	return KErrNone;
	}

/**
  Write the event specific values for an AddProcess event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateAddProcessInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iAddProcessInfo.iFileNameLength = iFileName.Length();
	TPtr8 fileNamePtr(&(aEventInfo.iAddProcessInfo.iFileName[0]), iFileName.Length());
	fileNamePtr = iFileName;

	const TInt uid3offset = 2;
	aEventInfo.iAddProcessInfo.iUid3 = iUids.iUid[uid3offset].iUid;
	aEventInfo.iAddProcessInfo.iCreatorThreadId = iCreatorThreadId;

	return KErrNone;
	}

/**
  Write the event specific values for a RemoveProcess event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateRemoveProcessInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iRemoveProcessInfo.iFileNameLength = iFileName.Length();
	TPtr8 fileNamePtr(&(aEventInfo.iRemoveProcessInfo.iFileName[0]), iFileName.Length());
	fileNamePtr = iFileName;
	
	return KErrNone;
	}

/**
  Write the event specific values for a library loaded event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateLibraryLoadedInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iLibraryLoadedInfo.iCodeAddress = iCodeAddress;
	aEventInfo.iLibraryLoadedInfo.iDataAddress = iDataAddress;
	aEventInfo.iLibraryLoadedInfo.iFileNameLength = iFileName.Length();
	TPtr8 fileNamePtr(&(aEventInfo.iLibraryLoadedInfo.iFileName[0]), iFileName.Length());
	fileNamePtr = iFileName;
	
	return KErrNone;
	}

/**
  Write the event specific values for a library unloaded event into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateLibraryUnloadedInfo(TEventInfo& aEventInfo) const
	{
	aEventInfo.iLibraryUnloadedInfo.iFileNameLength = iFileName.Length();
	TPtr8 fileNamePtr(&(aEventInfo.iLibraryUnloadedInfo.iFileName[0]), iFileName.Length());
	fileNamePtr = iFileName;
	
	return KErrNone;
	}

/**
  Write the ArmExcInfo values into TEventInfo

  @param aEventInfo TEventInfo object to write data into
  */
TInt TDriverEventInfo::PopulateRmdArmExcInfo(TEventInfo& aEventInfo) const
	{
	switch(iEventType)
		{
		case EEventsProcessBreakPoint:
		case EEventsBreakPoint:
			aEventInfo.iThreadBreakPointInfo.iRmdArmExcInfo = iRmdArmExcInfo;
			break;
		case EEventsHwExc:
			aEventInfo.iThreadHwExceptionInfo.iRmdArmExcInfo = iRmdArmExcInfo;
			break;
		}
	
	return KErrNone;
	}

/**
 * Writes the user trace into TEventInfo
 * 
 * @param aEventInfo TEventInfo object to write data into
 */
TInt TDriverEventInfo::PopulateUserTraceInfo(TEventInfo& aEventInfo) const
	{	
	aEventInfo.iUserTraceInfo.iUserTraceLength = (TInt)iArg2;
	
	TPtr8 ptr(aEventInfo.iUserTraceInfo.iUserTraceText, (TInt)iArg2, TUserTraceSize );
	ptr.Copy(iUserTraceText, (TInt)iArg2);
		
	return KErrNone;
	}

TBool TDriverEventInfo::FreezeOnSuspend() const
	{
	switch(iEventType)
		{
		case EEventsHwExc:
		case EEventsBreakPoint:
		case EEventsProcessBreakPoint:
			return ETrue;
		case EEventsKillThread:
			{
			return (iExitType == EExitPanic);
			}
		}
	return EFalse;
	}

TBool TDriverEventInfo::TookException() const
	{
	return iExitType == EExitPanic &&
		iExceptionNumber == ECausedException &&
		iPanicCategory == KLitKernExec;
	}

