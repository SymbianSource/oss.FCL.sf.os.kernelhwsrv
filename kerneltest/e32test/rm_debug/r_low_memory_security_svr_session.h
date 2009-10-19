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
// Version of security server session to enable testing of low memory conditions
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef R_LOW_MEMORY_SECURITY_SVR_SESSION_H
#define R_LOW_MEMORY_SECURITY_SVR_SESSION_H

#include <rm_debug_api.h>

class RLowMemorySecuritySvrSession : public Debug::RSecuritySvrSession
	{
public:
	TInt GetList(const Debug::TListId aListId, TDes8& aListData, TUint32& aDataSize);
	TInt GetList(const TThreadId aThreadId, const Debug::TListId aListId, TDes8& aListData, TUint32& aDataSize);
	TInt GetList(const TProcessId aProcessId, const Debug::TListId aListId, TDes8& aListData, TUint32& aDataSize);
protected:
	virtual void FailAlloc(const TInt aCount) = 0;
	virtual void HeapReset() = 0;
	virtual void MarkHeap() = 0;
	virtual void MarkHeapEnd() = 0;
	};

#endif //R_LOW_MEMORY_SECURITY_SVR_SESSION_H

