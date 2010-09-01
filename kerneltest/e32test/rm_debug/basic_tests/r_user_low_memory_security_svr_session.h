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
// Version of security server session to enable testing of low memory conditions on user side
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef R_USER_LOW_MEMORY_SECURITY_SVR_SESSION_H
#define R_USER_LOW_MEMORY_SECURITY_SVR_SESSION_H

#include "r_low_memory_security_svr_session.h"

class RUserLowMemorySecuritySvrSession : public RLowMemorySecuritySvrSession
	{
protected:
	void FailAlloc(const TInt aCount);
	void HeapReset();
	void MarkHeap();
	void MarkHeapEnd();
	};

#endif //R_USER_LOW_MEMORY_SECURITY_SVR_SESSION_H

