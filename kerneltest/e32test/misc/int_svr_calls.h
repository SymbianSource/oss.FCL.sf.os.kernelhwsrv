// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\int_svr_calls.h
// 
//

#ifndef __INT_SVR_CALLS_H__
#define __INT_SVR_CALLS_H__

#include <e32def.h>

IMPORT_C TInt SessionCreate(const TDesC8& aName, TInt aMsgSlots, const TSecurityPolicy* aPolicy, TInt aType);
IMPORT_C TInt SessionSend(TInt aHandle, TInt aFunction, TAny* aArgs, TRequestStatus* aStatus);
IMPORT_C TInt SessionSendSync(TInt aHandle, TInt aFunction, TAny* aArgs, TRequestStatus* aStatus);
IMPORT_C void SetSessionPtr(TInt aHandle, const TAny* aPtr);

#endif //__INT_SVR_CALLS_H__
