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
// Purpose: Kernel-side tracking of event information
//
//

#ifndef T_DRIVER_EVENT_INFO_H
#define T_DRIVER_EVENT_INFO_H

#include <rm_debug_api.h>
#include <kernel/kernel.h> 

/**
@file
@internalComponent
*/

class TDriverEventInfo
	{
public:
	TDriverEventInfo();
	void Reset();
	TInt WriteEventToClientThread(TClientDataRequest<Debug::TEventInfo>* aAsyncGetValueRequest, DThread* aClientThread) const;
	TBool FreezeOnSuspend() const;

private:
	TInt PopulateCommonEventInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateEventSpecificInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateThreadBreakPointInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateThreadHwExceptionInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateThreadSwExceptionInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateThreadKillInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateLibraryLoadedInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateLibraryUnloadedInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateRmdArmExcInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateUserTraceInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateStartThreadInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateAddProcessInfo(Debug::TEventInfo& aEventInfo) const;
	TInt PopulateRemoveProcessInfo(Debug::TEventInfo& aEventInfo) const;
	TBool TookException() const;

public:
	Debug::TEventType iEventType;
	TUint64 iProcessId;
	TUint64 iThreadId;
	TUint64 iCreatorThreadId;
	TUint32 iCurrentPC;
	TInt iExceptionNumber;
	TBuf8<KMaxName> iFileName;
	TBuf8<Debug::KPanicCategoryMaxName> iPanicCategory;
	TUint32 iCodeAddress;
	TUint32 iDataAddress;
	TUint8 iExitType;
	TUint8 iThreadIdValid;
	TUint8 iProcessIdValid;
	TUidType iUids;
	TUint8 iUidsValid;

	//The objects that these pointers point to are not
	//owned by the Debug::TEventInfo class so no cleanup is required
	TAny* iArg1;	// a1
	TAny* iArg2;	// a2

	union
	{
		Debug::TRmdArmExcInfo iRmdArmExcInfo;
		//To store Trace info
		TUint8 iUserTraceText[Debug::TUserTraceSize];
	};

	//status of trace message
	Debug::TUserTraceMessageContext iMessageStatus;

	};


#endif //T_DRIVER_EVENT_INFO_H
